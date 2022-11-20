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



#ifndef CLIPGRAB_H
#define CLIPGRAB_H

#include <QApplication>
#include <QtGui>
#include <QtNetwork>
#include <QtDebug>
#include <QtWidgets>

#include "video.h"
#include "converter.h"
#include "converter_ffmpeg.h"

#include "helper_downloader.h"

#if CLIPGRAB_ORG_UPDATER
#include <QtXml>
#endif
#if USE_WEBENGINE
#include "ui_update_message.h"
#include "message_dialog.h"
#endif

struct format
{
    converter* _converter;
    QString _name;
    int _mode;
};

struct language
{
    language(): isRTL(false) {}

    QString name;
    QString code;
    bool isRTL;
};


#if CLIPGRAB_ORG_UPDATER

struct updateInfo
{
    QString version;
    QString url;
    QString sha1;
    QDomNodeList domNodes;

    friend bool operator>(const updateInfo &a, const updateInfo &b)
    {
        return compare(a, b) > 0;
    }

    friend bool operator<(const updateInfo &a, const updateInfo &b)
    {
        return compare(a, b) < 0;
    }

    friend bool operator==(const updateInfo &a, const updateInfo &b)
    {
        return compare(a, b) == 0;
    }

    static int compare(const updateInfo &a, const updateInfo&b)
    {
        QRegExp regexp("^(\\d+)\\.(\\d+)\\.(\\d+)(-.*)?$");
        if (regexp.indexIn(a.version) > -1 && regexp.indexIn((b.version)) > -1)
        {
            regexp.indexIn(a.version);
            int majorA = regexp.cap(1).toInt();
            int minorA = regexp.cap(2).toInt();
            int patchA = regexp.cap(3).toInt();
            QString restA = regexp.cap(4);

            regexp.indexIn(b.version);
            int majorB = regexp.cap(1).toInt();
            int minorB = regexp.cap(2).toInt();
            int patchB = regexp.cap(3).toInt();
            QString restB = regexp.cap(4);

            if (majorA > majorB) return 1;
            if (majorA < majorB) return -1;
            if (minorA > minorB) return 1;
            if (minorA < minorB) return -1;
            if (patchA > patchB) return 1;
            if (patchA < patchB) return -1;
            if (restA.isEmpty() && !restB.isEmpty()) return 1;
            if (!restA.isEmpty() && restB.isEmpty()) return -1;
            return restA.compare(restB);
        }
        return b.version.compare(a.version);
    }
};

#endif

class ClipGrab : public QObject
{
    Q_OBJECT

    public:
    ClipGrab();

    static const char * version_url;   // check for line: VERSION = 3.9.6
    static const char * homepage_url;
    static const char * homepage_short;

    QList<format> formats;
    QList<video*> downloads;
    QList<converter*> converters;
    QSettings settings;
    QSystemTrayIcon trayIcon;
    bool isKnownVideoUrl(QString url) const;
    bool isUrl(QString url) const;
    QClipboard* clipboard;
    QString clipboardUrl;
    QString version;
    void openTargetFolder(video*);
    void openDownload(video*);

    QList<language> languages;

    video* getCurrentVideo();
    int getRunningDownloadsCount();
    QPair<qint64, qint64> getDownloadProgress();

#if CLIPGRAB_ORG_UPDATER
    void getUpdateInfo();
#endif
    void getProgramVersion();
    void getYtDlVersion();
    void getFFmpegReleases();

    void downloadYoutubeDl(bool force = false);
    void updateYoutubeDl();
    QProcess* runYouTubeDl(QStringList);
    void startYoutubeDlDownload();
    void startFFmpegDownload(QString url);
    void showDownloaderDlg(TypedHelperDownloader::DownloaderType dlgType);

    void clearTempFiles();

    void updateFFmpeg();
    QString ffmpegPath() const { return ffmpegPath_; }
    QString ffmpegVersion() const { return ffmpegVersion_; }

    // Helpers
    QString humanizeBytes(qint64);
    QString humanizeSeconds(qint64);

    protected:
#if USE_WEBENGINE
        QDialog* updateMessageDialog;
        Ui::UpdateMessage* updateMessageUi;
#endif
#if CLIPGRAB_ORG_UPDATER
        QList<updateInfo> availableUpdates;
#endif
        QTemporaryFile* updateFile;
        QDialog* helperDownloaderDialog;
        TypedHelperDownloader* helperDownloaderUi;
        QFile* youtubeDlFile;
        QFile* ffmpegFile;
        QNetworkReply* updateReply;
        video* currentVideo;
        video* currentSearch;
        QString youtubeDlPath;
        QProcess *youtubeDlUpdateProcess;
        converter_ffmpeg * conv_ffmpeg;
        QString ffmpegPath_;
        QString ffmpegVersion_;
        QList<QRegularExpression> re_video_urls;
        const QRegularExpression re_is_url;

    private slots:
#if CLIPGRAB_ORG_UPDATER
        void parseUpdateInfo(QNetworkReply* reply);
#endif
        void parseYtDlVersion(QNetworkReply* reply);
        void parseProgramVersion(QNetworkReply* reply);
        void parseFFmpegReleases(QNetworkReply* reply);
        void handleFFmpegPathAndVersion(QString path, QString version);

    public slots:
        void errorHandler(QString);
        void errorHandler(QString, video*);

        void fetchVideoInfo(const QString & url);
        void clearCurrentVideo();
        void enqueueDownload(video* video);
        void cancelAllDownloads();
        void search(QString keywords = "");
        void clipboardChanged();
        void activateProxySettings();

#if CLIPGRAB_ORG_UPDATER
        void startUpdateDownload();
        void skipUpdate();
        void updateDownloadFinished();
        void updateDownloadProgress(qint64, qint64);
        void updateReadyRead();
#endif

    signals:
        void currentVideoStateChanged(video*);
        void downloadEnqueued();
        void downloadFinished(video*);
        void searchFinished(video*);
        void youtubeDlDownloadFinished();
        void FFmpegDownloadFinished(QString filePath);
        void compatibleUrlFoundInClipboard(QString url);
        void UrlFoundInClipboard(QString url);
        void allDownloadsCanceled();
#if CLIPGRAB_ORG_UPDATER
        void updateInfoProcessed();
#endif
        void updateYtDlVersion(QString version);
        void updateFFmpegVersions(QStringList releases);
        void updateProgramVersion(QString version);
};

#endif // CLIPGRAB_H
