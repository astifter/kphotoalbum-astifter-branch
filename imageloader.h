/*
 *  Copyright (c) 2003-2004 Jesper K. Pedersen <blackie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#ifndef IMAGELOADER_H
#define IMAGELOADER_H
#include <qthread.h>
class ImageManager;
class QWaitCondition;
class LoadInfo;

class ImageLoader :public QThread {
public:
    ImageLoader( QWaitCondition* sleeper );
    static QImage rotateAndScale( QImage, int width, int height, int angle );
protected:
    virtual void run();

private:
    QWaitCondition* _sleeper;
};

#endif /* IMAGELOADER_H */

