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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef BROWSER_H
#define BROWSER_H
#include <q3listview.h>
#include <q3iconview.h>
//Added by qt3to4:
#include <Q3ValueList>
#include "Settings/SettingsData.h"

class Q3ListViewItem;
class QStackedWidget;

namespace DB
{
    class ImageSearchInfo;
}

namespace Browser
{
class BrowserIconViewItemFactory;
class FolderAction;
class BrowserItemFactory;

class BrowserWidget :public QWidget {
    Q_OBJECT
    friend class ImageFolderAction;

public:
    BrowserWidget( QWidget* parent );
    ~BrowserWidget();
    void addSearch( DB::ImageSearchInfo& info );
    void addImageView( const QString& context );

    static BrowserWidget* instance();
    void load( const QString& category, const QString& value );
    bool allowSort();
    DB::ImageSearchInfo currentContext();
    void clear();
    void setFocus();
    QString currentCategory() const;

public slots:
    void back();
    void forward();
    void go();
    void home();
    void reload();
    void slotSmallListView();
    void slotLargeListView();
    void slotSmallIconView();
    void slotLargeIconView();
    void slotLimitToMatch( const QString& );
    void slotInvokeSeleted();
    void scrollLine( int direction );
    void scrollPage( int direction );

signals:
    void canGoBack( bool );
    void canGoForward( bool );
    void showingOverview();
    void pathChanged( const QString& );
    void showsContentView( bool );
    void currentViewTypeChanged( DB::Category::ViewType );
    void viewChanged();

protected slots:
    void init();
    void select( Q3ListViewItem* item );
    void select( Q3IconViewItem* item );
    void select( FolderAction* action );
    void resetIconViewSearch();

protected:
    void addItem( FolderAction* );
    void emitSignals();
    void setupFactory();
    void setViewType( DB::Category::ViewType type );

private:
    static BrowserWidget* _instance;
    Q3ValueList<FolderAction*> _list;
    int _current;
    BrowserItemFactory* _listViewFactory;
    BrowserIconViewItemFactory* _iconViewFactory;
    BrowserItemFactory* _currentFactory;
    QStackedWidget* _stack;
    Q3IconView* _iconView;
    Q3ListView* _listView;
};

}

#endif /* BROWSER_H */

