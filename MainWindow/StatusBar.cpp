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
#include "StatusBar.h"
#include <QApplication>
#include <QToolButton>
#include <QTimer>
#include <QProgressBar>
#include <QSlider>
#include "DB/ImageDB.h"
#include "ImageCounter.h"
#include "Settings/SettingsData.h"
#include <QLabel>
#include "DirtyIndicator.h"
#include <KHBox>
#include <KVBox>
#include <kiconloader.h>
#include <KIcon>
#include "BackgroundTaskManager/StatusIndicator.h"
#include "RemoteControl/ConnectionIndicator.h"
#include "ThumbnailView/ThumbnailFacade.h"

MainWindow::StatusBar::StatusBar()
    : KStatusBar()
{
    QPalette pal = palette();
    pal.setBrush( QPalette::Base, QApplication::palette().color( QPalette::Background ) );
    pal.setBrush( QPalette::Background, QApplication::palette().color( QPalette::Background ) );
    setPalette( pal );

    setupGUI();
    m_pendingShowTimer = new QTimer(this);
    m_pendingShowTimer->setSingleShot( true );
    connect( m_pendingShowTimer, SIGNAL(timeout()), this, SLOT(showStatusBar()) );
}

void MainWindow::StatusBar::setupGUI()
{
    setContentsMargins(7,2,7,2);

    KHBox* indicators = new KHBox( this );
    indicators->setSpacing(10);
    mp_dirtyIndicator = new DirtyIndicator( indicators );
    connect( DB::ImageDB::instance(), SIGNAL(dirty()), mp_dirtyIndicator, SLOT(markDirtySlot()) );

    new RemoteControl::ConnectionIndicator(indicators);

    KVBox* statusIndicatorBox = new KVBox(indicators);
    new BackgroundTaskManager::StatusIndicator(statusIndicatorBox);
    statusIndicatorBox->setContentsMargins(0,7,0,0);

    m_progressBar = new QProgressBar( this );
    m_progressBar->setMinimumWidth( 400 );
    addPermanentWidget( m_progressBar, 0 );

    m_cancel = new QToolButton( this );
    m_cancel->setIcon( KIcon( QString::fromLatin1( "dialog-close" ) ) );
    m_cancel->setShortcut( Qt::Key_Escape );
    addPermanentWidget( m_cancel, 0 );
    connect( m_cancel, SIGNAL(clicked()), this, SIGNAL(cancelRequest()) );
    connect( m_cancel, SIGNAL(clicked()), this, SLOT(hideStatusBar()) );

    m_lockedIndicator = new QLabel( indicators );

    addPermanentWidget( indicators, 0 );

    mp_partial = new ImageCounter( this );
    addPermanentWidget( mp_partial, 0 );

    mp_selected = new ImageCounter( this );
    addPermanentWidget( mp_selected, 0);

    ImageCounter* total = new ImageCounter( this );
    addPermanentWidget( total, 0 );
    total->setTotal( DB::ImageDB::instance()->totalCount() );
    connect( DB::ImageDB::instance(), SIGNAL(totalChanged(uint)), total, SLOT(setTotal(uint)) );

    mp_pathIndicator = new BreadcrumbViewer;
    addWidget( mp_pathIndicator, 1 );

    setProgressBarVisible( false );

    m_thumbnailSizeSlider = ThumbnailView::ThumbnailFacade::instance()->createResizeSlider();
    addPermanentWidget( m_thumbnailSizeSlider, 0 );
    // prevent stretching:
    m_thumbnailSizeSlider->setMaximumSize( m_thumbnailSizeSlider->size());
    m_thumbnailSizeSlider->setMinimumSize( m_thumbnailSizeSlider->size());
    m_thumbnailSizeSlider->hide();
}

void MainWindow::StatusBar::setLocked( bool locked )
{
    static QPixmap* lockedPix = new QPixmap( SmallIcon( QString::fromLatin1( "object-locked" ) ) );
    m_lockedIndicator->setFixedWidth( lockedPix->width() );

    if ( locked )
        m_lockedIndicator->setPixmap( *lockedPix );
    else
        m_lockedIndicator->setPixmap( QPixmap() );

}

void MainWindow::StatusBar::startProgress( const QString& text, int total )
{
    m_progressBar->setFormat( text + QString::fromLatin1( ": %p%" ) );
    m_progressBar->setMaximum( total );
    m_progressBar->setValue(0);
    m_pendingShowTimer->start( 2000 ); // To avoid flicker we will only show the statusbar after 2 seconds.
}

void MainWindow::StatusBar::setProgress( int progress )
{
    if ( progress == m_progressBar->maximum() )
        hideStatusBar();

    // If progress comes in to fast, then the UI will freeze from all time spent on updating the progressbar.
    static QTime time;
    if ( time.elapsed() > 200 ) {
        m_progressBar->setValue( progress );
        time.restart();
    }
}

void MainWindow::StatusBar::setProgressBarVisible( bool show )
{
    m_progressBar->setVisible(show);
    m_cancel->setVisible(show);
}

void MainWindow::StatusBar::showThumbnailSlider()
{
    m_thumbnailSizeSlider->setVisible( true );
}

void MainWindow::StatusBar::hideThumbnailSlider()
{
    m_thumbnailSizeSlider->setVisible( false );
}

void MainWindow::StatusBar::hideStatusBar()
{
    setProgressBarVisible( false );
    m_pendingShowTimer->stop();
}

void MainWindow::StatusBar::showStatusBar()
{
    setProgressBarVisible( true );
}

// vi:expandtab:tabstop=4 shiftwidth=4:
