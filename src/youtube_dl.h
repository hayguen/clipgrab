#ifndef YOUTUBEDL_H
#define YOUTUBEDL_H

#include <QtCore>
#include <QDebug>

class YoutubeDl
{
public:
    YoutubeDl();

    static const char * executable;
    static const char * version_url;   // check for line: __version__ = '2022.10.04'
    static const char * download_url;
    static const char * homepage_url;
    static const char * homepage_short;

    static QString find(bool force = false);  // find the YouTube Downloader
    static QString getVersion();
    static QProcess* instance(QStringList arguments);

    static QString findPython();
    static QString getPythonVersion();

private:
    static QProcess* instance(QString python_program, QStringList arguments, bool addNetworkArgs);

    static QString yt_dn_path;
    static const char * python;
};

#endif // YOUTUBEDL_H
