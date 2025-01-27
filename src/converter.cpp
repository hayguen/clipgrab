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



#include <QtGui>
#include "converter.h"

converter::converter()
{
}

void converter::startConversion(
    QFile* /*inputFile*/,
    QString& /*targetPath*/,
    QString /*originalExtension*/,
    QString /*metaTitle*/,
    QString /*metaArtist*/,
    int /*mode*/,
    QString /*audio_bitrate*/,
    QString /*audio_quality*/
    )
{
}

void converter::abortConversion()
{
}

QList<QString> converter::getModes() const
{
    return _modes;
}

QString converter::conversion_str() const
{
    return "";
}

QString converter::getExtensionForMode(int /*mode*/) const
{
    return "";
}

bool converter::isAvailable()
{
    return false;
}

converter* converter::createNewInstance()
{
    return new converter();
}

void converter::report_progress(double percent)
{
    emit progress(percent);
}
