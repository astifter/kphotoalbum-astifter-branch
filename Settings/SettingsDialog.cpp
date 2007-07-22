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

#include "SettingsDialog.h"
#include <kfiledialog.h>
#include <klocale.h>
#include <qlayout.h>
#include <qlabel.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include "Settings/SettingsData.h"
#include <kicondialog.h>
#include <qlistbox.h>
#include <kmessagebox.h>
#include "DB/ImageDB.h"
#include <qcheckbox.h>
#include <kinputdialog.h>
#include <qwhatsthis.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <qhgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qgrid.h>
#include <qvbuttongroup.h>
#include <qtabwidget.h>
#include "ViewerSizeConfig.h"
#include <limits.h>
#include <config.h>
#ifdef HASKIPI
#  include <libkipi/pluginloader.h>
#endif
#include "DB/CategoryCollection.h"
#include "Utilities/ShowBusyCursor.h"
#include "SettingsDialog.moc"
#include <kapplication.h>
#include "MainWindow/Window.h"

#ifdef HASEXIV2
#  include "Exif/Info.h"
#  include "Exif/TreeView.h"
#endif

#ifdef SQLDB_SUPPORT
#  include "SQLDB/DatabaseAddress.h"
#  include "SQLDB/SQLSettingsWidget.h"
#endif

#include "CategoryItem.h"
#include "Exif/SyncWidget.h"

Settings::SettingsDialog::SettingsDialog( QWidget* parent, const char* name )
    :KDialogBase( IconList, i18n( "Settings" ), Apply | Ok | Cancel, Ok, parent, name, false ), _currentCategory( QString::null ), _currentGroup( QString::null )
{
    createGeneralPage();
    createThumbNailPage();
    createOptionGroupsPage();
    createGroupConfig();
    createViewerPage();
    createPluginPage();
    createEXIFPage();
    createSyncPage();
    createDatabaseBackendPage();

    connect( this, SIGNAL( aboutToShowPage( QWidget* ) ), this, SLOT( slotPageChange() ) );
    connect( this, SIGNAL( applyClicked() ), this, SLOT( slotMyOK() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotMyOK() ) );
}

void Settings::SettingsDialog::createGeneralPage()
{
    QWidget* top = addPage( i18n("General" ), i18n("General" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "kphotoalbum" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    QVGroupBox* box = new QVGroupBox( i18n( "New Images" ), top );
    lay1->addWidget( box );

    // Thrust time stamps
    QWidget* container = new QWidget( box );
    QHBoxLayout* hlay = new QHBoxLayout( container, 0, 6 );
    hlay->addStretch( 1 );

    // Search for images on startup
    _searchForImagesOnStartup = new QCheckBox( i18n("Search for new images and videos on startup"), box );
    _dontReadRawFilesWithOtherMatchingFile = new QCheckBox( i18n("Don't read RAW files if matching JPEG/TIFF file exists"), box );

    // Datebar size
    container = new QWidget( top );
    lay1->addWidget( container );
    hlay = new QHBoxLayout( container, 0, 6 );
    QLabel* datebarSize = new QLabel( i18n("Size of histogram columns in datebar:"), container );
    hlay->addWidget( datebarSize );
    _barWidth = new QSpinBox( 1, 100, 1, container );
    hlay->addWidget( _barWidth );
    QLabel* cross = new QLabel( QString::fromLatin1( " x " ), container );
    hlay->addWidget( cross );
    _barHeight = new QSpinBox( 15, 100, 1, container );
    hlay->addWidget( _barHeight );
    hlay->addStretch( 1 );

    // Show splash screen
    _showSplashScreen = new QCheckBox( i18n("Show splash screen"), top );
    lay1->addWidget( _showSplashScreen );

    // Album Category
    QLabel* albumCategoryLabel = new QLabel( i18n("Category for virtual albums:" ), top, "albumCategoryLabel" );
    _albumCategory = new QComboBox( top, "_albumCategory" );
    QHBoxLayout* lay7 = new QHBoxLayout( lay1, 6 );
    lay7->addWidget( albumCategoryLabel );
    lay7->addWidget( _albumCategory );
    lay7->addStretch(1);

    QValueList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    for( QValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it ) {
        _albumCategory->insertItem( (*it)->text() );
    }

    lay1->addStretch( 1 );


    // Whats This
    QString txt;

    txt = i18n( "<p>KPhotoAlbum is capable of searching for new images and videos when started, this does, "
                "however, take some time, so instead you may wish to manually tell KPhotoAlbum to search for new images "
                "using <b>Maintenance->Rescan for new images</b></p>");
    QWhatsThis::add( _searchForImagesOnStartup, txt );

    txt = i18n( "<p>KPhotoAlbum is capable of reading certain kinds of RAW images.  "
		"Some cameras store both a RAW image and a matching JPEG or TIFF image.  "
		"This causes duplicate images to be stored in KPhotoAlbum, which may be undesirable.  "
		"If this option is checked, KPhotoAlbum will not read RAW files for which matching image files also exist.</p>");
    QWhatsThis::add( _dontReadRawFilesWithOtherMatchingFile, txt );

    txt = i18n("<p>KPhotoAlbum shares plugins with other imaging applications, some of which have the concept of albums. "
               "KPhotoAlbum does not have this concept; nevertheless, for certain plugins to function, KPhotoAlbum behaves "
               "to the plugin system as if it did.</p>"
               "<p>KPhotoAlbum does this by defining the current album to be the current view - that is, all the images the "
               "browser offers to display.</p>"
               "<p>In addition to the current album, KPhotoAlbum must also be able to give a list of all albums; "
               "the list of all albums is defined in the following way:"
               "<ul><li>When KPhotoAlbum's browser displays the content of a category, say all People, then each item in this category "
               "will look like an album to the plugin."
               "<li>Otherwise, the category you specify using this option will be used; e.g. if you specify People "
               "with this option, then KPhotoAlbum will act as if you had just chosen to display people and then invoke "
               "the plugin which needs to know about all albums.</p>"
               "<p>Most users would probably want to specify Keywords here.</p>");
    QWhatsThis::add( albumCategoryLabel, txt );
    QWhatsThis::add( _albumCategory, txt );

    txt = i18n( "Show the KPhotoAlbum splash screen on start up" );
    QWhatsThis::add( _showSplashScreen, txt );
}

void Settings::SettingsDialog::createThumbNailPage()
{
    QWidget* top = addPage( i18n("Thumbnail View" ), i18n("Thumbnail View" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "view_icon" ),
                                                             KIcon::Desktop, 32 ) );

    QGridLayout* lay = new QGridLayout( top );
    lay->setSpacing( 6 );
    int row = 0;

    // Preview size
    QLabel* previewSizeLabel = new QLabel( i18n("Tooltip preview image size:" ), top, "previewSizeLabel" );
    _previewSize = new QSpinBox( 0, 2000, 10, top, "_previewSize" );
    _previewSize->setSpecialValueText( i18n("No Image Preview") );
    lay->addWidget( previewSizeLabel, row, 0 );
    lay->addWidget( _previewSize, row, 1 );

    // Thumbnail size
    ++row;
    QLabel* thumbnailSizeLabel = new QLabel( i18n("Thumbnail image size:" ), top, "thumbnailSizeLabel" );
    _thumbnailSize = new QSpinBox( 0, 512, 16, top, "_thumbnailSize" );
    lay->addWidget( thumbnailSizeLabel, row, 0 );
    lay->addWidget( _thumbnailSize, row, 1 );

    // Thumbnail aspect ratio
    ++row;
    QLabel* thumbnailAspectRatioLabel = new QLabel( i18n("Thumbnail table cells aspect ratio"), top, "thumbnailAspectRatioLabel");
    _thumbnailAspectRatio = new KComboBox( top );
    _thumbnailAspectRatio->insertStringList( QStringList() << i18n("1:1") << i18n("4:3")
        << i18n("3:2") << i18n("16:9") << i18n("3:4") << i18n("2:3") << i18n("9:16"));
    lay->addWidget( thumbnailAspectRatioLabel, row, 0 );
    lay->addWidget( _thumbnailAspectRatio, row, 1 );

    // Space around cells
    ++row;
    QLabel* thumbnailSpaceLabel = new QLabel( i18n("Space around cells"), top, "thumbnailSpaceLabel");
    _thumbnailSpace = new QSpinBox( 0, 20, 1, top );
    lay->addWidget( thumbnailSpaceLabel, row, 0 );
    lay->addWidget( _thumbnailSpace, row, 1 );

    // Display dark background
    ++row;
    _thumbnailDarkBackground = new QCheckBox( i18n("Show thumbnails on dark background" ), top, "thumbnailDarkBackground");
    lay->addMultiCellWidget( _thumbnailDarkBackground, row, row, 0, 1 );

    // Display grid lines in the thumbnail view
    ++row;
    _thumbnailDisplayGrid = new QCheckBox( i18n("Display grid around thumbnails" ),
                                           top, "_thumbnailDisplayGrid");
    lay->addMultiCellWidget( _thumbnailDisplayGrid, row, row, 0, 1 );

    // Display Labels
    ++row;
    _displayLabels = new QCheckBox( i18n("Display labels in thumbnail view" ), top, "displayLabels" );
    lay->addMultiCellWidget( _displayLabels, row, row, 0, 1 );

    // Display Categories
    ++row;
    _displayCategories = new QCheckBox( i18n("Display categories in thumbnail view" ), top, "displayCategories" );
    lay->addMultiCellWidget( _displayCategories, row, row, 0, 1 );

    // Auto Show Thumbnail view
    ++row;
    QLabel* autoShowLabel = new QLabel( i18n("Auto display limit: "), top );
    _autoShowThumbnailView = new QSpinBox( 0, 10000, 10, top );
    _autoShowThumbnailView->setSpecialValueText( i18n("Never") );
    lay->addWidget( autoShowLabel, row, 0 );
    lay->addWidget( _autoShowThumbnailView, row, 1 );

    // Thumbnail Cache
    ++row;
    QLabel* cacheLabel = new QLabel( i18n( "Thumbnail cache:" ), top );
    _thumbnailCache = new QSpinBox( 1, 256, 1, top );
    _thumbnailCache->setSuffix( i18n("Mbytes" ) );
    lay->addWidget( cacheLabel, row, 0 );
    lay->addWidget( _thumbnailCache, row, 1 );

    lay->setColStretch( 1, 1 );
    lay->setRowStretch( ++row, 1 );

    // Whats This
    QString txt;

    txt = i18n( "<p>If you select <b>Settings -&gt; Show Tooltips</b> in the thumbnail view, then you will see a small tool tip window "
                "displaying information about the thumbnails. This window includes a small preview image. "
                "This option configures the image size.</p>" );
    QWhatsThis::add( previewSizeLabel, txt );
    QWhatsThis::add( _previewSize, txt );


    txt = i18n( "<p>Thumbnail image size. You may also set the size simply by dragging the thumbnail view using the middle mouse button.</p>" );
    QWhatsThis::add( thumbnailSizeLabel, txt );
    QWhatsThis::add( _thumbnailSize, txt );

    txt = i18n("<p>Choose what aspect ratio the cells holding thumbnails should have.</p>");
    QWhatsThis::add( _thumbnailAspectRatio, txt );

    txt = i18n("<p>How thick the cell padding should be.</p>");
    QWhatsThis::add( thumbnailSpaceLabel, txt );

    txt = i18n("<p>The background color in the thumbnail view is "
               "the systems' default background; by checking this option, "
               "you can set it to be dark instead.</p>");
    QWhatsThis::add( _thumbnailDarkBackground, txt );

    txt = i18n("<p>If you want to see grid around your thumbnail images, "
               "select this option.</p>");
    QWhatsThis::add( _thumbnailDisplayGrid, txt );

    txt = i18n("<p>Checking this option will show the base name for the file under "
               "thumbnails in the thumbnail view.</p>");
    QWhatsThis::add( _displayLabels, txt );

    txt = i18n("<p>Checking this option will show the Categories for the file under "
        "thumbnails in the thumbnail view</p>");
    QWhatsThis::add( _displayCategories, txt );

    txt = i18n("<p>When you are browsing, and the count gets below the value specified here, "
               "the thumbnails will be shown automatically. The alternative is to continue showing the "
               "browser until you press <i>Show Images</i></p>");
    QWhatsThis::add( _autoShowThumbnailView, txt );
    QWhatsThis::add( autoShowLabel, txt );

    txt = i18n("<p>Specify the size of the cache used to hold thumbnails.</p>");
    QWhatsThis::add( cacheLabel, txt );
    QWhatsThis::add( _thumbnailCache, txt );
}


void Settings::SettingsDialog::createOptionGroupsPage()
{
    QWidget* top = addPage( i18n("Categories"), i18n("Categories"),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "identity" ),
                                                             KIcon::Desktop, 32 ) );

    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );
    QHBoxLayout* lay2 = new QHBoxLayout( lay1, 6 );

    _categories = new QListBox( top );
    connect( _categories, SIGNAL( clicked( QListBoxItem* ) ), this, SLOT( edit( QListBoxItem* ) ) );
    lay2->addWidget( _categories );


    QGridLayout* lay3 = new QGridLayout( lay2, 6 );

    _labelLabel = new QLabel( i18n( "Label:" ), top );
    lay3->addWidget( _labelLabel, 0, 0 );

    _text = new QLineEdit( top );
    connect( _text, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( slotLabelChanged( const QString& ) ) );

    lay3->addWidget( _text, 0, 1 );


    // Icon
    _iconLabel = new QLabel( i18n("Icon:" ), top );
    lay3->addWidget( _iconLabel, 1, 0 );

    _icon = new KIconButton(  top );
    lay3->addWidget( _icon, 1, 1 );
    _icon->setIconSize(32);
    _icon->setIcon( QString::fromLatin1( "personsIcon" ) );
    connect( _icon, SIGNAL( iconChanged( QString ) ), this, SLOT( slotIconChanged( QString ) ) );


    // Thumbnail size
    _thumbnailSizeInCategoryLabel = new QLabel( i18n( "Thumbnail Size: " ), top );
    lay3->addWidget( _thumbnailSizeInCategoryLabel, 2, 0 );

    _thumbnailSizeInCategory = new QSpinBox( 32, 512, 32, top );
    lay3->addWidget( _thumbnailSizeInCategory, 2, 1 );
    connect( _thumbnailSizeInCategory, SIGNAL( valueChanged( int ) ), this, SLOT( thumbnailSizeChanged( int ) ) );


    // Prefered View
    _preferredViewLabel = new QLabel( i18n("Preferred view:"), top );
    lay3->addWidget( _preferredViewLabel, 3, 0 );

    _preferredView = new QComboBox( top );
    lay3->addWidget( _preferredView, 3, 1 );
    QStringList list;
    list << i18n("List View") << i18n("List View with Custom Thumbnails") << i18n("Icon View") << i18n("Icon View with Custom Thumbnails");
    _preferredView->insertStringList( list );
    connect( _preferredView, SIGNAL( activated( int ) ), this, SLOT( slotPreferredViewChanged( int ) ) );

    QHBoxLayout* lay4 = new QHBoxLayout( lay1, 6 );
    KPushButton* newItem = new KPushButton( i18n("New"), top );
    connect( newItem, SIGNAL( clicked() ), this, SLOT( slotNewItem() ) );

    _delItem = new KPushButton( i18n("Delete"), top );
    connect( _delItem, SIGNAL( clicked() ), this, SLOT( slotDeleteCurrent() ) );

    lay4->addStretch(1);
    lay4->addWidget( newItem );
    lay4->addWidget( _delItem );

    _current = 0;
}



void Settings::SettingsDialog::show()
{
    Settings::SettingsData* opt = Settings::SettingsData::instance();

    // General page
    _previewSize->setValue( opt->previewSize() );
    _thumbnailSize->setValue( opt->thumbSize() );
    _searchForImagesOnStartup->setChecked( opt->searchForImagesOnStartup() );
    _dontReadRawFilesWithOtherMatchingFile->setChecked( opt->dontReadRawFilesWithOtherMatchingFile() );
    _compressedIndexXML->setChecked( opt->useCompressedIndexXML() );
    _showSplashScreen->setChecked( opt->showSplashScreen() );
    _autosave->setValue( opt->autoSave() );
    _barWidth->setValue( opt->histogramSize().width() );
    _barHeight->setValue( opt->histogramSize().height() );
    _backupCount->setValue( opt->backupCount() );
    _compressBackup->setChecked( opt->compressBackup() );

    DB::CategoryPtr cat = DB::ImageDB::instance()->categoryCollection()->categoryForName( opt->albumCategory() );
    if ( !cat )
        cat = DB::ImageDB::instance()->categoryCollection()->categories()[0];
    _albumCategory->setCurrentText( cat->text() );

    _thumbnailDarkBackground->setChecked( opt->thumbnailDarkBackground() );
    _thumbnailDisplayGrid->setChecked( opt->thumbnailDisplayGrid() );
    _thumbnailAspectRatio->setCurrentItem( opt->thumbnailAspectRatio() );
    _thumbnailSpace->setValue( opt->thumbnailSpace() );
    _displayLabels->setChecked( opt->displayLabels() );
    _displayCategories->setChecked( opt->displayCategories() );
    _viewImageSetup->setSize( opt->viewerSize() );
    _viewImageSetup->setLaunchFullScreen( opt->launchViewerFullScreen() );
    _slideShowSetup->setSize( opt->slideShowSize() );
    _slideShowSetup->setLaunchFullScreen( opt->launchSlideShowFullScreen() );
    _slideShowInterval->setValue( opt->slideShowInterval() );
    _cacheSize->setValue( opt->viewerCacheSize() );
    _thumbnailCache->setValue( opt->thumbnailCache() );
    _smoothScale->setCurrentItem( opt->smoothScale() );
    _autoShowThumbnailView->setValue( opt->autoShowThumbnailView() );
    _viewerStandardSize->setCurrentItem( opt->viewerStandardSize() );

#ifdef HASKIPI
    _delayLoadingPlugins->setChecked( opt->delayLoadingPlugins() );
#endif

    // Config Groups page
    _categories->clear();
    QValueList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    for( QValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it ) {
        if( !(*it)->isSpecialCategory() ) {
            CategoryItem* item = new CategoryItem( (*it)->name(), (*it)->text(),(*it)->iconName(),(*it)->viewType(), (*it)->thumbnailSize(), _categories );
            connect( item, SIGNAL( categoryRenamed( const QString&, const QString& ) ), this, SLOT( slotCategoryRenamed( const QString&, const QString& ) ) );
            connect( item, SIGNAL( categoryAdded( const QString& ) ), this, SLOT( slotCategoryAdded( const QString& ) ) );
            connect( item, SIGNAL( categoryRemoved( const QString& ) ), this, SLOT( slotCategoryRemoved( const QString& ) ) );
        }
    }

#ifdef HASEXIV2
    _exifForViewer->reload();
    _exifForDialog->reload();
    _exifForViewer->setSelected( Settings::SettingsData::instance()->exifForViewer() );
    _exifForDialog->setSelected( Settings::SettingsData::instance()->exifForDialog() );
    _iptcCharset->setCurrentItem( opt->iptcCharset() );
#endif

    // Synchronization page
    _orientationRead->updatePreferred( Settings::SettingsData::instance()->orientationSyncing( false ) );
    _orientationWrite->updatePreferred( Settings::SettingsData::instance()->orientationSyncing( true ) );
    _labelRead->updatePreferred( Settings::SettingsData::instance()->labelSyncing( false ) );
    _labelWrite->updatePreferred( Settings::SettingsData::instance()->labelSyncing( true ) );
    _descriptionRead->updatePreferred( Settings::SettingsData::instance()->descriptionSyncing( false ) );
    _descriptionWrite->updatePreferred( Settings::SettingsData::instance()->descriptionSyncing( true ) );
    _dateRead->updatePreferred( Settings::SettingsData::instance()->dateSyncing( false ) );
    _dateWrite->updatePreferred( Settings::SettingsData::instance()->dateSyncing( true ) );
    for( QValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it )
        if( !(*it)->isSpecialCategory() ) {
            if ( !_catFieldsRead[ (*it)->name() ] ) {
                qDebug("No record for category \"%s\"", (*it)->name().ascii() );
                continue;
            }
            _catFieldsRead[ (*it)->name() ]->updatePreferred( Settings::SettingsData::instance()->categorySyncingFields( false, (*it)->name() ) );
            _catFieldsWrite[ (*it)->name() ]->updatePreferred( Settings::SettingsData::instance()->categorySyncingFields( true, (*it)->name() ) );
            _catSuper[ (*it)->name() ]->setCurrentItem( opt->categorySyncingSuperGroups( (*it)->name() ) );
            _catMulti[ (*it)->name() ]->setCurrentItem( opt->categorySyncingMultiValue( (*it)->name() ) );
        }

    QString backend = Settings::SettingsData::instance()->backend();
    if (backend == QString::fromLatin1("xml"))
        _backendButtons->setButton(0);
#ifdef SQLDB_SUPPORT
    else if (backend == QString::fromLatin1("sql"))
        _backendButtons->setButton(1);

    _sqlSettings->setSettings(Settings::SettingsData::instance()->getSQLParameters());
#endif

    enableDisable( false );
    KDialogBase::show();
}



// KDialogBase has a slotOK which we do not want to override.
void Settings::SettingsDialog::slotMyOK()
{
    Utilities::ShowBusyCursor dummy;
    Settings::SettingsData* opt = Settings::SettingsData::instance();

    // General
    const char* backendNames[] = { "xml", "sql" };
    int backendIndex = _backendButtons->selectedId();
    if (backendIndex < 0 || backendIndex >= 2)
        backendIndex = 0;
    opt->setBackend(QString::fromLatin1(backendNames[backendIndex]));

    opt->setPreviewSize( _previewSize->value() );
    opt->setThumbSize( _thumbnailSize->value() );
    opt->setThumbnailAspectRatio( (ThumbnailAspectRatio) _thumbnailAspectRatio->currentItem() );
    opt->setSearchForImagesOnStartup( _searchForImagesOnStartup->isChecked() );
    opt->setDontReadRawFilesWithOtherMatchingFile( _dontReadRawFilesWithOtherMatchingFile->isChecked() );
    opt->setBackupCount( _backupCount->value() );
    opt->setCompressBackup( _compressBackup->isChecked() );
    opt->setUseCompressedIndexXML( _compressedIndexXML->isChecked() );
    opt->setShowSplashScreen( _showSplashScreen->isChecked() );
    opt->setAutoSave( _autosave->value() );
    QString name = DB::ImageDB::instance()->categoryCollection()->nameForText( _albumCategory->currentText() );
    if ( name.isNull() )
        name = DB::ImageDB::instance()->categoryCollection()->categoryNames()[0];
    opt->setHistogramSize( QSize( _barWidth->value(), _barHeight->value() ) );

    opt->setAlbumCategory( name );
    opt->setThumbnailDarkBackground( _thumbnailDarkBackground->isChecked() );
    opt->setThumbnailDisplayGrid( _thumbnailDisplayGrid->isChecked() );
    opt->setThumbnailSpace( _thumbnailSpace->value() );
    opt->setDisplayLabels( _displayLabels->isChecked() );
    opt->setDisplayCategories( _displayCategories->isChecked() );
    opt->setViewerSize( _viewImageSetup->size() );
    opt->setLaunchViewerFullScreen( _viewImageSetup->launchFullScreen() );
    opt->setSlideShowInterval( _slideShowInterval->value() );
    opt->setViewerCacheSize( _cacheSize->value() );
    opt->setSmoothScale( _smoothScale->currentItem() );
    opt->setThumbnailCache( _thumbnailCache->value() );
    opt->setSlideShowSize( _slideShowSetup->size() );
    opt->setLaunchSlideShowFullScreen( _slideShowSetup->launchFullScreen() );
    opt->setAutoShowThumbnailView( _autoShowThumbnailView->value() );
    opt->setViewerStandardSize((StandardViewSize) _viewerStandardSize->currentItem());

    // ----------------------------------------------------------------------
    // Categories

    // Delete items
    for( QValueList<CategoryItem*>::Iterator it = _deleted.begin(); it != _deleted.end(); ++it ) {
        (*it)->removeFromDatabase();
    }

    // Created or Modified items
    for ( QListBoxItem* i = _categories->firstItem(); i; i = i->next() ) {
        CategoryItem* item = static_cast<CategoryItem*>( i );
        item->submit( &_memberMap );
    }

    saveOldGroup();
    DB::ImageDB::instance()->memberMap() = _memberMap;

    // misc stuff
#ifdef HASKIPI
    _pluginConfig->apply();
    opt->setDelayLoadingPlugins( _delayLoadingPlugins->isChecked() );
#endif

    // EXIF
#ifdef HASEXIV2
    opt->setExifForViewer( _exifForViewer->selected() ) ;
    opt->setExifForDialog( _exifForDialog->selected() ) ;
    opt->setIptcCharset( static_cast<Utilities::IptcCharset>(_iptcCharset->currentItem()) );
#endif

    // Synchronization
    opt->setOrientationSyncing( false, _orientationRead->items() );
    opt->setOrientationSyncing( true, _orientationWrite->items() );
    opt->setLabelSyncing( false, _labelRead->items() );
    opt->setLabelSyncing( true, _labelWrite->items() );
    opt->setDescriptionSyncing( false, _descriptionRead->items() );
    opt->setDescriptionSyncing( true, _descriptionWrite->items() );
    opt->setDateSyncing( false, _dateRead->items() );
    opt->setDateSyncing( true, _dateWrite->items() );
    for (QDictIterator<Exif::SyncWidget> it( _catFieldsRead ); it.current(); ++it ) {
        opt->setCategorySyncingFields( false, it.currentKey(), it.current()->items() );
    }
    for (QDictIterator<Exif::SyncWidget> it( _catFieldsWrite ); it.current(); ++it ) {
        opt->setCategorySyncingFields( true, it.currentKey(), it.current()->items() );
    }
    for (QDictIterator<KComboBox> it( _catSuper ); it.current(); ++it ) {
        opt->setCategorySyncingSuperGroups( it.currentKey(), static_cast<Exif::Syncable::SuperGroupHandling>( it.current()->currentItem() ) );
    }
    for (QDictIterator<KComboBox> it( _catMulti ); it.current(); ++it ) {
        opt->setCategorySyncingMultiValue( it.currentKey(), static_cast<Exif::Syncable::MultiValueHandling>( it.current()->currentItem() ) );
    }

    // SQLDB
#ifdef SQLDB_SUPPORT
    if (_sqlSettings->hasSettings())
        opt->setSQLParameters(_sqlSettings->getSettings());
#endif

    emit changed();
    kapp->config()->sync();
}


void Settings::SettingsDialog::edit( QListBoxItem* i )
{
    if ( i == 0 )
        return;

    CategoryItem* item = static_cast<CategoryItem*>(i);
    _current = item;
    _text->setText( item->text() );
    _icon->setIcon( item->icon() );
    _thumbnailSizeInCategory->setValue( item->thumbnailSize() );
    _preferredView->setCurrentItem( static_cast<int>(item->viewType()) );
    enableDisable( true );
}

void Settings::SettingsDialog::slotLabelChanged( const QString& label)
{
    if( _current ) {
        if ( _currentCategory == _current->text() )
            _currentCategory = label;
        _current->setLabel( label );
    }
}

void Settings::SettingsDialog::slotPreferredViewChanged( int i )
{
    if ( _current ) {
        _current->setViewType( static_cast<DB::Category::ViewType>(i) );
    }
}

void Settings::SettingsDialog::thumbnailSizeChanged( int size )
{
    if ( _current )
        _current->setThumbnailSize( size );
}



void Settings::SettingsDialog::slotIconChanged( QString icon )
{
    if( _current )
        _current->setIcon( icon );
}

void Settings::SettingsDialog::slotNewItem()
{
    bool ok;
    QString name = KInputDialog::getText( i18n("New category"), i18n("Name"), QString::null, &ok );
    if ( !ok )
        return;

    _current = new CategoryItem( QString::null, name, QString::null, DB::Category::ListView, 64, _categories );
    connect( _current, SIGNAL( categoryRenamed( const QString&, const QString& ) ), this, SLOT( slotCategoryRenamed( const QString&, const QString& ) ) );
    connect( _current, SIGNAL( categoryAdded( const QString& ) ), this, SLOT( slotCategoryAdded( const QString& ) ) );
    connect( _current, SIGNAL( categoryRemoved( const QString& ) ), this, SLOT( slotCategoryRemoved( const QString& ) ) );

    _text->setText( name );
    _icon->setIcon( QString::null );
    _thumbnailSizeInCategory->setValue( 64 );
    enableDisable( true );
    _categories->setSelected( _current, true );
    _text->setFocus();
}

void Settings::SettingsDialog::slotDeleteCurrent()
{
    int answer = KMessageBox::questionYesNo( this, i18n("<p>Really delete category '%1'?</p>").arg( _current->text()) );
    if ( answer == KMessageBox::No )
        return;

    //slotCategoryRemoved( _current->text() );
    _deleted.append( _current );
    _categories->takeItem( _current );
    _current = 0;
    _text->setText( QString::fromLatin1( "" ) );
    _icon->setIcon( QString::null );
    _thumbnailSizeInCategory->setValue(64);
    enableDisable(false);
}

void Settings::SettingsDialog::enableDisable( bool b )
{
    _delItem->setEnabled( b );
    _labelLabel->setEnabled( b );
    _text->setEnabled( b );
    _icon->setEnabled( b );
    _iconLabel->setEnabled( b );
    _thumbnailSizeInCategoryLabel->setEnabled( b );
    _thumbnailSizeInCategory->setEnabled( b );
    _preferredViewLabel->setEnabled( b );
    _preferredView->setEnabled( b );
}

void Settings::SettingsDialog::createGroupConfig()
{
    QWidget* top = addPage( i18n("Subcategories" ), i18n("Subcategories" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "editcopy" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    // Category
    QHBoxLayout* lay2 = new QHBoxLayout( lay1, 6 );
    QLabel* label = new QLabel( i18n( "Category:" ), top );
    lay2->addWidget( label );
    _category = new QComboBox( top );
    lay2->addWidget( _category );
    lay2->addStretch(1);

    QHBoxLayout* lay3 = new QHBoxLayout( lay1, 6 );

    // Groups
    QVBoxLayout* lay4 = new QVBoxLayout( lay3, 6 );
    label = new QLabel( i18n( "Super Categories:" ), top );
    lay4->addWidget( label );
    _groups = new QListBox( top );
    lay4->addWidget( _groups );

    // Members
    QVBoxLayout* lay5 = new QVBoxLayout( lay3, 6 );
    label = new QLabel( i18n( "Items of Category:" ), top );
    lay5->addWidget( label );
    _members = new QListBox( top );
    lay5->addWidget( _members );

    // Buttons
    QHBoxLayout* lay6 = new QHBoxLayout( lay1, 6 );
    lay6->addStretch(1);

    QPushButton* add = new QPushButton( i18n("Add Super Category..." ), top );
    lay6->addWidget( add );
    _rename = new QPushButton( i18n( "Rename Super Category..."), top );
    lay6->addWidget( _rename );
    _del = new QPushButton( i18n("Delete Super Category" ), top );
    lay6->addWidget( _del );

    // Notice
    QLabel* notice = new QLabel( i18n("<b>Notice:</b> It is also possible to set up subcategories in the annotation dialog, simply by dragging items." ), top );
    lay1->addWidget( notice );

    // Setup the actions
    _memberMap = DB::ImageDB::instance()->memberMap();
    connect( DB::ImageDB::instance()->categoryCollection(),
             SIGNAL( itemRemoved( DB::Category*, const QString& ) ),
             &_memberMap, SLOT( deleteItem( DB::Category*, const QString& ) ) );
    connect( DB::ImageDB::instance()->categoryCollection(),
             SIGNAL( itemRenamed( DB::Category*, const QString&, const QString& ) ),
             &_memberMap, SLOT( renameItem( DB::Category*, const QString&, const QString& ) ) );
    connect( _category, SIGNAL( activated( const QString& ) ), this, SLOT( slotCategoryChanged( const QString& ) ) );
    connect( _groups, SIGNAL( currentChanged( QListBoxItem* ) ), this, SLOT( slotGroupSelected( QListBoxItem* ) ) );
    connect( _rename, SIGNAL( clicked() ), this, SLOT( slotRenameGroup() ) );
    connect( add, SIGNAL( clicked() ), this, SLOT( slotAddGroup() ) );
    connect( _del, SIGNAL( clicked() ), this, SLOT( slotDelGroup() ) );

    _members->setSelectionMode( QListBox::Multi );
}

/**
   When the user selects a new category from the combo box then this method is called
   Its purpose is too fill the groups and members listboxes.
*/
void Settings::SettingsDialog::slotCategoryChanged( const QString& text )
{
    slotCategoryChanged( DB::ImageDB::instance()->categoryCollection()->nameForText(text), true );
}

void Settings::SettingsDialog::slotCategoryChanged( const QString& name, bool saveGroups )
{
    if ( saveGroups ) {
        // We do not want to save groups when renaming categories
        saveOldGroup();
    }

    _groups->blockSignals(true);
    _groups->clear();
    _groups->blockSignals(false);

    _currentCategory = name;
    if (name.isNull())
        return;
    QStringList groupList = _memberMap.groups( name );

    _groups->blockSignals(true);
    _groups->insertStringList( groupList );
    _groups->blockSignals(false);

    _members->clear();
    QStringList list = DB::ImageDB::instance()->categoryCollection()->categoryForName(name)->items();
    list += _memberMap.groups( name );
    QStringList uniq;
    for( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if ( !uniq.contains(*it) )
            uniq << *it;
    }

    uniq.sort();
    _members->insertStringList( uniq );

    _currentGroup = QString::null;

    _members->clearSelection();
    _members->setEnabled(false);

    setButtonStates();
}

void Settings::SettingsDialog::slotGroupSelected( QListBoxItem* item )
{
    saveOldGroup();
    if ( item )
        selectMembers( item->text() );
}

void Settings::SettingsDialog::slotAddGroup()
{
    bool ok;
    QString text = KInputDialog::getText( i18n( "New Group" ), i18n("Group name:"), QString::null, &ok );
    if ( ok ) {
        saveOldGroup();
        DB::ImageDB::instance()->categoryCollection()->categoryForName( _currentCategory )->addItem( text );
        _memberMap.addGroup(_currentCategory, text);
        slotCategoryChanged( _currentCategory, false );
        QListBoxItem* item = _groups->findItem(text, Qt::ExactMatch);
        _groups->setCurrentItem( item ); // also emits currentChanged()
        // selectMembers() is called automatically by slotGroupSelected()
    }
}

void Settings::SettingsDialog::slotRenameGroup()
{
    Q_ASSERT( !_currentGroup.isNull() );
    bool ok;
    QString text = KInputDialog::getText( i18n( "New Group" ), i18n("Group name:"), _currentGroup, &ok );
    if ( ok ) {
        saveOldGroup();
        _memberMap.renameGroup( _currentCategory, _currentGroup, text );
        DB::ImageDB::instance()->categoryCollection()->categoryForName( _currentCategory )->renameItem( _currentGroup, text );
        slotCategoryChanged( _currentCategory, false );
        QListBoxItem* item = _groups->findItem(text, Qt::ExactMatch);
        _groups->setCurrentItem( item );
    }
}

void Settings::SettingsDialog::slotDelGroup()
{
    Q_ASSERT( !_currentGroup.isNull() );
    int res = KMessageBox::warningContinueCancel( this, i18n( "Really delete group %1?" ).arg( _currentGroup ),i18n("Delete Group"),KGuiItem(i18n("&Delete"),QString::fromLatin1("editdelete")) );
    if ( res == KMessageBox::Cancel )
        return;

    _memberMap.deleteGroup( _currentCategory, _currentGroup );
    DB::ImageDB::instance()->categoryCollection()->categoryForName( _currentCategory )->removeItem( _currentGroup );
    _currentGroup = QString::null;
    slotCategoryChanged( _currentCategory, false );
}

void Settings::SettingsDialog::saveOldGroup()
{
    if ( _currentCategory.isNull() || _currentGroup.isNull() )
        return;

    QStringList list;
    for( QListBoxItem* item = _members->firstItem(); item; item = item->next() ) {
        if ( item->isSelected() )
            list << item->text();
    }

    _memberMap.setMembers(_currentCategory, _currentGroup, list);
}

void Settings::SettingsDialog::selectMembers( const QString& group )
{
    _currentGroup = group;

    QStringList list = _memberMap.members(_currentCategory,group, false );

    for( QListBoxItem* item = _members->firstItem(); item; item = item->next() ) {
        if (!_memberMap.canAddMemberToGroup(_currentCategory, group, item->text())) {
            _members->setSelected(item, false);
            item->setSelectable(false);
        }
        else {
            item->setSelectable(true);
            _members->setSelected(item, list.contains(item->text()));
        }
    }

    _members->setEnabled(true);

    setButtonStates();
}


int Settings::SettingsDialog::exec()
{
    slotCategoryChanged( _currentCategory, false );
    return KDialogBase::exec();
}

void Settings::SettingsDialog::setButtonStates()
{
    bool b = !_currentGroup.isNull();
    _rename->setEnabled( b );
    _del->setEnabled( b );
}


void Settings::SettingsDialog::slotPageChange()
{
    _category->clear();
    QValueList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    for( QValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it ) {
        if ( !(*it)->isSpecialCategory() )
            _category->insertItem( (*it)->text() );
    }

    slotCategoryChanged( _category->currentText() );
}






void Settings::SettingsDialog::createViewerPage()
{
    QWidget* top = addPage( i18n("Viewer" ), i18n("Viewer" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "viewmag" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    _slideShowSetup = new ViewerSizeConfig( i18n( "Running Slide Show From Thumbnail View" ), top, "_slideShowSetup" );
    lay1->addWidget( _slideShowSetup );

    _viewImageSetup = new ViewerSizeConfig( i18n( "Viewing Images and Videos From Thumbnail View" ), top, "_viewImageSetup" );
    lay1->addWidget( _viewImageSetup );

    QGridLayout* glay = new QGridLayout( lay1, 2, 2, 6 );

    QLabel* label = new QLabel( i18n("Slideshow interval:" ), top );
    glay->addWidget( label, 0, 0 );

    _slideShowInterval = new QSpinBox( 1, INT_MAX, 1, top );
    glay->addWidget( _slideShowInterval, 0, 1 );
    _slideShowInterval->setSuffix( i18n( " sec" ) );
    label->setBuddy( _slideShowInterval );

    label = new QLabel( i18n("Image cache:"), top );
    glay->addWidget( label, 1, 0 );

    _cacheSize = new QSpinBox( 0, 2000, 10, top, "_cacheSize" );
    _cacheSize->setSuffix( i18n(" Mbytes") );
    glay->addWidget( _cacheSize, 1, 1 );
    label->setBuddy( _cacheSize );

    QString txt;

    QLabel* standardSizeLabel = new QLabel( i18n("Standard size in viewer:"), top );
    _viewerStandardSize = new KComboBox( top );
    _viewerStandardSize->insertStringList( QStringList() << i18n("Full Viewer Size") << i18n("Natural Image Size") << i18n("Natural Image Size If Possible") );
    glay->addWidget( standardSizeLabel, 2, 0);
    glay->addWidget( _viewerStandardSize, 2, 1 );
    standardSizeLabel->setBuddy( _viewerStandardSize );

    txt = i18n("<p>Set the standard size for images to be displayed in the viewer.</p> "
	       "<p><b>Full Viewer Size</b> indicates that the image will be stretched or shrunk to fill the viewer window.</p> "
	       "<p><b>Natural Image Size</b> indicates that the image will be displayed pixel for pixel.</p> "
	       "<p><b>Natural Image Size If Possible</b> indicates that the image will be displayed pixel for pixel if it would fit the window, "
	       "otherwise it will be shrunk to fit the viewer.</p>");
    QWhatsThis::add(_viewerStandardSize, txt);

    QLabel* scalingLabel = new QLabel( i18n("Scaling Algorithm"), top );
    _smoothScale = new QComboBox( top );
    _smoothScale->insertStringList( QStringList() << i18n("Fastest" ) << i18n("Best")  );
    scalingLabel->setBuddy( _smoothScale );

    glay->addWidget( scalingLabel, 3, 0 );
    glay->addWidget( _smoothScale, 3, 1 );
    txt = i18n("<p>When displaying images, KPhotoAlbum normally performs smooth scaling of the image. "
		       "If this option is not set, KPhotoAlbum will use a faster but less smooth scaling method.</p>");
    QWhatsThis::add( scalingLabel, txt );
    QWhatsThis::add( _smoothScale, txt );
}


void Settings::SettingsDialog::createPluginPage()
{
#ifdef HASKIPI
    ::MainWindow::Window::theMainWindow()->loadPlugins();
    QWidget* top = addPage( i18n("Plugins" ), i18n("Plugins" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "share" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    QLabel* label = new QLabel( i18n("Choose Plugins to load:"), top );
    lay1->addWidget( label );

    _pluginConfig = KIPI::PluginLoader::instance()->configWidget( top );
    lay1->addWidget( _pluginConfig );

    _delayLoadingPlugins = new QCheckBox( i18n("Delay loading plug-ins till plug-in menu is opened"), top );
    lay1->addWidget( _delayLoadingPlugins );
#endif
}

void Settings::SettingsDialog::createEXIFPage()
{
#ifdef HASEXIV2
    QWidget* top = addPage( i18n("EXIF View" ), i18n("EXIF/IPTC Information" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "contents" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* vlay = new QVBoxLayout( top );
    QHBoxLayout* hlay1 = new QHBoxLayout( vlay );
    QHBoxLayout* hlay2 = new QHBoxLayout( vlay );
    hlay1->setSpacing( 6 );
    vlay->setSpacing( 6 );
    hlay2->setSpacing( 6 );

    _exifForViewer = new Exif::TreeView( i18n( "EXIF/IPTC info to show in the Viewer" ), top );
    hlay1->addWidget( _exifForViewer );

    _exifForDialog = new Exif::TreeView( i18n("EXIF/IPTC info to show in the EXIF dialog"), top );
    hlay1->addWidget( _exifForDialog );

    QLabel* _iptcCharsetLabel = new QLabel( i18n("Character set for IPTC data:"), top, "iptcCharsetLabel" );
    _iptcCharset = new KComboBox( top );
    _iptcCharset->insertStringList( QStringList() << i18n("UTF-8") << i18n("Local 8-bit") << i18n("ISO 8859-2") << i18n("CP 1250") );
    QWhatsThis::add( _iptcCharset, i18n("<p>Which character set to use for reading/writing of IPTC data</p>") );

    hlay2->addStretch( 1 );
    hlay2->addWidget( _iptcCharsetLabel );
    hlay2->addWidget( _iptcCharset );

#endif
}

void Settings::SettingsDialog::createSyncPage()
{
    _syncPage = addPage( i18n("Synchronization" ), i18n("Metadata Synchronization" ),
                        KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "saveas" ),
                                                         KIcon::Desktop, 32 ) );

    QVBoxLayout* _lay = new QVBoxLayout( _syncPage );
    _syncTabs = new QTabWidget( _syncPage );
    _syncTabs->setMargin( 6 );
    QHBox* hbox;
    QValueList< Exif::Syncable::Kind > rValues, wValues;

    hbox = new QHBox( _syncPage );
    hbox->setSpacing( 6 );
    rValues << Exif::Syncable::EXIF_ORIENTATION << Exif::Syncable::STOP;
    wValues << Exif::Syncable::EXIF_ORIENTATION << Exif::Syncable::STOP;
    _orientationRead = new Exif::SyncWidget( i18n("Fields to get value from"), hbox, rValues );
    _orientationWrite = new Exif::SyncWidget( i18n("Fields to write value to"), hbox, wValues );
    _syncTabs->addTab( hbox, i18n("Image orientation") );

    hbox = new QHBox( _syncPage );
    hbox->setSpacing( 6 );
    rValues.clear(); wValues.clear();
    rValues << Exif::Syncable::IPTC_HEADLINE <<
            Exif::Syncable::EXIF_USER_COMMENT << Exif::Syncable::EXIF_DESCRIPTION <<
            Exif::Syncable::JPEG_COMMENT << Exif::Syncable::EXIF_XPTITLE <<
            Exif::Syncable::EXIF_XPSUBJECT << Exif::Syncable::IPTC_OBJECT_NAME << 
            Exif::Syncable::FILE_NAME << Exif::Syncable::STOP << Exif::Syncable::IPTC_CAPTION;
    wValues << Exif::Syncable::IPTC_HEADLINE <<
            Exif::Syncable::STOP << Exif::Syncable::EXIF_USER_COMMENT <<
            Exif::Syncable::EXIF_DESCRIPTION << Exif::Syncable::JPEG_COMMENT <<
            Exif::Syncable::EXIF_XPTITLE << Exif::Syncable::EXIF_XPSUBJECT <<
            Exif::Syncable::IPTC_CAPTION;
    _labelRead = new Exif::SyncWidget( i18n("Fields to get value from"), hbox, rValues );
    _labelWrite = new Exif::SyncWidget( i18n("Fields to write value to"), hbox, wValues );
    _syncTabs->addTab( hbox, i18n("Label") );

    hbox = new QHBox( _syncPage );
    hbox->setSpacing( 6 );
    rValues.clear(); wValues.clear();
    rValues << Exif::Syncable::IPTC_CAPTION <<
            Exif::Syncable::EXIF_USER_COMMENT << Exif::Syncable::EXIF_DESCRIPTION <<
            Exif::Syncable::JPEG_COMMENT << Exif::Syncable::EXIF_XPCOMMENT <<
            Exif::Syncable::EXIF_XPSUBJECT << Exif::Syncable::IPTC_OBJECT_NAME << 
            Exif::Syncable::STOP << Exif::Syncable::IPTC_HEADLINE;
    wValues << Exif::Syncable::IPTC_CAPTION <<
            Exif::Syncable::STOP << Exif::Syncable::EXIF_USER_COMMENT <<
            Exif::Syncable::EXIF_DESCRIPTION << Exif::Syncable::JPEG_COMMENT <<
            Exif::Syncable::EXIF_XPCOMMENT << Exif::Syncable::EXIF_XPSUBJECT <<
            Exif::Syncable::IPTC_HEADLINE;
    _descriptionRead = new Exif::SyncWidget( i18n("Fields to get value from"), hbox, rValues );
    _descriptionWrite = new Exif::SyncWidget( i18n("Fields to write value to"), hbox, wValues );
    _syncTabs->addTab( hbox, i18n("Description") );

    hbox = new QHBox( _syncPage );
    hbox->setSpacing( 6 );
    rValues.clear(); wValues.clear();
    rValues << Exif::Syncable::EXIF_DATETIME << Exif::Syncable::EXIF_DATETIME_ORIGINAL <<
        Exif::Syncable::EXIF_DATETIME_DIGITIZED << Exif::Syncable::FILE_MTIME <<
        Exif::Syncable::FILE_CTIME << Exif::Syncable::STOP;
    wValues << Exif::Syncable::EXIF_DATETIME << Exif::Syncable::STOP <<
        Exif::Syncable::FILE_MTIME << Exif::Syncable::EXIF_DATETIME_ORIGINAL <<
        Exif::Syncable::EXIF_DATETIME_DIGITIZED << Exif::Syncable::FILE_CTIME;

    _dateRead = new Exif::SyncWidget( i18n("Fields to get value from"), hbox, rValues );
    _dateWrite = new Exif::SyncWidget( i18n("Fields to write value to"), hbox, wValues );
    _syncTabs->addTab( hbox, i18n("Date") );

    QValueList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    for( QValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it )
        if ( !(*it)->isSpecialCategory() )
            slotCategoryAdded( (*it)->name() );

    _lay->addWidget( _syncTabs );
}

void Settings::SettingsDialog::slotCategoryAdded( const QString& name )
{
    // we need at least one radnom category for that vocabulary thingy...
    DB::CategoryPtr someCategory = *( DB::ImageDB::instance()->categoryCollection()->categories().begin() );
    for (int i = 0; _syncTabs->label( i ) != QString::null; ++i )
        if ( _syncTabs->label( i ).remove( '&' ) == name ) {
            qDebug("Attempted to add category \"%s\" that already exists, skipping...", name.ascii());
            return;
        }

    QValueList< Exif::Syncable::Kind > rValues, wValues;
    QGrid* box = new QGrid( 2, Horizontal, _syncPage );
    box->setSpacing( 6 );
    rValues.clear(); wValues.clear();
    if ( ( name == QString::fromLatin1("Keywords") ) ||
            ( someCategory->standardCategories()[ QString::fromLatin1("Keywords") ] == name ) ) {
        rValues << Exif::Syncable::IPTC_KEYWORDS << Exif::Syncable::EXIF_XPKEYWORDS <<
            Exif::Syncable::STOP << Exif::Syncable::IPTC_SUPP_CAT;
        wValues << Exif::Syncable::IPTC_KEYWORDS << Exif::Syncable::STOP << 
            Exif::Syncable::EXIF_XPKEYWORDS << Exif::Syncable::IPTC_SUPP_CAT;
    } else if ( ( name == QString::fromLatin1("Places") ) ||
            ( someCategory->standardCategories()[ QString::fromLatin1("Places") ] == name ) ) {
        rValues << Exif::Syncable::IPTC_LOCATION_CODE << Exif::Syncable::IPTC_LOCATION_NAME <<
            Exif::Syncable::IPTC_CITY << Exif::Syncable::IPTC_SUB_LOCATION <<
            Exif::Syncable::IPTC_PROVINCE_STATE << Exif::Syncable::IPTC_COUNTRY_NAME <<
            Exif::Syncable::IPTC_COUNTRY_CODE << Exif::Syncable::STOP << Exif::Syncable::IPTC_SUPP_CAT;
        wValues << Exif::Syncable::IPTC_LOCATION_NAME << Exif::Syncable::STOP <<
            Exif::Syncable::IPTC_LOCATION_CODE << Exif::Syncable::IPTC_CITY << Exif::Syncable::IPTC_SUB_LOCATION <<
            Exif::Syncable::IPTC_PROVINCE_STATE << Exif::Syncable::IPTC_COUNTRY_NAME <<
            Exif::Syncable::IPTC_COUNTRY_CODE << Exif::Syncable::IPTC_SUPP_CAT;
    } else {
        rValues << Exif::Syncable::STOP << Exif::Syncable::EXIF_DESCRIPTION <<
            Exif::Syncable::EXIF_USER_COMMENT << Exif::Syncable::EXIF_XPTITLE <<
            Exif::Syncable::EXIF_XPCOMMENT << Exif::Syncable::EXIF_XPKEYWORDS <<
            Exif::Syncable::EXIF_XPSUBJECT << Exif::Syncable::IPTC_HEADLINE <<
            Exif::Syncable::IPTC_CAPTION << Exif::Syncable::IPTC_OBJECT_NAME <<
            Exif::Syncable::IPTC_SUBJECT << Exif::Syncable::IPTC_SUPP_CAT <<
            Exif::Syncable::IPTC_KEYWORDS << Exif::Syncable::IPTC_LOCATION_CODE <<
            Exif::Syncable::IPTC_LOCATION_NAME << Exif::Syncable::IPTC_CITY <<
            Exif::Syncable::IPTC_SUB_LOCATION << Exif::Syncable::IPTC_PROVINCE_STATE <<
            Exif::Syncable::IPTC_COUNTRY_CODE << Exif::Syncable::IPTC_COUNTRY_NAME;
        wValues = rValues;
    }
    _catFieldsRead.replace( name, new Exif::SyncWidget( i18n("Fields to get value from"), box, rValues ) );
    _catFieldsWrite.replace( name, new Exif::SyncWidget( i18n("Fields to write value to"), box, wValues ) );
    QLabel* lbl = new QLabel( i18n("Supercategories"), box );
    lbl->setAlignment( AlignRight | AlignVCenter );
    _catSuper.replace( name, new KComboBox( box ) );
    _catSuper[ name ]->insertStringList( QStringList() <<
            i18n("New tag for each level, one level per tag") <<
            i18n("Multiple fields, slash separated values") << i18n("Multiple fields, comma separated values") <<
            i18n("One field, slash separated") << i18n("One field, comma separated") );
    lbl = new QLabel( i18n("Multiple values"), box );
    lbl->setAlignment( AlignRight | AlignVCenter );
    _catMulti.replace( name, new KComboBox( box ) );
    _catMulti[ name ]->insertStringList( QStringList() <<
            i18n("Repeat field") << i18n("Comma separated values") << i18n("Slash separated values") );
    _syncTabs->addTab( box, name );
}

void Settings::SettingsDialog::slotCategoryRemoved( const QString& name )
{
    for (int i = 0; _syncTabs->page( i ); ++i )
        if ( _syncTabs->label( i ).remove( '&' ) == name ) {
            _catFieldsRead.remove( name );
            _catFieldsWrite.remove( name );
            _catMulti.remove( name );
            _catSuper.remove( name );
            QWidget* tab = _syncTabs->page( i );
            _syncTabs->removePage( tab );
            delete tab;
            return;
        }
}

void Settings::SettingsDialog::slotCategoryRenamed( const QString& oldName, const QString& newName )
{
    slotCategoryRemoved( oldName );
    slotCategoryAdded( newName );
}

void Settings::SettingsDialog::showBackendPage()
{
    showPage(_backendPageIndex);
}

void Settings::SettingsDialog::createDatabaseBackendPage()
{
    // TODO: add notification: New backend will take effect only after restart
    QWidget* top = addPage(i18n("Database backend"),
                           i18n("Database backend"),
                           KGlobal::iconLoader()->loadIcon(QString::fromLatin1("kfm"),
                                                           KIcon::Desktop, 32));
    _backendPageIndex = pageIndex(top);

    QVBoxLayout* lay1 = new QVBoxLayout(top, 6);

    _backendButtons = new QButtonGroup(1, Qt::Horizontal,
                                       i18n("Database backend to use"), top);
    lay1->addWidget(_backendButtons);

    new QRadioButton(i18n("XML backend (recommended)"), _backendButtons);
#ifdef SQLDB_SUPPORT
    //QRadioButton* sqlButton =
    new QRadioButton(i18n("SQL backend (experimental)"), _backendButtons);
#endif


    // XML Backend
    QVGroupBox* xmlBox = new QVGroupBox( i18n("XML Database Setting"), top );
    lay1->addWidget( xmlBox );

    // Compressed index.xml
    _compressedIndexXML = new QCheckBox( i18n("Choose speed over readability for index.xml file"), xmlBox );
    _compressBackup = new QCheckBox( i18n( "Compress backup file" ), xmlBox );

    // Auto save
    QWidget* box = new QWidget( xmlBox );
    QLabel* label = new QLabel( i18n("Auto save every:"), box );
    _autosave = new QSpinBox( 1, 120, 1, box );
    _autosave->setSuffix( i18n( "min." ) );

    QHBoxLayout* lay = new QHBoxLayout( box, 6 );
    lay->addWidget( label );
    lay->addWidget( _autosave );
    lay->addStretch( 1 );

    // Backup
    box = new QWidget( xmlBox );
    lay = new QHBoxLayout( box, 6 );
    QLabel* backupLabel = new QLabel( i18n( "Number of backups to keep:" ), box );
    lay->addWidget( backupLabel );

    _backupCount = new QSpinBox( -1, 100, 1, box );
    _backupCount->setSpecialValueText( i18n( "Infinite" ) );
    lay->addWidget( _backupCount );
    lay->addStretch( 1 );

    QString txt;
    txt = i18n("<p>KPhotoAlbum is capable of backing up the index.xml file by keeping copies named index.xml~1~ index.xml~2~ etc. "
               "and you can use the spinbox to specify the number of backup files to keep. "
               "KPhotoAlbum will delete the oldest backup file when it reaches "
               "the maximum number of backup files.</p>"
               "<p>The index.xml file may grow substantially if you have many images, and in that case it is useful to ask KPhotoAlbum to zip "
               "the backup files to preserve disk space.</p>" );
    QWhatsThis::add( backupLabel, txt );
    QWhatsThis::add( _backupCount, txt );
    QWhatsThis::add( _compressBackup, txt );

    txt = i18n( "<p>KPhotoAlbum is using a single index.xml file as its <i>data base</i>. With lots of images it may take "
                "a long time to read this file. You may cut down this time to approximately half, by checking this check box. "
                "The disadvantage is that the index.xml file is less readable by human eyes.</p>");
    QWhatsThis::add( _compressedIndexXML, txt );



    // SQL Backend
#ifdef SQLDB_SUPPORT
    QVGroupBox* sqlBox = new QVGroupBox(i18n("SQL Database Settings"), top);
    //sqlBox->setEnabled(false);
    lay1->addWidget(sqlBox);

    _sqlSettings = new SQLDB::SQLSettingsWidget(sqlBox);

    QLabel* passwordWarning =
        new QLabel(i18n("Warning! The password is saved as plain text to the configuration file."), top);
    passwordWarning->hide();
    lay1->addWidget(passwordWarning);

    QSpacerItem* spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
    lay1->addItem(spacer);

    //connect(sqlButton, SIGNAL(toggled(bool)), sqlBox, SLOT(setEnabled(bool)));
    connect(_sqlSettings, SIGNAL(passwordChanged(const QString&)), passwordWarning, SLOT(show()));
#endif /* SQLDB_SUPPORT */
}
