#include "viewer.h"
#include <qlayout.h>
#include <qlabel.h>
#include "imageinfo.h"
#include "imagemanager.h"
#include <qsizepolicy.h>
#include <qsimplerichtext.h>
#include <qapplication.h>
#include <qpainter.h>
#include <qrect.h>
#include <qcursor.h>
#include <qpopupmenu.h>
#include <qaction.h>
#include "displayarea.h"
#include <qtoolbar.h>
#include <ktoolbar.h>
#include <kiconloader.h>
#include <kaction.h>
#include <klocale.h>
#include "util.h"

Viewer* Viewer::_instance = 0;

Viewer* Viewer::instance( QWidget* parent )
{
    if ( !_instance )
        _instance = new Viewer( parent , "viewer" );
    return _instance;
}

Viewer::Viewer( QWidget* parent, const char* name )
    :KMainWindow( parent,  name ), _width(800), _height(600)
{
    _label = new DisplayArea( this );
    setCentralWidget( _label );
    setPaletteBackgroundColor( black );
    _label->setFixedSize( _width, _height );

    KIconLoader loader;
    KActionCollection* actions = new KActionCollection( this, "actions" );
    _toolbar = new KToolBar( this );
    _select = new KToggleAction( i18n("Select"), loader.loadIcon(QString::fromLatin1("selecttool"), KIcon::Toolbar),
                         0, _label, SLOT( slotSelect() ),actions, "_select");
    _select->plug( _toolbar );
    _select->setExclusiveGroup( QString::fromLatin1("ViewerTools") );

    _line = new KToggleAction( i18n("Line"), loader.loadIcon(QString::fromLatin1("linetool"), KIcon::Toolbar),
                         0, _label, SLOT( slotLine() ),actions, "_line");
    _line->plug( _toolbar );
    _line->setExclusiveGroup( QString::fromLatin1("ViewerTools") );

    _rect = new KToggleAction( i18n("Rectangle"), loader.loadIcon(QString::fromLatin1("recttool"), KIcon::Toolbar),
                         0, _label, SLOT( slotRectangle() ),actions, "_rect");
    _rect->plug( _toolbar );
    _rect->setExclusiveGroup( QString::fromLatin1("ViewerTools") );

    _circle = new KToggleAction( i18n("Circle"), loader.loadIcon(QString::fromLatin1("ellipsetool"), KIcon::Toolbar),
                           0, _label, SLOT( slotCircle() ),actions, "_circle");
    _circle->plug( _toolbar );
    _circle->setExclusiveGroup( QString::fromLatin1("ViewerTools") );

    _delete = KStdAction::cut( _label, SLOT( cut() ), actions, "cutAction" );
    _delete->plug( _toolbar );

    KAction* close = KStdAction::close( this,  SLOT( stopDraw() ),  actions,  "stopDraw" );
    close->plug( _toolbar );

    _toolbar->hide();
    setupContextMenu();
}


void Viewer::setupContextMenu()
{
    _popup = new QPopupMenu( this );
    QAction* action;

    _firstAction = new QAction( i18n("First"), QIconSet(), i18n("First"), Key_Home, this );
    connect( _firstAction,  SIGNAL( activated() ), this, SLOT( showFirst() ) );
    _firstAction->addTo( _popup );

    _lastAction = new QAction( i18n("Last"), QIconSet(), i18n("Last"), Key_End, this );
    connect( _lastAction,  SIGNAL( activated() ), this, SLOT( showLast() ) );
    _lastAction->addTo( _popup );

    _nextAction = new QAction( i18n("Show Next"), QIconSet(), i18n("Show Next"), Key_PageDown, this );
    connect( _nextAction,  SIGNAL( activated() ), this, SLOT( showNext() ) );
    _nextAction->addTo( _popup );

    _prevAction = new QAction( i18n("Show Previous"),  QIconSet(), i18n("Show Previous"), Key_PageUp, this );
    connect( _prevAction,  SIGNAL( activated() ), this, SLOT( showPrev() ) );
    _prevAction->addTo( _popup );

    _popup->insertSeparator();

    action = new QAction( i18n("Zoom In"),  QIconSet(), i18n("Zoom In"), Key_Plus, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( zoomIn() ) );
    action->addTo( _popup );

    action = new QAction( i18n("Zoom Out"),  QIconSet(), i18n("Zoom Out"), Key_Minus, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( zoomOut() ) );
    action->addTo( _popup );

    _popup->insertSeparator();

    action = new QAction( i18n("Rotate 90 Degrees"),  QIconSet(), i18n("Rotate 90 Degrees"), Key_9, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( rotate90() ) );
    action->addTo( _popup );

    action = new QAction( i18n("Rotate 180 Degrees"),  QIconSet(), i18n("Rotate 180 Degrees"), Key_8, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( rotate180() ) );
    action->addTo( _popup );

    action = new QAction( i18n("Rotate 270 Degrees"),  QIconSet(), i18n("Rotate 270 degrees"), Key_7, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( rotate270() ) );
    action->addTo( _popup );

    _popup->insertSeparator();

    action = new QAction( i18n("Show Info Box"), QIconSet(), i18n("Show Info Box"), Key_I, this, "showInfoBox", true );
    connect( action, SIGNAL( toggled( bool ) ), this, SLOT( toggleShowInfoBox( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showInfoBox() );

    action = new QAction( i18n("Show Drawing"), QIconSet(), i18n("Show Drawing"), CTRL+Key_I, this, "showDrawing", true );
    connect( action, SIGNAL( toggled( bool ) ), _label, SLOT( toggleShowDrawings( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showDrawings() );

    action = new QAction( i18n("Show Description"), QIconSet(), i18n("Show Description"), Key_D, this, "showDescription", true );
    connect( action, SIGNAL( toggled( bool ) ), this, SLOT( toggleShowDescription( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showDescription() );

    action = new QAction( i18n("Show Time"), QIconSet(), i18n("Show Time"), Key_T, this, "showTime", true );
    connect( action, SIGNAL( toggled( bool ) ), this, SLOT( toggleShowDate( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showDate() );

    action = new QAction( i18n("Show Names"), QIconSet(), i18n("Show Names"), Key_N, this, "showNames", true );
    connect( action, SIGNAL( toggled( bool ) ), this, SLOT( toggleShowNames( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showNames() );

    action = new QAction( i18n("Show Location"), QIconSet(), i18n("Show Location"), Key_L, this, "showLocation", true );
    connect( action,  SIGNAL( toggled( bool ) ), this, SLOT( toggleShowLocation( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showLocation() );

    action = new QAction( i18n("Show Keywords"), QIconSet(), i18n("Show Keywords"), Key_K, this, "showKeywords", true );
    connect( action,  SIGNAL( toggled( bool ) ), this, SLOT( toggleShowKeyWords( bool ) ) );
    action->addTo( _popup );
    action->setOn( Options::instance()->showKeyWords() );

    _popup->insertSeparator();

    action = new QAction( i18n("Draw on Image"),  QIconSet(),  i18n("Draw on Image"),  0, this );
    connect( action,  SIGNAL( activated() ),  this, SLOT( startDraw() ) );
    action->addTo( _popup );

    action = new QAction( i18n("Close"),  QIconSet(), i18n("Close"), Key_Q, this );
    connect( action,  SIGNAL( activated() ), this, SLOT( close() ) );
    action->addTo( _popup );
}

void Viewer::load( const ImageInfoList& list, int index )
{
    _list = list;
    _current = index;
    load();
}

void Viewer::pixmapLoaded( const QString&, int w, int h, int, const QImage& image )
{
    // Erase
    QPainter p( _label );
    p.fillRect( 0, 0, _label->width(), _label->height(), paletteBackgroundColor() );
    w = QMIN( w, image.width() );
    h = QMIN( h, image.height() );
    _label->setFixedSize( w, h );
    _label->updateGeometry();

    _pixmap.convertFromImage( image );
    setDisplayedPixmap();
    _label->setDrawList( currentInfo()->drawList() );
}

void Viewer::load()
{
    QRect rect = QApplication::desktop()->screenGeometry( this );
    int w, h;

    if ( currentInfo()->angle() == 0 || currentInfo()->angle() == 180 )  {
        w = _width;
        h = _height;
    }
    else {
        h = _width;
        w = _height;
    }

    if ( w > rect.width() )  {
        h = (int) (h*((double)rect.width()/w));
        w = rect.width();
    }
    if ( h > rect.height() )  {
        w = (int) (w*((double)rect.height()/h));
        h = rect.height();
    }

    _label->setText( i18n("Loading...") );

    ImageManager::instance()->load( currentInfo()->fileName( false ), this, currentInfo()->angle(), w,  h, false, true, false );
    _nextAction->setEnabled( _current +1 < (int) _list.count() );
    _prevAction->setEnabled( _current > 0 );
    _firstAction->setEnabled( _current > 0 );
    _lastAction->setEnabled( _current +1 < (int) _list.count() );
}

void Viewer::setDisplayedPixmap()
{
    QPixmap pixmap = _pixmap;
    if ( pixmap.isNull() )
        return;

    if ( Options::instance()->showInfoBox() )  {
        QPainter p( &pixmap );

        QString text = Util::createInfoText( currentInfo() );

        Options::Position pos = Options::instance()->infoBoxPosition();
        if ( !text.isEmpty() )  {
            text = QString::fromLatin1("<qt>") + text + QString::fromLatin1("</qt>");

            QSimpleRichText txt( text, qApp->font() );

            if ( pos == Options::Top || pos == Options::Bottom )  {
                txt.setWidth( pixmap.width() - 20 ); // 2x5 pixels for inside border + 2x5 outside border.
            }
            else {
                int width = 25;
                do {
                    width += 25;
                    txt.setWidth( width );
                }  while ( txt.height() > width + 25 );
            }

            // -------------------------------------------------- Position rectangle
            QRect rect;
            rect.setWidth( txt.widthUsed() + 10 ); // 5 pixels border in all directions
            rect.setHeight( txt.height() + 10 );

            // x-coordinate
            if ( pos == Options::TopRight || pos == Options::BottomRight || pos == Options::Right )
                rect.moveRight( pixmap.width() - 5 );
            else if ( pos == Options::TopLeft || pos == Options::BottomLeft || pos == Options::Left )
                rect.moveLeft( 5 );
            else
                rect.moveLeft( (pixmap.width() - txt.widthUsed())/2+5 );

            // Y-coordinate
            if ( pos == Options::TopLeft || pos == Options::TopRight || pos == Options::Top )
                rect.moveTop( 5 );
            else if ( pos == Options::BottomLeft || pos == Options::BottomRight || pos == Options::Bottom )
                rect.moveBottom( pixmap.height() - 5 );
            else
                rect.moveTop( (pixmap.height()-txt.height())/2 + 5 );

            p.fillRect( rect, white );
            txt.draw( &p,  rect.left()+5, rect.top()+5,  QRect(),  _label->colorGroup());
            _textRect = rect;
        }
    }

    _label->setPixmap( pixmap );
}

void Viewer::mousePressEvent( QMouseEvent* e )
{
    if ( e->button() == LeftButton )  {
        _moving = Options::instance()->showInfoBox() && _textRect.contains( _label->mapFromParent( e->pos() ) );
        _startPos = Options::instance()->infoBoxPosition();
        if ( _moving )
            _label->setCursor( PointingHandCursor );
    }
    else
        _moving = false;
    KMainWindow::mousePressEvent( e );
}

void Viewer::mouseMoveEvent( QMouseEvent* e )
{
    if ( !_moving )
        return;

    Options::Position oldPos = Options::instance()->infoBoxPosition();
    Options::Position pos = oldPos;
    int x = _label->mapFromParent( e->pos() ).x();
    int y = _label->mapFromParent( e->pos() ).y();
    int w = _label->width();
    int h = _label->height();

    if ( x < w/3 )  {
        if ( y < h/3  )
            pos = Options::TopLeft;
        else if ( y > h*2/3 )
            pos = Options::BottomLeft;
        else
            pos = Options::Left;
    }
    else if ( x > w*2/3 )  {
        if ( y < h/3  )
            pos = Options::TopRight;
        else if ( y > h*2/3 )
            pos = Options::BottomRight;
        else
            pos = Options::Right;
    }
    else {
        if ( y < h/3  )
            pos = Options::Top;
        else if ( y > h*2/3 )
            pos = Options::Bottom;
    }
    if ( pos != oldPos )  {
        Options::instance()->setInfoBoxPosition( pos );
        setDisplayedPixmap();
    }
    KMainWindow::mouseMoveEvent( e );
}

void Viewer::mouseReleaseEvent( QMouseEvent* e )
{
    _label->setCursor( ArrowCursor  );
    if ( Options::instance()->infoBoxPosition() != _startPos )
        Options::instance()->save();
    KMainWindow::mouseReleaseEvent( e );
}

void Viewer::contextMenuEvent( QContextMenuEvent * e )
{
    _popup->exec( e->globalPos() );
}

void Viewer::showNext()
{
    save();
    if ( _current +1 < (int) _list.count() )  {
        _current++;
        load();
    }
}

void Viewer::showPrev()
{
    save();
    if ( _current > 0  )  {
        _current--;
        load();
    }
}

void Viewer::zoomIn()
{
    _width = _width*4/3;
    _height = _height*4/3;
    resize( _width, _height );
    load();
}

void Viewer::zoomOut()
{
    _width = _width*3/4;
    _height = _height*3/4;
    _label->setMinimumSize(0,0);
    resize( _width, _height );
    load();
}

void Viewer::rotate90()
{
    currentInfo()->rotate( 90 );
    load();
}

void Viewer::rotate180()
{
    currentInfo()->rotate( 180 );
    load();
}

void Viewer::rotate270()
{
    currentInfo()->rotate( 270 );
    load();
}

void Viewer::toggleShowInfoBox( bool b )
{
    Options::instance()->setShowInfoBox( b );
    setDisplayedPixmap();
    Options::instance()->save();
}

void Viewer::toggleShowDescription( bool b )
{
    Options::instance()->setShowDescription( b );
    setDisplayedPixmap();
    Options::instance()->save();
}

void Viewer::toggleShowDate( bool b )
{
    Options::instance()->setShowDate( b );
    setDisplayedPixmap();
    Options::instance()->save();
}

void Viewer::toggleShowNames( bool b )
{
    Options::instance()->setShowNames( b );
    setDisplayedPixmap();
    Options::instance()->save();
}

void Viewer::toggleShowLocation( bool b )
{
    Options::instance()->setShowLocation( b );
    setDisplayedPixmap();
    Options::instance()->save();
}

void Viewer::toggleShowKeyWords( bool b )
{
    Options::instance()->setShowKeyWords( b );
    setDisplayedPixmap();
    Options::instance()->save();
}


Viewer::~Viewer()
{
    _instance = 0;
}

void Viewer::showFirst()
{
    save();
    _current = 0;
    load();
}

void Viewer::showLast()
{
    save();
     _current = _list.count() -1;
     load();
}

void Viewer::closeEvent( QCloseEvent* )
{
    close();
}

void Viewer::save()
{
    currentInfo()->setDrawList( _label->drawList() );
}

void Viewer::startDraw()
{
    _label->slotSelect();
    _toolbar->show();
}

void Viewer::stopDraw()
{
    _label->stopDrawings();
    _toolbar->hide();
}

void Viewer::close()
{
    save();
    hide();
}

ImageInfo* Viewer::currentInfo()
{
    return _list.at( _current );
}

#include "viewer.moc"
