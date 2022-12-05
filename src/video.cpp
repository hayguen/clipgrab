/*
    ClipGrab³
    Copyright (C) The ClipGrab Project
    http://clipgrab.de
    feedback [at] clipgrab [dot] de

    This file is part of ClipGrab.
    ClipGrab is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    ClipGrab is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ClipGrab.  If not, see <http://www.gnu.org/licenses/>.
*/



#include "video.h"

static const int num_download_progress_reports = 100;   // => 1.0% steps on console with qDebug()

video::video()
: re_destination("^\\[download\\] destination: (.+)", QRegularExpression::CaseInsensitiveOption)
, re_downloaded("^\\[download\\] (.+?) has already been downloaded( and merged)?", QRegularExpression::CaseInsensitiveOption)
, re_ffmpeg_merging("^\\[ffmpeg\\] merging formats into \"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption)
, re_merger_merging("^\\[merger\\] merging formats into \"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption)
, re_ytdl_progress("^\\[download\\]\\s+(\\d+\\.\\d)%\\s+of\\s+~?\\s*(\\d+\\.\\d+)(T|G|M|K)iB", QRegularExpression::CaseInsensitiveOption)
, re_error("ERROR:\\s+(.*)", QRegularExpression::CaseInsensitiveOption)
{
    selectedQuality = -1;
    state = state::empty;
    conversion_progress = -1.0;

    youtubeDl = nullptr;

    targetConverter = nullptr;
    cachedDownloadSize = 0;
    cachedDownloadProgress = 0;
    audioOnly = false;

    QSettings settings;
    verbose = settings.value("verbose", false).toBool();
}

void video::startYoutubeDl(QStringList arguments) {
    if (youtubeDl != nullptr) youtubeDl->deleteLater();

    youtubeDl = YoutubeDl::instance(arguments);
    connect(youtubeDl , QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &video::handleProcessFinished);
    connect(youtubeDl , &QProcess::readyRead, this, &video::handleProcessReadyRead);

    qDebug().noquote().nospace() << "\nStarting download with " << youtubeDl->program() << " " << youtubeDl->arguments();
    youtubeDl->start();
}

void video::fetchPlaylistInfo(QString url) {
    if (state != state::empty) return;
    qDebug().noquote().nospace() << "State fetching for '" << url << "'.";
    state = state::fetching;

    this->url = url;

    QStringList arguments;
    arguments << "-J" << url;
    arguments << "--yes-playlist";
    arguments << "--flat-playlist";
    startYoutubeDl(arguments);
}

void video::fetchInfo(QString url) {
    if (state != state::empty) return;
    qDebug().noquote().nospace() << "State fetching for '" << url << "'.";
    state = state::fetching;

#if 0
    // youtube-dl fails on YouTube links that include ?list parameter
    // looks, yt-dlp doesn't fail
    QUrl parsedUrl = QUrl(url);
    if (parsedUrl.host() == "www.youtube.com" && parsedUrl.path() == "/watch") {
        QString v = QUrlQuery(parsedUrl.query()).queryItemValue("v");
        parsedUrl.setQuery("v=" + v);
        url = parsedUrl.toString();
    } else if (parsedUrl.host() == "youtu.be") {
        QString v = parsedUrl.path();
        parsedUrl.setUrl("https://www.youtube.com/watch?v=" + v);
    }
#endif

    this->url = url;

    QStringList arguments;
    arguments << "-J";
    arguments << "--no-playlist";
    arguments << url;

    qDebug().noquote() << "check/retrieve URL " << url;
    startYoutubeDl(arguments);
}

void video::download() {
    if ((state != state::fetched && state != state::paused && state != state::canceled) || selectedQuality <= -1) return;
    qDebug().noquote().nospace() << "State downloading for '" << url << "'.";
    state = state::downloading;

    videoQuality quality = qualities.at(selectedQuality);
    downloadSize = quality.videoFileSize + quality.audioFileSize;

    QStringList arguments;

    arguments << "--newline";
    arguments << "--no-playlist";
    arguments << "--no-mtime";

    QString fileTemplate = QDir::cleanPath(
                QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                QDir::separator() +
                "/cg-youtube-dl-%(id)s-%(format_id)s.%(ext)s"
    );
    arguments << "-o" << fileTemplate;

    if (quality.audioFormat.isEmpty()) {
        arguments << "-f" << quality.videoFormat;
    } else if (audioOnly) {
        arguments << "-f" << quality.audioFormat;
    } else {
        arguments << "-f" << quality.videoFormat + "+" + quality.audioFormat;
    }

    arguments << url;

    startYoutubeDl(arguments);
}

void video::cancel() {
    if ((state != state::downloading && state != state::converting) && state != state::paused) return;

    if (state == state::downloading)
        qDebug().noquote().nospace() << "State canceling for '" << url << "' .. terminating ytdl!";
    if (state == state::converting)
        qDebug().noquote().nospace() << "State canceling for '" << url << "' .. terminating ffmpeg!";

    state = state::canceling;
    if (youtubeDl != nullptr) youtubeDl->terminate();
    if (targetConverter) targetConverter->abortConversion();
}

void video::pause() {
    if (state != state::downloading && state != state::converting) return;

    if (state == state::downloading)
        qDebug().noquote().nospace() << "State pausing download of '" << url << "' .. terminating ytdl!";
    if (state == state::converting)
        qDebug().noquote().nospace() << "State pausing conversion of '" << url << "' .. terminating ffmpeg!";

    state = state::pausing;
    if (youtubeDl != nullptr) youtubeDl->terminate();
    if (targetConverter) targetConverter->abortConversion();
}

void video::resume() {
    if (state != state::paused) return;

    qDebug().noquote().nospace() << "resuming download of '" << url << "' ..";
    download();
}

void video::restart() {
    if (state != state::canceled && state != state::error) return;

    qDebug().noquote().nospace() << "restarting download of '" << url << "' ..";
    download();
}

void video::fromJson(QByteArray data) {
    if (state != state::empty) return;
    handleInfoJson(data);
}

void video::handleInfoJson(QByteArray data) {
    QJsonObject json = QJsonDocument::fromJson(data).object();

    title = json.value("title").toString();
    id = json.value("id").toString();

    if (json.value("_type").toString() == "playlist") {
        QJsonArray playlist = json.value("entries").toArray();
        for (int i = 0; i < playlist.size(); i++) {
            video* video = new class video;
            video->fromJson(QJsonDocument(playlist.at(i).toObject()).toJson());
            playlistVideos << video;
        }

        qDebug().noquote().nospace() << "State fetched info for '" << url << "'. title is '" << title << "'";
        state = state::fetched;
        return;
    } else if (json.value("_type").toString() == "url") {
        portal = json.value("ie_key").toString().toLower();
        if (portal == "generic") portal = QUrl(this->url).host();

        url = json.value("url").toString();
        if (QUrl(url).scheme().isEmpty() && portal == "youtube") {
            url = "https://www.youtube.com/watch?v=" + url;
        }

        qDebug().noquote().nospace() << "State unfetched info for '" << url << "' !?";
        state = state::unfetched;
        return;
    }

    portal = json.value("extractor").toString().toLower();
    if (portal.isEmpty()) portal = json.value("ie_key").toString().toLower();
    else if (portal == "generic") portal = QUrl(this->url).host();

    url = json.value("webpage_url").toString();
    if (url.isEmpty()) url = json.value("url").toString();
    if (QUrl(url).scheme().isEmpty() && portal == "youtube") {
        url = "https://www.youtube.com/watch?v=" + url;
    }

    artist = json.value("artist").toString();
    if (artist.isEmpty()) artist = json.value("user").toString();
    if (artist.isEmpty()) artist = json.value("uploader").toString();

    duration = json.value("duration").toDouble();

    qDebug().noquote().nospace() << "\nfetched infos for '" << title << "'  id " << id;

    QJsonArray formats = json.value("formats").toArray();
    QList<QJsonObject> videoFormats;
    QList<QJsonObject> audioFormats;
    // QStringList acceptedExts = {"mp4", "m4a"};
    // if (QSettings().value("UseWebM", false).toBool()) {
    //    acceptedExts << "webm" << "opus";
    // }
    for (int i = 0; i < formats.size(); i++) {
        QJsonObject format = formats.at(i).toObject();
        // accept ALL formats!
        // if (acceptedExts.contains(format.value("ext").toString())) {
            if (format.value("vcodec").toString() == "none") {
                audioFormats << format;
            } else {
                videoFormats << format;
            }
        //}
    }

    // Sort audio formats by bitrate
    std::sort(audioFormats.begin(), audioFormats.end(), [](QJsonObject a, QJsonObject b) {
        double tbrA = a.value("tbr").toDouble();
        double tbrB = b.value("tbr").toDouble();

        return tbrA > tbrB;
    });

    // Sort video formats by format and resolution
    std::sort(videoFormats.begin(), videoFormats.end(), [](QJsonObject a, QJsonObject b) {
        int heightA = a.value("height").toInt();
        int heightB = b.value("height").toInt();
        if (heightA != heightB) return heightA > heightB;

        int fpsA = a.value("fps").toInt();
        int fpsB = b.value("fps").toInt();
        if (fpsA != fpsB) return fpsA > fpsB;

        QString extA = a.value("ext").toString();
        QString extB = b.value("ext").toString();
        if (extA != extB) return extA > extB;

        // This makes sure AVC1 comes before AV01 and AV01 is removed as a duplicate
        QString vcodecA = a.value("vcodec").toString();
        QString vcodecB = b.value("vcodec").toString();
        if (vcodecA != vcodecB) return vcodecA > vcodecB;

        QString acodecA = a.value("acodec").toString();
        QString acodecB = b.value("acodec").toString();
        if (acodecA != acodecB) {
            if (acodecA == "none") return true;
            if (acodecB == "none") return false;
        }
        return false;
    });

    // Remove duplicates
    if (false)  // keep duplicates - probably with different externsion
    videoFormats.erase(
        std::unique(videoFormats.begin(), videoFormats.end(), [](QJsonObject a, QJsonObject b) {
            int heightA = a.value("height").toInt();
            int heightB = b.value("height").toInt();
            if (heightA != heightB) return false;

            int fpsA = a.value("fps").toInt();
            int fpsB = b.value("fps").toInt();
            return fpsA == fpsB;
        }),
        videoFormats.end()
    );

    for (int i = 0; i < videoFormats.size(); i ++) {
        QJsonObject videoFormat = videoFormats.at(i);
        int height = videoFormat.value("height").toInt();
        int width = videoFormat.value("width").toInt();
        QRegularExpression heightExp("^(\\d+)p");
        QRegularExpressionMatch match = heightExp.match(videoFormat.value("format_note").toString());
        if (match.hasMatch()) height = match.captured(1).toInt();
        int fps = videoFormat.value("fps").toInt();

        QString name = QString::number(height) + "p";
        if (width)
            name = QString::number(width) + "x" + name;
        if (name == "0p") name = tr("unknown");
        else // if (fps >= 59) name.append("60");
            name.append(QString::number(fps));

        if (height >= 4000) {
            name.append(" (8K)");
        } else if (height >= 2000) {
            name.append(" (4K)");
        } else if (height >= 700) {
            name.append(" (HD)");
        }

        QString extStr = videoFormat.value("ext").toString().trimmed();
        // if (extStr == "webm") {
        //     name.append(" WebM");
        // }
        name.append(" ");
        name.append(extStr);

        QString vCodec = videoFormat.value("vcodec").toString();
        QString aCodec = videoFormat.value("acodec").toString();
        QString avCodec;
        if (vCodec == "none") {
            if (aCodec == "none")
                avCodec = "";
            else
                avCodec = aCodec;
        }
        else {
            if (aCodec == "none")
                avCodec = vCodec;
            else
                avCodec = vCodec + "/" + aCodec;
        }
        if ( !avCodec.isEmpty() ) {
            name.append(QString(" (") + avCodec + ")");
        }

        videoQuality quality(name, videoFormat.value("format_id").toString());
        quality.resolution = height;
        quality.videoFileSize = videoFormat.value("filesize").toInt();
        quality.audioFileSize = 0;
        quality.containerName = videoFormat.value("ext").toString();
        quality.videoCodec = vCodec;
        quality.audioCodec = aCodec;

        qDebug().noquote().nospace()
                << "format " << i << "/" << videoFormats.size() << ": '" << name << "' resolution " << width << "x" << height << " fps " << fps
            << "\n  container ext '" << videoFormat.value("ext").toString() << "' "
            << "  format " << videoFormat.value("format_id").toString() << ": Codec(s): " << avCodec;

        QList<QJsonObject> compatibleAudioFormats(audioFormats);
        QStringList compatibleAudioExts = {"aac", "m4a"};
        if (videoFormat.value("ext") == "webm") {
             compatibleAudioExts.clear();
             compatibleAudioExts << "webm" << "ogg";
        }
        compatibleAudioFormats.erase(std::remove_if(compatibleAudioFormats.begin(), compatibleAudioFormats.end(), [videoFormat, compatibleAudioExts](QJsonObject audioFormat) {
            QString ext = audioFormat.value("ext").toString();

            return !compatibleAudioExts.contains(ext);
        }), compatibleAudioFormats.end());

        if (videoFormat.value("acodec") == "none" && audioFormats.size() > 0) {
            int audioIndex = (compatibleAudioFormats.size() -1) * i / videoFormats.size();
            quality.audioFormat = compatibleAudioFormats.at(audioIndex).value("format_id").toString();
            quality.audioFileSize = compatibleAudioFormats.at(audioIndex).value("filesize").toInt();
        }

        qualities << quality;
    }

    qDebug().noquote().nospace() << "\nState fetched info for '" << url << "'.";
    state = state::fetched;
}

void video::handleDownloadInfo(QString line) {
    QRegularExpressionMatch match;
    line = line.trimmed();

    // re.setPattern("^\\[download\\] Destination: (.+)");
    match = re_destination.match(line);
    if (!match.captured(1).isNull()) {
        QString filename = match.captured(1);
        if (!downloadFilenames.contains(filename)) {
            downloadFilenames << match.captured(1);
            downloadSizeEstimates << 0;
        }
        qDebug().noquote() << line;
        qDebug().noquote().nospace() << "captured filename for '" << url << "': " << filename;
        return;
    }

    // re.setPattern("^\\[download\\] (.+?) has already been downloaded( and merged)?");
    match = re_downloaded.match(line);
    if (!match.captured(1).isNull()) {
        finalDownloadFilename = match.captured(1);
        qDebug().noquote() << line;
        qDebug().noquote().nospace() << "captured finalDownloadFilename for '" << url << "': " << finalDownloadFilename;
        return;
    }
    // re.setPattern("^\\[Merger\\] Merging formats into \"([^\"]+)\"");
    match = re_merger_merging.match(line);
    if (!match.captured(1).isNull()) {
        finalDownloadFilename = match.captured(1);
        qDebug().noquote() << line;
        qDebug().noquote().nospace() << "captured " << YoutubeDl::ytdl_name << " Merger finalDownloadFilename for '" << url << "': " << finalDownloadFilename;
        return;
    }
    // re.setPattern("^\\[ffmpeg\\] Merging formats into \"([^\"]+)\"");
    match = re_ffmpeg_merging.match(line);
    if (!match.captured(1).isNull()) {
        finalDownloadFilename = match.captured(1);
        qDebug().noquote() << line;
        qDebug().noquote().nospace() << "captured FFmpeg finalDownloadFilename for '" << url << "': " << finalDownloadFilename;
        return;
    }

    // re.setPattern("^\\[download\\]\\s+(\\d+\\.\\d)%\\s+of\\s+~?\\s*(\\d+\\.\\d+)(T|G|M|K)iB");
    match = re_ytdl_progress.match(line);
    // const qint64 prev_downloadProgessPercent = (cachedDownloadProgress * num_download_progress_reports) / (cachedDownloadSize ? cachedDownloadSize : 1);
    bool matchedLine = match.hasMatch();
    if (matchedLine && !downloadFilenames.isEmpty()) {
        qint64 downloadProgress = 0;
        for (int i = 0; i < downloadFilenames.size(); i++) {
            downloadProgress += QFileInfo(downloadFilenames.at(i)).size();
            downloadProgress += QFileInfo(downloadFilenames.at(i) + ".part").size();
        }

        if (downloadSize > 0 && downloadProgress > 0) {
            cachedDownloadSize = downloadSize;
            cachedDownloadProgress = downloadProgress;
            // const qint64 new_downloadProgessPercent = (cachedDownloadProgress * num_download_progress_reports) / (cachedDownloadSize ? cachedDownloadSize : 1);
            // if ( new_downloadProgessPercent > prev_downloadProgessPercent )
            //     qDebug().noquote() << "download progress: " << new_downloadProgessPercent << ": " << line;
            emit downloadProgressChanged(downloadSize, downloadProgress);
        } else if (downloadProgress > 0) {
            QStringList prefixes {"K", "M", "G", "T"};
            qint64 downloadSizeEstimate = match.captured(2).toFloat() * pow(1024, 1 + prefixes.indexOf(match.captured(3)));
            downloadSizeEstimates.replace(downloadSizeEstimates.size() - 1, downloadSizeEstimate);

            qint64 totalDownloadSizeEstimate = 0;
            for (int i = 0; i < downloadSizeEstimates.size(); i++) {
                totalDownloadSizeEstimate += downloadSizeEstimates.at(i);
            }

            if (totalDownloadSizeEstimate > 0) {
                cachedDownloadSize = totalDownloadSizeEstimate;
                cachedDownloadProgress = downloadProgress;
                // const qint64 new_downloadProgessPercent = (cachedDownloadProgress * num_download_progress_reports) / (cachedDownloadSize ? cachedDownloadSize : 1);
                // if ( new_downloadProgessPercent > prev_downloadProgessPercent )
                //     qDebug().noquote() << "download progress: " << new_downloadProgessPercent << ": " << line;
                emit downloadProgressChanged(totalDownloadSizeEstimate, downloadProgress);
            }
        }
    }

    // re.setPattern("ERROR:\\s+(.*)");
    match = re_error.match(line);
    if (match.hasMatch()) {
        qDebug().noquote() << "ERROR!" << match.captured(1);
        qDebug().noquote().nospace() << "State error: killing " << YoutubeDl::ytdl_name << " for '" << url << "': " << match.captured(1);
        state = state::error;
        emit stateChanged();
        youtubeDl->kill();
        return;
    }

    if ( !matchedLine )
        qDebug().noquote() << "unmatched output: " << line;
}

bool video::setQuality(int index) {
    if (index >= qualities.size()) return false;

    selectedQuality = index;
    return true;
}


void video::setTargetFilename(QString filename) {
    this->targetFilename = filename;
}

QString video::getSafeFilename() {
    return title.replace(QRegularExpression("#|%|&|\\{|\\}|\\\\|<|>|\\*|\\?|/|\\$|!|'|\"|:|@|\\+|`|\\||=|"), "");
}

void video::setConverter(converter* targetConverter, int targetConverterMode, QString audio_bitrate, QString audio_quality) {
    this->targetConverter = targetConverter->createNewInstance();
    this->targetConverterMode = targetConverterMode;
    this->audioOnly = targetConverter->isAudioOnly(targetConverterMode);
    this->audio_bitrate = audio_bitrate;
    this->audio_quality = audio_quality;

    connect(this->targetConverter, &converter::conversionFinished, this, &video::handleConversionFinished);
    connect(this->targetConverter, &converter::progress, this, &video::handleConversionProgress);
    connect(this->targetConverter, &converter::error, this, &video::handleConversionError);
}

QString video::getTitle() {
    return title;
}

QString video::getArtist() {
    return artist;
}

void video::setMetaTitle(QString title) {
    metaTitle = title;
}

void video::setMetaArtist(QString artist) {
    metaArtist = artist;
}

QString video::getThumbnail() {
    if (portal == "youtube") {
        return "https://i.ytimg.com/vi/" + id + "/hqdefault.jpg";
    }
    return "";
}

qint64 video::getDuration() {
    return duration;
}

QString video::getUrl() {
    return url;
}

QList<videoQuality> video::getQualities() {
    return qualities;
}

QString video::getSelectedQualityName() {
    if (selectedQuality == -1) return "";

    return qualities.at(selectedQuality).name;
}

QString video::getPortalName() {
    if (portal == "youtube") return "YouTube";
    return portal;
}

qint64 video::getDownloadSize() {
    if (cachedDownloadSize < cachedDownloadProgress) return cachedDownloadProgress;
    return cachedDownloadSize;
}

qint64 video::getDownloadProgress() {
    return cachedDownloadProgress;
}

QString video::getTargetFormatName() {
    if (targetConverter == nullptr) return "";
    return targetConverter->getModes().at(targetConverterMode);
}

QList<video*> video::getPlaylistVideos() {
    return playlistVideos;
}

void video::handleProcessFinished(int /*exitCode*/, QProcess::ExitStatus exitStatus) {
    switch (state) {
    case state::fetching:
        if (exitStatus == QProcess::ExitStatus::NormalExit) {
            QByteArray out = youtubeDl->readAllStandardOutput();
            if (verbose) {
                if (!out.size()) {
                    // make the JSON readable
                    QString all_lines = QString::fromUtf8(out);
                    all_lines = all_lines.replace("{", "{\n");
                    all_lines = all_lines.replace("[", "[\n");
                    all_lines = all_lines.replace("\"}", "\"\n}");
                    all_lines = all_lines.replace("\",", "\",\n");
                    QFile infos("video_info.json");
                    if ( infos.open(QIODevice::WriteOnly | QIODevice::Text) ) {
                        infos.write(all_lines.toUtf8());
                        infos.close();
                        qDebug() << "wrote video_info.json";
                    }
                }
            }
            QByteArray errs = youtubeDl->readAllStandardError();
            if (errs.size()) {
                qDebug() << "Error fetching video infos:";
                const QStringList lines = QString::fromUtf8(errs).split("\n");
                for (const QString &ln : lines)
                    qDebug().noquote() << ln.trimmed();
                if (verbose) {
                    QFile err_file("video_info_errors.txt");
                    if ( err_file.open(QIODevice::WriteOnly | QIODevice::Text) ) {
                        err_file.write(errs);
                        err_file.close();
                    }
                }
            }
            handleInfoJson(out);
            youtubeDl->close();

            if (qualities.length() > 0) {
                qDebug().noquote() << "Discovered video: " << title;
                qDebug().noquote() << "State fetched - after normal exit - for '" << url << "'";
                state = state::fetched;
            } else {
                qDebug().noquote() << "State error from fetching - after normal exit - for '" << url << "'";
                state = state::error;
            }
        } else {
            qDebug().noquote() << "State error from fetching - after crash exit - for '" << url << "'";
            state = state::error;
        }
        emit stateChanged();
        break;
    case state::downloading:
        if (exitStatus == QProcess::ExitStatus::NormalExit) {

            if (finalDownloadFilename.isEmpty() && !downloadFilenames.empty()) finalDownloadFilename = downloadFilenames.last();
            if (finalDownloadFilename.isEmpty()) {
                qDebug().noquote().nospace() << "State error from downloading - after normal exit of " << YoutubeDl::ytdl_name << " with empty filename for '" << url << "'";
                state = state::error;
                emit stateChanged();
                return;
            }

            qDebug().noquote().nospace() << "State converting from downloading - after normal exit of " << YoutubeDl::ytdl_name << " for '" << url << "'";
            qDebug().noquote() << "start " << targetConverter->conversion_str() << " from '" << finalDownloadFilename << "' ..";
            state = state::converting;
            QFile* file = new QFile();
            file->setFileName(finalDownloadFilename);
            targetConverter->startConversion(
                file, targetFilename, qualities.at(selectedQuality).containerName, metaTitle, metaArtist,
                targetConverterMode, this->audio_bitrate, this->audio_quality
                );
        } else {
            qDebug().noquote().nospace() << "State error from downloading - after crash exit of " << YoutubeDl::ytdl_name << " for '" << url << "'";
            state = state::error;
        }
        emit stateChanged();
        break;
    case state::pausing:
        qDebug().noquote().nospace() << "State paused from pausing - after exit of " << YoutubeDl::ytdl_name << " for '" << url << "'";
        state = state::paused;
        emit stateChanged();
        return;
    case state::canceling:
        removeTempFiles();
        qDebug().noquote().nospace() << "State canceled from canceling - after exit of " << YoutubeDl::ytdl_name << " for '" << url << "'";
        state = state::canceled;
        emit stateChanged();
        return;
    default:
        break;
    }
}

void video::handleProcessReadyRead() {
    switch (state) {
    case state::fetching:
       // data is read all at once when process finishes
       break;
     case state::downloading:
        while (youtubeDl->canReadLine()) {
            handleDownloadInfo(QString::fromLocal8Bit(youtubeDl->readLine()));
        }
        break;
     default:
        break;
    }
}

void video::handleConversionFinished() {
    removeTempFiles();
    finalFilename = targetConverter->target;
    qDebug().noquote().nospace() << targetConverter->conversion_str() << " finished for '" << url << "'";
    state = state::finished;
    emit stateChanged();
}

void video::handleConversionProgress(double progress)
{
    conversion_progress = progress;
    emit stateChanged();
}

void video::handleConversionError(QString error) {
    qDebug().noquote().nospace() << "conversion error for '" << url << "': " << error;
    state = state::error;
    emit stateChanged();
}

void video::removeTempFiles() {
    const bool keep = QSettings().value("keep", false).toBool();
    if (keep)
    {
        QString fn;
        for (int i = 0; i < downloadFilenames.size(); i++) {
            fn = downloadFilenames.at(i) + ".part";
            if ( !fn.isEmpty() && QFile::exists(fn) )
                qDebug().noquote() << "removing temp file " << fn;
        }
        for (int i = 0; i < downloadFilenames.size(); i++) {
            fn = downloadFilenames.at(i);
            if ( !fn.isEmpty() && QFile::exists(fn) )
                qDebug().noquote() << "keeping temp file " << fn;
        }
        fn = finalDownloadFilename;
        if ( !fn.isEmpty() && QFile::exists(fn) )
            qDebug().noquote() << "keeping file " << fn;
        return;
    }
    for (int i = 0; i < downloadFilenames.size(); i++) {
        QFile::remove(downloadFilenames.at(i));
        QFile::remove(downloadFilenames.at(i) + ".part");
    }
    QFile::remove(finalDownloadFilename);
}
