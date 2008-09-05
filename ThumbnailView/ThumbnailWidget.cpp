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
#include "ThumbnailWidget.h"

//Added by qt3to4:
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <Q3PointArray>
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QWheelEvent>

#include "Browser/BrowserWidget.h"
#include "DB/ImageDB.h"
#include "DB/ImageInfoPtr.h"
#include "ImageManager/Manager.h"
#include "MainWindow/DirtyIndicator.h"
#include "Settings/SettingsData.h"
#include "ThumbnailRequest.h"
#include "ThumbnailToolTip.h"
#include "ThumbnailWidget.moc"

#include <math.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qcursor.h>
#include <qfontmetrics.h>
#include <qpainter.h>
#include <qpixmapcache.h>

/**
 * \namespace ThumbnailView
 * The thumbnail view.
 */

/**
 * \class ThumbnailView
 * This is the widget which shows the thumbnails.
 *
 * In previous versions this was implemented using a QIconView, but there
 * simply was too many problems, so after years of tears and pains I
 * rewrote it.
 */

ThumbnailView::ThumbnailWidget* ThumbnailView::ThumbnailWidget::_instance = 0;
ThumbnailView::ThumbnailWidget::ThumbnailWidget( QWidget* parent )
    :Q3GridView( parent ),
     _gridResizeInteraction( this ),
     _wheelResizing( false ),
     _selectionInteraction( this ),
     _mouseTrackingHandler( this ),
     _mouseHandler( &_mouseTrackingHandler ),
     _sortDirection( Settings::SettingsData::instance()->showNewestThumbnailFirst() ? NewestFirst : OldestFirst ),
     _cellOnFirstShiftMovementKey(Cell::invalidCell()),
    _cursorWasAtStackIcon(false)
{
    _instance = this;
    setFocusPolicy( Qt::WheelFocus );
    updateCellSize();

    // It beats me why I need to set mouse tracking on both, but without it doesn't work.
    viewport()->setMouseTracking( true );
    setMouseTracking( true );

    connect( this, SIGNAL( contentsMoving( int, int ) ), this, SLOT( emitDateChange( int, int ) ) );

    _toolTip = new ThumbnailToolTip( this );

    viewport()->setAcceptDrops( true );

    _repaintTimer = new QTimer( this );
    _repaintTimer->setSingleShot(true);
    connect( _repaintTimer, SIGNAL( timeout() ), this, SLOT( slotRepaint() ) );

    setVScrollBarMode( AlwaysOn );
    setHScrollBarMode( AlwaysOff );
}


void ThumbnailView::ThumbnailWidget::paintCell( QPainter * p, int row, int col )
{
    QPixmap doubleBuffer( cellRect().size() );
    QPainter painter( &doubleBuffer );
    paintCellBackground( &painter, row, col );
    if ( !_mouseHandler->isResizingGrid() && !_wheelResizing ) {
        paintCellPixmap( &painter, row, col );
        paintCellText( &painter, row, col );
    }
    painter.end();
    p->drawPixmap( cellRect(), doubleBuffer );
}

static DB::StackID getStackId(const QString& fileName)
{
    if (fileName.isNull()) return 0;
    return DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->stackId();
}

void ThumbnailView::ThumbnailWidget::paintStackedIndicator( QPainter* painter,
                                                            const QRect& rect,
                                                            const QString& fileName)
{
    DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath );
    if (!imageInfo || !imageInfo->isStacked())
        return;

    const DB::StackID stackId  = imageInfo->stackId();
    bool isFirst = true;
    bool isLast = true;

    // A bit ugly: determine where we are within the stack.
    {
        int prev = _fileNameToIndex[fileName] - 1;
        int next = _fileNameToIndex[fileName] + 1;
        isFirst = (prev < 0) || getStackId(_displayList[prev]) != stackId;
        isLast  = (next >= _displayList.count()) || getStackId(_displayList[next]) != stackId;
    }

    const int thickness = 1;
    const int space = 0;
    const int corners = 8;
    const int w = rect.width();
    const int h = rect.height();
    int bottom_w, corner_h;
    bottom_w = corner_h = qMin(w / 2, h / 2);
    if (!isFirst)
        bottom_w = w;
    QPen pen;
    pen.setWidth(thickness);

    for (int c = 0; c < corners; ++c) {
        pen.setColor(c % 2 == 0 ? Qt::black : Qt::white);
        painter->setPen(pen);
        int step = (thickness + space) * c;
        int x = rect.x() + w - bottom_w - (thickness + space) * corners + step;
        int y = rect.y() + h - (thickness + space) * corners + step;
        painter->drawLine(x, y, rect.x() + w, y);
        if (isLast)
            painter->drawLine(x + bottom_w, y, x + bottom_w, y - corner_h);
    }
}

/**
 * Paint the pixmap in the cell (row,col)
 */
void ThumbnailView::ThumbnailWidget::paintCellPixmap( QPainter* painter, int row, int col )
{
    QString fileName = fileNameInCell( row, col );
    if ( !fileName.isNull() ) {
        QPixmap* pix = QPixmapCache::find( fileName );
        if ( pix ) {
            QRect rect = iconGeometry( row, col );
            Q_ASSERT( !rect.isNull() );
            painter->drawPixmap( rect, *pix );

            rect = QRect( 0, 0, cellWidth(), cellHeight() );
            if ( _leftDrop == fileName )
                painter->fillRect( rect.left(), rect.top(), 3, rect.height(), QBrush( Qt::red ) );
            else if ( _rightDrop == fileName )
                painter->fillRect( rect.right() -2, rect.top(), 3, rect.height(), QBrush( Qt::red ) );
            paintStackedIndicator(painter, rect, fileName);
        }
        else {
            QRect dimensions = cellDimensions();
            int angle = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->angle();
            ThumbnailRequest* request = new ThumbnailRequest(
                fileName, QSize( dimensions.width() - 2 * Settings::SettingsData::instance()->thumbnailSpace(),
                                 dimensions.height() - 2 * Settings::SettingsData::instance()->thumbnailSpace() ),
                angle, this );
            request->setCache();
            request->setPriority( ImageManager::ThumbnailVisible );
            ImageManager::Manager::instance()->load( request );
        }
    }
}

/**
 * Returns the text under the thumbnails
 */
QString ThumbnailView::ThumbnailWidget::thumbnailText( const QString& fileName ) const
{
    QString text, line;

    QRect dimensions = cellDimensions();
    int thumbnailHeight = dimensions.height() - 2 * Settings::SettingsData::instance()->thumbnailSpace();
    int thumbnailWidth = dimensions.width(); // no substracting here
    int maxCharacters = thumbnailHeight / QFontMetrics( font() ).maxWidth() * 2;

    if ( Settings::SettingsData::instance()->displayLabels()) {
        text += DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->label();
        text += QString::fromLatin1("\n");
    }

    if ( Settings::SettingsData::instance()->displayCategories()) {
        QStringList grps = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->availableCategories();
        for( QStringList::const_iterator it = grps.constBegin(); it != grps.constEnd(); ++it ) {
            QString category = *it;
            if ( category != QString::fromLatin1( "Folder" ) && category != QString::fromLatin1( "Media Type" ) ) {
                StringSet items = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->itemsOfCategory( category );
                if (!items.empty()) {
                    line = QString::fromLatin1( "%1: " )
                           .arg( category );
                    bool first = true;
                    for( StringSet::const_iterator it2 = items.begin(); it2 != items.end(); ++it2 ) {
                        QString item = *it2;
                        if ( first )
                            first = false;
                        else
                            line += QString::fromLatin1( ", " );
                        line += item;
                    }
                    if ( QFontMetrics( font() ).width( line ) > thumbnailWidth ) {
                        line = line.left( maxCharacters );
                        line += QString::fromLatin1( " ..." );
                    }
                    text += line + QString::fromLatin1( "\n" );
                }
            }
        }
    }

    if(text.isEmpty())
        text = QString::fromLatin1( "" );

    return text.trimmed();
}

/**
 * Draw the title under the thumbnail
 */
void ThumbnailView::ThumbnailWidget::paintCellText( QPainter* painter, int row, int col )
{
    QString fileName = fileNameInCell( row, col );
    if ( fileName.isNull() )
        return;

    QString title = thumbnailText( fileName );
    QRect rect = cellTextGeometry( row, col );
    painter->setPen( palette().color( QPalette::WindowText ) );

    //Qt::TextWordWrap just in case, if the text's width is wider than the cell's width
    painter->drawText( rect, Qt::AlignCenter | Qt::TextWordWrap, title );
}


void ThumbnailView::ThumbnailWidget::generateMissingThumbnails( const QStringList& list ) const {
    QRect dimensions = cellDimensions();
    QSize size( dimensions.width() - 2 * Settings::SettingsData::instance()->thumbnailSpace(),
                dimensions.height() - 2 * Settings::SettingsData::instance()->thumbnailSpace() );

    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
        ImageManager::ImageRequest* request = new ImageManager::ImageRequest( *it, size, DB::ImageDB::instance()->info( *it, DB::AbsolutePath )->angle(), NULL );
        request->setPriority( ImageManager::ThumbnailInvisible );
        ImageManager::Manager::instance()->load( request );
    }
}

static bool stackOrderComparator(const QString& a, const QString& b) {
    DB::ImageInfoPtr aInfo = DB::ImageDB::instance()->info( a, DB::AbsolutePath );
    DB::ImageInfoPtr bInfo = DB::ImageDB::instance()->info( b, DB::AbsolutePath );
    return aInfo->stackOrder() < bInfo->stackOrder();
}

void ThumbnailView::ThumbnailWidget::updateDisplayModel()
{
    ImageManager::Manager::instance()->stop( this, ImageManager::StopOnlyNonPriorityLoads );

    // Note, this can be simplified, if we make the database backend already
    // return things in the right order. Then we only need one pass while now
    // we need to go through the list two times.

    /* Extract all stacks we have first. Different stackid's might be
     * intermingled in the result so we need to know this ahead before
     * creating the display list.
     */
    _stackContents.clear();
    for (QStringList::const_iterator it = _imageList.begin(); it != _imageList.end(); ++it) {
        DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( *it, DB::AbsolutePath );
        if (imageInfo->isStacked()) {
            DB::StackID stackid = imageInfo->stackId();
            // TODO(hzeller): right now, we only put the elements in order of
            // appearance, but should sort them.
            _stackContents[ stackid ].append(*it);
        }
    }

    /*
     * All stacks need to be ordered in their stack order. We don't rely that
     * the images actually came in the order necessary.
     */
    for (StackMap::iterator it = _stackContents.begin(); it != _stackContents.end(); ++it) {
        qStableSort(it->begin(), it->end(), stackOrderComparator);
    }

    /* Build the final list to be displayed. That is basically the sequence
     * we got from the original, but the stacks shown with all images together
     * in the right sequence or collapsed showing only the top image.
     */
    _displayList.clear();
    QSet<DB::StackID> alreadyShownStacks;
    for (QStringList::const_iterator it = _imageList.begin(); it != _imageList.end(); ++it) {
        DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( *it, DB::AbsolutePath );
        if (imageInfo->isStacked()) {
            DB::StackID stackid = imageInfo->stackId();
            if (alreadyShownStacks.contains(stackid))
                continue;
            const QStringList& stackList = _stackContents[ stackid ];
            Q_ASSERT(!stackList.empty());
            if (_expandedStacks.contains(stackid))
                _displayList += stackList;
            else
                _displayList += stackList[0];
            alreadyShownStacks.insert(stackid);
        }
        else {
            _displayList.append(*it);
        }
    }

    if ( _sortDirection != OldestFirst )
        _displayList = reverseList(_displayList);

    updateIndexCache();

    collapseAllStacksEnabled( _expandedStacks.size() > 0);
    if ( isVisible() ) {
        updateGridSize();
        repaintScreen();
    }
}

void ThumbnailView::ThumbnailWidget::setImageList( const QStringList& list )
{
    _imageList = list;
    generateMissingThumbnails( list );
    updateDisplayModel();
}

void ThumbnailView::ThumbnailWidget::toggleStackExpansion(const QString& filename) {
    DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( filename, DB::AbsolutePath );
    if (imageInfo) {
        DB::StackID stackid = imageInfo->stackId();
        if (_expandedStacks.contains(stackid))
            _expandedStacks.remove(stackid);
        else
            _expandedStacks.insert(stackid);
        updateDisplayModel();
    }
}

void ThumbnailView::ThumbnailWidget::collapseAllStacks() {
    _expandedStacks.clear();
    updateDisplayModel();
}

/**
 * Return the file name shown in cell (row,col) if a thumbnail is shown in this cell or null otherwise.
 */
QString ThumbnailView::ThumbnailWidget::fileNameInCell( int row, int col ) const
{
    const int index = row * numCols() + col;
    if ( index >= _displayList.count() )
        return QString::null;
    else
        return _displayList[index];
}

/** @short It seems that Q3GridView's viewportToContents() is slightly off */
QPoint ThumbnailView::ThumbnailWidget::viewportToContentsAdjusted( const QPoint& coordinate, CoordinateSystem system ) const
{
    QPoint contentsPos = coordinate;
    if ( system == ViewportCoordinates ) {
        contentsPos = viewportToContents( coordinate );
        contentsPos.rx() -= 3;
        contentsPos.ry() -= 3;
    }
    return contentsPos;
}

/**
 * Returns the file name shown at viewport position (x,y) if a thumbnail is shown at this position or QString::null otherwise.
 */
QString ThumbnailView::ThumbnailWidget::fileNameAtCoordinate( const QPoint& coordinate, CoordinateSystem system ) const
{
    QPoint contentsPos = viewportToContentsAdjusted( coordinate, system );
    int col = columnAt( contentsPos.x() );
    int row = rowAt( contentsPos.y() );

    QRect cellRect = const_cast<ThumbnailWidget*>(this)->cellGeometry( row, col );

    if ( cellRect.contains( contentsPos ) )
        return fileNameInCell( row, col );
    else
        return QString::null;
}

/**
 * Return desired size of the whole cell
 */
QRect ThumbnailView::ThumbnailWidget::cellDimensions() const
{
    int width = Settings::SettingsData::instance()->thumbSize();
    int height = width;

    switch (Settings::SettingsData::instance()->thumbnailAspectRatio()) {
        case Settings::Aspect_16_9:
	    height = (int) (height * 9.0 / 16);
	    break;
        case Settings::Aspect_4_3:
	    height = (int) (height * 3.0 / 4);
	    break;
        case Settings::Aspect_3_2:
	    height = (int) (height * 2.0 / 3);
	    break;
        case Settings::Aspect_9_16:
	    width = (int) (width * 9.0 / 16);
	    break;
        case Settings::Aspect_3_4:
	    width = (int) (width * 3.0 / 4);
	    break;
        case Settings::Aspect_2_3:
	    width = (int) (width * 2.0 / 3);
	    break;
	case Settings::Aspect_1_1:
	    // nothing
	    ;
    }
    return QRect(0, 0, width, height);
}

/**
 * Return the geometry for the icon in the cell (row,col). The returned coordinates are local to the cell.
 */
QRect ThumbnailView::ThumbnailWidget::iconGeometry( int row, int col ) const
{
    QString fileName = fileNameInCell( row, col );
    if ( fileName.isNull() ) // empty cell
        return QRect();

    QRect dimensions = cellDimensions();
    const int space = Settings::SettingsData::instance()->thumbnailSpace();
    int width = dimensions.width() - 2 * space;
    int height = dimensions.height() - 2 * space;

    QPixmap* pix = QPixmapCache::find( fileName );
    if ( !pix || (pix->width() == 0 && pix->height() == 0) )
        return QRect( space, space, width, height );

    int xoff = space + (width - pix->width()) / 2;
    int yoff = space + (height - pix->height()) / 2;

    return QRect( xoff, yoff, pix->width(), pix->height() );
}

/**
 * Returns the max. count of categories of an image with number of groups > 0
 */
int ThumbnailView::ThumbnailWidget::noOfCategoriesForImage(const QString& image ) const
{
    int catsInText = 0;
    QStringList grps = DB::ImageDB::instance()->info( image, DB::AbsolutePath )->availableCategories();
    for( QStringList::const_iterator it = grps.constBegin(); it != grps.constEnd(); ++it ) {
        QString category = *it;
        if ( category != QString::fromLatin1( "Folder" ) && category != QString::fromLatin1( "Media Type" ) ) {
            StringSet items = DB::ImageDB::instance()->info( image, DB::AbsolutePath )->itemsOfCategory( category );
            if (!items.empty()) {
                catsInText++;
            }
        }
    }
    return catsInText;
}

/**
 * Return the height of the text under the thumbnails.
 */
int ThumbnailView::ThumbnailWidget::textHeight( bool reCalc ) const
{
    int h = 0;
    static int maxCatsInText = 0;

    if ( Settings::SettingsData::instance()->displayLabels() )
        h += QFontMetrics( font() ).height() +2;
    if ( Settings::SettingsData::instance()->displayCategories()) {
        if ( reCalc ) {
            if (!_selectedFiles.empty()) {
                for( StringSet::const_iterator itImg = _selectedFiles.begin(); itImg != _selectedFiles.end(); ++itImg ) {
                    maxCatsInText = qMax( noOfCategoriesForImage( *itImg ), maxCatsInText );
                }
            } else {
                maxCatsInText = 0;
                for( QStringList::ConstIterator itImg = _displayList.begin(); itImg != _displayList.end(); ++itImg ) {
                    maxCatsInText = qMax( noOfCategoriesForImage( *itImg ), maxCatsInText );
                }
            }
        }
        h += QFontMetrics( font() ).height() * ( maxCatsInText ) +5;
    }
    return h;
}


QRect ThumbnailView::ThumbnailWidget::cellTextGeometry( int row, int col ) const
{
    if ( !Settings::SettingsData::instance()->displayLabels() && !Settings::SettingsData::instance()->displayCategories() )
        return QRect();

    QString fileName = fileNameInCell( row, col );
    if ( fileName.isNull() ) // empty cell
        return QRect();

    int h = textHeight( false );

    QRect iconRect = iconGeometry( row, col );
    QRect cellRect = const_cast<ThumbnailWidget*>(this)->cellGeometry( row, col );

    return QRect( 1, cellRect.height() -h -1, cellRect.width()-2, h );
}


// ImageManager::ImageClient interface. Callback from the
// ImageManager when the image is loaded.
void ThumbnailView::ThumbnailWidget::pixmapLoaded( const QString& fileName, const QSize& size, const QSize& fullSize, int,
                                                   const QImage& image, const bool loadedOK, const bool cache )
{
    QPixmap pixmap( size );
    if ( loadedOK && !image.isNull() )
        pixmap = QPixmap::fromImage( image );
    else if ( !loadedOK )
        pixmap.fill( palette().color( QPalette::Dark));

    if ( !loadedOK || !DB::ImageInfo::imageOnDisk( fileName ) ) {
        QPainter p( &pixmap );
        p.setBrush( palette().base() );
        p.setWindow( 0, 0, 100, 100 );
        Q3PointArray pts;
        pts.setPoints( 3, 70,-1,  100,-1,  100,30 );
        p.drawConvexPolygon( pts );
    }

    DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath );

    if ( fullSize.isValid() ) {
        imageInfo->setSize( fullSize );
    }

    if ( cache ) {
        QPixmapCache::insert( fileName, pixmap );
    }

    updateCell( fileName );
}

/**
 * Request a repaint of the cell showing filename
 *
 * QGridView::updateCell has some problems when we scroll during a keyboard selection, so thatswhy we role our own one.
 */
void ThumbnailView::ThumbnailWidget::updateCell( const QString& fileName )
{
    if ( fileName.isNull() )
        return;

    _pendingRepaintLock.lock();
    _pendingRepaint.insert( fileName );
    _pendingRepaintLock.unlock();

    // Do not trigger immediatly the repaint but wait a tiny bit - there might
    // be more coming which we then can paint in one shot.
    _repaintTimer->start( 10 );
}

void ThumbnailView::ThumbnailWidget::updateCell( int row, int col )
{
    updateCell( fileNameInCell( row, col ) );
}


/**
 * Update the grid size depending on the size of the widget
 */
void ThumbnailView::ThumbnailWidget::updateGridSize()
{
    int thumbnailsPerRow = width() / cellWidth();
    int numRowsPerPage = height() / cellHeight();
    setNumCols( thumbnailsPerRow );
    setNumRows( qMax( numRowsPerPage, (int) ceil( 1.0 * _displayList.size() / thumbnailsPerRow ) ) );
}

void ThumbnailView::ThumbnailWidget::showEvent( QShowEvent* )
{
    updateGridSize();
}

/**
 * Paint the cell back ground, and the outline
 */
void ThumbnailView::ThumbnailWidget::paintCellBackground( QPainter* p, int row, int col )
{
    QRect rect = cellRect();
    if (_selectedFiles.contains(fileNameInCell(row, col)))
        p->fillRect( rect, palette().highlight() );
    else
        p->fillRect( rect, palette().color( QPalette::Base) );

    if (_mouseHandler->isResizingGrid() || _wheelResizing ||
        Settings::SettingsData::instance()->thumbnailDisplayGrid()) {
        p->setPen( palette().color( QPalette::Dark) );
        // left of frame
        if ( col != 0 )
            p->drawLine( rect.left(), rect.top(), rect.left(), rect.bottom() );

        // bottom line
        if ( row != numRows() -1 ) {
            p->drawLine( rect.left(), rect.bottom() -1, rect.right(), rect.bottom()-1 );
        }
    }
}

void ThumbnailView::ThumbnailWidget::keyPressEvent( QKeyEvent* event )
{
    if ( event->modifiers() == Qt::NoModifier && ( event->key() >= Qt::Key_A && event->key() <= Qt::Key_Z ) ) {
        QString token = event->text().toUpper().left(1);
        bool mustRemoveToken = false;
        bool hadHit          = false;

        for( StringSet::const_iterator it = _selectedFiles.begin(); it != _selectedFiles.end(); ++it ) {
            if ( ! hadHit ) {
                mustRemoveToken = DB::ImageDB::instance()->info( *it, DB::AbsolutePath )->hasCategoryInfo( QString::fromLatin1("Tokens"), token );
                hadHit = true;
            }

            if ( mustRemoveToken )
                DB::ImageDB::instance()->info( *it, DB::AbsolutePath )->removeCategoryInfo( QString::fromLatin1("Tokens"), token );
            else
                DB::ImageDB::instance()->info( *it, DB::AbsolutePath )->addCategoryInfo( QString::fromLatin1("Tokens"), token );

            updateCell( *it );
        }

        if ( hadHit )
            updateCellSize();

        DB::ImageDB::instance()->categoryCollection()->categoryForName( QString::fromLatin1("Tokens") )->addItem( token );
        MainWindow::DirtyIndicator::markDirty();
    }

    if ( isMovementKey( event->key() ) )
        keyboardMoveEvent( event );

    if ( event->key() == Qt::Key_Return )
        emit showSelection();

    if ( event->key() == Qt::Key_Space )
        toggleSelection( _currentItem );

    possibleEmitSelectionChanged();
}

void ThumbnailView::ThumbnailWidget::keyReleaseEvent( QKeyEvent* event )
{
    if ( _wheelResizing && event->key() == Qt::Key_Control ) {
        _wheelResizing = false;
        repaintScreen();
    }
    else {
        if ( event->key() == Qt::Key_Shift )
            _cellOnFirstShiftMovementKey = Cell::invalidCell();
        Q3GridView::keyReleaseEvent(event);
    }
}

void ThumbnailView::ThumbnailWidget::keyboardMoveEvent( QKeyEvent* event )
{
    if ( !( event->modifiers()& Qt::ShiftModifier ) && !( event->modifiers() &  Qt::ControlModifier ) ) {
        clearSelection();
    }

    // Decide the next keyboard focus cell
    Cell currentPos(0,0);
    if ( !_currentItem.isNull() )
        currentPos = positionForFileName( _currentItem );

    // Update current position if it is outside view and we do not have any modifiers
    // that is if we just scroll arround.
    //
    // Use case is following: There is a selected item which is not
    // visible because user has scrolled by other means than the
    // keyboard (scrollbar or mouse wheel). In that case if the user
    // presses keyboard movement key, the selection is forgotten and
    // instead a currently visible cell is selected. So no scrolling
    // of the view will be done.
    if ( !( event->modifiers()& Qt::ShiftModifier ) && !( event->modifiers() &  Qt::ControlModifier ) ) {
        if ( currentPos.row() < firstVisibleRow( PartlyVisible ) )
            currentPos = Cell( firstVisibleRow( FullyVisible ), currentPos.col() );
        else if ( currentPos.row() > lastVisibleRow( PartlyVisible ) )
            currentPos = Cell( lastVisibleRow( FullyVisible ), currentPos.col() );
    }

    Cell newPos;
    switch (event->key() ) {
    case Qt::Key_Left:
        newPos = currentPos;
        newPos.col()--;

        if ( newPos.col() < 0 )
            newPos = Cell( newPos.row()-1, numCols()-1 );
        break;

    case Qt::Key_Right:
        newPos = currentPos;
        newPos.col()++;
        if ( newPos.col() == numCols() )
            newPos = Cell( newPos.row()+1, 0 );
        break;

    case Qt::Key_Down:
        newPos = Cell( currentPos.row()+1, currentPos.col() );
        break;

    case Qt::Key_Up:
        newPos = Cell( currentPos.row()-1, currentPos.col() );
        break;

    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
    {
        int rows = (event->key() == Qt::Key_PageDown) ? 1 : -1;
        if ( event->modifiers() & (Qt::AltModifier | Qt::MetaModifier) )
            rows *= numRows() / 20;
        else
            rows *= numRowsPerPage();

        newPos = Cell( currentPos.row() + rows, currentPos.col() );
        break;
    }
    case Qt::Key_Home:
        newPos = Cell( 0, 0 );
        break;

    case Qt::Key_End:
        newPos = lastCell();
        break;
    }

    // Check for overruns
    if ( newPos > lastCell() )
        newPos = lastCell();
    if ( newPos < Cell(0,0) )
        newPos = Cell(0,0);

    if ( event->modifiers() & Qt::ShiftModifier ) {
        if ( _cellOnFirstShiftMovementKey == Cell::invalidCell() ) {
            _cellOnFirstShiftMovementKey = currentPos;
            _selectionOnFirstShiftMovementKey = _selectedFiles;
        }

        StringSet oldSelection = _selectedFiles;

        _selectedFiles = _selectionOnFirstShiftMovementKey;
        selectAllCellsBetween( _cellOnFirstShiftMovementKey, newPos, false );

        repaintAfterChangedSelection( oldSelection );
    }

    if ( ! (event->modifiers() & Qt::ControlModifier ) ) {
        selectCell( newPos );
        updateCell( currentPos.row(), currentPos.col() );
    }
    _currentItem = fileNameInCell( newPos );

    // Scroll if necesary
    if ( newPos.row() > lastVisibleRow( ThumbnailWidget::FullyVisible ) )
        setContentsPos( contentsX(), cellGeometry( newPos.row(), newPos.col() ).top() -
                        (numRowsPerPage()-1)*cellHeight()  );

    if  ( newPos.row() < firstVisibleRow( ThumbnailWidget::FullyVisible ) )
        setContentsPos( contentsX(), cellGeometry( newPos.row(), newPos.col() ).top() );
}

/**
 * Update selection to include files from start to end
 */
void ThumbnailView::ThumbnailWidget::selectItems( const Cell& start, const Cell& end )
{
    StringSet oldSelection = _selectedFiles;

    _selectedFiles.clear();

    selectAllCellsBetween( start, end, false );

    repaintAfterChangedSelection(oldSelection);
}

/**
 * Repaint cells that are in different state than in given selection.
 */
void ThumbnailView::ThumbnailWidget::repaintAfterChangedSelection( const StringSet& oldSelection )
{
    for( StringSet::const_iterator it = oldSelection.begin(); it != oldSelection.end(); ++it ) {
        if ( !_selectedFiles.contains( *it ) )
            updateCell( *it );
    }

    for( StringSet::const_iterator it = _selectedFiles.begin(); it != _selectedFiles.end(); ++it ) {
        if ( !oldSelection.contains( *it ) )
            updateCell( *it );
    }
}

/**
 * Return the number of complete rows per page
 */
int ThumbnailView::ThumbnailWidget::numRowsPerPage() const
{
    return height() / cellHeight();
}

bool ThumbnailView::ThumbnailWidget::isMouseOverStackIndicator( const QPoint& point )
{
    Cell pos = cellAtCoordinate( point, ViewportCoordinates );
    QRect cellRect = cellGeometry(pos.row(), pos.col() ).adjusted( 0, 0, -10, -10 ); // FIXME: what area should be "hot"?
    bool correctArea = !cellRect.contains( viewportToContentsAdjusted( point, ViewportCoordinates ) );
    if (!correctArea)
        return false;
    DB::ImageInfoPtr imageInfo = DB::ImageDB::instance()->info( fileNameUnderCursor(), DB::AbsolutePath );
    return imageInfo && imageInfo->isStacked();
}

void ThumbnailView::ThumbnailWidget::mousePressEvent( QMouseEvent* event )
{
    bool interestingArea = isMouseOverStackIndicator( event->pos() );
    if ( interestingArea ) {
        toggleStackExpansion( fileNameUnderCursor() );
        return;
    }

    if ( (event->button() & Qt::MidButton) ||
         ((event->modifiers() & Qt::ControlModifier) && (event->modifiers() & Qt::AltModifier)) )
        _mouseHandler = &_gridResizeInteraction;
    else
        _mouseHandler = &_selectionInteraction;

    _mouseHandler->mousePressEvent( event );
}

void ThumbnailView::ThumbnailWidget::mouseMoveEvent( QMouseEvent* event )
{
    if ( _mouseHandler == &_mouseTrackingHandler ) {
        bool interestingArea = isMouseOverStackIndicator( event->pos() );
        if ( interestingArea && ! _cursorWasAtStackIcon ) {
            setCursor( Qt::PointingHandCursor );
            _cursorWasAtStackIcon = true;
        } else if ( ! interestingArea && _cursorWasAtStackIcon ) {
            unsetCursor();
            _cursorWasAtStackIcon = false;
        }
    }

    _mouseHandler->mouseMoveEvent( event );
}

void ThumbnailView::ThumbnailWidget::mouseReleaseEvent( QMouseEvent* event )
{
    _mouseHandler->mouseReleaseEvent( event );
    _mouseHandler = &_mouseTrackingHandler;
}

void ThumbnailView::ThumbnailWidget::mouseDoubleClickEvent( QMouseEvent * event )
{
    if ( isMouseOverStackIndicator( event->pos() ) ) {
        toggleStackExpansion( fileNameUnderCursor() );
    } else if ( !( event->modifiers() & Qt::ControlModifier ) ) {
        QString fileName = fileNameAtCoordinate( event->pos(), ViewportCoordinates );
        if ( !fileName.isNull() )
            emit showImage( fileName );
    }
}

void ThumbnailView::ThumbnailWidget::wheelEvent( QWheelEvent* event )
{
    if ( event->modifiers() & Qt::ControlModifier ) {
        event->setAccepted(true);

        _wheelResizing = true;

        int delta = event->delta() / 20;

        Settings::SettingsData::instance()->setThumbSize( qMax( 32, cellWidth() + delta ) );

        updateCellSize();
    }
    else
        Q3GridView::wheelEvent(event);
}


void ThumbnailView::ThumbnailWidget::emitDateChange( int x, int y )
{
    if ( _isSettingDate )
        return;

    // Unfortunately the contentsMoving signal is emitted *before* the move, so we need to find out what is on the new position ourself.
    QString fileName = fileNameInCell( rowAt(y), columnAt(x) );
    if ( fileName.isNull() )
        return;

    static QDateTime lastDate;
    QDateTime date = DB::ImageDB::instance()->info( fileName, DB::AbsolutePath )->date().start();
    if ( date != lastDate ) {
        lastDate = date;
        if ( date.date().year() != 1900 )
            emit currentDateChanged( date );
    }
}

/**
 * scroll to the date specified with the parameter date.
 * The boolean includeRanges tells whether we accept range matches or not.
 */
void ThumbnailView::ThumbnailWidget::gotoDate( const DB::ImageDate& date, bool includeRanges )
{
    _isSettingDate = true;
    QString candidate = DB::ImageDB::instance()->
                        findFirstItemInRange(date, includeRanges, _displayList);
    if ( !candidate.isNull() ) {
        Cell pos = positionForFileName( candidate );
        QRect contentsRect = cellGeometry( pos.row(), pos.col() );
        setContentsPos( contentsRect.x(), contentsRect.y() );
        _currentItem = candidate;
    }
    _isSettingDate = false;
}

/**
 * return the position (row,col) for the given file name
 */
ThumbnailView::Cell ThumbnailView::ThumbnailWidget::positionForFileName( const QString& fileName ) const
{
    Q_ASSERT( !fileName.isNull() );
    int index = _fileNameToIndex[fileName];
    if ( index == -1 )
        return Cell( 0, 0 );

    int row = index / numCols();
    int col = index % numCols();
    return Cell( row, col );
}

/**
 * \brief Returns whether the thumbnail for fileName is still needed.
 *
 * If the user scrolls down through the view, a back log of thumbnail
 * request may build up, which will slow down scrolling a lot. Therefore
 * the ImageManger has the capability to check whether a thumbnail
 * request is really needed, when it gets to load the given thumbnail.
 */
bool ThumbnailView::ThumbnailWidget::thumbnailStillNeeded( const QString& fileName ) const
{
    Cell pos = positionForFileName( fileName );
    return pos.row() >= firstVisibleRow( PartlyVisible ) && pos.row() <= lastVisibleRow( PartlyVisible );
}

/*
 * Returns the first row that is at least partly visible.
 */
int ThumbnailView::ThumbnailWidget::firstVisibleRow( VisibleState state ) const
{
    int firstRow = rowAt( contentsY() );
    if ( state == FullyVisible && rowAt( contentsY() + cellHeight()-1 ) != firstRow )
        firstRow += 1;

    return firstRow;
}

int ThumbnailView::ThumbnailWidget::lastVisibleRow( VisibleState state ) const
{
    int lastRow = rowAt( contentsY() + visibleHeight() );
    if ( state == FullyVisible && rowAt( contentsY() + visibleHeight() - cellHeight() -1 ) != lastRow )
        lastRow -= 1;
    return lastRow;
}

void ThumbnailView::ThumbnailWidget::selectCell( const Cell& cell )
{
    selectCell( cell.row(), cell.col() );
}

void ThumbnailView::ThumbnailWidget::selectCell( int row, int col, bool repaint )
{
    QString file = fileNameInCell( row, col );
    if ( !file.isNull() ) {
        _selectedFiles.insert( file );
        if ( repaint )
            updateCell( row, col );
    }
}

ThumbnailView::Cell ThumbnailView::ThumbnailWidget::cellAtCoordinate( const QPoint& pos, CoordinateSystem system ) const
{
    QPoint contentsPos = pos;
    if ( system == ViewportCoordinates )
        contentsPos = viewportToContentsAdjusted( pos, system );

    int col = columnAt( contentsPos.x() );
    int row = rowAt( contentsPos.y() );
    return Cell( row, col );
}

void ThumbnailView::ThumbnailWidget::selectAllCellsBetween( Cell pos1, Cell pos2, bool repaint )
{
    ensureCellsSorted( pos1, pos2 );

    if ( pos1.row() == pos2.row() ) {
        // This is the case where images from only one row is selected.
        for ( int col = pos1.col(); col <= pos2.col(); ++ col )
            selectCell( pos1.row(), col, repaint );
    }
    else {
        // We know we have at least two rows.

        // first row
        for ( int col = pos1.col(); col < numCols(); ++ col )
            selectCell( pos1.row(), col, repaint );

        // rows in between
        for ( int row = pos1.row()+1; row < pos2.row(); ++row )
            for ( int col = 0; col < numCols(); ++ col )
                selectCell( row, col, repaint );

        // last row
        for ( int col = 0; col <= pos2.col(); ++ col )
            selectCell( pos2.row(), col, repaint );
    }
}

void ThumbnailView::ThumbnailWidget::resizeEvent( QResizeEvent* e )
{
    Q3GridView::resizeEvent( e );
    updateGridSize();
}

void ThumbnailView::ThumbnailWidget::clearSelection()
{
    StringSet oldSelection = _selectedFiles;
    _selectedFiles.clear();
    for( StringSet::const_iterator fileIt = oldSelection.begin(); fileIt != oldSelection.end(); ++fileIt ) {
        updateCell( *fileIt );
    }
}

QString ThumbnailView::ThumbnailWidget::fileNameInCell( const Cell& cell ) const
{
    return fileNameInCell( cell.row(), cell.col() );
}

bool ThumbnailView::ThumbnailWidget::isFocusAtLastCell() const
{
    return positionForFileName(_currentItem) == lastCell();
}

bool ThumbnailView::ThumbnailWidget::isFocusAtFirstCell() const
{
    return positionForFileName(_currentItem) == Cell(0,0);
}

/**
 * Return the coordinates of the last cell with a thumbnail in
 */
ThumbnailView::Cell ThumbnailView::ThumbnailWidget::lastCell() const
{
    return Cell( (_displayList.count()-1) / numCols(),
                 (_displayList.count()-1) % numCols());
}

bool ThumbnailView::ThumbnailWidget::isMovementKey( int key )
{
    return ( key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right ||
             key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageUp || key == Qt::Key_PageDown );
}

void ThumbnailView::ThumbnailWidget::toggleSelection( const QString& fileName )
{
    if ( _selectedFiles.contains( fileName ) )
        _selectedFiles.erase( fileName );
    else
        _selectedFiles.insert( fileName );

    updateCell( fileName );
}

QStringList ThumbnailView::ThumbnailWidget::selection( bool keepSortOrderOfDatabase ) const
{
    QStringList images = _displayList;
    if ( keepSortOrderOfDatabase && _sortDirection == NewestFirst )
        images = reverseList( images );

    QStringList res;
    for( QStringList::ConstIterator it = images.begin(); it != images.end(); ++it ) {
        if ( _selectedFiles.contains( *it ) )
            res.append( *it );
    }
    return res;
}

void ThumbnailView::ThumbnailWidget::possibleEmitSelectionChanged()
{
    static StringSet oldSelection;
    if ( oldSelection != _selectedFiles ) {
        oldSelection = _selectedFiles;
        emit selectionChanged();
    }
}

// TODO(hzeller) figure out if this should return the _imageList or _displayList.
QStringList ThumbnailView::ThumbnailWidget::imageList( Order order ) const
{
    if ( order == SortedOrder &&  _sortDirection == NewestFirst )
        return reverseList( _displayList );
    else
        return _displayList;
}

void ThumbnailView::ThumbnailWidget::selectAll()
{
    _selectedFiles.clear();
    for( QStringList::ConstIterator it = _displayList.begin(); it != _displayList.end(); ++it ) {
        _selectedFiles.insert(*it);
    }
    possibleEmitSelectionChanged();
    repaintScreen();
}

void ThumbnailView::ThumbnailWidget::reload(bool flushCache, bool clearSelection)
{
    if ( flushCache )
        QPixmapCache::clear();
    if ( clearSelection )
        _selectedFiles.clear();
    updateCellSize();
    repaintScreen();
}

void ThumbnailView::ThumbnailWidget::repaintScreen()
{
    QPalette p;
    if ( Settings::SettingsData::instance()->thumbnailDarkBackground() ) {
        p.setColor( QPalette::Base, Qt::black );
        p.setColor( QPalette::Foreground, Qt::white );

        QColor c = p.color(QPalette::Active, QColorGroup::Highlight);
        if ((c.red() < 0x30 && c.green() < 0x30 && c.blue() < 0x30) ||
            (c.red() > 0xd0 && c.green() > 0xd0 && c.blue() > 0xd0)) {
            // Not enough contrast to bg or fg. Use light blue instead.
            static QColor highlightColor(0x67, 0x8d, 0xb2);
            p.setColor(QPalette::Active, QColorGroup::Highlight, highlightColor);
            p.setColor(QPalette::Inactive, QColorGroup::Highlight, highlightColor);
            p.setColor(QPalette::Disabled, QColorGroup::Highlight, highlightColor);
        }
    }
    setPalette(p);  // fallback to default.

    for ( int row = firstVisibleRow( PartlyVisible ); row <= lastVisibleRow( PartlyVisible ); ++row )
        for ( int col = 0; col < numCols(); ++col )
            Q3GridView::repaintCell( row, col );
}

QString ThumbnailView::ThumbnailWidget::fileNameUnderCursor() const
{
    return fileNameAtCoordinate( mapFromGlobal( QCursor::pos() ), ViewportCoordinates );
}

QString ThumbnailView::ThumbnailWidget::currentItem() const
{
    return _currentItem;
}

ThumbnailView::ThumbnailWidget* ThumbnailView::ThumbnailWidget::theThumbnailView()
{
    return _instance;
}

void ThumbnailView::ThumbnailWidget::setCurrentItem( const QString& fileName )
{
    Cell cell = positionForFileName( fileName );
    _currentItem = fileName;

    _selectedFiles.clear();
    _selectedFiles.insert( fileName );
    updateCell( fileName );
    ensureCellVisible( cell.row(), cell.col() );
}

void ThumbnailView::ThumbnailWidget::showToolTipsOnImages( bool on )
{
    _toolTip->setActive( on );
}


void ThumbnailView::ThumbnailWidget::contentsDragMoveEvent( QDragMoveEvent* event )
{
    if ( event->provides( "text/uri-list" ) && _selectionInteraction.isDragging() )
        event->accept();
    else {
        event->ignore();
        return;
    }

    int row = rowAt( event->pos().y() );
    int col = columnAt( event->pos().x() );
    QString fileName = fileNameInCell( row, col );

    removeDropIndications();

    QRect rect = cellGeometry( row, col );
    bool left = ( event->pos().x() - rect.x() < rect.width()/2 );
    if ( left ) {
        _leftDrop = fileName;
        int index = _fileNameToIndex[fileName] -1;
        if ( index != -1 )
            _rightDrop = _displayList[index];
    }

    else {
        _rightDrop = fileName;
        const int index = _fileNameToIndex[fileName] +1;
        if ( index != _displayList.count() )
            _leftDrop = _displayList[index];
    }

    updateCell( _leftDrop );
    updateCell( _rightDrop );
}

void ThumbnailView::ThumbnailWidget::contentsDragLeaveEvent( QDragLeaveEvent* )
{
    removeDropIndications();
}

void ThumbnailView::ThumbnailWidget::contentsDropEvent( QDropEvent* )
{
    QTimer::singleShot( 0, this, SLOT( realDropEvent() ) );
}

/**
 * Do the real work for the drop event.
 * We can't bring up the dialog in the contentsDropEvent, as Qt is still in drag and drop mode with a different cursor etc.
 * That's why we use a QTimer to get this call back executed.
 */
void ThumbnailView::ThumbnailWidget::realDropEvent()
{
    QString msg =
        i18n( "<p><b>Really reorder thumbnails?</b></p>"
              "<p>By dragging images around in the thumbnail viewer, you actually reorder them. "
              "This is very useful in case you don't know the exact date for the images. On the other hand, "
              "if the images themself have valid timestamps, you should use "
              "<b>Images -&gt; Sort Selected By Date and Time</b></p>" );

    if ( KMessageBox::questionYesNo( this, msg, i18n("Reorder Thumbnails") , KStandardGuiItem::yes(), KStandardGuiItem::no(),
                                     QString::fromLatin1( "reorder_images" ) ) == KMessageBox::Yes ) {

        // protect against self drop
        if ( !_selectedFiles.contains( _leftDrop ) && ! _selectedFiles.contains( _rightDrop ) ) {
            QStringList selected = selection();
            if ( _rightDrop.isNull() ) {
                // We dropped onto the first image.
                DB::ImageDB::instance()->reorder( _leftDrop, selected, false );
            }
            else
                DB::ImageDB::instance()->reorder( _rightDrop, selected, true );

            Browser::BrowserWidget::instance()->reload();
        }
    }
    removeDropIndications();
}

void ThumbnailView::ThumbnailWidget::removeDropIndications()
{
    QString left = _leftDrop;
    QString right = _rightDrop;
    _leftDrop = QString::null;
    _rightDrop = QString::null;

    updateCell( left );
    updateCell( right );
}

void ThumbnailView::ThumbnailWidget::ensureCellsSorted( Cell& pos1, Cell& pos2 )
{
    if ( pos2.row() < pos1.row() || ( pos2.row() == pos1.row() && pos2.col() < pos1.col() ) ) {
        Cell tmp = pos1;
        pos1 = pos2;
        pos2 = tmp;
    }
}

void ThumbnailView::ThumbnailWidget::slotRepaint()
{
    // Create a local copy to make the _pendingRepaint accessible as soon as
    // possible.
    _pendingRepaintLock.lock();
    StringSet toRepaint(_pendingRepaint);
    _pendingRepaint.clear();
    _pendingRepaintLock.unlock();

    if ( (int) toRepaint.size() > numCols() * numRowsPerPage() / 2 )
        repaintScreen();
    else {
        for( StringSet::const_iterator it = toRepaint.begin(); it != toRepaint.end(); ++it ) {
            Cell cell = positionForFileName( *it );
            Q3GridView::repaintCell( cell.row(), cell.col() );
        }
    }
}

void ThumbnailView::ThumbnailWidget::dimensionChange( int oldNumRows, int /*oldNumCols*/ )
{
    if ( oldNumRows != numRows() )
        repaintScreen();
}

void ThumbnailView::ThumbnailWidget::setSortDirection( SortDirection direction )
{
    if ( direction == _sortDirection )
        return;

    Settings::SettingsData::instance()->setShowNewestFirst( direction == NewestFirst );
    _displayList = reverseList( _displayList );
    updateIndexCache();
    if ( !_currentItem.isNull() )
        setCurrentItem( _currentItem );
    repaintScreen();

    _sortDirection = direction;
}

QStringList ThumbnailView::ThumbnailWidget::reverseList( const QStringList& list) const
{
    QStringList res;
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
        res.prepend(*it);
    }
    return res;
}

void ThumbnailView::ThumbnailWidget::updateCellSize()
{
    QRect dimensions = cellDimensions();
    setCellWidth( dimensions.width() );

    const int oldHeight = cellHeight();
    const int height = dimensions.height() + 2 + textHeight( true );
    setCellHeight( height );
    updateGridSize();
    if ( height != oldHeight && ! _currentItem.isNull() ) {
        const Cell c = positionForFileName(_currentItem);
        ensureCellVisible( c.row(), c.col() );
    }
}

void ThumbnailView::ThumbnailWidget::viewportPaintEvent( QPaintEvent* e )
{
    QPainter p( viewport() );
    p.fillRect( numCols() * cellWidth(), 0, width(), height(), palette().color(QPalette::Base) );
    p.fillRect( 0, numRows() * cellHeight(), width(), height(), palette().color(QPalette::Base) );
    p.end();
    Q3GridView::viewportPaintEvent( e );
}

void ThumbnailView::ThumbnailWidget::updateIndexCache()
{
    _fileNameToIndex.clear();
    int index = 0;
    for( QStringList::ConstIterator it = _displayList.begin(); it != _displayList.end(); ++it,++index ) {
        _fileNameToIndex.insert( *it, index );
    }
}

void ThumbnailView::ThumbnailWidget::contentsDragEnterEvent( QDragEnterEvent * event )
{
    if ( event->provides( "text/uri-list" ) && _selectionInteraction.isDragging() )
        event->accept();
    else
        event->ignore();
}
