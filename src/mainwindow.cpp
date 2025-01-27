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



#include "mainwindow.h"

MainWindow::MainWindow(ClipGrab* cg, QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    this->ffmpegUnpackProcess = NULL;
    this->cg = cg;
    ui.setupUi(this);
}


MainWindow::~MainWindow()
{
}

void MainWindow::init()
{
    DownloadListModel* l = new DownloadListModel(cg, this);
    this->ui.downloadTree->setModel(l);
    // QTreeView::dataChanged() is no signal!
    //connect(ui.downloadTree, &QTreeView::dataChanged,[=] {
    //    handle_downloadTree_currentChanged(ui.downloadTree->currentIndex(), QModelIndex());
    //});
    connect(cg, &ClipGrab::downloadFinished, this, &MainWindow::handleFinishedConversion);

    //*
    //* Adding version info to the footer
    //*
    {
        QLabel * footer = this->ui.label_status_version;
        footer->setText(footer->text().replace("%version", "ClipGrab " + QCoreApplication::applicationVersion()));
#if CLIPGRAB_ORG_UPDATER == 0
        footer->hide();
#endif
    }

    //*
    //* Tray Icon
    //*
    systemTrayIcon.setIcon(QIcon(":/img/icon.png"));
    systemTrayIcon.show();
    connect(&systemTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::systemTrayIconActivated);
    connect(&systemTrayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::systemTrayMessageClicked);

    //*
    //* Clipboard Handling
    //*
    connect(cg, &ClipGrab::compatibleUrlFoundInClipboard, this, &MainWindow::compatibleUrlFoundInClipBoard );
    connect(cg, &ClipGrab::UrlFoundInClipboard, this, &MainWindow::UrlFoundInClipBoard );

    //*
    //* Download Tab
    //*
    connect(ui.downloadStart, &QPushButton::clicked, this, &MainWindow::startDownload);
    connect(ui.downloadLineEdit, &QLineEdit::textChanged, this, [=](const QString url) {
        if (!url.startsWith("http://") && !url.startsWith("https://")) return;

        ui.downloadLineEdit->setReadOnly(true);
        ui.downloadInfoBox->setText(tr("Please wait while ClipGrab is loading information about the video ..."));
        disableDownloadUi();
        cg->fetchVideoInfo(url);
    });
    connect(cg, &ClipGrab::currentVideoStateChanged, this, &MainWindow::handleCurrentVideoStateChanged);

    connect(ui.copy_from_clip, &QPushButton::clicked, this, [=]() {
        ui.downloadLineEdit->setText( ui.clipboard_content->text() );
    });

    connect(ui.downloadTree->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &MainWindow::handle_downloadTree_currentChanged);
    ui.downloadTree->header()->setStretchLastSection(false);
    ui.downloadTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui.downloadTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui.downloadTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui.downloadTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui.downloadTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    ui.downloadLineEdit->setFocus(Qt::OtherFocusReason);

    updateAvailableFormats();

    //*
    //* Settings Tab
    //*
    connect(this->ui.settingsRadioClipboardAlways, &QRadioButton::toggled, this, &MainWindow::settingsClipboard_toggled);
    connect(this->ui.settingsRadioClipboardNever, &QRadioButton::toggled, this, &MainWindow::settingsClipboard_toggled);
    connect(this->ui.settingsRadioClipboardAsk, &QRadioButton::toggled, this, &MainWindow::settingsClipboard_toggled);
    connect(this->ui.settingsRadioNotificationsAlways, &QRadioButton::toggled, this, &MainWindow::settingsNotifications_toggled);
    connect(this->ui.settingsRadioNotificationsFinish, &QRadioButton::toggled, this, &MainWindow::settingsNotifications_toggled);
    connect(this->ui.settingsRadioNotificationsNever, &QRadioButton::toggled, this, &MainWindow::settingsNotifications_toggled);


    this->ui.settingsSavedPath->setText(cg->settings.value("savedPath", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString());
    this->ui.settingsSaveLastPath->setChecked(cg->settings.value("saveLastPath", true).toBool());
    ui.settingsNeverAskForPath->setChecked(cg->settings.value("NeverAskForPath", false).toBool());
    QString audio_bitrate = cg->settings.value("FFmpeg-audio-bitrate", "256").toString();
    QString audio_quality = cg->settings.value("FFmpeg-audio-quality", "9").toString();
    this->ui.settingsAudioBitrate->setText(audio_bitrate);
    this->ui.settingsAudioQuality->setText(audio_quality);

    ui.settingsUseMetadata->setChecked(cg->settings.value("UseMetadata", false).toBool());
    connect(this->ui.settingsUseMetadata, &QCheckBox::stateChanged, this, &MainWindow::on_settingsUseMetadata_stateChanged);

    ui.settingsUseProxy->setChecked(cg->settings.value("UseProxy", false).toBool());
    ui.settingsProxyAuthenticationRequired->setChecked(cg->settings.value("ProxyAuthenticationRequired", false).toBool());
    ui.settingsProxyHost->setText(cg->settings.value("ProxyHost", "").toString());
    ui.settingsProxyPassword->setText(cg->settings.value("ProxyPassword", "").toString());
    ui.settingsProxyPort->setValue(cg->settings.value("ProxyPort", "").toInt());
    ui.settingsProxyUsername->setText(cg->settings.value("ProxyUsername", "").toString());
    ui.settingsProxyType->setCurrentIndex(cg->settings.value("ProxyType", 0).toInt());

    connect(this->ui.settingsUseProxy, &QCheckBox::toggled, this, &MainWindow::settingsProxyChanged);
    connect(this->ui.settingsProxyAuthenticationRequired, &QCheckBox::toggled, this, &MainWindow::settingsProxyChanged);
    connect(this->ui.settingsProxyHost, &QLineEdit::textChanged, this, &MainWindow::settingsProxyChanged);
    connect(this->ui.settingsProxyPassword, &QLineEdit::textChanged, this, &MainWindow::settingsProxyChanged);
    connect(this->ui.settingsProxyPort, SIGNAL(valueChanged(int)), this, SLOT(settingsProxyChanged()));
    connect(this->ui.settingsProxyUsername, &QLineEdit::textChanged, this, &MainWindow::settingsProxyChanged);
    connect(this->ui.settingsProxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(settingsProxyChanged()));
    settingsProxyChanged();
    ui.settingsMinimizeToTray->setChecked(cg->settings.value("MinimizeToTray", false).toBool());

    if (cg->settings.value("Clipboard", "ask") == "always")
    {
        ui.settingsRadioClipboardAlways->setChecked(true);
    }
    else if (cg->settings.value("Clipboard", "ask") == "never")
    {
        ui.settingsRadioClipboardNever->setChecked(true);
    }
    else
    {
        ui.settingsRadioClipboardAsk->setChecked(true);
    }

    if (cg->settings.value("Notifications", "finish") == "finish")
    {
        ui.settingsRadioNotificationsFinish->setChecked(true);
    }
    else if (cg->settings.value("Notifications", "finish") == "always")
    {
        ui.settingsRadioNotificationsAlways->setChecked(true);
    }
    else
    {
        ui.settingsRadioNotificationsNever->setChecked(true);
    }

    ui.settingsRememberVideoQuality->setChecked(cg->settings.value("rememberVideoQuality", true).toBool());
    ui.settingsRememberLogins->hide();
//    ui.settingsRememberLogins->setChecked(cg->settings.value("rememberLogins", true).toBool());
    ui.settingsRemoveFinishedDownloads->setChecked(cg->settings.value("RemoveFinishedDownloads", false).toBool());
    ui.settingsIgnoreSSLErrors->hide();
//    ui.settingsIgnoreSSLErrors->setChecked(cg->settings.value(("IgnoreSSLErrors"), false).toBool());
    ui.settingsForceIpV4->setChecked(cg->settings.value("forceIpV4", false).toBool());
    ui.settingsUpdateCheck->setChecked(cg->settings.value("Check-Updates", true).toBool());
    ui.settingsShowLogo->setChecked(cg->settings.value("Show-Logo", true).toBool());
#if CLIPGRAB_ORG_UPDATER == 0
    ui.settingsUpdateCheck->hide();
    ui.settingsShowLogo->hide();
    on_settingsShowLogo_toggled(false);
#endif
#if HIDE_DONATION
    ui.labelSupport->hide();
    ui.buttonDonate->hide();
#endif

    ui.settings_mp4_accepts_h264->setChecked(cg->settings.value("mp4_accepts_h264", true).toBool());
    ui.settings_mp4_accepts_av1->setChecked(cg->settings.value("mp4_accepts_av1", true).toBool());

    int langIndex = 0;
    for (int i=0; i < cg->languages.count(); i++)
    {
        if (cg->languages.at(i).code == cg->settings.value("Language", "auto").toString())
        {
            langIndex = i;
        }
    }
    for (int i=0; i < cg->languages.count(); i++)
    {
        ui.settingsLanguage->addItem(cg->languages.at(i).name, cg->languages.at(i).code);
    }
    ui.settingsLanguage->setCurrentIndex(langIndex);


    this->ui.tabWidget->removeTab(2); //fixme!

    //*
    //* About Tab
    //*

    this->ui.labelThanks->setText(QApplication::translate(
        "MainWindowClass",
        "<h2>Thanks</h2>\n"
        "ClipGrab relies on the work of the Qt project, the ffmpeg team, and the youtube-dl team.<br>\n"
        "Visit <a href=\"https://www.qt.io\">qt.io</a>, <a href=\"https://ffmpeg.org\">ffmpeg.org</a>, and <a href=\"%1\">%2</a> for further information.",
        nullptr).arg(YoutubeDl::homepage_url, YoutubeDl::homepage_short));
    #ifdef Q_OS_MAC
        this->ui.downloadOpen->hide();
        this->cg->settings.setValue("Clipboard", "always");
        this->ui.generalSettingsTabWidget->removeTab(2);
        //FIXME this->setStyleSheet("#label_4{padding-top:25px}#label{font-size:10px}#centralWidget{background:#fff};#mainTab{margin-top;-20px};#label_4{padding:10px}#downloadInfoBox, #settingsGeneralInfoBox, #settingsLanguageInfoBox, #aboutInfoBox, #searchInfoBox{color: background: #00B4DE;}");
        // this->ui.label_4->setMinimumHeight(120);
    #endif
    updateYoutubeDlVersionInfo();

    //*
    //* Drag and Drop
    //*
    this->setAcceptDrops(true);

    //*
    //*Keyboard shortcuts
    //*
    QSignalMapper* tabShortcutMapper = new QSignalMapper(this);

    QShortcut* tabShortcutDownload = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_1), this);
    tabShortcutMapper->setMapping(tabShortcutDownload, 0);
    QShortcut* tabShortcutSettings = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_2), this);
    tabShortcutMapper->setMapping(tabShortcutSettings, 1);
    QShortcut* tabShortcutAbout = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_3), this);
    tabShortcutMapper->setMapping(tabShortcutAbout, 2);

    connect(tabShortcutDownload, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutSettings, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutAbout, SIGNAL(activated()), tabShortcutMapper, SLOT(map()));
    connect(tabShortcutMapper, SIGNAL(mapped(int)), this->ui.mainTab, SLOT(setCurrentIndex(int)));

    pasteShortcut = new QShortcut(QKeySequence::Paste, this);
    connect(pasteShortcut, SIGNAL(activated()), this, SLOT(copyFromClipBoard()));

    grabShortcutA = new QShortcut(QKeySequence("G"), this);
    grabShortcutB = new QShortcut(QKeySequence("Ctrl+G"), this);
    connect(grabShortcutA, &QShortcut::activated, this, &MainWindow::activate_grab);
    connect(grabShortcutB, &QShortcut::activated, this, &MainWindow::activate_grab);

    formatShortcutA = new QShortcut(QKeySequence("F"), this);
    formatShortcutB = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(formatShortcutA, &QShortcut::activated, this, &MainWindow::activate_format_cb);
    connect(formatShortcutB, &QShortcut::activated, this, &MainWindow::activate_format_cb);

    qualityShortcutA = new QShortcut(QKeySequence("Q"), this);
    qualityShortcutB = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(qualityShortcutA, &QShortcut::activated, this, &MainWindow::activate_quality_cb);
    connect(qualityShortcutB, &QShortcut::activated, this, &MainWindow::activate_quality_cb);

    const QString tt_stream = tr("Select the stream\n- with it's CODEC, provided at URL");
    const QString tt_save_as = tr("Select the container (file extension),\nthe acceptable CODECs, to save without conversion,\nor the targeted CODEC for conversion.\n('Original' is faster, saving CPU and energy)");
    ui.label_stream->setToolTip(tt_stream);
    ui.downloadComboQuality->setToolTip(tt_stream);
    ui.label_dn_format->setToolTip(tt_save_as);
    ui.downloadComboFormat->setToolTip(tt_save_as);


    //*
    //*Miscellaneous
    //*
    this->ui.mainTab->setCurrentIndex(cg->settings.value("MainTab", 0).toInt());

    //Prevent updating remembered resolution when updating programatically
    this->updatingComboQuality = false;

    #ifdef Q_OS_MACX
    //fixme
    //if ( QSysInfo::MacintoshVersion > QSysInfo::MV_10_8 )
    //{
        // fix Mac OS X 10.9 (mavericks) font issue
        // https://bugreports.qt-project.org/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
    //}
    #endif

    bool show_logo = cg->settings.value("Show-Logo", true).toBool();
    if (!show_logo) {
        ui.label_4->hide();
        ui.verticalSpacer_9->changeSize(10, 0);
    }


    this->ui.yt_version_check->setEnabled(false);
    this->ui.yt_update->setText(tr("Update"));
    this->ui.yt_update->setEnabled(true);

    QObject::connect(cg, &ClipGrab::updateYtDlVersion, this, &MainWindow::handleYtDlVersion );

    connect(this->ui.yt_version_check, &QPushButton::clicked, [=]() {
        this->ui.yt_version_check->setEnabled(false);
        this->cg->getYtDlVersion();
    });
    connect(this->ui.yt_update, &QPushButton::clicked, [=]() {
        this->cg->startYoutubeDlDownload();
    });
    connect(this->ui.yt_delete, &QPushButton::clicked, [=]() {
        QString youtubeDlPath = YoutubeDl::find();
        QFile::remove(youtubeDlPath);
        this->cg->getYtDlVersion();
    });

    QObject::connect(cg, &ClipGrab::youtubeDlDownloadFinished, this, [=]() {
        this->ui.yt_version_check->setEnabled(false);
        this->cg->getYtDlVersion();
    });


    QObject::connect(cg, &ClipGrab::updateFFmpegVersions, this, &MainWindow::handleFFmpegReleases );
    QObject::connect(cg, &ClipGrab::FFmpegDownloadFinished, this, &MainWindow::handleFFmpegDownloadFinished );

    connect(this->ui.ff_version_check, &QPushButton::clicked, [=]() {
        this->cg->updateFFmpeg();
        this->updateAvailableFormats();
        this->ui.ff_version_check->setEnabled(false);
        this->cg->getFFmpegReleases();
    });
    connect(this->ui.ff_update, &QPushButton::clicked, [=]() {
        QString branch = ui.ff_branch->currentText();
        QString url = ui.ff_branch->currentData().toString();
        qDebug().noquote() << "update FFmpeg branch " << branch << " from URL " << url;
        cg->startFFmpegDownload(url);
    });
    connect(this->ui.ff_delete, &QPushButton::clicked, this, &MainWindow::handleFFmpegDelete);


#if CLIPGRAB_ORG_UPDATER == 0
    QObject::connect(cg, &ClipGrab::updateProgramVersion, this, &MainWindow::handleProgramVersion );
    connect(this->ui.program_version_check, &QPushButton::clicked, [=]() {
        this->cg->getProgramVersion();
        this->ui.program_version_check->setEnabled(false);
    });
    handleProgramVersion(QString());
#else
    ui.program_version_check->hide();
    ui.labelProgramVersionInfo->hide();
    ui.verticalSpacer_buttons_program->hide();
    ui.verticalSpacer_program->hide();
#endif


    QTimer::singleShot(300, [=] {
        cg->clipboardChanged();
        this->cg->getYtDlVersion();
        this->handleFFmpegVersion( this->cg->ffmpegPath(), this->cg->ffmpegVersion(), QStringList() );
        this->cg->getFFmpegReleases();
#if CLIPGRAB_ORG_UPDATER == 0
        this->cg->getProgramVersion();
#endif
    });
}

void MainWindow::handleFFmpegDelete() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString executable = dir + "/" + converter_ffmpeg::executable;
    if ( QFile::exists(executable) )
        QFile::remove(executable);
    cg->updateFFmpeg();
    updateAvailableFormats();
    cg->getFFmpegReleases();
}

void MainWindow::updateAvailableFormats() {
    int lastFormat = cg->settings.value("LastFormat", 0).toInt();
    ui.downloadComboFormat->clear();
    for (int i = 0; i < cg->formats.size(); ++i) {
        ui.downloadComboFormat->addItem(cg->formats.at(i)._name);
    }
    this->ui.downloadComboFormat->setCurrentIndex(lastFormat);
}

void MainWindow::hide_logo() {
    ui.label_4->hide();
    ui.verticalSpacer_9->changeSize(10, 0);
}

void MainWindow::startDownload() {
    video* video = cg->getCurrentVideo();
    if (video  == nullptr) return;

    QString targetDirectory = cg->settings.value("savedPath", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();
    QString filename = video->getSafeFilename();

    if (cg->settings.value("NeverAskForPath", false).toBool() == false) {
       targetFileSelected(video, QFileDialog::getSaveFileName(this, tr("Select Target"), targetDirectory +"/" + filename));
    } else {
        targetFileSelected(video, targetDirectory + "/" + filename);
    }
}

void MainWindow::targetFileSelected(video* video, QString target)
{
    if (target.isEmpty()) return;

    if ( ! QFileInfo(target).absoluteDir().exists() ) {
        ui.settingsNeverAskForPath->setChecked(false);
        this->startDownload();
        return;
    }

    if (cg->settings.value("saveLastPath", true).toBool() == true) {
        QString targetDir = target;
        // targetDir.remove(targetDir.split("/", Qt::SkipEmptyParts).last()).replace(QRegExp("/+$"), "/");
        targetDir.remove(targetDir.split("/", QString::SkipEmptyParts).last()).replace(QRegExp("/+$"), "/");
        ui.settingsSavedPath->setText(targetDir);
    }

    const int formatIdx = ui.downloadComboFormat->currentIndex();
    if (cg->settings.value("UseMetadata", false).toBool() == true) {
        int mode = cg->formats.at(formatIdx)._mode;
        bool hasMetaInfo = cg->formats.at(formatIdx)._converter->hasMetaInfo(mode);
        if (hasMetaInfo) {
            qDebug() << "opening meta info dialog";
            metadataDialog = new QDialog;
            mdui.setupUi(metadataDialog);
            mdui.title->setText(video->getTitle());
            mdui.artist->setText(video->getArtist());
            metadataDialog->setModal(true);
            metadataDialog->exec();

            video->setMetaTitle(mdui.title->text());
            video->setMetaArtist(mdui.artist->text());

            delete metadataDialog;
        }
    }

    video->setQuality(ui.downloadComboQuality->currentIndex());
    QString audio_bitrate = cg->settings.value("FFmpeg-audio-bitrate", "256").toString();
    QString audio_quality = cg->settings.value("FFmpeg-audio-quality", "9").toString();
    video->setConverter(cg->formats.at(formatIdx)._converter, cg->formats.at(formatIdx)._mode, audio_bitrate, audio_quality);
    video->setTargetFilename(target);

    qDebug().noquote() << "enqueuing download to " << target;
    qDebug().noquote() << "  enqueued with bitrate " << audio_bitrate << " and quality " << audio_quality;

    cg->enqueueDownload(video);

    ui.downloadLineEdit->clear();
    cg->clearCurrentVideo();
}

void MainWindow::handleCurrentVideoStateChanged(video* video) {
    if (video != cg->getCurrentVideo()) return;

    if (video == nullptr) {
        ui.downloadInfoBox->setText(tr("Please enter the link to the video you want to download in the field below."));
        disableDownloadUi(true);
        return;
    }

    if (video->getState() == video::state::error) {
        ui.downloadInfoBox->setText(tr("No downloadable video could be found.<br />Maybe you have entered the wrong link or there is a problem with your connection."));
        ui.downloadLineEdit->setReadOnly(false);
        disableDownloadUi(true);
    }

    if (video->getState() != video::state::fetched) return;

    ui.mainTab->setCurrentIndex(0);
    disableDownloadUi(false);
    ui.downloadLineEdit->setReadOnly(false);
    ui.downloadInfoBox->setText("<strong>" + video->getTitle() + "</strong>");

    this->updatingComboQuality = true;
    ui.downloadComboQuality->clear();
    QList<videoQuality> qualities = video->getQualities();
    for (int i = 0; i < qualities.size(); i++) {
        QString reso_ext = QString("%1;;%2;;%3;;%4")
                               .arg(qualities.at(i).resolution)
                               .arg(
                                   qualities.at(i).containerName,
                                   qualities.at(i).videoCodec,
                                   qualities.at(i).audioCodec
                                   );
        ui.downloadComboQuality->addItem(qualities.at(i).name, reso_ext);
    }

    if (cg->settings.value("rememberVideoQuality", true).toBool()) {
        const int rememberedResolution = cg->settings.value("rememberedVideoQuality", -1).toInt();
        const QString rememberedContainer = cg->settings.value("rememberedVideoContainer", "").toString();
        QString rememberedVideoCodec = cg->settings.value("rememberedVideoCodec", "none").toString();
        QString rememberedAudioCodec = cg->settings.value("rememberedAudioCodec", "none").toString();

        // qDebug() << "auto-select: try to restore last remembered reso: " << rememberedResolution << " container: " << rememberedContainer
        //          << "  video-codec: " << rememberedVideoCodec << "  audio-codec: " << rememberedAudioCodec;

        int bestResolutionMatch = 0;
        int bestResolutionMatchPosition = 0;
        for (int i = 0; i < qualities.length(); i++) {
            const int resolution = qualities.at(i).resolution;
            if (resolution <= rememberedResolution && resolution > bestResolutionMatch) {
                bestResolutionMatch = resolution;
            }
        }

        QList<int> best_qualitiesA;
        QList<int> best_qualitiesB;
        while (true) {
            for (int i = 0; i < qualities.length(); i++) {
                if ( qualities.at(i).resolution == bestResolutionMatch )
                    best_qualitiesA.append(i);
            }
            if (best_qualitiesA.isEmpty())
                break;  // no matching resolutions => break

            // qDebug() << "auto-select last remembered: best reso: " << bestResolutionMatch << "#" << best_qualitiesA.size();
            bestResolutionMatchPosition = best_qualitiesA.at(0);
            best_qualitiesB.clear();
            for (const int i : best_qualitiesA) {
                if ( qualities.at(i).containerName == rememberedContainer ) {
                    best_qualitiesB.append(i);
                }
            }
            // qDebug() << "auto-select last remembered: found matching container: " << rememberedContainer << "#" << best_qualitiesB.size();
            if (best_qualitiesB.isEmpty())
                break;

            bestResolutionMatchPosition = best_qualitiesB.at(0);
            best_qualitiesA.clear();
            for (const int i : best_qualitiesB) {
                if ( qualities.at(i).videoCodec == rememberedVideoCodec ) {
                    best_qualitiesA.append(i);
                }
            }
            // qDebug() << "auto-select last remembered: found matching video-codec: " << rememberedVideoCodec << "#" << best_qualitiesA.size();
            if (best_qualitiesA.isEmpty())
                break;

            bestResolutionMatchPosition = best_qualitiesA.at(0);
            best_qualitiesB.clear();
            for (const int i : best_qualitiesA) {
                if ( qualities.at(i).audioCodec == rememberedAudioCodec ) {
                    best_qualitiesB.append(i);
                }
            }
            // qDebug() << "auto-select last remembered: found matching audio-codec: " << rememberedAudioCodec << "#" << best_qualitiesB.size();
            if (best_qualitiesB.isEmpty())
                break;

            bestResolutionMatchPosition = best_qualitiesB.at(0);
            break;
        }

        // if ( bestResolutionMatchPosition >= 0 && bestResolutionMatchPosition < qualities.size() ) {
        //     const int i = bestResolutionMatchPosition;
        //     const videoQuality & v = qualities.at(i);
        //     qDebug() << "auto-select: final select is: reso: " << v.resolution << " container: " << v.containerName
        //              << "  video-codec: " << v.videoCodec << "  audio-codec: " << v.audioCodec;
        // }
        ui.downloadComboQuality->setCurrentIndex(bestResolutionMatchPosition);
    }
    this->updatingComboQuality = false;
}

void MainWindow::on_settingsSavedPath_textChanged(QString string)
{
    this->cg->settings.setValue("savedPath", string);
}

void MainWindow::on_settingsAudioBitrate_textChanged(QString s )
{
    this->cg->settings.setValue("FFmpeg-audio-bitrate", s);
}

void MainWindow::on_settingsAudioQuality_textChanged(QString s)
{
    this->cg->settings.setValue("FFmpeg-audio-quality", s);
}

void MainWindow::on_settingsBrowseTargetPath_clicked()
{
    QString newPath;
    newPath = QFileDialog::getExistingDirectory(this, tr("ClipGrab - Select target path"), ui.settingsSavedPath->text());
    if (!newPath.isEmpty())
    {
        ui.settingsSavedPath->setText(newPath);
    }
}

void MainWindow::on_downloadCancel_clicked() {
    video* selectedVideo = ((DownloadListModel*) ui.downloadTree->model())->getVideo(ui.downloadTree->currentIndex());
    if (selectedVideo == nullptr) return;

    selectedVideo->cancel();
}

void MainWindow::on_downloadPause_clicked() {
    video* selectedVideo = ((DownloadListModel*) ui.downloadTree->model())->getVideo(ui.downloadTree->currentIndex());
    if (selectedVideo == nullptr) return;

    if (selectedVideo->getState() == video::state::paused) {
        selectedVideo->resume();
        return;
    }
    selectedVideo->pause();
}

void MainWindow::on_downloadOpen_clicked() {
    video* selectedVideo = ((DownloadListModel*) ui.downloadTree->model())->getVideo(ui.downloadTree->currentIndex());
    if (selectedVideo == nullptr) return;
    cg->openTargetFolder(selectedVideo);
}

void MainWindow::on_settingsSaveLastPath_stateChanged(int state)
{
    cg->settings.setValue("saveLastPath", state == Qt::Checked);
}

void MainWindow::compatibleUrlFoundInClipBoard(QString url) {
    this->ui.clipboard_content->setText(url);
    this->ui.copy_from_clip->setEnabled(true);
    if (QApplication::activeModalWidget() == nullptr) {
        if (cg->settings.value("Clipboard", "ask") == "ask") {
            Notifications::showMessage(tr("ClipGrab: Video discovered in your clipboard"), tr("ClipGrab has discovered the address of a compatible video in your clipboard. Click on this message to download it now."), &systemTrayIcon);
        } else if (cg->settings.value("Clipboard", "ask") == "always") {
            this->ui.downloadLineEdit->setText(url);
            this->ui.mainTab->setCurrentIndex(0);
            this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            this->show();
            this->activateWindow();
            this->raise();
        }
    }
}

void MainWindow::UrlFoundInClipBoard(QString url) {
    this->ui.clipboard_content->setText(url);
    this->ui.copy_from_clip->setEnabled(true);
}

void MainWindow::copyFromClipBoard() {
    QString url = QApplication::clipboard()->text();
    ui.mainTab->setCurrentIndex(0);
    if (!url.isEmpty()) {
        this->ui.downloadLineEdit->setText(url);
    }
}

void MainWindow::activate_grab()
{
    ui.mainTab->setCurrentIndex(0);
    if ( ui.downloadLineEdit->text().isEmpty() && !QApplication::clipboard()->text().isEmpty() )
        ui.downloadLineEdit->setText( QApplication::clipboard()->text() );
    else
        ui.downloadStart->click();
}

void MainWindow::activate_format_cb()
{
    ui.mainTab->setCurrentIndex(0);
    if (ui.downloadComboFormat->isEnabled())
        ui.downloadComboFormat->showPopup();
}

void MainWindow::activate_quality_cb()
{
    ui.mainTab->setCurrentIndex(0);
    if (ui.downloadComboQuality->isEnabled())
        ui.downloadComboQuality->showPopup();
}

void MainWindow::systemTrayMessageClicked() {
    if (QApplication::activeModalWidget() == nullptr) {
        this->ui.downloadLineEdit->setText(cg->clipboardUrl);
        this->ui.mainTab->setCurrentIndex(0);
        this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        this->show();
        this->activateWindow();
        this->raise();
    }
}

void MainWindow::systemTrayIconActivated(QSystemTrayIcon::ActivationReason) {
    if (cg->settings.value("MinimizeToTray", false).toBool() && !this->isHidden()) {
        this->hide();
    } else {
        this->setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        this->show();
        this->activateWindow();
        this->raise();
    }
}

void MainWindow::settingsClipboard_toggled(bool)
{
    if (ui.settingsRadioClipboardAlways->isChecked()) {
        cg->settings.setValue("Clipboard", "always");
    } else if (ui.settingsRadioClipboardNever->isChecked()) {
        cg->settings.setValue("Clipboard", "never");
    } else if (ui.settingsRadioClipboardAsk->isChecked()) {
        cg->settings.setValue("Clipboard", "ask");
    }
}

void MainWindow::disableDownloadUi(bool disable)
{
    ui.downloadComboFormat->setDisabled(disable);
    ui.downloadComboQuality->setDisabled(disable);
    ui.downloadStart->setDisabled(disable);
    ui.label_dn_format->setDisabled(disable);
    ui.label_stream->setDisabled(disable);
}

void MainWindow::disableDownloadTreeButtons(bool disable)
{
    ui.downloadOpen->setDisabled(disable);
    ui.downloadCancel->setDisabled(disable);
    ui.downloadPause->setDisabled(disable);
}

void MainWindow::handle_downloadTree_currentChanged(const QModelIndex &current, const QModelIndex & /*previous*/)
{
    video* selectedVideo = ((DownloadListModel*) ui.downloadTree->model())->getVideo(current);
    if (selectedVideo == nullptr) {
        disableDownloadTreeButtons();
    } else {
        disableDownloadTreeButtons(false);
        if (selectedVideo->getState() == video::state::finished || selectedVideo->getState() == video::state::canceled) {
            ui.downloadCancel->setDisabled(true);
            ui.downloadPause->setDisabled(true);
        }
    }
}

void MainWindow::on_settingsNeverAskForPath_stateChanged(int state)
{
    cg->settings.setValue("NeverAskForPath", state == Qt::Checked);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (cg->getRunningDownloadsCount() > 0) {
        QMessageBox* exitBox;
        exitBox = new QMessageBox(QMessageBox::Question, tr("ClipGrab - Exit confirmation"), tr("There is still at least one download in progress.<br />If you exit the program now, all downloads will be canceled and cannot be recovered later.<br />Do you really want to quit ClipGrab now?"), QMessageBox::Yes|QMessageBox::No);
        if (exitBox->exec() == QMessageBox::Yes) {
            systemTrayIcon.hide();
            connect(cg, &ClipGrab::allDownloadsCanceled, [event] {
                event->accept();
            });
            cg->cancelAllDownloads();
        } else {
            event->ignore();
        }
    } else {
        systemTrayIcon.hide();
        event->accept();
    }
}

void MainWindow::settingsNotifications_toggled(bool) {
    if (ui.settingsRadioNotificationsAlways->isChecked()) {
        cg->settings.setValue("Notifications", "always");
    } else if (ui.settingsRadioNotificationsFinish->isChecked()) {
        cg->settings.setValue("Notifications", "finish");
    } else if (ui.settingsRadioNotificationsNever->isChecked()) {
        cg->settings.setValue("Notifications", "never");
    }
}

void MainWindow::handleFinishedConversion(video* finishedVideo) {
     if (cg->settings.value("Notifications", "finish") == "always") {
         Notifications::showMessage(tr("Download finished"), tr("Downloading and converting “%title” is now finished.").replace("%title", finishedVideo->getTitle()), &systemTrayIcon);
     } else if (cg->getRunningDownloadsCount() == 0 && cg->settings.value("Notifications", "finish") == "finish") {
         Notifications::showMessage(tr("All downloads finished"), tr("ClipGrab has finished downloading and converting all selected videos."), &systemTrayIcon);
     }
}

void MainWindow::on_settingsRemoveFinishedDownloads_stateChanged(int state) {
    cg->settings.setValue("RemoveFinishedDownloads", state == Qt::Checked);
}

void MainWindow::settingsProxyChanged() {
    cg->settings.setValue("UseProxy", ui.settingsUseProxy->isChecked());
    cg->settings.setValue("ProxyHost", ui.settingsProxyHost->text());
    cg->settings.setValue("ProxyPort", ui.settingsProxyPort->value());
    cg->settings.setValue("ProxyAuthenticationRequired", ui.settingsProxyAuthenticationRequired->isChecked());
    cg->settings.setValue("ProxyPassword", ui.settingsProxyPassword->text());
    cg->settings.setValue("ProxyUsername", ui.settingsProxyUsername->text());
    cg->settings.setValue("ProxyType", ui.settingsProxyType->currentIndex());
    ui.settingsProxyGroup->setEnabled(ui.settingsUseProxy->isChecked());
    ui.settingsProxyAuthenticationRequired->setEnabled(ui.settingsUseProxy->isChecked());

    ui.settingsProxyAuthenticationGroup->setEnabled(ui.settingsUseProxy->isChecked() && ui.settingsProxyAuthenticationRequired->isChecked());
    cg->activateProxySettings();
}

void MainWindow::timerEvent(QTimerEvent *)
{
//    QPair<qint64, qint64> downloadProgress = cg->downloadProgress();
//    if (downloadProgress.first != 0 && downloadProgress.second != 0)
//    {
//        #ifdef Q_WS_X11
//            systemTrayIcon.setToolTip("<strong style=\"font-size:14px\">" + tr("ClipGrab") + "</strong><br /><span style=\"font-size:13px\">" + QString::number(downloadProgress.first*100/downloadProgress.second) + " %</span><br />" + QString::number((double)downloadProgress.first/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB") + "/" + QString::number((double)downloadProgress.second/1024/1024, QLocale::system().decimalPoint().toAscii(), 1) + tr(" MiB"));
//        #else
//        systemTrayIcon.setToolTip(tr("ClipGrab") + " - " + QString::number(downloadProgress.first*100/downloadProgress.second) + " % - " + QString::number((double)downloadProgress.first/1024/1024, QLocale::system().decimalPoint().toLatin1(), 1) + tr(" MiB") + "/" + QString::number((double)downloadProgress.second/1024/1024, QLocale::system().decimalPoint().toLatin1(), 1) + tr(" KiB"));
//        #endif
//        setWindowTitle("ClipGrab - " + QString::number(downloadProgress.first*100/downloadProgress.second) + " %");
//    }
//    else
//    {
//        #ifdef Q_WS_X11
//            systemTrayIcon.setToolTip("<strong style=\"font-size:14px\">" + tr("ClipGrab") + "</strong><br /><span style=\"font-size:13px\">" + tr("Currently no downloads in progress."));
//        #endif
//        systemTrayIcon.setToolTip(tr("ClipGrab") + tr("Currently no downloads in progress."));
//        setWindowTitle(tr("ClipGrab - Download and Convert Online Videos"));
//    }
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && cg->settings.value("MinimizeToTray", false).toBool()) {
            QTimer::singleShot(500, this, SLOT(hide()));
            event->ignore();
        }
    }
}

void MainWindow::on_settingsMinimizeToTray_stateChanged(int state) {
    cg->settings.setValue("MinimizeToTray", state == Qt::Checked);
}


void MainWindow::on_downloadLineEdit_returnPressed() {
    startDownload();
}

void MainWindow::on_label_status_version_linkActivated(QString link)
{
    QDesktopServices::openUrl(link);
}

void MainWindow::on_settingsUseMetadata_stateChanged(int state)
{
    cg->settings.setValue("UseMetadata", state == Qt::Checked);
}

void MainWindow::on_downloadComboFormat_currentIndexChanged(int index)
{
    cg->settings.setValue("LastFormat", index);
}

void MainWindow::on_mainTab_currentChanged(int index)
{
    cg->settings.setValue("MainTab", index);
}

void MainWindow::on_settingsLanguage_currentIndexChanged(int index)
{
    cg->settings.setValue("Language", cg->languages.at(index).code);
}

void MainWindow::on_buttonDonate_clicked()
{
    QDesktopServices::openUrl(QUrl("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=AS6TDMR667GJL"));
}

void MainWindow::on_settingsIgnoreSSLErrors_toggled(bool checked)
{
    cg->settings.setValue("IgnoreSSLErrors", checked);
}

void MainWindow::on_downloadTree_customContextMenuRequested(const QPoint &point)
{
    QModelIndex i = ui.downloadTree->indexAt(point);
    if (!i.isValid()) return;

    video* selectedVideo = ((DownloadListModel*) ui.downloadTree->model())->getVideo(i);
    if (selectedVideo == nullptr) return;

    QMenu contextMenu;
    QAction* openDownload = contextMenu.addAction(tr("&Open downloaded file"));
    QAction* openFolder = contextMenu.addAction(tr("Open &target folder"));
    contextMenu.addSeparator();
    QAction* pauseDownload = contextMenu.addAction(tr("&Pause download"));
    QAction* restartDownload = contextMenu.addAction(tr("&Restart download"));
    QAction* cancelDownload = contextMenu.addAction(tr("&Cancel download"));
    contextMenu.addSeparator();
    QAction* copyLink = contextMenu.addAction(tr("Copy &video link"));
    QAction* openLink = contextMenu.addAction(tr("Open video link in &browser"));

    if (selectedVideo->getState() == video::state::paused) {
        pauseDownload->setText(tr("Resume download"));
    }

    if (selectedVideo->getState() == video::state::canceled) {
        contextMenu.removeAction(pauseDownload);
        contextMenu.removeAction(cancelDownload);
    }

    if (selectedVideo->getState() == video::state::finished) {
        contextMenu.removeAction(pauseDownload);
        contextMenu.removeAction(restartDownload);
        contextMenu.removeAction(cancelDownload);
        #ifdef Q_OS_MAC
            openFolder->setText(tr("Show in &Finder"));
        #endif
    } else {
        contextMenu.removeAction(openDownload);
    }


    QAction* selectedAction = contextMenu.exec(ui.downloadTree->mapToGlobal(point));
    if (selectedAction == restartDownload) {
        selectedVideo->restart();
    } else if (selectedAction == cancelDownload) {
        selectedVideo->cancel();
    } else if (selectedAction == pauseDownload) {
        if (selectedVideo->getState() == video::state::paused) {
            selectedVideo->resume();
        } else {
            selectedVideo->pause();
        }
    } else if (selectedAction == openFolder) {
        cg->openTargetFolder(selectedVideo);
    } else if (selectedAction == openDownload) {
        cg->openDownload(selectedVideo);
    } else if (selectedAction == openLink) {
        QString link = selectedVideo->getUrl();
        QDesktopServices::openUrl(QUrl(link));
    } else if (selectedAction == copyLink) {
        QApplication::clipboard()->setText(selectedVideo->getUrl());
    }

    contextMenu.deleteLater();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    ui.downloadLineEdit->setText(event->mimeData()->text());
    ui.mainTab->setCurrentIndex(0);
}

void MainWindow::updateYoutubeDlVersionInfo()
{
    handleYtDlVersion(QString());
}

void MainWindow::handleYtDlVersion(QString online_version) {
    if (online_version.isEmpty())
        online_version = tr("not available");
    qDebug().noquote() << "MainWindow received YT online version: " << online_version;

    QString youtubeDlVersion = YoutubeDl::getVersion();
    QString pythonVersion = YoutubeDl::getPythonVersion();
    QString youtubeDlPath = YoutubeDl::find();
    QString pythonPath = YoutubeDl::findPython();
    QString label = tr("<h2>YouTube Downloader</h2>\nyoutube-dl: %1 (%2)<br>youtube-dl at <a href=\"%3\">%4</a> (%5)")
                        .arg(youtubeDlPath, youtubeDlVersion)
                        .arg(YoutubeDl::homepage_url, YoutubeDl::homepage_short)
                        .arg(online_version);
    if ( youtubeDlVersion.isEmpty() )
        label = tr("<h2>Versions</h2>\nyoutube-dl: <b>not found!</b><br>youtube-dl at <a href=\"%1\">%2</a> (%3)")
                    .arg(YoutubeDl::homepage_url, YoutubeDl::homepage_short)
                    .arg(online_version);
    if ( !pythonPath.isEmpty() || !pythonVersion.isEmpty() )
        label = label + tr("<br>Python: %1 (%2)").arg(pythonPath, pythonVersion);

    ui.labelYoutubeDlVersionInfo->setText(label);

    ui.yt_version_check->setEnabled(true);
    bool normal_update = (youtubeDlVersion.isEmpty() || youtubeDlVersion < online_version );
    ui.yt_update->setText(normal_update ? tr("Update") : tr("Force Update!"));
    ui.yt_update->setEnabled(true);
    ui.yt_delete->setEnabled(!youtubeDlVersion.isEmpty());
}

void MainWindow::handleProgramVersion(QString online_version)
{
    if (online_version.isEmpty())
        online_version = tr("not available");
    qDebug().noquote() << "online available ClipGrab version: " << online_version;

    QString label = tr("<h2>ClipGrab</h2>\nClipGrab: %1<br>ClipGrab at <a href=\"%3\">%4</a> (%5)").arg(
                QCoreApplication::applicationVersion(),
                ClipGrab::homepage_url, ClipGrab::homepage_short,
                online_version
                );

    ui.labelProgramVersionInfo->setText(label);
    ui.program_version_check->setEnabled(true);
}


void MainWindow::handleFFmpegReleases(QStringList releases)
{
    handleFFmpegVersion(cg->ffmpegPath(), cg->ffmpegVersion(), releases);
}

void MainWindow::handleFFmpegDownloadFinished(QString filePath)
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    ffmpegDownloadedArchive = filePath;
    qDebug().noquote() << "FFmpeg download finished: " << filePath;

    handleFFmpegDelete();
    if ( !ffmpegUnpackProcess ) {
        ffmpegUnpackProcess = new QProcess(this);
        QObject::connect(ffmpegUnpackProcess, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &MainWindow::handleFFmpegUnpackFinished );
    }

#if defined(Q_OS_LINUX)
    if (!filePath.endsWith(".tar.xz")) {
        qDebug() << "Error: expected .tar.gz!";
        QFile::remove(filePath);
        return;
    }
    // "tar -xf ffmpeg-n5.1-latest-linux64-gpl-5.1.tar.xz --no-anchored ffmpeg -C ./ --strip-components 2"
    ffmpegUnpackProcess->setWorkingDirectory(dir);
    ffmpegUnpackProcess->setProgram("tar");
    ffmpegUnpackProcess->setArguments(
        QStringList()
        << "-xvf"
        << filePath
        << "--no-anchored"
        << converter_ffmpeg::executable
        << "-C"
        << dir
        << "--strip-components"
        << "2"
        );
#elif defined(Q_OS_WIN)
    if (!filePath.endsWith(".zip", Qt::CaseInsensitive)) {
        qDebug() << "Error: expected .zip!";
        QFile::remove(filePath);
        return;
    }
    QDir(dir+"/temp").removeRecursively();
    QDir(dir).mkdir("temp");
    // https://learn.microsoft.com/de-de/powershell/module/microsoft.powershell.archive/expand-archive?view=powershell-5.1
    QString cmd = QString("& { Add-Type -A 'System.IO.Compression.FileSystem'; [IO.Compression.ZipFile]::ExtractToDirectory('%1', 'temp'); }").arg(filePath);
    ffmpegUnpackProcess->setWorkingDirectory(dir);
    ffmpegUnpackProcess->setProgram("powershell.exe");
    ffmpegUnpackProcess->setArguments( QStringList() << "-nologo" << "-noprofile" << "-command" << cmd );

// #elif defined(Q_OS_MAC)
#else
    qDebug() << "Error: Unpacking not implemented on this platform!";
    //QFile::remove(filePath);
    return;
#endif
    ffmpegUnpackProcess->start();
}

void MainWindow::handleFFmpegUnpackFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    (void)exitStatus;
    qDebug().noquote() << "FFmpeg unpacking finished with exitCode " << exitCode;
    if (ffmpegDownloadedArchive.isEmpty() || !QFile::exists(ffmpegDownloadedArchive)) {
        qDebug() << "FFmpeg archive not found!";
        return;
    }
#if defined(Q_OS_WIN)
    {
        QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFileInfo fi( ffmpegDownloadedArchive );
        QString zip_fn = fi.fileName();
        QString zip_dir = zip_fn.left(zip_fn.length()-4);
        QString executable = dir + "/temp/" + zip_dir + "/bin/" + converter_ffmpeg::executable;
        QString target = dir + "/" + converter_ffmpeg::executable;
        if ( QFile::exists(executable) )
        {
            if ( QFile::exists(target) )
                QFile::remove(target);
            bool ok = QFile::copy(executable, target);
            if (ok)
                QDir(dir+"/temp").removeRecursively();
            else
                qDebug().noquote() << "Error: failed to copy " << executable << " to " << target;
        }
        else
            qDebug().noquote() << "Error: " << executable << " not found in unpacked zip";
    }
#endif

    cg->updateFFmpeg();
    updateAvailableFormats();
    cg->getFFmpegReleases();
    QFile::remove(ffmpegDownloadedArchive);
    ffmpegDownloadedArchive.clear();
}

void MainWindow::handleFFmpegVersion(QString path, QString version, QStringList releases)
{
    if (version.isEmpty())
        version = tr("not available");
    QString label = tr("<h2>A/V Converter</h2>\nFFmpeg: %1 (%2)").arg(path, version);
    if ( path.isEmpty() )
        label = tr("<h2>A/V Converter</h2>\nFFmpeg: <b>not found!</b>");
    label = label + "<br>"
            + tr("FFmpeg at <a href=\"%1\">%2</a> (daily builds)")
                  .arg(converter_ffmpeg::homepage_url, converter_ffmpeg::homepage_short);
    ui.labelFFmpegVersionInfo->setText(label);

    {
        const QString old_sel = cg->settings.value("FFmpeg_Update_Branch", QString("master")).toString();
        int next_idx = -1;
        ui.ff_branch->clear();
        for (int k = 0; k < releases.size(); k += 2) {
            ui.ff_branch->addItem(releases.at(k), releases.at(k+1));
            if ( old_sel == releases.at(k) )
                next_idx = ui.ff_branch->count() - 1;
        }
        ui.ff_branch->setCurrentIndex( next_idx );
    }
    ui.ff_update->setText(tr("Update"));
    ui.ff_version_check->setEnabled(true);
}


void MainWindow::on_settingsRememberLogins_toggled(bool /*checked*/)
{
//    cg->settings.setValue("rememberLogins", checked);
//    cg->settings.setValue("youtubeRememberLogin", checked);
//    cg->settings.setValue("facebookRememberLogin", checked);
//    cg->settings.setValue("vimeoRememberLogin", checked);
//    if (!checked)
//    {
//        cg->settings.remove("youtubeCookies");
//        cg->settings.remove("facebookCookies");
//        cg->settings.remove("vimeoCookies");
//    }
}

void MainWindow::on_settingsRememberVideoQuality_toggled(bool checked)
{
    cg->settings.setValue("rememberVideoQuality", checked);
}

void MainWindow::on_downloadComboQuality_currentIndexChanged(int index)
{
    if (this->updatingComboQuality || !cg->settings.value("rememberVideoQuality", true).toBool() || cg->getCurrentVideo() == nullptr) {
        return;
    }

    QString reso_ext = ui.downloadComboQuality->itemData(index, Qt::UserRole).toString();
    QStringList lst = reso_ext.split(";;");
    int reso = 0;
    QString ext;
    QString vCodec;
    QString aCodec;
    if ( lst.size() <= 1)
        reso = reso_ext.toInt();
    else if ( 1 < lst.size() ) {
        reso = lst.at(0).toInt();
        ext = lst.at(1);
        if ( 2 < lst.size() )
            vCodec = lst.at(2);
        if ( 3 < lst.size() )
            aCodec = lst.at(3);
    }
    if ( vCodec.isEmpty() )
        vCodec = "none";
    if ( aCodec.isEmpty() )
        aCodec = "none";

    cg->settings.setValue("rememberedVideoQuality", reso);
    cg->settings.setValue("rememberedVideoContainer", ext);
    cg->settings.setValue("rememberedVideoCodec", vCodec);
    cg->settings.setValue("rememberedAudioCodec", aCodec);
    // qDebug() << "remembering for later auto-select: reso: " << reso << " container: " << ext
    //          << "  video-codec: " << vCodec << "  audio-codec: " << aCodec;
}

void MainWindow::on_downloadTree_doubleClicked(const QModelIndex &index)
{
    video* video = ((DownloadListModel*) ui.downloadTree->model())->getVideo(index);
    if (video == nullptr) return;
    cg->openDownload(video);
}


void MainWindow::on_settingsForceIpV4_toggled(bool checked) {
    cg->settings.setValue("forceIpV4", checked);
}

void MainWindow::on_settingsUpdateCheck_toggled(bool checked) {
    cg->settings.setValue("Check-Updates", checked);
}

void MainWindow::on_settingsShowLogo_toggled(bool checked) {
    cg->settings.setValue("Show-Logo", checked);
    if (checked) {
        ui.label_4->show();
        ui.verticalSpacer_9->changeSize(10, 10);
    }
    else {
        ui.label_4->hide();
        ui.verticalSpacer_9->changeSize(10, 0);
    }
}

void MainWindow::on_settings_mp4_accepts_h264_toggled(bool checked) {
    cg->settings.setValue("mp4_accepts_h264", checked);
}

void MainWindow::on_settings_mp4_accepts_av1_toggled(bool checked) {
    cg->settings.setValue("mp4_accepts_av1", checked);
}

void MainWindow::on_ff_branch_currentIndexChanged(int index) {
    ui.ff_update->setEnabled( index >= 0 );
    QString branch = ui.ff_branch->currentText();
    if ( index >= 0 && !branch.isEmpty() )
        cg->settings.setValue("FFmpeg_Update_Branch", branch);
}
