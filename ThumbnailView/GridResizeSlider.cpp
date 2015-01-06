/* Copyright (C) 2015 Johannes Zarl <johannes@zarl.at>

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
#include "GridResizeSlider.h"

#include <QScrollBar>
#include <QTimer>
#include <KSharedConfig>
#include <QDebug>

#include "CellGeometry.h"
#include "ThumbnailModel.h"
#include "ThumbnailWidget.h"
#include "Settings/SettingsData.h"
#include "ImageManager/ThumbnailBuilder.h"

#ifdef DEBUG_ResizeSlider
#define Debug qDebug
#else
#define Debug if (0) qDebug
#endif


ThumbnailView::GridResizeSlider::GridResizeSlider( ThumbnailFactory* factory )
    : QSlider( Qt::Horizontal ), ThumbnailComponent( factory )
{
    Settings::SettingsData *settings = Settings::SettingsData::instance();
    setMinimum( settings->minimumThumbnailSize() );
    setMaximum( settings->thumbnailSize() );
    setValue( settings->actualThumbnailSize() );

    // timer for event-timeout:
    m_timer = new QTimer( this );
    m_timer->setSingleShot(true);

    // we have no definitive leave event when using the mousewheel -> use a timeout
    connect( m_timer, SIGNAL(timeout()), this, SLOT(leaveGridResizingMode()) );

    connect( settings, SIGNAL(actualThumbnailSizeChanged(int)), this , SLOT(setValue(int)) );
    connect( settings, SIGNAL(thumbnailSizeChanged(int)), this, SLOT(setMaximum(int)) );

    connect( this, SIGNAL(sliderPressed()), this, SLOT(enterGridResizingMode()) );
    connect( this, SIGNAL(valueChanged(int)), this, SLOT(setCellSize(int)) );

    // disable drawing of thumbnails while resizing:
    connect( this, SIGNAL(isResizing(bool)), widget(), SLOT(setExternallyResizing(bool)) );
}

ThumbnailView::GridResizeSlider::~GridResizeSlider()
{
    delete m_timer;
}

void ThumbnailView::GridResizeSlider::mousePressEvent( QMouseEvent* event)
{
    Debug() << "Mouse pressed";
    enterGridResizingMode();
    QSlider::mousePressEvent( event );
}

void ThumbnailView::GridResizeSlider::mouseReleaseEvent( QMouseEvent* event)
{
    Debug() << "Mouse released";
    leaveGridResizingMode();
    QSlider::mouseReleaseEvent( event );
}

void ThumbnailView::GridResizeSlider::wheelEvent( QWheelEvent* event)
{
    // set (or reset) the timer to leave resizing mode:
    m_timer->start(200);
    Debug() << "(Re)starting timer";
    if (!m_resizing) {
        enterGridResizingMode();
    }
    QSlider::wheelEvent( event );
}

void ThumbnailView::GridResizeSlider::enterGridResizingMode()
{
    if (m_resizing)
        return; //already resizing
    m_resizing = true;

    Debug() << "Entering grid resizing mode";
    ImageManager::ThumbnailBuilder::instance()->cancelRequests();
    emit isResizing( true );
}

void ThumbnailView::GridResizeSlider::leaveGridResizingMode()
{
    if (!m_resizing)
        return; //not resizing
    m_resizing = false;
    Debug() << "Leaving grid resizing mode";

    model()->reset();
    cellGeometryInfo()->flushCache();
    model()->updateVisibleRowInfo();
    emit isResizing( false );
}

void ThumbnailView::GridResizeSlider::setCellSize(int size)
{
    blockSignals(true);
    Settings::SettingsData::instance()->setActualThumbnailSize( size );
    blockSignals(false);

    model()->reset();
    cellGeometryInfo()->calculateCellSize();
}

void ThumbnailView::GridResizeSlider::setMaximum(int size)
{
    // QSlider::setMaximum() is not a slot, which is why we need this slot as workaround
    QSlider::setMaximum(size);
}

#include "GridResizeSlider.moc"

// vi:expandtab:tabstop=4 shiftwidth=4:
