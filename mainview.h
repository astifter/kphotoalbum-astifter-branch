/*
 *  Copyright (c) 2003 Jesper K. Pedersen <blackie@kde.org>
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

#ifndef MAINVIEW_H
#define MAINVIEW_H
class OptionsDialog;
class ImageConfig;
class QWidgetStack;
class ImageCounter;
class QTimer;
class Browser;
class KTipDialog;
class DeleteDialog;
class QLabel;
class HTMLExportDialog;
#include "imageinfo.h"
#include <qdict.h>
#include <kmainwindow.h>
#include "thumbnailview.h"

class MainView :public KMainWindow
{
    Q_OBJECT

public:
    MainView( QWidget* parent,  const char* name = 0 );
    static void configureImages( const ImageInfoList& list, bool oneAtATime );

protected slots:
    bool slotExit();
    void slotOptions();
    void slotConfigureAllImages();
    void slotConfigureImagesOneAtATime();
    void slotSave();
    void slotDeleteSelected();
    void slotSearch();
    void slotViewSelected( bool reuse = true );
    void slotViewSelectedNewWindow();
    void slotChanges();
    void slotLimitToSelected();
    void slotExportToHTML();
    void slotAutoSave();
    void showThumbNails();
    void showBrowser();
    void slotOptionGroupChanged();
    void showTipOfDay();
    void pathChanged( const QString& );
    void runDemo();
    void lockToDefaultScope();
    void setDefaultScopePositive();
    void setDefaultScopeNegative();
    void unlockFromDefaultScope();
    void changePassword();
    void slotConfigureKeyBindings();

protected:
    void configureImages( bool oneAtATime );
    ImageInfoList selected();
    QString welcome();
    virtual void closeEvent( QCloseEvent* e );
    void setupMenuBar();
    void startAutoSaveTimer();
    void createImageConfig();
    void load();
    void save( const QString& fileName );
    virtual void contextMenuEvent( QContextMenuEvent* e );
    void setDirty( bool b );
    void setLocked( bool b );

private:
    static MainView* _instance;

    ThumbNailView* _thumbNailView;
    OptionsDialog* _optionsDialog;
    ImageConfig* _imageConfigure;
    bool _dirty;
    QWidgetStack* _stack;
    QWidget* _welcome;
    QTimer* _autoSaveTimer;
    Browser* _browser;
    KTipDialog* _tipDialog;
    DeleteDialog* _deleteDialog;
    QLabel* _dirtyIndicator;
    QLabel* _lockedIndicator;
    KAction* _lock;
    KAction* _unlock;
    KAction* _setDefaultPos;
    KAction* _setDefaultNeg;
    HTMLExportDialog* _htmlDialog;
};


#endif /* MAINVIEW_H */

