/* Copyright (C) 2003-2006 Jesper K. Pedersen <blackie@kde.org>

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

#ifndef VIEWER_H
#define VIEWER_H

#include <QStackedWidget>
#include "Settings/SettingsData.h"
#include <kaction.h>
#include <qdialog.h>
#include <qimage.h>
//Added by qt3to4:
#include <QContextMenuEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPixmap>
#include <QLabel>
#include <Q3PtrList>
#include <QMenu>
#include <QKeyEvent>
#include <config-kpa-exiv2.h>

class QStackedWidget;
class KActionCollection;
class QLabel;
class QMenu;
class KAction;

namespace DB { class ImageInfo; }
namespace MainWindow { class ExternalPopup; }

namespace Viewer
{
class VideoDisplay;
class ImageDisplay;
class TextDisplay;
class Display;
class SpeedDisplay;
class InfoBox;

class ViewerWidget :public QStackedWidget
{
    Q_OBJECT
public:
    ViewerWidget();
    ~ViewerWidget();
    static ViewerWidget* latest();
    void load( const QStringList& list, int index = 0 );
    void infoBoxMove();
    bool showingFullScreen() const;
    void setShowFullScreen( bool on );
    void show( bool slideShow );
    KActionCollection* actions();

public slots:
    virtual bool close(bool alsoDelete = false );
    void updateInfoBox();
    void test();
    void moveInfoBox( int );

protected:
    virtual void contextMenuEvent ( QContextMenuEvent * e );
    virtual void resizeEvent( QResizeEvent* );
    virtual void keyPressEvent( QKeyEvent* );
    virtual void wheelEvent( QWheelEvent* event );

    void moveInfoBox();
    void setAsWallpaper(int mode);

    void load();
    void setupContextMenu();
    void createShowContextMenu();
    void createWallPaperMenu();
    void createInvokeExternalMenu();
    void createRotateMenu();
    void createSkipMenu();
    void createZoomMenu();
    void createSlideShowMenu();
    void createVideoMenu();
    void changeSlideShowInterval( int delta );

    DB::ImageInfoPtr currentInfo() const;
    friend class InfoBox;

private:
    void showNextN(int);
    void showPrevN(int);

protected slots:
    void showNext();
    void showNext10();
    void showNext100();
    void showNext1000();
    void showPrev();
    void showPrev10();
    void showPrev100();
    void showPrev1000();
    void showFirst();
    void showLast();
    void rotate90();
    void rotate180();
    void rotate270();
    void toggleShowInfoBox( bool );
    void toggleShowLabel( bool );
    void toggleShowDescription( bool );
    void toggleShowDate( bool );
    void toggleShowTime( bool );
    void toggleShowFilename( bool );
    void toggleShowEXIF( bool );
    void toggleShowImageSize( bool );
    void toggleShowOption( const QString& category, bool b ) ;
    void toggleFullScreen();
    void slotStartStopSlideShow();
    void slotSlideShowNext();
    void slotSlideShowNextFromTimer();
    void slotSlideShowFaster();
    void slotSlideShowSlower();
    void editImage();
    void makeCategoryImage();
    void updateCategoryConfig();
    void slotSetWallpaperC();
    void slotSetWallpaperT();
    void slotSetWallpaperCT();
    void slotSetWallpaperCM();
    void slotSetWallpaperTM();
    void slotSetWallpaperS();
    void slotSetWallpaperCAF();
    void populateExternalPopup();
    void videoStopped();
    void showExifViewer();
    void zoomIn();
    void zoomOut();
    void zoomFull();
    void zoomPixelForPixel();

    /** Set the current window title (filename) and add the given detail */
    void setCaptionWithDetail( const QString& detail );

private:
    static ViewerWidget* _latest;

    QList<KAction*> _forwardActions;
    QList<KAction*> _backwardActions;

    KAction* _startStopSlideShow;
    KAction* _slideShowRunFaster;
    KAction* _slideShowRunSlower;

    Display* _display;
    ImageDisplay* _imageDisplay;
    VideoDisplay* _videoDisplay;
    TextDisplay* _textDisplay;

    QStringList _list;
    int _current;
    QRect _textRect;
    QMenu* _popup;
    QMenu* _rotateMenu;
    QMenu* _wallpaperMenu;
    MainWindow::ExternalPopup* _externalPopup;
    int _width, _height;
    QPixmap _pixmap;

    KAction* _delete;
    KAction* _categoryEditor;
#ifdef HAVE_EXIV2
    KAction* _showExifViewer;
#endif

    InfoBox* _infoBox;
    QImage _currentImage;

    bool _showingFullScreen;

    int _slideShowPause;
    SpeedDisplay* _speedDisplay;
    KActionCollection* _actions;
    bool _forward;
    QTimer* _slideShowTimer;
    bool _isRunningSlideShow;

    QList<QAction*> _videoActions;
    KAction* _stop;
    KAction* _playPause;
};

}

#endif /* VIEWER_H */

