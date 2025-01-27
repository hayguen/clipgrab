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


#include "clipgrab.h"
#include "converter_copy.h"
#include "helper_downloader.h"

const char * ClipGrab::version_url = "https://raw.githubusercontent.com/hayguen/clipgrab/master/clipgrab.pro";
const char * ClipGrab::homepage_url = "https://github.com/hayguen/clipgrab";
const char * ClipGrab::homepage_short = "github.com/hayguen/clipgrab";


ClipGrab::ClipGrab()
    : re_is_url("^http[s]?://")
{
    helperDownloaderDialog = NULL;

    //*
    //* Set current version
    //*
    version = QCoreApplication::applicationVersion();

    //*
    //* Initialise languages
    //*
    language newLang;
    newLang.name = tr("Automatic language recognition");
    newLang.code = "auto";
    languages.append(newLang);

    newLang.name = "Bahasa Indonesia";
    newLang.code = "id";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Català");
    newLang.code = "ca";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Česky");
    newLang.code = "cs";
    languages.append(newLang);

    newLang.name = "Deutsch";
    newLang.code = "de";
    languages.append(newLang);

    newLang.name = "English";
    newLang.code = "en";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Español");
    newLang.code = "es";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Euskara");
    newLang.code = "eu";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Français");
    newLang.code = "fr";
    languages.append(newLang);

    newLang.name = "Hrvatski";
    newLang.code = "hr";
    languages.append(newLang);

    newLang.name = "Italiano";
    newLang.code = "it";
    languages.append(newLang);

    newLang.name = "Kiswahili";
    newLang.code = "sw";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Lietuvių");
    newLang.code = "lt";
    languages.append(newLang);

    newLang.name = "Magyar";
    newLang.code = "hu";
    languages.append(newLang);

    newLang.name = "Nederlands";
    newLang.code = "nl";
    languages.append(newLang);

    newLang.name = "Norsk (Bokmål)";
    newLang.code = "no";
    languages.append(newLang);

    newLang.name = "Polski";
    newLang.code = "pl";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Português");
    newLang.code = "pt";
    languages.append(newLang);

    newLang.name = "Romana";
    newLang.code = "ro";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Slovenščina");
    newLang.code = "si";
    languages.append(newLang);

    newLang.name = "Suomi";
    newLang.code = "fi";
    languages.append(newLang);

    newLang.name = "Svenska";
    newLang.code = "sv";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Türkçe");
    newLang.code = "tr";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Tiếng Việt");
    newLang.code = "vi";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("български");
    newLang.code = "bg";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("বাংলা");
    newLang.code = "bn";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Ελληνικά¨");
    newLang.code = "el";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("فارسی");
    newLang.code = "fa";
    newLang.isRTL = true;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("日本語");
    newLang.code = "ja";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("한국어");
    newLang.code = "ko";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("македонски");
    newLang.code = "mk";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("ਪੰਜਾਬੀ");
    newLang.code = "pa";
    newLang.isRTL = false;
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Русский");
    newLang.code = "ru";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("Српски");
    newLang.code = "sr";
    languages.append(newLang);

    newLang.name = QString::fromUtf8("中文");
    newLang.code = "zh";
    languages.append(newLang);

    //*
    //* Initialise the supported converters
    //*
    conv_ffmpeg = new converter_ffmpeg;
    converters.append(new converter_copy);
    converters.append(conv_ffmpeg);
    QObject::connect(conv_ffmpeg, &converter_ffmpeg::ffmpegPathAndVersion, this, &ClipGrab::handleFFmpegPathAndVersion );
    updateFFmpeg();

    //*
    //* Add Mozilla Root CA certificats to avoid errors from missing system certificates
    //*
    // QSslConfiguration::defaultConfiguration().addCaCertificates(":/crt/mozilla-root-cas.txt");
    QSslSocket::addDefaultCaCertificates(":/crt/mozilla-root-cas.txt");

    //*
    //* Remove previously downloaded update
    //*
    QString updateFileName = settings.value("updateFile", "").toString();
    settings.remove("updateFile");
    if (!updateFileName.isEmpty())
    {
        QFile updateFile;
        updateFile.setFileName(updateFileName);
        if (updateFile.exists())
        {
            updateFile.remove();
        }
    }

    re_video_urls << QRegularExpression("^http[s]?://\\w*\\.dailymotion\\.com/video/([^?/]+)");
    re_video_urls << QRegularExpression("^http[s]?://dai\\.ly/([^?/]+)");
    re_video_urls << QRegularExpression("^(http[s]?://(www\\.)?facebook\\.com.*/videos).*/(\\d+)");
    re_video_urls << QRegularExpression("^http[s]?://(www\\.)?facebook\\.com.*/(?:pg/)?.*/videos/.*");
    re_video_urls << QRegularExpression("^http[s]?://(www\\.)?vimeo\\.com/(groups/[a-z0-9]+/videos/)?([0-9]+)");
    re_video_urls << QRegularExpression("^http[s]?://(player.)vimeo\\.com/video/([0-9]+)");
    re_video_urls << QRegularExpression("^http[s]?://\\w*\\.youtube\\.com/watch.*v\\=.*");
    re_video_urls << QRegularExpression("^http[s]?://\\w*\\.youtube\\.com/view_play_list\\?p\\=.*&v\\=.*");
    re_video_urls << QRegularExpression("^http[s]?://youtu.be/.*");
    re_video_urls << QRegularExpression("^http[s]?://w*\\.youtube\\.com/embed/.*");
    re_video_urls << QRegularExpression("^https://www\\.ardmediathek\\.de/ard/video/.*");
    re_video_urls << QRegularExpression("^https://www\\.zdf\\.de/.*-\\d+\\.html");

    //*
    //* Miscellaneous
    //*
    activateProxySettings();
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));

    currentVideo = nullptr;
    currentSearch = nullptr;
}

void ClipGrab::updateFFmpeg()
{
    formats.clear();
    for (int i = 0; i < converters.size(); ++i)
    {
        converter* tmpConverter = converters.at(i);
        connect(tmpConverter, SIGNAL(error(QString)), this, SLOT(errorHandler(QString)));
        if (tmpConverter->isAvailable())
        {
            QList<QString> tmpModes = tmpConverter->getModes();
            for (int i = 0; i < tmpModes.size(); ++i)
            {
                format tmpFormat;
                tmpFormat._converter = tmpConverter;
                tmpFormat._mode = i;
                tmpFormat._name = tmpModes.at(i);
                formats.append(tmpFormat);
            }
        }
    }
}

void ClipGrab::fetchVideoInfo(const QString& url) {
    if (currentVideo != nullptr) {
        if (!downloads.contains(currentVideo)) {
            currentVideo->deleteLater();
        }
    }

    currentVideo = new video();
    connect(currentVideo, &video::stateChanged, [=] {
        emit currentVideoStateChanged(currentVideo);
    });
    currentVideo->fetchInfo(url);
}

void ClipGrab::clearCurrentVideo() {
    if (currentVideo != nullptr) {
        if (!downloads.contains(currentVideo)) {
            currentVideo->deleteLater();
        }
        currentVideo = nullptr;
        emit currentVideoStateChanged(nullptr);
    }
}

#if CLIPGRAB_ORG_UPDATER

void ClipGrab::getUpdateInfo()
{
    //*
    //* Make update info request to clipgrab.org
    //*
    QString sys = "x11";

    #if defined Q_OS_WIN
        sys = "win";
    #endif
    #if defined Q_OS_MAC
         sys = "mac";
    #endif

    QString firstStarted = settings.value("firstStarted", "").toString();

    QUrlQuery updateInfoRequestUrlQuery;
    updateInfoRequestUrlQuery.addQueryItem("v", this->version);
    updateInfoRequestUrlQuery.addQueryItem("l", QLocale::system().name().split("_")[0]);
    if (!firstStarted.isEmpty())
    {
        updateInfoRequestUrlQuery.addQueryItem("t", firstStarted);
    }
    QUrl updateInfoRequestUrl("https://clipgrab.org/update/" + sys + "/");
    updateInfoRequestUrl.setQuery(updateInfoRequestUrlQuery);

    QNetworkRequest updateInfoRequest;
    updateInfoRequest.setUrl(updateInfoRequestUrl);
    QNetworkAccessManager* updateInfoNAM = new QNetworkAccessManager;
    qDebug().noquote() << "requesting update info from " << updateInfoRequestUrl.toString();
    updateInfoNAM->get(updateInfoRequest);
    connect(updateInfoNAM, &QNetworkAccessManager::finished, this, &ClipGrab::parseUpdateInfo);
}

#endif


void ClipGrab::getProgramVersion()
{
    QUrlQuery updateInfoRequestUrlQuery;
    QUrl updateInfoRequestUrl(ClipGrab::version_url);
    updateInfoRequestUrl.setQuery(updateInfoRequestUrlQuery);

    QNetworkRequest updateInfoRequest;
    updateInfoRequest.setUrl(updateInfoRequestUrl);
    QNetworkAccessManager* updateInfoNAM = new QNetworkAccessManager;
    qDebug().noquote() << "requesting version info from " << updateInfoRequestUrl.toString();
    updateInfoNAM->get(updateInfoRequest);
    connect(updateInfoNAM, &QNetworkAccessManager::finished, this, &ClipGrab::parseProgramVersion);
}

void ClipGrab::getYtDlVersion()
{
    QUrlQuery updateInfoRequestUrlQuery;
    QUrl updateInfoRequestUrl(YoutubeDl::version_url);
    updateInfoRequestUrl.setQuery(updateInfoRequestUrlQuery);

    QNetworkRequest updateInfoRequest;
    updateInfoRequest.setUrl(updateInfoRequestUrl);
    QNetworkAccessManager* updateInfoNAM = new QNetworkAccessManager;
    qDebug().noquote() << "requesting version info from " << updateInfoRequestUrl.toString();
    updateInfoNAM->get(updateInfoRequest);
    connect(updateInfoNAM, &QNetworkAccessManager::finished, this, &ClipGrab::parseYtDlVersion);
}

void ClipGrab::getFFmpegReleases()
{
    QUrlQuery updateInfoRequestUrlQuery;
    QUrl updateInfoRequestUrl(converter_ffmpeg::releases);
    updateInfoRequestUrl.setQuery(updateInfoRequestUrlQuery);

    QNetworkRequest updateInfoRequest;
    updateInfoRequest.setUrl(updateInfoRequestUrl);
    QNetworkAccessManager* updateInfoNAM = new QNetworkAccessManager;
    qDebug().noquote() << "requesting version infos from " << updateInfoRequestUrl.toString();
    updateInfoNAM->get(updateInfoRequest);
    connect(updateInfoNAM, &QNetworkAccessManager::finished, this, &ClipGrab::parseFFmpegReleases);
}

void ClipGrab::parseProgramVersion(QNetworkReply* reply)
{
    if (!reply->bytesAvailable())
    {
        qDebug().noquote() << "Could not retrieve ClipGrab version info from " << ClipGrab::version_url;
        emit updateProgramVersion(QString());
        return;
    }

    QStringList lines = QString(reply->readAll()).split("\n");
    QRegExp rx( "VERSION\\s*=\\s*(\\S+)");
    for (int i = 0; i < lines.size(); ++i)
    {
        if ( lines.at(i).contains("VERSION"))
        {
            QString line = lines.at(i).trimmed();
            int r = rx.indexIn(line);
            int cc = rx.captureCount();
            if (r < 0 || cc != 1)
                continue;
            QString v = rx.cap(1);
            emit updateProgramVersion(v);
            return;
        }
    }
    emit updateProgramVersion(QString());
}


void ClipGrab::parseYtDlVersion(QNetworkReply* reply)
{
    if (!reply->bytesAvailable())
    {
        qDebug().noquote() << "Could not retrieve " << YoutubeDl::ytdl_name << " version info from " << YoutubeDl::version_url;
        emit updateYtDlVersion(QString());
        return;
    }

    QStringList lines = QString(reply->readAll()).split("\n");
    QRegExp rx( "__version__\\s*=\\s*['\"](\\S+)['\"]");
    for (int i = 0; i < lines.size(); ++i)
    {
        if ( lines.at(i).contains("__version__"))
        {
            QString line = lines.at(i).trimmed();
            int r = rx.indexIn(line);
            int cc = rx.captureCount();
            if (r < 0 || cc != 1)
                continue;
            QString v = rx.cap(1);
            emit updateYtDlVersion(v);
            return;
        }
    }
    emit updateYtDlVersion(QString());
}

void ClipGrab::parseFFmpegReleases(QNetworkReply* reply)
{
    QStringList releases;  // pairs of name and URL

    if (!reply->bytesAvailable())
    {
        qDebug().noquote() << "Could not retrieve FFmpeg version info from " << converter_ffmpeg::releases;
        emit updateFFmpegVersions(releases);
        return;
    }

    QString all_lines = QString(reply->readAll());
    all_lines = all_lines.replace("{", "{\n");
    all_lines = all_lines.replace("[", "[\n");
    all_lines = all_lines.replace("\"}", "\"\n}");
    all_lines = all_lines.replace("\",", "\",\n");
    QStringList lines = all_lines.split("\n");
    QRegExp rx( "\"browser_download_url\"\\s*:\\s*\"(\\S+)\"\\s*" );
#if defined(Q_OS_LINUX)
  #if defined(__aarch64__)
    const QString os_id = "-linuxarm64";
  #else
    const QString os_id = "-linux64";
  #endif
#elif defined(Q_OS_WIN)
    const QString os_id = "-win64";
// #elif defined(Q_OS_MAC)
#else
    emit updateFFmpegVersions(releases);
    return;
#endif

    for (int i = 0; i < lines.size(); ++i)
    {
        if ( lines.at(i).contains("browser_download_url"))
        {
            QString line = lines.at(i).trimmed();
            //qDebug().noquote() << "ffmpeg release download: " << line;
            int r = rx.indexIn(line);
            int cc = rx.captureCount();
            if (r < 0 || cc != 1)
                continue;
            QString url = rx.cap(1);
            int posLastSlash = url.lastIndexOf("/");
            if ( posLastSlash < 0 || posLastSlash >= url.length() - 1 )
                continue;
            QString name = url.mid(posLastSlash+1);
            if ( name.contains("-shared") || name.contains("-lgpl") || !name.contains(os_id) )
                continue;
            name = name.replace("ffmpeg-", "").replace("latest-", "").replace(os_id, "");
            name = name.replace("-lgpl", "").replace("-gpl", "");
            name = name.replace(".tar.xz", "").replace(".zip", "");
            if ( name.startsWith("n") && name.contains("-") ) {
                QStringList parts = name.split("-");
                for (int k = parts.size() - 1; k >= 1; --k) {
                    if ( parts[0].mid(1) == parts[k] )
                        parts.removeAt(k);
                }
                name = parts.join("-");
            }
            // qDebug().noquote() << "ffmpeg release " << name << " @ " << url;
            releases << name << url;
        }
    }
    emit updateFFmpegVersions(releases);
}

#if CLIPGRAB_ORG_UPDATER
void ClipGrab::parseUpdateInfo(QNetworkReply* reply)
{
    if (!reply->bytesAvailable())
    {
        qDebug() << "Could not reach update server.";
        emit updateInfoProcessed();
        return;
    }

    QDomDocument updateInfoXml;
    updateInfoXml.setContent(reply->readAll());

    //Process timestamp
    QDomNodeList timestamps = updateInfoXml.elementsByTagName("timestamp");
    if (!timestamps.isEmpty() && settings.value("firstStarted", "").toString().indexOf(".") == -1)
    {
        settings.setValue("firstStarted", timestamps.at(0).toElement().attribute("time"));
    }

    //Processing available updates
    QDomNodeList updates = updateInfoXml.elementsByTagName("update");
    if (!updates.isEmpty() && this->settings.value("DisableUpdateNotifications", false) == false)
    {
        updateInfo currentVersion;
        currentVersion.version = QCoreApplication::applicationVersion();

        //Get update information
        for (int i = 0; i < updates.length(); i++)
        {
            QDomElement update = updates.at(i).toElement();

            updateInfo newUpdateInfo;
            newUpdateInfo.version = update.attribute("version", "0");
            newUpdateInfo.url = update.attribute("uri");
            newUpdateInfo.sha1 = update.attribute("sha1");
            newUpdateInfo.domNodes = update.childNodes();

            //Only consider newer versions
            if (newUpdateInfo > currentVersion) {
                this->availableUpdates.append(newUpdateInfo);
            }
        }


        if (!this->availableUpdates.isEmpty())
        {
            qDebug() << "processing updates ..";
            qSort(this->availableUpdates.begin(), this->availableUpdates.end());

            //Create changelog document
            QDomDocument updateNotesDocument("html");
            updateNotesDocument.setContent(QString("<style>body{font-family:'Segoe UI', Ubuntu, sans-serif};h3{font-size: 100%;}</style>"));

            for (int i = (this->availableUpdates.length() - 1); i >= 0; i--)
            {
                QDomElement updateNotes = updateNotesDocument.createElement("div");
                updateNotes.appendChild(updateNotesDocument.createElement("h3"));
                updateNotes.childNodes().at(0).appendChild(updateNotesDocument.createTextNode(this->availableUpdates.at(i).version));

                for (int j = 0; j < this->availableUpdates.at(i).domNodes.length(); j++)
                {
                    updateNotes.appendChild(this->availableUpdates.at(i).domNodes.at(j).cloneNode());
                }

                updateNotesDocument.appendChild(updateNotes);
            }

            if (settings.value("skip-" + this->availableUpdates.last().version , false).toBool() == false)
            {
#if USE_WEBENGINE
                this->updateMessageUi = new Ui::UpdateMessage();
                this->updateMessageDialog = new QDialog(QApplication::activeWindow());
                this->updateMessageUi->setupUi(this->updateMessageDialog);

                this->updateMessageUi->labelInfoText->setText(this->updateMessageUi->labelInfoText->text().arg(this->availableUpdates.last().version, currentVersion.version));
                this->updateMessageUi->progressBar->hide();
                this->updateMessageUi->labelDownloadProgress->hide();

                this->updateMessageUi->webEngineView->setPage(new QWebEnginePage(new QWebEngineProfile));
                this->updateMessageUi->webEngineView->setHtml(updateNotesDocument.toString());
                this->updateMessageUi->webEngineView->setContextMenuPolicy(Qt::NoContextMenu);
                this->updateMessageUi->webEngineView->setAcceptDrops(false);
#endif

                this->updateReply = nullptr;
                this->updateFile = nullptr;

#if USE_WEBENGINE
                connect(this->updateMessageUi->buttonConfirm, SIGNAL(clicked()), this, SLOT(startUpdateDownload()));
                connect(this->updateMessageUi->buttonSkip, SIGNAL(clicked()), this, SLOT(skipUpdate()));

                if (this->updateMessageDialog->exec() == QDialog::Rejected)
                {
                    //Cancel any ongoing update operations
                    if (this->updateReply != nullptr)
                    {
                        this->updateReply->disconnect();
                        this->updateReply->abort();
                        this->updateReply->deleteLater();
                    }
                    if (this->updateFile != nullptr)
                    {
                        this->updateFile->close();
                        this->updateFile->deleteLater();
                    }
                }
#endif
            }
        }
    }

    //Processing commands
    QDomNodeList commands = updateInfoXml.elementsByTagName("command");
    if (!commands.isEmpty())
    {
        qDebug() << "processing update commands ..";
        for (int i = 0; i < commands.length(); i++)
        {
            QDomElement command = commands.at(i).toElement();

            if (command.attribute("repeat", "false") != "true")
            {
                QString id = "cmd-";
                if (command.hasAttribute("id"))
                {
                    id.append(command.attribute("id"));
                }
                else
                {
                    QDomDocument doc("cmd");
                    doc.appendChild(command.cloneNode());
                    QCryptographicHash hash(QCryptographicHash::Sha1);
                    hash.addData(doc.toString().toUtf8());
                    id.append(hash.result().toHex());
                }

                if (settings.value(id, false) == true)
                {
                    continue;
                }
                settings.setValue(id, true);
            }

            if (command.attribute("type") == "message")
            {
#if USE_WEBENGINE
                messageDialog* dialog = new messageDialog(QApplication::activeWindow());
                dialog->setUrl(QUrl(command.attribute("uri")));

                if (command.hasAttribute("title")) {
                    dialog->setWindowTitle(command.attribute("title"));
                }
                if (command.hasAttribute("link-policy")) {
                    if (command.attribute("link-policy") == "open")
                    {
                        dialog->setLinkPolicy("open");
                    }
                    else if (command.attribute("link-policy") == "open-external")
                    {
                        dialog->setLinkPolicy("open-external");
                    }
                    else if (command.attribute("link-policy") == "follow")
                    {
                        dialog->setLinkPolicy("follow");
                    }
                }
                dialog->exec();
#endif
            }
            else if ((command.attribute("type") == "open"))
            {
                QDesktopServices::openUrl(QUrl(command.attribute("uri")));
            }
            else if ((command.attribute("type") == "set"))
            {
                QSettings().setValue(command.attribute("key"), command.attribute("value"));
            }
            else if (command.attribute("type") == "die")
            {
                QApplication::quit();
            }
        }
    }
    emit updateInfoProcessed();
}

void ClipGrab::startUpdateDownload()
{
    QString updateUrl = this->availableUpdates.last().url;
    QString updateFilePattern = updateUrl.split("/").last();
    updateFilePattern.insert(updateFilePattern.lastIndexOf(QRegExp("\\.dmg|\\.exe|\\.tar")), "-XXXXXX");
    this->updateFile = new QTemporaryFile(QDir::tempPath() + "/" + updateFilePattern);
    this->updateFile->open();
    qDebug().noquote() << "Downloading update to " << this->updateFile->fileName();

#if USE_WEBENGINE
    this->updateMessageUi->buttonConfirm->setDisabled(true);
    this->updateMessageUi->buttonLater->setDisabled(true);
    this->updateMessageUi->buttonSkip->setDisabled(true);
    this->updateMessageUi->progressBar->setMaximum(1);
    this->updateMessageUi->progressBar->setValue(0);
    this->updateMessageUi->progressBar->show();
    this->updateMessageUi->labelDownloadProgress->show();
#endif

    QNetworkAccessManager* updateNAM = new QNetworkAccessManager();
    this->updateReply = updateNAM->get(QNetworkRequest(QUrl(updateUrl)));
    connect(updateReply, SIGNAL(readyRead()), this, SLOT(updateReadyRead()));
    connect(this->updateReply, SIGNAL(finished()), this, SLOT(updateDownloadFinished()));
    connect(this->updateReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(updateDownloadProgress(qint64, qint64)));
}

void ClipGrab::updateReadyRead()
{
    this->updateFile->write(this->updateReply->readAll());
}

void ClipGrab::updateDownloadProgress(qint64 progress, qint64 total)
{
#if USE_WEBENGINE
    int updateProgress = progress/1024/1024;
    int updateSize = total/1024/1024;

    this->updateMessageUi->labelDownloadProgress->setText(tr("Downloading update … %1/%2 MBytes").arg(QString::number(updateProgress), QString::number(updateSize)));
    this->updateMessageUi->progressBar->setMaximum(updateSize);
    this->updateMessageUi->progressBar->setValue(updateProgress);
#else
    (void)progress;
    (void)total;
#endif
}

void ClipGrab::updateDownloadFinished()
{
    if (this->updateReply->error())
    {
        this->errorHandler(tr("There was an error while downloading the update.: %1").arg(this->updateReply->errorString()));
#if USE_WEBENGINE
        this->updateMessageDialog->reject();
#endif
        return;
    }

    this->updateFile->flush();

    QCryptographicHash fileHash(QCryptographicHash::Sha1);
    updateFile->seek(0);
    fileHash.addData(this->updateFile->readAll());
    QString hashResult = fileHash.result().toHex();
    QString updateSha1 = this->availableUpdates.last().sha1;
    if (hashResult != updateSha1)
    {
        this->errorHandler(tr("The fingerprint of the downloaded update file could not be verified: %1 should have been %2").arg(hashResult, updateSha1));
#if USE_WEBENGINE
        this->updateMessageDialog->reject();
#endif
        return;
    }

    //Close and rename to avoid problems with file locks
    updateFile->setAutoRemove(false);
    updateFile->close();
    updateFile->rename(updateFile->fileName().insert(updateFile->fileName().lastIndexOf(QRegExp("\\.dmg|\\.exe|\\.tar")), "-update"));

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(updateFile->fileName())))
    {
        errorHandler(tr("Could not open update file."));
        this->updateFile->remove();
#if USE_WEBENGINE
        this->updateMessageDialog->reject();
#endif
        return;
    }

    settings.setValue("updateFile", this->updateFile->fileName());
    QApplication::quit();
}
#endif

void ClipGrab::downloadYoutubeDl(bool force) {
    QString minVersion = QSettings().value("minYoutubeDlVersion", "2020.12.07").toString();
    bool youtubeDlInstalled = !YoutubeDl::find().isEmpty();
    if (force == false && youtubeDlInstalled) {
        QString installedVersion = YoutubeDl::getVersion();

        qDebug().noquote() << "Found " << YoutubeDl::executable << " " << installedVersion;
        if (installedVersion >= minVersion) return;
    }
    if (QSettings().value("disableYoutubeDlDownload", false).toBool()) return;

    showDownloaderDlg(TypedHelperDownloader::YTDL);
    if (this->helperDownloaderDialog->exec() != QDialog::Accepted) {
        QApplication::quit();
        QApplication::exit();
    }
}

void ClipGrab::showDownloaderDlg(TypedHelperDownloader::DownloaderType dlgType)
{
    if ( !this->helperDownloaderDialog )
    {
        this->helperDownloaderDialog = new QDialog(QApplication::activeWindow());
        this->helperDownloaderUi = new TypedHelperDownloader();
        this->helperDownloaderUi->setupUi(this->helperDownloaderDialog);

        connect(this->helperDownloaderUi->exitButton, &QPushButton::clicked, this->helperDownloaderDialog, &QDialog::reject);
        connect(this->helperDownloaderUi->continueButton, &QPushButton::clicked, this, &ClipGrab::startYoutubeDlDownload);
    }
    this->helperDownloaderUi->setDialogType(dlgType);
    this->helperDownloaderDialog->show();
}

void ClipGrab::startYoutubeDlDownload()
{
    showDownloaderDlg(TypedHelperDownloader::YTDL);
    this->helperDownloaderDialog->setDisabled(true);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().exists(dir)) {
        QDir().mkpath(dir);
    }

    this->youtubeDlFile = new  QFile(dir + "/" + YoutubeDl::executable);
    youtubeDlFile->open(QFile::WriteOnly);
    if (!youtubeDlFile->isOpen()) {
        errorHandler(tr("Unable to write to %1").arg(youtubeDlFile->fileName()));
        QApplication::quit();
    }

    QString youtubeDlUrl = settings.value("youtubeDlUrl", YoutubeDl::download_url).toString();
    QNetworkRequest request;
    request.setUrl(QUrl(youtubeDlUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkAccessManager* youtubeDlNAM = new QNetworkAccessManager();
    QNetworkReply* reply = youtubeDlNAM->get(request);

    connect(reply, &QNetworkReply::readyRead, [=]() {
        youtubeDlFile->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal) {
        this->helperDownloaderUi->progressBar->setMaximum(bytesTotal);
        this->helperDownloaderUi->progressBar->setValue(bytesReceived);
    });

    #if QT_VERSION >= 0x051200
    connect(reply, &QNetworkReply::sslErrors, [=](QList<QSslError> errors) {
        for (int i = 0; i < errors.length(); i++) {
            QString errorString = errors.at(i).errorString();
            QString certInfo = errors.at(i).certificate().issuerDisplayName() + " " + errors.at(i).certificate().subjectDisplayName() + " " +  errors.at(i).certificate().serialNumber() + " " + errors.at(i).error();
            errorHandler(tr("SSL error: %1 \n%2").arg(errorString).arg(certInfo));
        }
    });
    #endif
    connect(reply, &QNetworkReply::finished, [=] {
        youtubeDlFile->close();

        if (reply->error() != QNetworkReply::NetworkError::NoError) {
            errorHandler(tr("Error downloading youtube-dl: %1").arg(reply->errorString()));
            QApplication::quit();
        }

        this->helperDownloaderDialog->accept();
        emit youtubeDlDownloadFinished();
    });
}

void ClipGrab::startFFmpegDownload(QString url)
{
    showDownloaderDlg(TypedHelperDownloader::FFMPEG);
    this->helperDownloaderDialog->setDisabled(true);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!QDir().exists(dir)) {
        QDir().mkpath(dir);
    }

    int posLastSlash = url.lastIndexOf("/");
    QString filename = url.mid(posLastSlash+1);
    QString filePath = dir + "/" + filename;

    this->ffmpegFile = new  QFile(filePath);
    ffmpegFile->open(QFile::WriteOnly);
    if (!ffmpegFile->isOpen()) {
        errorHandler(tr("Unable to write to %1").arg(ffmpegFile->fileName()));
        QApplication::quit();
    }

    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkAccessManager* youtubeDlNAM = new QNetworkAccessManager();
    QNetworkReply* reply = youtubeDlNAM->get(request);

    connect(reply, &QNetworkReply::readyRead, [=]() {
        ffmpegFile->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::downloadProgress, [=](qint64 bytesReceived, qint64 bytesTotal) {
        this->helperDownloaderUi->progressBar->setMaximum(bytesTotal);
        this->helperDownloaderUi->progressBar->setValue(bytesReceived);
    });

#if QT_VERSION >= 0x051200
    connect(reply, &QNetworkReply::sslErrors, [=](QList<QSslError> errors) {
        for (int i = 0; i < errors.length(); i++) {
            QString errorString = errors.at(i).errorString();
            QString certInfo = errors.at(i).certificate().issuerDisplayName() + " " + errors.at(i).certificate().subjectDisplayName() + " " +  errors.at(i).certificate().serialNumber() + " " + errors.at(i).error();
            errorHandler(tr("SSL error: %1 \n%2").arg(errorString).arg(certInfo));
        }
    });
#endif
    connect(reply, &QNetworkReply::finished, [=] {
        ffmpegFile->close();

        if (reply->error() != QNetworkReply::NetworkError::NoError) {
            errorHandler(tr("Error downloading ffmpeg: %1").arg(reply->errorString()));
            QApplication::quit();
        }

        this->helperDownloaderDialog->accept();
        emit FFmpegDownloadFinished(filePath);
    });
}

void ClipGrab::updateYoutubeDl() {
    if (QSettings().value("disableYoutubeDlUpdate", false).toBool()) return;
    youtubeDlUpdateProcess = YoutubeDl::instance(QStringList() << "--update");
    youtubeDlUpdateProcess->start();
    connect(youtubeDlUpdateProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=] {
        youtubeDlUpdateProcess->deleteLater();
        youtubeDlUpdateProcess = nullptr;
    });
}

#if CLIPGRAB_ORG_UPDATER
void ClipGrab::skipUpdate()
{
    settings.setValue("skip-" + this->availableUpdates.last().version, true);
}
#endif

void ClipGrab::errorHandler(QString error)
{
    QMessageBox box;
    box.setText(error);
    box.setStandardButtons(QMessageBox::Ok);
    box.exec();
}

void ClipGrab::errorHandler(QString error, video* /*video*/)
{
    bool solved = false;
    if (!solved)
    {
        QMessageBox box;
        box.setText(error);
        box.exec();
    }
}

video* ClipGrab::getCurrentVideo() {
    return currentVideo;
}

void ClipGrab::enqueueDownload(video* video) {
    if (video == nullptr || video->getState() != video::state::fetched || downloads.contains(video)) return;

    connect(video, &video::stateChanged, [=] {
        if (video->getState() == video::state::finished) {
            emit downloadFinished(video);
            if (QSettings().value("RemoveFinishedDownloads", false).toBool()) {
                downloads.removeAll(video);
            }
        }
       if (getRunningDownloadsCount() == 0) emit allDownloadsCanceled();
    });

    downloads.append(video);
    emit downloadEnqueued();
    video->download();
}

void ClipGrab::cancelAllDownloads() {
    std::for_each(downloads.begin(), downloads.end(), [=] (video* listVideo) {
        connect(listVideo, &video::stateChanged, [=] {
           if (getRunningDownloadsCount() == 0) emit allDownloadsCanceled();
        });
        listVideo->cancel();
    });
}

void ClipGrab::clipboardChanged() {
    QString url = QApplication::clipboard()->text();
    if (isKnownVideoUrl(url)) {
        this->clipboardUrl = url;
        emit compatibleUrlFoundInClipboard(url);
    }
    else if (isUrl(url)) {
        this->clipboardUrl = url;
        emit UrlFoundInClipboard(url);
    }
}

int ClipGrab::getRunningDownloadsCount() {
    return std::count_if(downloads.begin(), downloads.end(), [](video* listVideo) {
        return listVideo->getState() != video::state::finished
            && listVideo->getState() != video::state::error
            && listVideo->getState() != video::state::canceled;
    });
}

QPair<qint64, qint64> ClipGrab::getDownloadProgress() {
    QPair<qint64, qint64> returnValue;
    returnValue.first = 0;
    returnValue.second = 0;
    for (int i = 0; i < downloads.size(); ++i) {
        if (downloads.at(i)->getState() != video::state::downloading) continue;
        returnValue.first += downloads.at(i)->getDownloadSize();
        returnValue.second += downloads.at(i)->getDownloadProgress();
    }
    return returnValue;
}

void ClipGrab::activateProxySettings() {
    if (settings.value("UseProxy", false).toBool()) {
        QNetworkProxy proxy;
        if (settings.value("ProxyType", false).toInt() == 0) {
            proxy.setType(QNetworkProxy::HttpProxy);
        } else {
            proxy.setType(QNetworkProxy::Socks5Proxy);
        }
        proxy.setHostName(settings.value("ProxyHost", "").toString());
        proxy.setPort(settings.value("ProxyPort", "").toInt());
        if (settings.value("ProxyAuthenticationRequired", false).toBool() == true) {
            proxy.setUser(settings.value("ProxyUsername", "").toString());
            proxy.setPassword(settings.value("ProxyPassword").toString());
        }
        QNetworkProxy::setApplicationProxy(proxy);
    } else {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
}

bool ClipGrab::isKnownVideoUrl(QString url) const {
    return std::any_of(re_video_urls.begin(), re_video_urls.end(), [=](QRegularExpression re) {
        QRegularExpressionMatch match = re.match(url);
        return match.hasMatch();
    });
}

bool ClipGrab::isUrl(QString url) const {
    QRegularExpressionMatch match = re_is_url.match(url);
    return match.hasMatch();
}

void ClipGrab::openTargetFolder(video *video)
{
    if (video->getState() == video::state::finished)
    {
        #ifdef Q_OS_MAC
            QProcess* finderProcess = new QProcess();
            QStringList arguments;
            arguments << "-e" << "tell application \"Finder\"";
            arguments << "-e" << "reveal POSIX file \"" + video->getFinalFilename() + "\"";
            arguments << "-e" << "activate";
            arguments << "-e" << "end tell";
            finderProcess->start("osascript", arguments);
            return;
        #elif defined(Q_OS_WIN)
            QProcess* explorerProcess = new QProcess();
            explorerProcess->start("explorer.exe /select,"  + QDir::toNativeSeparators(video->getFinalFilename()));
            return;
        #endif
    }

    QString path = QFileInfo(video->getTargetFilename()).absoluteDir().path();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void ClipGrab::openDownload(video* video)
{
    if (video->getState() == video::state::finished) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(video->getFinalFilename()));
    } else {
        openTargetFolder(video);
    }
}

QString ClipGrab::humanizeBytes(qint64 bytes) {
    QStringList units {tr("B"), tr("KiB"), tr("MiB"), tr("GiB"), tr("TiB")};

    for (int i = units.size() - 1; i >= 0; i--) {
        qint64 factor = pow(1024, i);
        if (bytes > factor) {
            return QString::number((double) bytes / factor, 'f', 1) + QChar(0x2009) + units.at(i);
        }
    }

    return "";
}

QString ClipGrab::humanizeSeconds(qint64 seconds) {
    if (seconds <= 0) return "";

    QString hours = QString::number(seconds / (60 * 60)).rightJustified(2, '0');
    QString minutes = QString::number(seconds % (60 * 60) / 60).rightJustified(2, '0');
    QString remainingSeconds = QString::number(seconds % 60).rightJustified(2, '0');

    if (hours != "00") {
        return hours + ":" + minutes + ":" + remainingSeconds;
    } else {
        return minutes + ":" + remainingSeconds;
    }
}

void ClipGrab::search(QString keywords) {
    if (currentSearch != nullptr) currentVideo->deleteLater();

    currentSearch = new video();
    connect(currentSearch, &video::stateChanged, [=] {emit searchFinished(currentSearch);});

    if (keywords.isEmpty()) {
        currentSearch->fetchPlaylistInfo("https://www.youtube.com/playlist?list=PL6B3937A5D230E335");
    } else {
        currentSearch->fetchPlaylistInfo("ytsearch16:\"" + keywords + "\"");
    }
}

void ClipGrab::clearTempFiles() {
    QString fileTemplate = QDir::cleanPath(
                QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                QDir::separator() +
                "/youtube-dl-%(id)s-%(format_id)s.%(ext)s"
    );
    QDir tempLocation = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList files = tempLocation.entryList(QStringList("cg-youtube-dl-*"));
    for (int i = 0; i < files.length(); i++) {
        QFile::remove(files.at(i));
    }
}

void ClipGrab::handleFFmpegPathAndVersion(QString path, QString version) {
    qDebug().noquote() << "ffmpeg converter reported path " << path << " version " << version;
    ffmpegPath_ = path;
    ffmpegVersion_ = version;
}
