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

#include "browser.h"
#include "folder.h"
#include <klocale.h>
#include "imagesearchinfo.h"
#include "options.h"
#include "contentfolder.h"
#include "imagefolder.h"
#include <qtimer.h>
#include "imagedb.h"
#include "util.h"
#include <qlistview.h>
#include "showbusycursor.h"
#include "browseritemfactory.h"
#include <qwidgetstack.h>
#include <qlayout.h>
#include <qlabel.h>

Browser* Browser::_instance = 0;

Browser::Browser( QWidget* parent, const char* name )
    :QWidget( parent, name ), _current(0)
{
    Q_ASSERT( !_instance );
    _instance = this;

    _stack = new QWidgetStack( this );
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->addWidget( _stack );

    _listView = new QListView ( _stack );
    _stack->addWidget( _listView );

    _iconView = new QIconView( _stack );
    _stack->addWidget( _iconView );

    _listViewFactory = new BrowserListViewItemFactory( _listView );
    _iconViewFactory = new BrowserIconViewItemFactory( _iconView );

    _listView->addColumn( i18n("What") );
    _listView->addColumn( i18n("Count") );

    _listView->setSelectionMode( QListView::NoSelection );
    _iconView->setSelectionMode( QIconView::NoSelection );
    connect( _listView, SIGNAL( clicked( QListViewItem* ) ), this, SLOT( select( QListViewItem* ) ) );
    connect( _listView, SIGNAL( returnPressed( QListViewItem* ) ), this, SLOT( select( QListViewItem* ) ) );
    connect( _iconView, SIGNAL( clicked( QIconViewItem* ) ), this, SLOT( select( QIconViewItem* ) ) );
    connect( _iconView, SIGNAL( returnPressed( QIconViewItem* ) ), this, SLOT( select( QIconViewItem* ) ) );
    connect( Options::instance(), SIGNAL( optionGroupsChanged() ), this, SLOT( reload() ) );

    // I got to wait till the event loops runs, so I'm sure that the image database has been loaded.
    QTimer::singleShot( 0, this, SLOT( init() ) );
}

void Browser::init()
{
    FolderAction* action = new ContentFolderAction( QString::null, QString::null, ImageSearchInfo(), this );
    _list.append( action );
    forward();
}

void Browser::select( QListViewItem* item )
{
    if ( !item )
        return;

    BrowserListItem* folder = static_cast<BrowserListItem*>( item );
    FolderAction* action = folder->_folder->action( Util::ctrlKeyDown() );
    select( action );
}

void Browser::select( QIconViewItem* item )
{
    if ( !item )
        return;

    BrowserIconItem* folder = static_cast<BrowserIconItem*>( item );
    FolderAction* action = folder->_folder->action( Util::ctrlKeyDown() );
    select( action );
}

void Browser::select( FolderAction* action )
{
    ShowBusyCursor dummy;
    if ( action ) {
        addItem( action );
        setupFactory();
        action->action( _currentFactory );
    }
}

void Browser::forward()
{
    _current++;
    go();
}

void Browser::back()
{
    _current--;
    go();
}

void Browser::go()
{
    setupFactory();
    FolderAction* a = _list[_current-1];
    a->action( _currentFactory );
    emitSignals();
}

void Browser::addSearch( ImageSearchInfo& info )
{
    FolderAction* a;
    if ( ImageDB::instance()->count( info ) > Options::instance()->maxImages() )
        a = new ContentFolderAction( QString::null, QString::null, info, this );
    else
        a = new ImageFolderAction( info, -1, -1, this );
    addItem(a);
    go();
}

void Browser::addItem( FolderAction* action )
{
    while ( (int) _list.count() > _current )
        _list.pop_back();

    _list.append(action);
    _current++;
    emitSignals();
}

void Browser::emitSignals()
{
    FolderAction* a = _list[_current-1];
    emit canGoBack( _current > 1 );
    emit canGoForward( _current < (int)_list.count() );
    if ( !a->showsImages() )
        emit showingOverview();
    emit pathChanged( a->path() );
    _listView->setColumnText( 0, a->title() );
    emit showsContentView( a->contentView() );

    if ( a->contentView() && _list.size() > 0 ) {
        QString grp = a->optionGroup();
        Q_ASSERT( !grp.isNull() );
        Options::ViewSize size = Options::instance()->viewSize( grp );
        Options::ViewType type = Options::instance()->viewType( grp );
        emit currentSizeAndTypeChanged( size, type );
    }
}

void Browser::home()
{
    FolderAction* action = new ContentFolderAction( QString::null, QString::null, ImageSearchInfo(), this );
    addItem( action );
    go();
}

void Browser::reload()
{
    setupFactory();

    if ( _current != 0 ) {
        // _current == 0 when browser hasn't yet been initialized (Which happens through a zero-timer.)
        FolderAction* a = _list[_current-1];
        if ( !a->showsImages() )
            a->action( _currentFactory );
    }
}

Browser* Browser::theBrowser()
{
    Q_ASSERT( _instance );
    return _instance;
}

void Browser::load( const QString& optionGroup, const QString& value )
{
    ImageSearchInfo info;
    info.addAnd( optionGroup, value );
    FolderAction* a;
    if (  Util::ctrlKeyDown() && ImageDB::instance()->count( info ) < Options::instance()->maxImages() )
        a = new ImageFolderAction( info, -1, -1, this );
    else {
        a = new ContentFolderAction( optionGroup, value, info, this );
    }

    addItem( a );
    a->action( _currentFactory );
    topLevelWidget()->raise();
    setActiveWindow();
}

bool Browser::allowSort()
{
    return _list[_current-1]->allowSort();
}


ImageSearchInfo Browser::current()
{
    return _list[_current-1]->_info;
}

void Browser::slotSmallListView()
{
    setSizeAndType( Options::ListView,Options::Small );
}

void Browser::slotLargeListView()
{
    setSizeAndType( Options::ListView,Options::Large );
}

void Browser::slotSmallIconView()
{
    setSizeAndType( Options::IconView,Options::Small );
}

void Browser::slotLargeIconView()
{
    setSizeAndType( Options::IconView,Options::Large );
}

void Browser::setSizeAndType( Options::ViewType type, Options::ViewSize size )
{
    Q_ASSERT( _list.size() > 0 );

    FolderAction* a = _list[_current-1];
    QString grp = a->optionGroup();
    Q_ASSERT( !grp.isNull() );

    Options::instance()->setViewType( grp, type );
    Options::instance()->setViewSize( grp, size );
    reload();
}

void Browser::clear()
{
    _iconView->clear();
    _listView->clear();
}

void Browser::setupFactory()
{
    Options::ViewType type = Options::ListView;
    if ( _list.size() == 0 )
        return;

    FolderAction* a = _list[_current-1];
    QString optionGroup = a->optionGroup();

    if ( !optionGroup.isNull() )
        type = Options::instance()->viewType( optionGroup );

    if ( type == Options::ListView ) {
        _currentFactory = _listViewFactory;
        _stack->raiseWidget( _listView );
    }
    else {
        _currentFactory = _iconViewFactory;
        _stack->raiseWidget( _iconView );
    }
    setFocus();
}

void Browser::setFocus()
{
    _stack->visibleWidget()->setFocus();
}

#include "browser.moc"
