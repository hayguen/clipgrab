
#include "helper_downloader.h"

#include "youtube_dl.h"
#include "converter_ffmpeg.h"

TypedHelperDownloader::TypedHelperDownloader()
    : Ui::HelperDownloader()
{
}

TypedHelperDownloader::~TypedHelperDownloader()
{
}

void TypedHelperDownloader::setDialogType(DownloaderType t)
{
    switch (t) {
    case YTDL:
        exitButton->show();
        labelInfoText->setText(QApplication::translate(
            "HelperDownloader",
            "<p>ClipGrab uses %1 in order to download videos from the Internet. "
            "%1 is developed by an independent team of Open Source developers and "
            "released into the public domain.<br>"
            "Learn more on <a href=\"%2\">%3</a>.</p>"
            "<p>Click on <em>Continue</em> to download %1.</p>",
            nullptr
        ).arg(YoutubeDl::executable, YoutubeDl::homepage_url, YoutubeDl::homepage_short) );
        label->setText(QApplication::translate("HelperDownloader", "Downloading %1", nullptr).arg(YoutubeDl::executable));
        break;
    case FFMPEG:
        exitButton->hide();
        labelInfoText->setText(QApplication::translate(
            "HelperDownloader",
            "<p>ClipGrab uses %1 in order to convert downloaded videos. "
            "%1 is developed by an independent team of Open Source developers. "
            "It is free software.<br>"
            "Learn more on <a href=\"%2\">%3</a>.</p>"
            "<p>Click on <em>Continue</em> to download %1.</p>",
            nullptr
        ).arg(converter_ffmpeg::name, converter_ffmpeg::homepage_url, converter_ffmpeg::homepage_short) );
        label->setText(QApplication::translate("HelperDownloader", "Downloading %1", nullptr).arg(converter_ffmpeg::name));
        break;
    }
}
