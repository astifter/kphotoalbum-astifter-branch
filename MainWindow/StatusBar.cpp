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
#include <QToolButton>
#include <QTimer>
#include <QProgressBar>
#include "DB/ImageDB.h"
#include "ImageCounter.h"
#include "Settings/SettingsData.h"
#include <QLabel>
#include "DirtyIndicator.h"
#include <KHBox>
#include <kiconloader.h>
#include <KIcon>

MainWindow::StatusBar::StatusBar()
    : KStatusBar()
{
    setupFixedFonts();
    setupGUI();
    m_pendingShowTimer = new QTimer(this);
    m_pendingShowTimer->setSingleShot( true );
    connect( m_pendingShowTimer, SIGNAL( timeout() ), this, SLOT( showStatusBar() ) );
}

void MainWindow::StatusBar::setupFixedFonts()
{
    // Avoid flicker in the statusbar when moving over dates from the datebar
    QFont f( font() );
    f.setStyleHint( QFont::TypeWriter );
    f.setFamily( QString::fromLatin1( "courier" ) );
    f.setBold( true );
    setFont( f );
}

void MainWindow::StatusBar::setupGUI()
{
    KHBox* indicators = new KHBox( this );
    _dirtyIndicator = new DirtyIndicator( indicators );
    connect( DB::ImageDB::instance(), SIGNAL( dirty() ), _dirtyIndicator, SLOT( markDirtySlot() ) );


    m_progressBar = new QProgressBar( this );
    m_progressBar->setMinimumWidth( 400 );
    addPermanentWidget( m_progressBar, 0 );

    m_cancel = new QToolButton( this );
    m_cancel->setIcon( KIcon( QString::fromLatin1( "dialog-close" ) ) );
    m_cancel->setShortcut( Qt::Key_Escape );
    addPermanentWidget( m_cancel, 0 );
    connect( m_cancel, SIGNAL( clicked() ), this, SIGNAL( cancelRequest() ) );
    connect( m_cancel, SIGNAL( clicked() ), this, SLOT( hideStatusBar() ) );

    _lockedIndicator = new QLabel( indicators );

    addPermanentWidget( indicators, 0 );

    _partial = new ImageCounter( this );
    addPermanentWidget( _partial, 0 );

    ImageCounter* total = new ImageCounter( this );
    addPermanentWidget( total, 0 );
    total->setTotal( DB::ImageDB::instance()->totalCount() );
    connect( DB::ImageDB::instance(), SIGNAL( totalChanged( uint ) ), total, SLOT( setTotal( uint ) ) );

    _pathIndicator = new BreadcrumbViewer;
    addWidget( _pathIndicator, 1 );

    setStatusBarVisible( false );
}

void MainWindow::StatusBar::setLocked( bool locked )
{
    static QPixmap* lockedPix = new QPixmap( SmallIcon( QString::fromLatin1( "object-locked" ) ) );
    _lockedIndicator->setFixedWidth( lockedPix->width() );

    if ( locked )
        _lockedIndicator->setPixmap( *lockedPix );
    else
        _lockedIndicator->setPixmap( QPixmap() );

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
    m_progressBar->setValue( progress );
    if ( progress == m_progressBar->maximum() )
        hideStatusBar();
}

void MainWindow::StatusBar::setStatusBarVisible( bool show )
{
    m_progressBar->setVisible(show);
    m_cancel->setVisible(show);
}

void MainWindow::StatusBar::hideStatusBar()
{
    setStatusBarVisible( false );
    m_pendingShowTimer->stop();
}

void MainWindow::StatusBar::showStatusBar()
{
    setStatusBarVisible( true );
}