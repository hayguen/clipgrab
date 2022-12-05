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
    : ffmpeg_info_run(this)
    , ffmpeg_conv_run(this)
    , re_duration("^duration\\s*:\\s*(\\d\\d):(\\d\\d):(\\d\\d.?\\d*),?\\s*", QRegularExpression::CaseInsensitiveOption)
    , re_progress("^frame=\\s*\\d+\\s+fps=\\s*\\d+.?\\d*\\s+q=\\s*-?\\d+.?\\d*\\s+L?size=\\s*\\d+\\w+\\s+time=(\\d\\d):(\\d\\d):(\\d\\d.?\\d*)\\s+", QRegularExpression::CaseInsensitiveOption)
    , duration_s(-1.0)
    , progress_s(-1.0)
    , progress_percent(-1.0)
    , progress_permille(-1)
    , re_audio_codec("Audio: (.*)\\n")
    , re_video_codec("Video: (.*)\\n")
    , re_video_bitrate("Video:.*([0-9]+) kb/s")
    , audioCodecAccepted(false)
    , videoCodecAccepted(false)
    , is_aborted(false)
{
    // re_duration example lines:
    // "DURATION        : 01:02:21.820000000"
    // "Duration: 00:03:28.73, start: 0.000000, bitrate: 9069 kb/s"
    //
    // re_progress example lines:
    // "frame= 3116 fps=778 q=3.1 size=    4352kB time=00:02:10.41 bitrate= 273.4kbits/s speed=32.6x"
    // "frame= 4706 fps= 15 q=27.0 size=  247552kB time=00:03:16.39 bitrate=10325.9kbits/s speed=0.61x"
    // "... q= -1.0 ..."
    converter = _converter;
}


void ffmpegThread::eval_info_run(const QString& videoInfo)
{
    bool verbose = QSettings().value("verbose", false).toBool();

    re_audio_codec.setMinimal(true);
    if (re_audio_codec.indexIn(videoInfo) !=-1)
    {
        audioCodec = re_audio_codec.cap(1);
        if (verbose) {
            QFile infos("audio_codecs.txt");
            if ( infos.open(QIODevice::Append | QIODevice::Text) ) {
                infos.write((audioCodec+"\n").toUtf8());
                infos.close();
            }
        }
    }

    re_video_codec.setMinimal(true);
    if (re_video_codec.indexIn(videoInfo) !=-1)
    {
        videoCodec = re_video_codec.cap(1);
        if (verbose) {
            QFile infos("video_codecs.txt");
            if ( infos.open(QIODevice::Append | QIODevice::Text) ) {
                infos.write((videoCodec+"\n").toUtf8());
                infos.close();
            }
        }
    }

    re_video_bitrate.setMinimal(true);
    if (re_video_bitrate.indexIn(videoInfo) !=-1)
    {
        videoBitrate = re_video_bitrate.cap(1);
    }

    for (int i = 0; i < acceptedAudioCodec.size(); ++i)
    {
        if (audioCodec.contains(acceptedAudioCodec[i]))
        {
            audioCodecAccepted = true;
            break;
        }
    }

    for (int i = 0; i < acceptedVideoCodec.size(); ++i)
    {
        if (videoCodec.contains(acceptedVideoCodec[i]))
        {
            videoCodecAccepted = true;
            break;
        }
    }

    QString vCodecAccept;
    QString aCodecAccept;

    if ( videoCodecAccepted )
    {
        if (acceptedVideoCodec[0] == "none")
            vCodecAccept = " removing video.";
        else
            vCodecAccept = " copying video codec without conversion";
    }
    else
        vCodecAccept = " will convert to video codec " + targetVideoCodec;

    if ( audioCodecAccepted )
    {
        if (acceptedAudioCodec[0] == "none")
            aCodecAccept = " removing audio.";
        else
            aCodecAccept = " copying audio codec without conversion";
    }
    else
        aCodecAccept = " will convert to audio codec " + targetAudioCodec;


    qDebug().noquote().nospace() << "Source video codec:   " << videoCodec;
    qDebug().noquote().nospace() << "Source audio codec:   " << audioCodec;
    qDebug().noquote().nospace() << "Source video bitrate: " << videoBitrate;
    qDebug().noquote().nospace() << "Target accepts following video codecs: " << acceptedVideoCodec << vCodecAccept;
    qDebug().noquote().nospace() << "Target accepts following audio codecs: " << acceptedAudioCodec << aCodecAccept;
}

void ffmpegThread::setup_conv_audio()
{
    if (acceptedAudioCodec[0] == "none")
    {
        ffmpegArgs << "-an";    // disable audio
    }
    else
    {
        if ( audio_to_mono )
        {
            ffmpegArgs << "-ac" << "1";
        }

        if (audioCodecAccepted == true)
        {
            ffmpegArgs << "-acodec" << "copy";
        }
        else
        {
            if (targetAudioCodec == "wav")
            {
                // just plain uncompressed wav"
            }
            else if (targetAudioCodec == "libvorbis")
            {
                bool conv_ok;
                QString opt_str = audio_quality.trimmed();
                int audio_qual = opt_str.toInt(&conv_ok);
                QString qual_opt = (!opt_str.isEmpty() && conv_ok && 1 <= audio_qual && audio_qual <= 9) ? opt_str : "9";
                qDebug().noquote().nospace() << "ffmpegThread::run(): encoding audio with 'libvorbis' and audio quality '-aq " << qual_opt << "'";
                if (audioCodec.contains("libvorbis"))
                {
                    ffmpegArgs << "-acodec" << "libvorbis" << "-aq" << qual_opt;
                }
                else
                {
                    ffmpegArgs << "-acodec" << "vorbis" << "-aq" << qual_opt << "-strict" << "experimental";
                }
            }
            else
            {
                bool conv_ok;
                QString opt_str = audio_bitrate.trimmed();
                int rate = opt_str.toInt(&conv_ok);
                QString rate_opt = (!opt_str.isEmpty() && conv_ok && 4 <= rate && rate <= 384) ? opt_str : "256";
                rate_opt = rate_opt + "k";
                qDebug().noquote().nospace() << "ffmpegThread::run(): encoding audio with '" << targetAudioCodec << "' with bitrate '-ab " << rate_opt << "'";
                ffmpegArgs << "-acodec" << targetAudioCodec << "-ab" << rate_opt;
            }
        }
    }
}

void ffmpegThread::setup_conv_video()
{
    if (acceptedVideoCodec[0] == "none")
    {
        ffmpegArgs << "-vn";    // disable video
    }
    else
    {
        if (videoCodecAccepted == true)
        {
            ffmpegArgs << "-vcodec" << "copy";
        }
        else
        {
            if (videoBitrate.toInt() > 100)
            {
                ffmpegArgs << "-vb" << (QString::number(videoBitrate.toInt()*1.2) + "k") << "-vcodec" << targetVideoCodec;
            }
            else
            {
                ffmpegArgs << "-vcodec" << targetVideoCodec;
            }
        }
    }

    ffmpegArgs << "-metadata" << "title=\"" + metaTitle + "\"";
    ffmpegArgs << "-metadata" << "author=\"" + metaArtist + "\"";
    ffmpegArgs << "-metadata" << "artist=\"" + metaArtist + "\"";

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
    ffmpegArgs << target;
}

void ffmpegThread::abortConversion()
{
    is_aborted = true;
    ffmpeg_info_run.kill();
    ffmpeg_conv_run.kill();
}


void ffmpegThread::run()
{
    QSettings settings;
    const QString ffmpegPath = settings.value("ffmpegPath", "ffmpeg").toString();

    is_aborted = false;
    ffmpeg_info_run.close();
    ffmpeg_conv_run.close();

    ffmpegArgs.clear();
    ffmpegArgs << "-y";

    if (!concatFiles.empty())
    {
        for (int i=0; i < concatFiles.size(); i++)
        {
            ffmpegArgs << "-i" << concatFiles.at(i)->fileName();
        }
        ffmpegArgs << "-acodec" << "copy";
        ffmpegArgs << "-vcodec" << "copy";
        ffmpegArgs << "-f" << originalFormat.split(".").at(1);  // force format
        ffmpegArgs << concatTarget->fileName();
    }
    else
    {
        ffmpegArgs << "-i" << inputFile->fileName();

        qDebug().noquote().nospace() << "\nStarting FFmpeg command: " << ffmpegPath << " " << ffmpegArgs;
        ffmpeg_info_run.start(ffmpegPath, ffmpegArgs);

        ffmpeg_info_run.waitForFinished(-1);
        const QString videoInfo = ffmpeg_info_run.readAllStandardError();
        ffmpeg_info_run.close();

        if ( is_aborted )
            return;

        eval_info_run(videoInfo);
        setup_conv_audio();
        setup_conv_video();
    }

    duration_s = -1.0;
    progress_s = -1.0;
    progress_percent = 0.0;
    progress_permille = 0;
    auto readLines = [=](bool is_stdout) {
        ffmpeg_conv_run.setReadChannel(is_stdout ? QProcess::StandardOutput : QProcess::StandardError);
        while (true)
        {
            if ( is_aborted )
                return;
            QByteArray rd = ffmpeg_conv_run.readLine();
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
            QRegularExpressionMatch match = re_progress.match(line);
            if (match.hasMatch()) {
                if ( duration_s <= 0.0 )
                    continue;
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
                        converter_ffmpeg& c = *converter;   // why does converter->report_progress() or emit converter->... produce C2059 !?
                        c.report_progress(progress_percent);
                    }
                    continue;
                }
            }

            if (is_stdout)
                qDebug().noquote() << "ffmpeg stdout: " << line;
            else
                qDebug().noquote() << "ffmpeg stderr: " << line;
        }
    };
    connect(&ffmpeg_conv_run, &QProcess::readyReadStandardOutput, this, [=]() {
        readLines(true);
    });
    connect(&ffmpeg_conv_run, &QProcess::readyReadStandardError, this, [=]() {
        readLines(false);
    });
    qDebug().noquote().nospace() << "\nStarting FFmpeg command: " << ffmpegPath << " " << ffmpegArgs;
    ffmpeg_conv_run.start(ffmpegPath, ffmpegArgs);

    ffmpeg_conv_run.waitForFinished(-1);
    ffmpeg_conv_run.close();

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
    this->_modes.append(tr("MPEG4/AVC (H.264)"));
    this->_modes.append(tr("MPEG4/HEVC (H.265)"));
    this->_modes.append(tr("WebM/VP8"));
    this->_modes.append(tr("WebM/VP9"));
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
        case mode_mp4_avc:
        case mode_mp4_hevc:
            return "mp4";
        case mode_webm_vp8:
        case mode_webm_vp9:
            return "webm";
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
        case mode_mp4_avc:
        case mode_mp4_hevc:
        case mode_webm_vp8:
        case mode_webm_vp9:
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
    case mode_mp4_avc:
    case mode_mp4_hevc:
    case mode_webm_vp8:
    case mode_webm_vp9:
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
    case mode_mp4_avc:
    case mode_mp4_hevc:
    case mode_webm_vp8:
    case mode_webm_vp9:
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
    QSettings settings;
    QStringList acceptedAudio;
    QStringList acceptedVideo;
    QString targetVideo;
    QString targetAudio;
    QString container;
    bool audio_to_mono = isMono(mode);

    switch ((Mode)mode)
    {
    case mode_mp4:
    case mode_mp4_avc:
    case mode_mp4_hevc:
        acceptedAudio <<  "aac" << "libvo_aacenc" << "mp3";
        acceptedVideo << "mpeg4";
        if (settings.value("mp4_accepts_h264", true).toBool())
            acceptedVideo << "h264";
        if (settings.value("mp4_accepts_av1", true).toBool())
            acceptedVideo << "av1";
        container = "mp4";
        if ( mode == mode_mp4_avc )
            targetVideo = "libx264";
        if ( mode == mode_mp4_hevc )
            targetVideo = "libx265";
        break;
    case mode_webm_vp8:
    case mode_webm_vp9:
        acceptedAudio << "opus"; // "libvorbis" << "vorbis" << "opus";
        acceptedVideo << "vp8" << "vp9";
        container = "webm";
        if ( mode == mode_webm_vp8 )
            targetVideo = "libvpx";
        if ( mode == mode_webm_vp9 )
            targetVideo = "libvpx-vp9";
        targetAudio = "libopus";
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

    if ( targetAudio.isEmpty() )
        targetAudio = acceptedAudio[0];
    if ( targetVideo.isEmpty() )
        targetVideo = acceptedVideo[0];

    ffmpeg.inputFile = inputFile;
    ffmpeg.acceptedAudioCodec = acceptedAudio;
    ffmpeg.acceptedVideoCodec = acceptedVideo;
    ffmpeg.targetAudioCodec = targetAudio;
    ffmpeg.targetVideoCodec = targetVideo;
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

void converter_ffmpeg::abortConversion()
{
    ffmpeg.abortConversion();
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
            qDebug().noquote() << "\nStarting FFmpeg test: " << ffmpegPath + " -v quiet";
            testProcess.start(ffmpegPath + " -v quiet");
        }
        else if ( k == 1) {
            ffmpegPath = QStandardPaths::findExecutable(converter_ffmpeg::executable);
            qDebug().noquote() << "\nStarting FFmpeg test: " << ffmpegPath + " -v quiet";
            testProcess.start(ffmpegPath + " -v quiet");
        }
        else {
            ffmpegPath = QStandardPaths::findExecutable("avconv");
            qDebug().noquote() << "\nStarting avconv test: " << ffmpegPath + " -v quiet";
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

    qDebug().noquote() << "\nStarting/checking FFmpeg formats: " << ffmpegPath + " -formats";
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
                qDebug().noquote() << "ffmpeg path: " << path;
                qDebug().noquote() << "ffmpeg version: " << line;
                emit ffmpegPathAndVersion(path, line);
                return;
            }
        }
    }
    emit ffmpegPathAndVersion(path, QString::null);
}

QString converter_ffmpeg::conversion_str() const
{
    return "conversion";
}
