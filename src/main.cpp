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



#include <QApplication>
#include <QGuiApplication>
#include <QTranslator>
#include <QDebug>
#include "mainwindow.h"
#include "clipgrab.h"
#include "video.h"

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)



int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ClipGrab");
    QCoreApplication::setOrganizationDomain("clipgrab.org");
    QCoreApplication::setApplicationName("ClipGrab");
    QCoreApplication::setApplicationVersion(QString(STRINGIZE_VALUE_OF(CLIPGRAB_VERSION)).replace("\"", ""));

    QCommandLineParser parser;
    parser.setApplicationDescription("ClipGrab");
    parser.addVersionOption();
    parser.addHelpOption();
    QCommandLineOption startMinimizedOption(QStringList() << "start-minimized", "Hide the ClipGrab window on launch");
    parser.addOption(startMinimizedOption);
    QCommandLineOption verboseOption(QStringList() << "verbose", "Print verbose output");
    parser.addOption(verboseOption);
    QCommandLineOption keepOption(QStringList() << "keep", "Keep yt-dl result file(s)");
    parser.addOption(keepOption);
#if CLIPGRAB_ORG_UPDATER
    QCommandLineOption suppress_update_option(QStringList() << "no-update", "Suppress update checking");
    QCommandLineOption perform_update_option(QStringList() << "update", "Check for updates");
    parser.addOption(perform_update_option);
    parser.addOption(suppress_update_option);
#endif
    parser.process(app);

    QSettings settings;
    if (settings.allKeys().isEmpty()) {
        static const QChar key[] = {
            0x0050, 0x0068, 0x0069, 0x006c, 0x0069, 0x0070, 0x0070, 0x0020, 0x0053, 0x0063, 0x0068, 0x006d, 0x0069, 0x0065, 0x0064, 0x0065, 0x0072};
        QSettings legacySettings(QString::fromRawData(key, sizeof(key) / sizeof(QChar)), "ClipGrab");
        QStringList legacyKeys = legacySettings.allKeys();
        QStringList ignoredKeys = {"youtubePlayerUrl", "youtubePlayerJS", "youtubePlayerSignatureMethodName"};
        for (int i = 0; i < legacyKeys.length(); i++) {
            if (ignoredKeys.contains(legacyKeys.at(i))) continue;
            settings.setValue(legacyKeys.at(i), legacySettings.value(legacyKeys.at(i)));
        }
        legacySettings.clear();
    }

    ClipGrab* cg = new ClipGrab();

    QTranslator translator;
    QString locale = settings.value("Language", "auto").toString();
    if (locale == "auto")
    {
        locale = QLocale::system().name();
    }
    translator.load(QString(":/lang/clipgrab_") + locale);
    app.installTranslator(&translator);
    for (int i=0; i < cg->languages.length(); i++)
    {
        if (cg->languages[i].code == locale) {
            if (cg->languages[i].isRTL)
            {
                QApplication::setLayoutDirection(Qt::RightToLeft);
            }
            break;
        }
    }

    bool verbose = parser.isSet(verboseOption);
    settings.setValue("verbose", verbose);

    bool keep = parser.isSet(keepOption);
    settings.setValue("keep", keep);

    MainWindow w(cg);
    w.init();
    if (!parser.isSet(startMinimizedOption)) {
        w.show();
    }

#if CLIPGRAB_ORG_UPDATER
    bool check_update = settings.value("Check-Updates", true).toBool();
    if ( (check_update || parser.isSet(perform_update_option) ) && !parser.isSet(suppress_update_option) ) {
        QTimer::singleShot(0, [=] {
           cg->getUpdateInfo();
           QObject::connect(cg, &ClipGrab::updateInfoProcessed, [cg] {
               bool force = QSettings().value("forceYoutubeDlDownload", false).toBool();
               if (force) QSettings().setValue("forceYoutubeDlDownload", false);
               cg->downloadYoutubeDl(force);
           });
        });
    }
    else {
        qDebug() << "not checking for updates. start clipgrab with command-line option '--update' or '--enable-updates', if desired";
    }
#endif

    return app.exec();
}
