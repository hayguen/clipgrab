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


#include "converter_ffmpeg.h"

const char * converter_ffmpeg::name = "FFmpeg";
#if defined(Q_OS_WIN)
const char * converter_ffmpeg::executable = "ffmpeg.exe";
#else
const char * converter_ffmpeg::executable = "ffmpeg";
#endif
const char * converter_ffmpeg::homepage_url = "https://github.com/BtbN/FFmpeg-Builds";
const char * converter_ffmpeg::homepage_short = "github.com/BtbN/FFmpeg-Builds";
const char * converter_ffmpeg::releases = "https://api.github.com/repos/BtbN/FFmpeg-Builds/releases/latest";


ffmpegThread::ffmpegThread(converter_ffmpeg * _converter)
    : re_duration("^DURATION\\s+:\\s+(\\d\\d):(\\d\\d):(\\d\\d.?\\d*)\\s*$")
    , re_progress("^frame=\\s*\\d+\\s+fps=\\d+.?\\d*\\s+q=\\d+.?\\d*\\s+L?size=\\s*\\d+\\w+\\s+time=(\\d\\d):(\\d\\d):(\\d\\d.?\\d*)\\s+")
    , duration_s(-1.0)
    , progress_s(-1.0)
    , progress_percent(-1.0)
    , progress_permille(-1)
{
    // "DURATION        : 01:02:21.820000000"
    // "frame= 3116 fps=778 q=3.1 size=    4352kB time=00:02:10.41 bitrate= 273.4kbits/s speed=32.6x"
    converter = _converter;
}

void ffmpegThread::run()
{
    QSettings settings;
    QString videoCodec;
    QString videoBitrate;
    QString audioCodec;

    QObject* parent = new QObject;

    bool audioCodecAccepted = false;
    bool videoCodecAccepted = false;

    ffmpegCall = settings.value("ffmpegPath", "ffmpeg").toString() + " -y";

    if (!concatFiles.empty())
    {
        for (int i=0; i < concatFiles.size(); i++)
        {
            ffmpegCall.append(" -i \"" + concatFiles.at(i)->fileName() + "\"" );
        }
        ffmpegCall.append(" -acodec copy -vcodec copy -f " + originalFormat.split(".").at(1) + " \"" + concatTarget->fileName() + "\"");
    }
    else
    {
        ffmpegCall.append(" -i \"" + inputFile->fileName() + "\"" );


        ffmpeg = new QProcess(parent);
        ffmpeg->start(ffmpegCall);
        ffmpeg->waitForFinished(-1);
        QString videoInfo = ffmpeg->readAllStandardError();
        ffmpeg->close();

        QRegExp expression;
        expression = QRegExp("Audio: (.*)\\n");
        expression.setMinimal(true);
        if (expression.indexIn(videoInfo) !=-1)
        {
            audioCodec = expression.cap(1);
        }
        expression = QRegExp("Video: (.*)\\n");
        expression.setMinimal(true);
        if (expression.indexIn(videoInfo) !=-1)
        {
            videoCodec = expression.cap(1);
        }

        expression = QRegExp("Video:.*([0-9]+) kb/s");
        expression.setMinimal(true);
        if (expression.indexIn(videoInfo) !=-1)
        {
            videoBitrate = expression.cap(1);
        }

        qDebug() << "Source video: " << videoCodec << videoBitrate << audioCodec;
        qDebug() << "Target video: " << acceptedVideoCodec << acceptedAudioCodec;

        for (int i = 0; i < acceptedAudioCodec.size(); ++i)
        {
            if (audioCodec.contains(acceptedAudioCodec[i]))
            {
                audioCodecAccepted = true;
            }
        }

        for (int i = 0; i < acceptedVideoCodec.size(); ++i)
        {
            if (videoCodec.contains(acceptedVideoCodec[i]))
            {
                videoCodecAccepted = true;
            }
        }

        if (acceptedAudioCodec[0] == "none")
        {
            ffmpegCall = ffmpegCall + " -an";  // disable audio
        }
        else
        {
            if ( audio_to_mono )
            {
                ffmpegCall = ffmpegCall + " -ac 1";
            }

            if (audioCodecAccepted == true)
            {
                ffmpegCall = ffmpegCall + " -acodec copy";
            }
            else
            {
                if (acceptedAudioCodec[0] == "wav")
                {
                    // just plain uncompressed wav"
                }
                else if (acceptedAudioCodec[0] == "libvorbis")
                {
                    bool conv_ok;
                    QString opt_str = audio_quality.trimmed();
                    int audio_qual = opt_str.toInt(&conv_ok);
                    QString qual_opt = (!opt_str.isEmpty() && conv_ok && 1 <= audio_qual && audio_qual <= 9) ? opt_str : "9";
                    qDebug() << "ffmpegThread::run(): accepting audio codec libvorbis with audio quality: -aq " << qual_opt;
                    if (audioCodec.contains("libvorbis"))
                    {
                        ffmpegCall = ffmpegCall + " -acodec libvorbis -aq " + qual_opt;
                    }
                    else
                    {
                        ffmpegCall = ffmpegCall + " -acodec vorbis -aq " + qual_opt + " -strict experimental";
                    }
                }
                else
                {
                    bool conv_ok;
                    QString opt_str = audio_bitrate.trimmed();
                    int rate = opt_str.toInt(&conv_ok);
                    QString rate_opt = (!opt_str.isEmpty() && conv_ok && 4 <= rate && rate <= 384) ? opt_str : "256";
                    rate_opt = rate_opt + "k";
                    qDebug() << "ffmpegThread::run(): accepting/using audio codec " << acceptedAudioCodec[0] << " of " << acceptedAudioCodec;
                    qDebug() << "ffmpegThread::run(): accepting/using audio codec .. with bitrate " + rate_opt;
                    ffmpegCall = ffmpegCall + " -acodec " + acceptedAudioCodec[0] + " -ab " + rate_opt;
                }
            }
        }

        if (acceptedVideoCodec[0] == "none")
        {
            ffmpegCall = ffmpegCall + " -vn";  // disable video
        }
        else
        {
            if (videoCodecAccepted == true)
            {
                ffmpegCall = ffmpegCall + " -vcodec copy";
            }
            else
            {
                if (videoBitrate.toInt() > 100)
                {
                    ffmpegCall = ffmpegCall + " -vb " + QString::number(videoBitrate.toInt()*1.2) + "k" + " -vcodec " + acceptedVideoCodec[0];
                }
                else
                {
                    ffmpegCall = ffmpegCall + " -vcodec " + acceptedVideoCodec[0];
                }
            }
        }

        ffmpegCall = ffmpegCall + " -metadata title=\"" + metaTitle + "\"";
        ffmpegCall = ffmpegCall + " -metadata author=\"" + metaArtist + "\"";
        ffmpegCall = ffmpegCall + " -metadata artist=\"" + metaArtist + "\"";

        //Determine container format if not given
        if (container.isEmpty())
        {
            if (acceptedVideoCodec[0] == "none")
            {
                if (audioCodec.contains("libvorbis") || audioCodec.contains("vorbis"))
                {
                    container = "ogg";
                }
                else if (audioCodec.contains("libmp3lame") || audioCodec.contains("mp3"))
                {
                    container = "mp3";
                }
                else if (audioCodec.contains("libvo_aacenc") || audioCodec.contains("aac"))
                {
                    container = "m4a";
                }
                else if (audioCodec.contains("opus")) {
                    container = "opus";
                }
                else {
                    container = "wav";  // last resort
                }

            }
        }

        //Make sure not to overwrite existing files
        QDir fileCheck;
        if (fileCheck.exists(target + "." + container))
        {
            int i = 1;
            while (fileCheck.exists(target + "-" + QString::number(i) + container))
            {
                i++;
            }
            target.append("-");
            target.append(QString::number(i));
        }

        target.append("." + container);
        ffmpegCall = ffmpegCall + " \"" + target + "\"";
    }

    qDebug().quote() << "Executing ffmpeg: " << ffmpegCall;

    ffmpeg = new QProcess(parent);
    duration_s = -1.0;
    progress_s = -1.0;
    progress_percent = 0.0;
    progress_permille = 0;
    auto readLines = [=](bool is_stdout) {
        ffmpeg->setReadChannel(is_stdout ? QProcess::StandardOutput : QProcess::StandardError);
        while (true)
        {
            QByteArray rd = ffmpeg->readLine();
            if (!rd.size())
                break;
            QString line = QString::fromLocal8Bit(rd).trimmed();

            if ( duration_s < 0.0 ) {
                QRegularExpressionMatch match = re_duration.match(line);
                if (match.hasMatch()) {
                    bool h_ok, m_ok, s_ok;
                    int hours = match.captured(1).toInt(&h_ok);
                    int mins = match.captured(2).toInt(&m_ok);
                    double secs = match.captured(3).toDouble(&s_ok);
                    if ( h_ok && m_ok && s_ok
                        && hours >= 0 && hours <= 24
                        && mins >= 0 && mins <= 60
                        && secs >= 0.0 && secs <= 60.0
                        && hours * 60.0 * 60.0 + mins * 60.0 + secs > 0.0
                        ) {
                        duration_s = hours * 60.0 * 60.0 + mins * 60.0 + secs;
                        qDebug().noquote() << "video duration to convert: " << QString::number(duration_s, 'f', 1) << " sec";
                        continue;
                    }
                }
            }
            else
            {
                QRegularExpressionMatch match = re_progress.match(line);
                if (match.hasMatch()) {
                    bool h_ok, m_ok, s_ok;
                    int hours = match.captured(1).toInt(&h_ok);
                    int mins = match.captured(2).toInt(&m_ok);
                    double secs = match.captured(3).toDouble(&s_ok);
                    if ( h_ok && m_ok && s_ok && hours >= 0 && hours <= 24 && mins >= 0 && mins <= 60 && secs >= 0.0 && secs <= 60.0 ) {
                        progress_s = hours * 60.0 * 60.0 + mins * 60.0 + secs;
                        progress_percent = progress_s * 100.0 / duration_s;
                        int per_mille = int(progress_percent * 10.0 + 0.5);
                        if (progress_permille < per_mille) {
                            progress_permille = per_mille;
                            //qDebug().noquote() << "conversion progress: " << QString::number(progress_s, 'f', 1) << "sec:" << QString::number(progress_percent, 'f', 1) << "%";
                            emit converter->progress(progress_percent);
                        }
                        continue;
                    }
                }
            }

            if (is_stdout)
                qDebug().noquote() << "ffmpeg stdout: " << line;
            else
                qDebug().noquote() << "ffmpeg stderr: " << line;
        }
    };
    connect(ffmpeg , &QProcess::readyReadStandardOutput, this, [=]() {
        readLines(true);
    });
    connect(ffmpeg , &QProcess::readyReadStandardError, this, [=]() {
        readLines(false);
    });
    ffmpeg->start(ffmpegCall);
    ffmpeg->waitForFinished(-1);
    ffmpeg->close();

    // patch added for Qt Version 4.5.0 by Günther Bauer
    #if QT_VERSION == 0x040500
        QFile::remove(randomFile);
    #endif
    // end of patch
};

converter_ffmpeg::converter_ffmpeg()
    : ffmpeg(this)
{
    QSettings settings;
    this->_modes.append(tr("MPEG4"));
    this->_modes.append(tr("WMV (Windows)"));
    this->_modes.append(tr("OGG Theora"));
    this->_modes.append(tr("Original (audio only)"));
    this->_modes.append(tr("MP3 (audio only)"));
    this->_modes.append(tr("OGG Vorbis (audio only)"));
    this->_modes.append(tr("PCM/WAV (audio only)"));
    this->_modes.append(tr("PCM/WAV (mono, audio only)"));
}

QString converter_ffmpeg::getExtensionForMode(int mode) const
{
    switch ((Mode)mode)
    {
        case mode_mp4:
            return "mp4";
        case mode_wmv:
            return "wmv";
        case mode_ogg:
            return "ogg";
        case mode_audio:
            return "";
        case mode_mp3:
            return "mp3";
        case mode_ogg_audio:
            return "ogg";
        case mode_pcm:
        case mode_pcm_mono:
            return "wav";
    }
    return "";
}

bool converter_ffmpeg::isAudioOnly(int mode) const {
    switch ((Mode)mode)
    {
        case mode_audio:
        case mode_mp3:
        case mode_ogg_audio:
        case mode_pcm:
        case mode_pcm_mono:
            return true;
        default:
        case mode_mp4:
        case mode_wmv:
        case mode_ogg:
            return false;
    }
}

bool converter_ffmpeg::isMono(int mode) const {
    switch ((Mode)mode)
    {
    case mode_pcm_mono:
        return true;
    default:
    case mode_mp4:
    case mode_wmv:
    case mode_ogg:
    case mode_audio:
    case mode_mp3:
    case mode_ogg_audio:
    case mode_pcm:
        return false;
    }
}

bool converter_ffmpeg::hasMetaInfo(int mode) const {
    switch ((Mode)mode)
    {
    case mode_mp3:
    case mode_ogg_audio:
        return true;
    case mode_mp4:
    case mode_wmv:
    case mode_ogg:
    case mode_audio:
    case mode_pcm:
    case mode_pcm_mono:
        return false;
    }
    return false;
}



void converter_ffmpeg::startConversion(
    QFile* inputFile,
    QString& target,
    QString /*originalExtension*/,
    QString metaTitle,
    QString metaArtist,
    int mode,
    QString audio_bitrate,
    QString audio_quality
    )
{
    QStringList acceptedAudio;
    QStringList acceptedVideo;
    QString container;
    bool audio_to_mono = isMono(mode);

    switch ((Mode)mode)
    {
    case mode_mp4:
        acceptedAudio <<  "aac" << "libvo_aacenc" << "mp3";
        acceptedVideo << "mpeg4" << "h264";
        container = "mp4";
        break;
    case mode_wmv:
        acceptedAudio << "wmav2";
        acceptedVideo << "wmv2";
        container = "wmv";
        break;
    case mode_ogg:
        acceptedAudio << "libvorbis" << "vorbis";
        acceptedVideo << "libtheora" << "theora";
        container = "ogv";
        break;
    case mode_mp3:
        acceptedAudio << "libmp3lame" << "mp3";
        acceptedVideo << "none";
        container = "mp3";
        break;
    case mode_ogg_audio:
        acceptedAudio << "libvorbis" << "vorbis";
        acceptedVideo << "none";
        container = "ogg";
        break;
    case mode_pcm:
    case mode_pcm_mono:
        acceptedAudio << "wav";
        acceptedVideo << "none";
        container = "wav";
        break;
    case mode_audio:
        acceptedAudio << "libvorbis" << "vorbis" << "libmp3lame" << "mp3" << "wmav2" << "libvo_aaenc" << "aac" << "opus";
        acceptedVideo << "none";
        container = "";
        break;
    }


    ffmpeg.inputFile = inputFile;
    ffmpeg.acceptedAudioCodec = acceptedAudio;
    ffmpeg.acceptedVideoCodec = acceptedVideo;
    ffmpeg.audio_to_mono = audio_to_mono;
    ffmpeg.metaTitle = metaTitle;
    ffmpeg.metaArtist = metaArtist;
    ffmpeg.audio_bitrate = audio_bitrate;
    ffmpeg.audio_quality = audio_quality;
    ffmpeg.target = target;
    ffmpeg.container = container;
    connect(&ffmpeg, SIGNAL(finished()), this, SLOT(emitFinished()));
    ffmpeg.start();

}

void converter_ffmpeg::concatenate(QList<QFile *> files, QFile *target, QString originalFormat)
{
    ffmpeg.concatFiles = files;
    ffmpeg.concatTarget = target;
    ffmpeg.originalFormat = originalFormat;
    connect(&ffmpeg, SIGNAL(finished()), this, SLOT(emitFinished()));
    ffmpeg.start();

}

converter* converter_ffmpeg::createNewInstance()
{
    return new converter_ffmpeg();
}

void converter_ffmpeg::emitFinished()
{
    this->target = ffmpeg.target;
    emit conversionFinished();
}

bool converter_ffmpeg::isAvailable()
{
    QSettings settings;

    QString ffmpegPath;
    QProcess testProcess;
    bool found_program = false;

#ifdef Q_OS_MAC
    ffmpegPath =  "\"" + QApplication::applicationDirPath() + "/ffmpeg\"";
#else
    for ( int k = 0; k < 3; ++k ) {
        if (k == 0) {
            QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            ffmpegPath = dir + "/" + converter_ffmpeg::executable;
            testProcess.start(ffmpegPath + " -v quiet");
        }
        else if ( k == 1) {
            ffmpegPath = QStandardPaths::findExecutable(converter_ffmpeg::executable);
            testProcess.start(ffmpegPath + " -v quiet");
        }
        else {
            ffmpegPath = QStandardPaths::findExecutable("avconv");
            testProcess.start(ffmpegPath + " -v quiet");
        }

        if (testProcess.waitForFinished())
        {
            found_program = true;
            break;
        }
    }
    if (!found_program) {
        emit error(tr("No installed version of avconv or ffmpeg coud be found. Converting files and downloading 1080p videos from YouTube is not supported."));
        settings.setValue("ffmpegPath", "");
        emit ffmpegPathAndVersion(QString::null, QString::null);
        return false;
    }
#endif

    testProcess.start(ffmpegPath + " -formats");
    testProcess.waitForFinished();
    QString outs = testProcess.readAllStandardOutput();
    QString errs = testProcess.readAllStandardError();

    settings.setValue("ffmpegPath", ffmpegPath);
    settings.setValue("DashSupported", (bool) outs.contains("hls"));
    if (settings.value("DashSupported", false) == false)
    {
        emit error(tr("The installed version of %1 is outdated.\nDownloading 1080p videos from YouTube is not supported.").arg(ffmpegPath));
    }

    parseVersion(ffmpegPath, outs+"\n"+errs);
    return true;
}


void converter_ffmpeg::parseVersion(QString path, QString output)
{
    QStringList lines = output.split("\n");
    const QString version("version");
    const QString copyright("Copyright");
    for (int i = 0; i < lines.size(); ++i)
    {
        if ( lines.at(i).contains(version))
        {
            QString line = lines.at(i).trimmed();
            int idx_ver = line.indexOf(version);
            if ( idx_ver >= 0 )
                line = line.mid(idx_ver + version.length());
            int idx_copy = line.indexOf(copyright);
            if ( idx_copy >= 0 )
                line = line.left(idx_copy);
            line = line.trimmed();
            if ( idx_ver >= 0 && idx_copy >= 0 && line.length() > 0 ) {
                qDebug() << "ffmpeg path: " << path;
                qDebug() << "ffmpeg version: " << line;
                emit ffmpegPathAndVersion(path, line);
                return;
            }
        }
    }
    emit ffmpegPathAndVersion(path, QString::null);
}
