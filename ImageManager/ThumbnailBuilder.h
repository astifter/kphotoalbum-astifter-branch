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

#ifndef THUMBNAILBUILDER_H
#define THUMBNAILBUILDER_H

#include <QList>
#include <QProgressDialog>
#include <QImage>
#include "ImageManager/ImageClient.h"
#include "DB/ImageInfoPtr.h"

namespace MainWindow { class StatusBar; }
namespace MainWindow { class Window; }


namespace ImageManager
{

class ThumbnailBuilder :public QObject, public ImageManager::ImageClient {
    Q_OBJECT

public:
    static ThumbnailBuilder* instance();
    void buildAll();
    void buildMissing();

    OVERRIDE void pixmapLoaded( const QString& fileName, const QSize& size, const QSize& fullSize, int angle, const QImage&, const bool loadedOK);
    OVERRIDE void requestCanceled();

public slots:
    void cancelRequests();
    void build( const QList<DB::ImageInfoPtr>& list );

private:
    friend class MainWindow::Window;
    static ThumbnailBuilder* m_instance;
    ThumbnailBuilder( MainWindow::StatusBar* statusBar, QObject* parent );
    MainWindow::StatusBar* m_statusBar;
    int m_count;
    bool m_isBuilding;
};

}

#endif /* THUMBNAILBUILDER_H */

