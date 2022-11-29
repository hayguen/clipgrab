#include "youtube_dl.h"

#if USE_YTDLP
// A youtube-dl fork with additional features and fixes
// see https://github.com/yt-dlp/yt-dlp
#if defined(Q_OS_WIN)
#define NEED_PYTHON 0
const char * YoutubeDl::python = "";
const char * YoutubeDl::executable = "yt-dlp.exe";
#else
#define NEED_PYTHON 1
const char * YoutubeDl::python = "python3";
const char * YoutubeDl::executable = "yt-dlp";
#endif
const char * YoutubeDl::ytdl_name = "yt-dlp";
const char * YoutubeDl::homepage_url = "https://github.com/yt-dlp/yt-dlp";
const char * YoutubeDl::homepage_short = "github.com/yt-dlp/yt-dlp";

const char * YoutubeDl::version_url = "https://raw.githubusercontent.com/yt-dlp/yt-dlp/master/yt_dlp/version.py";
#if defined(Q_OS_LINUX)
const char * YoutubeDl::download_url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp";
#elif defined(Q_OS_WIN)
const char * YoutubeDl::download_url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe";
#elif defined(Q_OS_MAC)
const char * YoutubeDl::download_url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_macos";
#else
const char * YoutubeDl::download_url = "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp";
#endif

#else
// (probably) the origin of youtube-dl
// see https://yt-dl.org
#define NEED_PYTHON 1
const char * YoutubeDl::python = "python";
const char * YoutubeDl::executable = "youtube-dl";
const char * YoutubeDl::ytdl_name = "youtube-dl";
const char * YoutubeDl::version_url = "https://raw.githubusercontent.com/ytdl-org/youtube-dl/master/youtube_dl/version.py";
const char * YoutubeDl::download_url = "https://yt-dl.org/downloads/latest/youtube-dl";
const char * YoutubeDl::homepage_url = "https://youtube-dl.org";
const char * YoutubeDl::homepage_short = "youtube-dl.org";

#endif


YoutubeDl::YoutubeDl()
{

}

QString YoutubeDl::yt_dn_path = QString();

QString YoutubeDl::find(bool force) {
    if (!force && !yt_dn_path.isEmpty()) return yt_dn_path;

    // Prefer downloaded youtube-dl
    QString localPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, YoutubeDl::executable);
    if (!localPath.isEmpty()) {
        QProcess* process = instance(localPath, QStringList() << "--version", false);
        process->start();
        process->waitForFinished();
        process->deleteLater();
        if (process->state() != QProcess::NotRunning) process->kill();
        if (process->exitCode() == QProcess::ExitStatus::NormalExit) {
            yt_dn_path = localPath;
            qDebug().noquote().nospace() << "Found " << YoutubeDl::executable << " in AppDataLocation at " << yt_dn_path;
            return yt_dn_path;
        }
    }

    // Try system-wide youtube-dl installation
    QString globalPath = QStandardPaths::findExecutable(YoutubeDl::executable);
    if (!globalPath.isEmpty()) {
        QProcess* process = instance(globalPath, QStringList() << "--version", false);
        process->start();
        process->waitForFinished();
        process->deleteLater();
        if (process->state() != QProcess::NotRunning) process->kill();
        if (process->exitCode() == QProcess::ExitStatus::NormalExit) {
            yt_dn_path = globalPath;
            qDebug().noquote().nospace() << "Found " << YoutubeDl::executable << " executable at " << yt_dn_path;
            return yt_dn_path;
        }
    }

    qDebug().noquote().nospace() << "Error: could not find " << YoutubeDl::executable;
    return "";
}

QProcess* YoutubeDl::instance(QStringList arguments) {
    return instance(find(), arguments, true);
}

QProcess* YoutubeDl::instance(QString python_program, QStringList arguments, bool addNetworkArgs) {
    QProcess *process = new QProcess();

    QString execPath = QCoreApplication::applicationDirPath();
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", execPath + ":" + env.value("PATH"));
    process->setEnvironment(env.toStringList());

    bool add_pyprog_arg = true;
#if defined Q_OS_WIN
    if ( python_program.endsWith(".exe", Qt::CaseInsensitive) ) {
        process->setProgram(python_program);
        add_pyprog_arg = false;
    }
    else
        process->setProgram(execPath + "/python/python.exe");
#else
    if ( !python_program.isEmpty() ) {
        QFileDevice::Permissions perm = QFile(python_program).permissions() | QFile::ExeUser | QFile::ExeGroup | QFile::ExeOther;
        QFile::setPermissions(python_program, perm);
        perm = QFile(python_program).permissions();
        if ( !( perm & (QFile::ExeGroup | QFile::ExeOther | QFile::ExeUser) ) ) {
            process->setProgram(python_program);
            add_pyprog_arg = false;
        }
    }
    if ( add_pyprog_arg )
        process->setProgram(QStandardPaths::findExecutable(YoutubeDl::python));
#endif

    QStringList args;
    if ( add_pyprog_arg )
        args << python_program;

    args << arguments;

    if ( addNetworkArgs ) {
        QSettings settings;
        if (settings.value("UseProxy", false).toBool()) {
            QUrl proxyUrl;

            proxyUrl.setHost(settings.value("ProxyHost", "").toString());
            proxyUrl.setPort(settings.value("ProxyPort", "").toInt());

            if (settings.value("ProxyType", false).toInt() == 0) {
                proxyUrl.setScheme("http");
            } else {
                proxyUrl.setScheme("socks5");
            }
            if (settings.value("ProxyAuthenticationRequired", false).toBool() == true) {
                proxyUrl.setUserName(settings.value("ProxyUsername", "").toString());
                proxyUrl.setPassword(settings.value("ProxyPassword").toString());
            }

            args << "--proxy" << proxyUrl.toString();
        }

        if (settings.value("forceIpV4", false).toBool()) {
            args << "--force-ipv4";
        }
    }

    while ( args.length() && args[0].isEmpty() )
        args.removeAt(0);  // python doesn't like empty argument "" before "--version" !

    process->setArguments(args);
#if 0
    if ( !add_pyprog_arg )
        qDebug().noquote().nospace() << "starting " << python_program << " without interpreter: " << process->arguments();
    else
        qDebug().noquote().nospace() << "starting " << python_program << " with " << process->program() << " interpreter: " << process->arguments();
#endif
    return process;
}

QString YoutubeDl::getVersion() {
    if (find().isEmpty())
        return QString::null;
    QProcess* youtubeDl = instance(QStringList("--version"));
    youtubeDl->start();
    youtubeDl->waitForFinished(10000);
    QString version = youtubeDl->readAllStandardOutput() + youtubeDl->readAllStandardError();
    youtubeDl->deleteLater();
    QString ret = version.replace("\n", "");
    // qDebug().noquote().nospace() << "getVersion() -> " << ret;
    return ret;
}

QString YoutubeDl::getPythonVersion() {
#if (NEED_PYTHON == 0)
    return "";
#endif
    QProcess* youtubeDl = instance("", QStringList("--version"), false);
    youtubeDl->start();
    youtubeDl->waitForFinished(10000);
    QString version = youtubeDl->readAllStandardOutput() + youtubeDl->readAllStandardError();
    youtubeDl->deleteLater();
    QString ret = version.replace("\n", "");
    // qDebug().noquote().nospace() << "getPythonVersion() -> " << ret;
    return ret;
}

QString YoutubeDl::findPython() {
#if (NEED_PYTHON == 0)
    return "";
#endif
    QProcess* youtubeDl = instance("", QStringList("--version"), false);
    QString program = youtubeDl->program();
    youtubeDl->deleteLater();
    // qDebug().noquote().nospace() << "findPython() -> " << program;
    return program;
}
