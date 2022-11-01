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



#ifndef CONVERTER_FFMPEG_H
#define CONVERTER_FFMPEG_H

#include "converter.h"
#include <QSysInfo>
#include <QApplication>

class ffmpegThread : public QThread
{
    public:
    QFile* inputFile;
    QString target;
    QString container;
    QString metaTitle;
    QString metaArtist;
    QString audio_bitrate;
    QString audio_quality;
    bool audio_to_mono;
    QStringList acceptedAudioCodec;
    QStringList acceptedVideoCodec;
    QString ffmpegCall;
    QProcess* ffmpeg;

    QList<QFile*> concatFiles;
    QFile* concatTarget;
    QString originalFormat;

    void run();

};

class converter_ffmpeg : public converter
{
Q_OBJECT
public:
    converter_ffmpeg();

    static const char * name;
    static const char * executable;
    static const char * homepage_url;
    static const char * homepage_short;
    static const char * releases;


    converter* createNewInstance();
    void startConversion(
        QFile* file,
        QString& target,
        QString originalExtension,
        QString metaTitle,
        QString metaArtist,
        int mode,
        QString audio_bitrate,
        QString audio_quality
        );
    void concatenate(QList<QFile*> files, QFile* target, QString originalFormat);
    bool isAvailable();
    QString getExtensionForMode(int mode) const;
    bool isAudioOnly(int mode) const;
    bool isMono(int /*mode*/) const;
    bool hasMetaInfo(int /*mode*/) const;

private:
    void parseVersion(QString path, QString output);

    ffmpegThread ffmpeg;

    enum Mode {
        mode_mp4 = 0,
        mode_wmv,
        mode_ogg,
        mode_audio,
        mode_mp3,
        mode_ogg_audio,
        mode_pcm,
        mode_pcm_mono
    };

signals:
    void ffmpegPathAndVersion(QString path, QString version);

public slots:
    void emitFinished();
};

#endif // CONVERTER_FFMPEG_H
