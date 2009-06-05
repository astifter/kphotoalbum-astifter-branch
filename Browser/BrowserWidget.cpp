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

#include "BrowserWidget.h"
#include <DB/ImageDB.h>
#include <QHeaderView>
#include "ImageViewAction.h"
#include "CategoryModel.h"
#include "TreeFilter.h"
#include <QTreeView>
#include <DB/ImageSearchInfo.h>
#include "OverviewModel.h"
#include "enums.h"

#include <klocale.h>
#include "DB/ImageSearchInfo.h"
#include "Settings/SettingsData.h"
#include <qtimer.h>
#include <QHBoxLayout>
#include "DB/ImageDB.h"
#include "Utilities/Util.h"
#include "Utilities/ShowBusyCursor.h"
#include <QStackedWidget>
#include <qlayout.h>
#include "DB/CategoryCollection.h"

Browser::BrowserWidget* Browser::BrowserWidget::_instance = 0;

Browser::BrowserWidget::BrowserWidget( QWidget* parent )
    :QWidget( parent ), _current(0)
{
    Q_ASSERT( !_instance );
    _instance = this;

    createWidgets();

    connect( DB::ImageDB::instance()->categoryCollection(), SIGNAL( categoryCollectionChanged() ), this, SLOT( reload() ) );
    connect( this, SIGNAL( viewChanged() ), this, SLOT( resetIconViewSearch() ) );

    _filterProxy = new TreeFilter(this);
    _filterProxy->setFilterKeyColumn(0);
    _filterProxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
    _filterProxy->setSortRole( ValueRole );

    addAction( new OverviewModel( Breadcrumb::home(), DB::ImageSearchInfo(), this ) );
    QTimer::singleShot( 0, this, SLOT( emitSignals() ) );
}

void Browser::BrowserWidget::forward()
{
    _current++;
    go();
}

void Browser::BrowserWidget::back()
{
    _current--;
    while ( _current > 0 ) {
        if ( currentAction()->showDuringBack() )
            break;
        else
            _current--;
    }
    go();
}

void Browser::BrowserWidget::go()
{
    currentAction()->activate();
    setBranchOpen(QModelIndex(), true);
    switchToViewType( currentAction()->viewType() );
    adjustTreeViewColumnSize();
    emitSignals();
}

void Browser::BrowserWidget::addSearch( DB::ImageSearchInfo& info )
{
    addAction( new OverviewModel( Breadcrumb::empty(), info, this ) );
}

void Browser::BrowserWidget::addImageView( const QString& context )
{
    addAction( new ImageViewAction( context, this ) );
}

void Browser::BrowserWidget::addAction( Browser::BrowserAction* action )
{
    while ( (int) _list.count() > _current ) {
        BrowserAction* m = _list.back();
        _list.pop_back();
        delete m;
    }

    _list.append(action);
    _current++;
    go();
}

void Browser::BrowserWidget::emitSignals()
{
    emit canGoBack( _current > 1 );
    emit canGoForward( _current < (int)_list.count() );
    if ( currentAction()->viewer() == ShowBrowser )
        emit showingOverview();

    emit isSearchable( currentAction()->isSearchable() );
    emit isViewChangeable( currentAction()->isViewChangeable() );

    bool isCategoryAction = (dynamic_cast<CategoryModel*>( currentAction() ) != 0);

    if ( isCategoryAction ) {
        DB::CategoryPtr category = DB::ImageDB::instance()->categoryCollection()->categoryForName( currentCategory() );
        Q_ASSERT( category.data() );

        emit currentViewTypeChanged( category->viewType());
    }

    emit pathChanged( createPath() );
    emit viewChanged();
    emit imageCount( DB::ImageDB::instance()->count(currentAction()->searchInfo()).total() );

#ifdef KDAB_TEMPORARILY_REMOVED
    bool showingCategory = dynamic_cast<TypeFolderAction*>( a );
    emit browsingInSomeCategory( showingCategory );
    _listView->setRootIsDecorated( showingCategory );
#else // KDAB_TEMPORARILY_REMOVED
    qWarning("Code commented out in Browser::BrowserWidget::emitSignals");
#endif //KDAB_TEMPORARILY_REMOVED
}

void Browser::BrowserWidget::home()
{
    addAction( new OverviewModel( Breadcrumb::home(), DB::ImageSearchInfo(), this ) );
}

void Browser::BrowserWidget::reload()
{
    currentAction()->activate();
}

Browser::BrowserWidget* Browser::BrowserWidget::instance()
{
    Q_ASSERT( _instance );
    return _instance;
}

void Browser::BrowserWidget::load( const QString& category, const QString& value )
{
    DB::ImageSearchInfo info;
    info.addAnd( category, value );

    DB::MediaCount counts = DB::ImageDB::instance()->count( info );
    bool loadImages = (counts.total() < static_cast<uint>(Settings::SettingsData::instance()->autoShowThumbnailView()));
    if ( Utilities::ctrlKeyDown() ) loadImages = !loadImages;

    if ( loadImages )
        addAction( new ImageViewAction( info, this ) );
    else
        addAction( new OverviewModel( Breadcrumb(value, true) , info, this ) );

    go();
    topLevelWidget()->raise();
    activateWindow();
}

DB::ImageSearchInfo Browser::BrowserWidget::currentContext()
{
    return currentAction()->searchInfo();
}

void Browser::BrowserWidget::slotSmallListView()
{
    changeViewTypeForCurrentView( DB::Category::ListView );
}

void Browser::BrowserWidget::slotLargeListView()
{
    changeViewTypeForCurrentView( DB::Category::ThumbedListView );
}

void Browser::BrowserWidget::slotSmallIconView()
{
    changeViewTypeForCurrentView( DB::Category::IconView );
}

void Browser::BrowserWidget::slotLargeIconView()
{
    changeViewTypeForCurrentView( DB::Category::ThumbedIconView );
}

void Browser::BrowserWidget::changeViewTypeForCurrentView( DB::Category::ViewType type )
{
    Q_ASSERT( _list.size() > 0 );

    DB::CategoryPtr category = DB::ImageDB::instance()->categoryCollection()->categoryForName( currentCategory() );
    Q_ASSERT( category.data() );
    category->setViewType( type );

    switchToViewType( type );
    reload();
}

void Browser::BrowserWidget::setFocus()
{
    _stack->currentWidget()->setFocus();
}

QString Browser::BrowserWidget::currentCategory() const
{
    if ( CategoryModel* action = dynamic_cast<CategoryModel*>( currentAction() ) )
        return action->category()->name();
    else
        return QString();
}

void Browser::BrowserWidget::slotLimitToMatch( const QString& str )
{
    _filterProxy->resetCache();
    _filterProxy->setFilterFixedString( str );
    setBranchOpen(QModelIndex(), true);
    adjustTreeViewColumnSize();
}

void Browser::BrowserWidget::resetIconViewSearch()
{
    _filterProxy->resetCache();
    _filterProxy->setFilterRegExp( QString() );
    adjustTreeViewColumnSize();
}

void Browser::BrowserWidget::slotInvokeSeleted()
{
    if ( _curView->currentIndex().isValid() )
        itemClicked( _curView->currentIndex() );
}


void Browser::BrowserWidget::scrollLine( int direction )
{
    _curView->verticalScrollBar()->setValue( _curView->verticalScrollBar()->value()+10*direction );
}

void Browser::BrowserWidget::scrollPage( int direction )
{
    int dist = direction * (height()-100);
    _curView->verticalScrollBar()->setValue( _curView->verticalScrollBar()->value()+dist );
}

void Browser::BrowserWidget::itemClicked( const QModelIndex& index )
{
    Utilities::ShowBusyCursor dummy;
    BrowserAction* action = currentAction()->generateChildAction( _filterProxy->mapToSource( index ) );
    if ( action )
        addAction( action );
}


Browser::BrowserAction* Browser::BrowserWidget::currentAction() const
{
    return _list[_current-1];
}

void Browser::BrowserWidget::setModel( QAbstractItemModel* model)
{
    _filterProxy->setSourceModel( model );
}

void Browser::BrowserWidget::switchToViewType( DB::Category::ViewType type )
{
    if ( _curView ) {
        // disconnect current view
        _curView->setModel(0);
//        disconnect( _curView, SIGNAL(activated( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );
        disconnect( _curView, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)) );
    }

    if ( type == DB::Category::ListView || type == DB::Category::ThumbedListView ) {
        _curView = _treeView;
        adjustTreeViewColumnSize();
    }
    else {
        _curView =_listView;
        _filterProxy->invalidate();
        _filterProxy->sort( 0, Qt::AscendingOrder );
    }


    // Hook up the new view
    _curView->setModel( _filterProxy );
//    connect( _curView, SIGNAL(activated( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );
    connect( _curView, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)) );

    _stack->setCurrentWidget( _curView );

}

void Browser::BrowserWidget::setBranchOpen( const QModelIndex& parent, bool open )
{
    if ( _curView != _treeView )
        return;

    const int count = _filterProxy->rowCount(parent);
    if ( count > 5 )
        open = false;

    _treeView->setExpanded( parent, open );
    for ( int row = 0; row < count; ++row )
        setBranchOpen( _filterProxy->index( row, 0 ,parent ), open );
}


Browser::BreadcrumbList Browser::BrowserWidget::createPath() const
{
    BreadcrumbList result;

    for ( int i = 0; i < _current; ++i )
        result.append(_list[i]->breadcrumb() );

    return result;
}

void Browser::BrowserWidget::widenToBreadcrumb( const Browser::Breadcrumb& breadcrumb )
{
    while ( currentAction()->breadcrumb() != breadcrumb )
        _current--;
    go();
}

void Browser::BrowserWidget::adjustTreeViewColumnSize()
{
    _treeView->header()->resizeSections(QHeaderView::ResizeToContents);
}


void Browser::BrowserWidget::createWidgets()
{
    _stack = new QStackedWidget;
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget( _stack );

    _listView = new QListView ( _stack );
    _listView->setIconSize( QSize(100,75) );
    _listView->setSelectionMode( QListView::SingleSelection );
    _listView->setViewMode( QListView::IconMode );
//    _listView->setGridSize( QSize(150, 200) );
    _listView->setSpacing(10);
    _listView->setUniformItemSizes(true);
    _stack->addWidget( _listView );

    _treeView = new QTreeView( _stack );
    _treeView->header()->setStretchLastSection(false);
    _treeView->header()->setSortIndicatorShown(true);
    _treeView->setSortingEnabled(true);
    _stack->addWidget( _treeView );

    connect( _treeView, SIGNAL( expanded( QModelIndex ) ), SLOT( adjustTreeViewColumnSize() ) );

    _listView->setMouseTracking(true);
    _treeView->setMouseTracking(true);
    connect( _listView, SIGNAL( entered( QModelIndex ) ), _listView, SLOT( setCurrentIndex( QModelIndex ) ) );
    connect( _treeView, SIGNAL( entered( QModelIndex ) ), _treeView, SLOT( setCurrentIndex( QModelIndex ) ) );

    _curView = 0;
}

#include "BrowserWidget.moc"
