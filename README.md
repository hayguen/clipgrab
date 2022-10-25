
# ClipGrab

ClipGrab was a GUI frontend for [youtube-dl](https://youtube-dl.org).
Now, it's a GUI frontend for [yt-dlp](https://github.com/yt-dlp/yt-dlp).

See below for instructions, if you are building yourself.

Prebuilt [AppImage Download](https://clipgrab.org/) images are available and recommended.

You'll need to make the AppImage executable, then you can run it:

```sh
# Set version of ClipGrab.  As of Oct 31, 2021 latest version is 3.9.7
CLIPGRAB_VER=3.9.7

# Change directory to ~/Downloads and download latest AppImage
cd ~/Downloads
wget "https://download.clipgrab.org/ClipGrab-${CLIPGRAB_VER}-x86_64.AppImage"

# Make downloaded AppImage executable
chmod +x "ClipGrab-${CLIPGRAB_VER}-x86_64.AppImage"

# Run it!
./ClipGrab-${CLIPGRAB_VER}-x86_64.AppImage
```


## Keyboard Shortcuts

A few keyboard shortcuts might ease usage:

* `Ctrl+1`: switch to Download Tab
* `Ctrl+2`: switch to Settings Tab
* `Ctrl+3`: switch to About Tab

* `Ctrl+V`: paste Clipboard contents into the Download-URL field
* `Ctrl+F` / `F`: popup Format combobox
* `Ctrl+Q` / `Q`: popup Quality combobox
* `Ctrl+G` / `G`: starts grabbing if ready or pastes Clipboard contents into the Download-URL field

`F` / `Q` / `G` : The Control modifier `Ctrl` is necessary, if the focus is inside the text field.


## YouTube Downloader

ClipGrab does utilize the YouTube Downloader.
As described in  https://github.com/yt-dlp/yt-dlp/wiki/Forks on 2022-09-19, there are 3 active forks:

* [youtube-dl](https://github.com/ytdl-org/youtube-dl) - The original. Also, see [youtube-dl](https://youtube-dl.org)
* [yt-dlp](https://github.com/yt-dlp/yt-dlp) - This one is currently used in ClipGrab
* [ytdl-patched](https://github.com/ytdl-patched/ytdl-patched) - A fork of yt-dlp
﻿
## Compilation & Translation

This sources on github are customized and modified - compared to the prebuilt download of ClipGrab (http://clipgrab.org/).

### Prerequisites

You need to install the Qt5 developer libraries and programs in order to compile the program.

On Ubuntu and Debian-based system, you can install the necessary libraries like this:
```sh
sudo apt install qtbase5-dev qtwebengine5-dev
```

Updating/editing translations additionally requires:
```sh
sudo apt install qttools5-dev-tools
```

Other Linux distributions might have slightly different package names. Packages for Windows and macOS can be downloaded here:
https://download.qt.io/archive/qt/5.12/
or here:
https://www.qt.io/offline-installers

In addition, `ffmpeg` (no developer libs necessary) and `python3` (for yt-dlp) needs to be installed:
```sh
sudo apt install ffmpeg python3
```

To install all prerequisites:
```sh
sudo apt install qtbase5-dev qtwebengine5-dev qttools5-dev-tools ffmpeg python3
```


### Compilation

To compile ClipGrab, simply execute the following command:
```sh
qmake clipgrab.pro && make
```

This will create an executable "clipgrab" that you can start via `./clipgrab`.


### Translation

When texts were edited in the source files, `lupdate` is to be run to update the `.ts` files before editing - after the qmake step:
```sh
qmake clipgrab.pro
lupdate clipgrab.pro
```

Then, translations can be edited with `linguist` opening the `.ts` files.

`lrelease` compiles the `.ts` files into binary `.qm`, finalizing the translation:
```sh
lrelease clipgrab.pro
```

The binary needs rebuilding to embed the resource files into the executable:
```sh
make
```


## Installation

The built binary executable can simply be copied. It does contain all necessary resources (translations and images):
```sh
mkdir -p ~/.local/bin
cp clipgrab ~/.local/bin/
```

For system-wide installation:
```sh
sudo cp clipgrab /usr/local/bin/
```

If a suitable YouTube-Downloader is found, it's used.
When missing or the version is outdated, it can be downloaded.


## Configuration / Update

It shouldn't be necessary, but the the configuration file can be found here (for Linux):
```
$HOME/.config/ClipGrab/ClipGrab.conf
```

You can edit it manually - or delete it. To prevent write collisions, ClipGrab should be closed for editing.


The downloaded and installed YouTube-Downloader is saved here:
```
$HOME/.local/share/ClipGrab/ClipGrab/yt-dlp
```

The exact filename is also displayed at the bottom of the 'About' tab with it's version.
Rename or delete that file, to enforce an update of the downloader, at next startup.
