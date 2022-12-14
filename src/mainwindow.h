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



#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QShortcut>
#include <QSignalMapper>
#include <QUrl>
#include <QUrlQuery>

#if CLIPGRAB_ORG_UPDATER
#include <QtXml>
#endif
#if USE_WEBENGINE
#include <QtWebEngineWidgets>
#endif

#include <QFontDatabase>
#include "ui_mainwindow.h"
#include "ui_metadata-dialog.h"
#include "clipgrab.h"
#include "video.h"
#include "notifications.h"
#include "download_list_model.h"


#if USE_WEBENGINE
class SearchWebEngineUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
public:
    void interceptRequest(QWebEngineUrlRequestInfo &info) {
        if (info.requestUrl().toString().startsWith("https://m.youtube.com/watch?")) {
            info.block(true);
            QUrl url;
            url.setScheme("https");
            url.setHost("www.youtube.com");
            url.setPath("/watch");
            url.setQuery("v=" + QUrlQuery(info.requestUrl().query()).queryItemValue("v"));
            emit intercepted(url);
        }
    }

signals:
        void intercepted(const QUrl & url);
};
#endif

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(ClipGrab* cg, QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow();
    void init();
    void hide_logo();

    ClipGrab* cg;

public slots:
    void startDownload();
    void copyFromClipBoard();
    void activate_grab();
    void activate_format_cb();
    void activate_quality_cb();
    void compatibleUrlFoundInClipBoard(QString url);
    void UrlFoundInClipBoard(QString url);
    void targetFileSelected(video* video, QString target);

private:
    Ui::MainWindowClass ui;
     QSignalMapper *changeTabMapper;
     QSignalMapper *downloadMapper;
     Ui::MetadataDialog mdui;
     QDialog* metadataDialog;
     QSystemTrayIcon systemTrayIcon;
     QShortcut * grabShortcutA;
     QShortcut * grabShortcutB;
     QShortcut * pasteShortcut;
     QShortcut * formatShortcutA;
     QShortcut * formatShortcutB;
     QShortcut * qualityShortcutA;
     QShortcut * qualityShortcutB;
     QProcess  * ffmpegUnpackProcess;
     QString     ffmpegDownloadedArchive;

     void disableDownloadUi(bool disable=true);
     void disableDownloadTreeButtons(bool disable=true);
     void closeEvent(QCloseEvent* event);
     void timerEvent(QTimerEvent*);
     void changeEvent(QEvent *);
     void dragEnterEvent(QDragEnterEvent *event);
     void dropEvent(QDropEvent *event);
     bool updatingComboQuality;
     void updateYoutubeDlVersionInfo();
     void handleYtDlVersion(QString version);
     void handleProgramVersion(QString version);
     void handleFFmpegVersion(QString path, QString version, QStringList releases);
     void handleFFmpegDelete();
     void updateAvailableFormats();

private slots:
    void handleCurrentVideoStateChanged(video*);
    void handleFFmpegReleases(QStringList releases);
    void handleFFmpegDownloadFinished(QString filePath);
    void handleFFmpegUnpackFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_mainTab_currentChanged(int index);
    void on_downloadComboFormat_currentIndexChanged(int index);
    void on_settingsUseMetadata_stateChanged(int );
    void on_label_status_version_linkActivated(QString link);
    void on_downloadLineEdit_returnPressed();
    void on_settingsMinimizeToTray_stateChanged(int );
    void on_downloadPause_clicked();
    void on_settingsRemoveFinishedDownloads_stateChanged(int );
    void handle_downloadTree_currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void systemTrayMessageClicked();
    void systemTrayIconActivated(QSystemTrayIcon::ActivationReason);
    void on_downloadOpen_clicked();
    void on_settingsSaveLastPath_stateChanged(int );
    void on_downloadCancel_clicked();
    void on_settingsBrowseTargetPath_clicked();
    void on_settingsSavedPath_textChanged(QString );
    void on_settingsNeverAskForPath_stateChanged(int);
    void on_settingsAudioBitrate_textChanged(QString );
    void on_settingsAudioQuality_textChanged(QString );

    void settingsClipboard_toggled(bool);
    void settingsNotifications_toggled(bool);
    void settingsProxyChanged();

    void handleFinishedConversion(video*);
    void on_settingsLanguage_currentIndexChanged(int index);
    void on_buttonDonate_clicked();
    void on_settingsIgnoreSSLErrors_toggled(bool checked);
    void on_downloadTree_customContextMenuRequested(const QPoint &pos);
    void on_settingsRememberLogins_toggled(bool checked);
    void on_settingsRememberVideoQuality_toggled(bool checked);
    void on_downloadComboQuality_currentIndexChanged(int index);
    void on_downloadTree_doubleClicked(const QModelIndex &index);
    void on_settingsForceIpV4_toggled(bool checked);
    void on_settingsUpdateCheck_toggled(bool checked);
    void on_settingsShowLogo_toggled(bool checked);

    void on_settings_mp4_accepts_h264_toggled(bool checked);
    void on_settings_mp4_accepts_av1_toggled(bool checked);

    void on_ff_branch_currentIndexChanged(int index);
};

#endif // MAINWINDOW_H
