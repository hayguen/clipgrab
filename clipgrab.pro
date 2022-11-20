# #####################################################################
# Automatically generated by qmake (2.01a) Mo 26. Okt 18:26:00 2009
# #####################################################################

VERSION = 3.9.11

TEMPLATE = app
TARGET = clipgrab
DEPENDPATH += . \
    release
INCLUDEPATH += src
QT += core
QT += widgets
QT += gui
QT += network

CLIPGRAB_ORG_UPDATER = 0
DEFINES += "HIDE_DONATION=1"

CLIPGRAB_ORG_UPDATER {
  QT += xml
  DEFINES += "CLIPGRAB_ORG_UPDATER=1"
  win32 {
    WEBENGINE = 0
  } else {
    WEBENGINE = 1
  }
} else {
  DEFINES += "CLIPGRAB_ORG_UPDATER=0"
  WEBENGINE = 0
}

WEBENGINE {
  QT += webenginewidgets
  DEFINES += "USE_WEBENGINE=1"
} else {
  DEFINES += "USE_WEBENGINE=0"
}


DEFINES += "USE_YTDLP=1"

MOC_DIR = temp
UI_DIR = temp
OBJECTS_DIR = temp
# QM_FILES_INSTALL_PATH = lang

# Input
HEADERS += \
    src/converter.h \
    src/converter_copy.h \
    src/converter_ffmpeg.h \
    src/download_list_model.h \
    src/helper_downloader.h \
    src/mainwindow.h \
    src/video.h \
    src/notifications.h \
    src/clipgrab.h \
    src/youtube_dl.h

SOURCES += \
    src/converter.cpp \
    src/converter_copy.cpp \
    src/converter_ffmpeg.cpp \
    src/download_list_model.cpp \
    src/helper_downloader.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/video.cpp \
    src/notifications.cpp \
    src/clipgrab.cpp \
    src/youtube_dl.cpp

FORMS += \
    src/ui/metadata-dialog.ui \
    src/ui/helper_downloader.ui \
    src/ui/mainwindow.ui

WEBENGINE {
  FORMS   += src/ui/message_dialog.ui src/ui/update_message.ui
  HEADERS += src/message_dialog.h     src/web_engine_view.h
  SOURCES += src/message_dialog.cpp   src/web_engine_view.cpp
}

RESOURCES += resources.qrc
TRANSLATIONS += lang/clipgrab_bg.ts \
                lang/clipgrab_bn.ts \
                lang/clipgrab_ca.ts \
                lang/clipgrab_cs.ts \
                lang/clipgrab_de.ts \
                lang/clipgrab_el.ts \
                lang/clipgrab_es.ts \
                lang/clipgrab_eu.ts \
                lang/clipgrab_fa.ts \
                lang/clipgrab_fr.ts \
                lang/clipgrab_fi.ts \
                lang/clipgrab_hr.ts \
                lang/clipgrab_hu.ts \
                lang/clipgrab_id.ts \
                lang/clipgrab_it.ts \
                lang/clipgrab_ja.ts \
                lang/clipgrab_ko.ts \
                lang/clipgrab_mk.ts \
                lang/clipgrab_lt.ts \
                lang/clipgrab_nl.ts \
                lang/clipgrab_no.ts \
                lang/clipgrab_pa.ts \
                lang/clipgrab_pl.ts \
                lang/clipgrab_pt.ts \
                lang/clipgrab_ro.ts \
                lang/clipgrab_ru.ts \
                lang/clipgrab_si.ts \
                lang/clipgrab_sr.ts \
                lang/clipgrab_sv.ts \
                lang/clipgrab_sw.ts \
                lang/clipgrab_tr.ts \
                lang/clipgrab_vi.ts \
                lang/clipgrab_zh.ts \
                lang/clipgrab_strings.ts
CODECFORTR = UTF-8
win32:RC_FILE = windows_icon.rc
macx { 
    TARGET = ClipGrab
    ICON = clipgrab.icns
    RC_FILE = clipgrab.icns
    QMAKE_INFO_PLIST = ClipGrab.plist
    LIBS += -framework AppKit -framework Foundation
}
DEFINES += "CLIPGRAB_VERSION=$$VERSION"

package.path = $$(PREFIX)/bin
package.files += $$TARGET$$TARGET_EXT
# package.CONFIG += no_check_exist

extra_install.path = $$(PREFIX)/share/pixmaps
# extra_install.extra = cp img/icon.png clipgrab.png
extra_install.files = img/icon.png


INSTALLS += package
INSTALLS += extra_install
