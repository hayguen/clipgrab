#ifndef HELPER_DOWNLOADER_H
#define HELPER_DOWNLOADER_H

#include <QDialog>

#include "ui_helper_downloader.h"


class TypedHelperDownloader: public Ui::HelperDownloader
{
public:
    explicit TypedHelperDownloader();
    ~TypedHelperDownloader();

    enum DownloaderType { YTDL, FFMPEG };
    void setDialogType(DownloaderType type);
};

#endif
