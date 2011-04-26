/* Copyright (C) 2003-2010 Jesper K. Pedersen <blackie@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "ImageLoader.h"
#include "ThumbnailCache.h"

#include "ImageDecoder.h"
#include "Manager.h"
#include "RawImageDecoder.h"
#include "Utilities/Util.h"

#include <qapplication.h>
#include <qfileinfo.h>

extern "C" {
 #include <limits.h>
 #include <setjmp.h>
 #include <stdio.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <unistd.h>
}

#include <kcodecs.h>
#include <kurl.h>
#include <qmatrix.h>
#include "ImageEvent.h"

namespace ImageManager
{
    // Create a global instance. Its constructor will itself register it.
    RAWImageDecoder rawdecoder;
}



void ImageManager::ImageLoader::run()
{
    while ( true ) {
        ImageRequest* request = Manager::instance()->next();
        Q_ASSERT( request );
        bool ok;

        QImage img = loadImage( request, ok );

        if ( ok ) {
            img = scaleAndRotate( request, img );
        }

        request->setLoadedOK( ok );
        ImageEvent* iew = new ImageEvent( request, img );
        QApplication::postEvent( Manager::instance(),  iew );
    }
}

QImage ImageManager::ImageLoader::rotateAndScale( QImage img, int width, int height, int angle )
{
    if ( angle != 0 )  {
        QMatrix matrix;
        matrix.rotate( angle );
        img = img.transformed( matrix );
    }
    img = Utilities::scaleImage(img, width, height, Qt::KeepAspectRatio );
    return img;
}

QImage ImageManager::ImageLoader::loadImage( ImageRequest* request, bool& ok )
{
    int dim = calcLoadSize( request );
    QSize fullSize;

    ok = false;
    if ( !QFile( request->fileName() ).exists() )
        return QImage();

    QImage img;
    if (Utilities::isJPEG(request->fileName())) {
        ok = Utilities::loadJPEG(&img, request->fileName(),  &fullSize, dim);
        if (ok == true)
            request->setFullSize( fullSize );
    }

    else {
        // At first, we have to give our RAW decoders a try. If we allowed
        // QImage's load() method, it'd for example load a tiny thumbnail from
        // NEF files, which is not what we want.
        ok = ImageDecoder::decode( &img, request->fileName(),  &fullSize, dim);
        if (ok)
            request->setFullSize( img.size() );
    }

    if (!ok) {
        // Now we can try QImage's stuff as a fallback...
        ok = img.load( request->fileName() );
        if (ok)
            request->setFullSize( img.size() );

    }

    return img;
}


int ImageManager::ImageLoader::calcLoadSize( ImageRequest* request )
{
    return qMax( request->width(), request->height() );
}

QImage ImageManager::ImageLoader::scaleAndRotate( ImageRequest* request, QImage img )
{
    if ( request->angle() != 0 )  {
        QMatrix matrix;
        matrix.rotate( request->angle() );
        img = img.transformed( matrix );
        int angle = (request->angle() + 360)%360;
        Q_ASSERT( angle >= 0 && angle <= 360 );
        if ( angle == 90 || angle == 270 )
            request->setFullSize( QSize( request->fullSize().height(), request->fullSize().width() ) );
    }

    // If we are looking for a scaled version, then scale
    if ( shouldImageBeScale( img, request ) )
        img = Utilities::scaleImage(img, request->width(), request->height(), Qt::KeepAspectRatio );

    return img;
}

bool ImageManager::ImageLoader::shouldImageBeScale( const QImage& img, ImageRequest* request )
{
    // No size specified, meaning we want it full size.
    if ( request->width() == -1 )
        return false;

    if ( img.width() < request->width() && img.height() < request->height() ) {
        // The image is smaller than the requets.
        return request->doUpScale();
    }

    return true;
}

