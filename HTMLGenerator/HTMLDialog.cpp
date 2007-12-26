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

#include "HTMLDialog.h"
#include <klocale.h>
#include <qlayout.h>
#include <klineedit.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <kfiledialog.h>
#include <qpushbutton.h>
#include "Settings/SettingsData.h"
#include <q3hgroupbox.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <q3textedit.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kiconloader.h>
#include "MainWindow/Window.h"
#include "DB/CategoryCollection.h"
#include "DB/ImageDB.h"
#include "Generator.h"
#include "ImageSizeCheckBox.h"
#include <QTextEdit>
using namespace HTMLGenerator;


HTMLDialog::HTMLDialog( QWidget* parent )
    :KPageDialog( parent )
{
    setWindowTitle( i18n("HTML Export") );
    setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Help );
    enableButtonOk( false );
    createContentPage();
    createLayoutPage();
    createDestinationPage();
    setHelp( QString::fromLatin1( "chp-generating-html" ) );
    connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
}

void HTMLDialog::createContentPage()
{
    QWidget* contentPage = new QWidget;
    KPageWidgetItem* page = new KPageWidgetItem( contentPage, i18n("Content" ) );
    page->setHeader( i18n("Content" ) );
    page->setIcon( KIcon( QString::fromLatin1( "document-properties" ) ) );
    addPage( page );

    QVBoxLayout* lay1 = new QVBoxLayout( contentPage );
    QGridLayout* lay2 = new QGridLayout;
    lay1->addLayout( lay2 );

    QLabel* label = new QLabel( i18n("Page title:"), contentPage );
    lay2->addWidget( label, 0, 0 );
    _title = new KLineEdit( contentPage );
    label->setBuddy( _title );
    lay2->addWidget( _title, 0, 1 );

    // Description
    label = new QLabel( i18n("Description:"), contentPage );
    label->setAlignment( Qt::AlignTop );
    lay2->addWidget( label, 1, 0 );
    _description = new QTextEdit( contentPage );
    label->setBuddy( _description );
    lay2->addWidget( _description, 1, 1 );

    _generateKimFile = new QCheckBox( i18n("Create .kim export file"), contentPage );
    _generateKimFile->setChecked( true );
    lay1->addWidget( _generateKimFile );

    _inlineMovies = new QCheckBox( i18n( "Inline Movies in pages" ), contentPage );
    _inlineMovies->setChecked( true );
    lay1->addWidget( _inlineMovies );

    // What to include
    QGroupBox* whatToInclude = new QGroupBox( i18n( "What to Include" ), contentPage );
    lay1->addWidget( whatToInclude );
    QGridLayout* lay3 = new QGridLayout( whatToInclude );

    QCheckBox* cb = new QCheckBox( i18n("Description"), whatToInclude );
    _whatToIncludeMap.insert( QString::fromLatin1("**DESCRIPTION**"), cb );
    lay3->addWidget( cb, 0, 0 );

    int row=0;
    int col=0;

    Q3ValueList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    for( Q3ValueList<DB::CategoryPtr>::Iterator it = categories.begin(); it != categories.end(); ++it ) {
        if ( ! (*it)->isSpecialCategory() ) {
            if ( ++col % 2 == 0 )
                ++row;
            QCheckBox* cb = new QCheckBox( (*it)->text(), whatToInclude );
            lay3->addWidget( cb, row, col%2 );
            _whatToIncludeMap.insert( (*it)->name(), cb );
        }
    }
}

void HTMLDialog::createLayoutPage()
{
    QWidget* layoutPage = new QWidget;
    KPageWidgetItem* page = new KPageWidgetItem( layoutPage, i18n("Layout" ) );
    page->setHeader( i18n("Layout" ) );
    page->setIcon( KIcon( QString::fromLatin1( "matrix" )) );
    addPage(page);

    QVBoxLayout* lay1 = new QVBoxLayout( layoutPage );
    QGridLayout* lay2 = new QGridLayout;
    lay1->addLayout( lay2 );

    // Thumbnail size
    QLabel* label = new QLabel( i18n("Thumbnail size:"), layoutPage );
    lay2->addWidget( label, 0, 0 );

    QHBoxLayout* lay3 = new QHBoxLayout;
    lay2->addLayout( lay3, 0, 1 );

    _thumbSize = new QSpinBox;
    _thumbSize->setRange( 16, 256 );

    _thumbSize->setValue( 128 );
    lay3->addWidget( _thumbSize );
    lay3->addStretch(1);
    label->setBuddy( _thumbSize );

    // Number of columns
    label = new QLabel( i18n("Number of columns:"), layoutPage );
    lay2->addWidget( label, 1, 0 );

    QHBoxLayout* lay4 = new QHBoxLayout;
    lay2->addLayout( lay4, 1, 1 );
    _numOfCols = new QSpinBox;
    _numOfCols->setRange( 1, 10 );

    label->setBuddy( _numOfCols);

    _numOfCols->setValue( 5 );
    lay4->addWidget( _numOfCols );
    lay4->addStretch( 1 );

    // Theme box
    label = new QLabel( i18n("Theme:"), layoutPage );
    lay2->addWidget( label, 2, 0 );
    lay4 = new QHBoxLayout;
    lay2->addLayout( lay4, 2, 1 );
    _themeBox = new QComboBox( layoutPage );
    label->setBuddy( _themeBox );
    lay4->addWidget( _themeBox );
    lay4->addStretch( 1 );
    populateThemesCombo();

    // Image sizes
    Q3HGroupBox* sizes = new Q3HGroupBox( i18n("Image Sizes"), layoutPage );
    lay1->addWidget( sizes );
    QWidget* content = new QWidget( sizes );
    QGridLayout* lay5 = new QGridLayout( content );
    ImageSizeCheckBox* size320  = new ImageSizeCheckBox( 320, 200, content );
    ImageSizeCheckBox* size640  = new ImageSizeCheckBox( 640, 480, content );
    ImageSizeCheckBox* size800  = new ImageSizeCheckBox( 800, 600, content );
    ImageSizeCheckBox* size1024 = new ImageSizeCheckBox( 1024, 768, content );
    ImageSizeCheckBox* size1280 = new ImageSizeCheckBox( 1280, 1024, content );
    ImageSizeCheckBox* size1600 = new ImageSizeCheckBox( 1600, 1200, content );
    ImageSizeCheckBox* sizeOrig = new ImageSizeCheckBox( i18n("Full size"), content );

    {
        int row = 0;
        int col = -1;
        lay5->addWidget( size320, row, ++col );
        lay5->addWidget( size640, row, ++col );
        lay5->addWidget( size800, row, ++col );
        lay5->addWidget( size1024, row, ++col );
        col =-1;
        lay5->addWidget( size1280, ++row, ++col );
        lay5->addWidget( size1600, row, ++col );
        lay5->addWidget( sizeOrig, row, ++col );
    }

    size800->setChecked( 1 );

    _cbs << size800 << size1024 << size1280 << size640 << size1600 << size320 << sizeOrig;

    lay1->addStretch(1);
}

void HTMLDialog::createDestinationPage()
{
    QWidget* destinationPage = new QWidget;

    KPageWidgetItem* page = new KPageWidgetItem( destinationPage, i18n("Destination" ) );
    page->setHeader( i18n("Destination" ) );
    page->setIcon( KIcon( QString::fromLatin1( "drive-harddisk" ) ) );
    addPage( page );

    QVBoxLayout* lay1 = new QVBoxLayout( destinationPage );
    QGridLayout* lay2 = new QGridLayout;
    lay1->addLayout( lay2 );

    // Base Directory
    QLabel* label = new QLabel( i18n("Base directory:"), destinationPage );
    lay2->addWidget( label, 0, 0 );

    QHBoxLayout* lay3 = new QHBoxLayout;
    lay2->addLayout( lay3, 0, 1 );

    _baseDir = new KLineEdit( destinationPage );
    lay3->addWidget( _baseDir );
    label->setBuddy( _baseDir );

    QPushButton* but = new QPushButton( QString::fromLatin1( ".." ), destinationPage );
    lay3->addWidget( but );
    but->setFixedWidth( 25 );

    connect( but, SIGNAL( clicked() ), this, SLOT( selectDir() ) );
    _baseDir->setText( Settings::SettingsData::instance()->HTMLBaseDir() );

    // Base URL
    label = new QLabel( i18n("Base URL:"), destinationPage );
    lay2->addWidget( label, 1, 0 );

    _baseURL = new KLineEdit( destinationPage );
    _baseURL->setText( Settings::SettingsData::instance()->HTMLBaseURL() );
    lay2->addWidget( _baseURL, 1, 1 );
    label->setBuddy( _baseURL );

    // Destination URL
    label = new QLabel( i18n("URL for final destination:" ), destinationPage );
    lay2->addWidget( label, 2, 0 );
    _destURL = new KLineEdit( destinationPage );
    _destURL->setText( Settings::SettingsData::instance()->HTMLDestURL() );
    lay2->addWidget( _destURL, 2, 1 );
    label->setBuddy( _destURL );

    // Output Directory
    label = new QLabel( i18n("Output directory:"), destinationPage );
    lay2->addWidget( label, 3, 0 );
    _outputDir = new KLineEdit( destinationPage );
    lay2->addWidget( _outputDir, 3, 1 );
    label->setBuddy( _outputDir );

    label = new QLabel( i18n("<b>Hint: Press the help button for descriptions of the fields</b>"), destinationPage );
    lay1->addWidget( label );
    lay1->addStretch( 1 );
}

void HTMLDialog::slotOk()
{
    if ( !checkVars() )
        return;

    if( activeResolutions().count() < 1 ) {
        KMessageBox::error( 0, i18n( "You must select at least one resolution." ) );
        return;
    }

    accept();

    Settings::SettingsData::instance()->setHTMLBaseDir( _baseDir->text() );
    Settings::SettingsData::instance()->setHTMLBaseURL( _baseURL->text() );
    Settings::SettingsData::instance()->setHTMLDestURL( _destURL->text() );

    Generator generator( setup(), this );
    generator.generate();
}

void HTMLDialog::selectDir()
{
    KUrl dir = KFileDialog::getExistingDirectoryUrl( _baseDir->text(), this );
    if ( !dir.url().isNull() )
        _baseDir->setText( dir.url() );
}

bool HTMLDialog::checkVars()
{
    QString outputDir = _baseDir->text() + QString::fromLatin1( "/" ) + _outputDir->text();


    // Ensure base dir is specified
    QString baseDir = _baseDir->text();
    if ( baseDir.isEmpty() ) {
        KMessageBox::error( this, i18n("<p>You did not specify a base directory. "
                                       "This is the topmost directory for your images. "
                                       "Under this directory you will find each generated collection "
                                       "in separate directories.</p>"),
                            i18n("No Base Directory Specified") );
        return false;
    }

    // ensure output directory is specified
    if ( _outputDir->text().isEmpty() ) {
        KMessageBox::error( this, i18n("<p>You did not specify an output directory. "
                                       "This is a directory containing the actual images. "
                                       "The directory will be in the base directory specified above.</p>"),
                            i18n("No Output Directory Specified") );
        return false;
    }

    // ensure base dir exists
    KIO::UDSEntry result;
    bool ok = KIO::NetAccess::stat( KUrl(baseDir), result, this );
    if ( !ok ) {
        KMessageBox::error( this, i18n("<p>Error while reading information about %1. "
                                       "This is most likely because the directory does not exist.</p>")
                            .arg( baseDir ) );
        return false;
    }

    KFileItem fileInfo( result, KUrl(baseDir) );
    if ( !fileInfo.isDir() ) {
        KMessageBox::error( this, i18n("<p>%1 does not exist, is not a directory or "
                                       "cannot be written to.</p>").arg( baseDir ) );
        return false;
    }


    // test if destination directory exists.
    bool exists = KIO::NetAccess::exists( KUrl(outputDir), KIO::NetAccess::DestinationSide, MainWindow::Window::theMainWindow() );
    if ( exists ) {
        int answer = KMessageBox::warningYesNo( this,
                                                i18n("<p>Output directory %1 already exists. "
                                                     "Usually you should specify a new directory.</p>"
                                                     "<p>Should I delete %2 first?</p>").arg( outputDir ).arg( outputDir ),
                                                i18n("Directory Exists"), KStandardGuiItem::yes(), KStandardGuiItem::no(),
                                                QString::fromLatin1("html_export_delete_original_directory") );
        if ( answer == KMessageBox::Yes ) {
            KIO::NetAccess::del( KUrl(outputDir), MainWindow::Window::theMainWindow() );
        }
        else
            return false;
    }
    return true;
}

QList<ImageSizeCheckBox*> HTMLDialog::activeResolutions() const
{
    QList<ImageSizeCheckBox*> res;
    for( QList<ImageSizeCheckBox*>::ConstIterator sizeIt = _cbs.begin(); sizeIt != _cbs.end(); ++sizeIt ) {
        if ( (*sizeIt)->isChecked() )
            res << *sizeIt;
    }
    return res;
}

void HTMLDialog::populateThemesCombo()
{
    QStringList dirs = KGlobal::dirs()->findDirs( "data", QString::fromLocal8Bit("kphotoalbum/themes/") );
    int i = 0;
    for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it) {
        QDir dir(*it);
        QStringList themes = dir.entryList( QDir::Dirs | QDir::Readable );
        for(QStringList::Iterator it = themes.begin(); it != themes.end(); ++it) {
            if(*it == QString::fromLatin1(".") || *it == QString::fromLatin1("..")) continue;
            QString themePath = QString::fromLatin1("%1/%2/").arg(dir.path()).arg(*it);

            KConfigGroup config = KGlobal::config()->group( QString::fromLatin1( "theme/%1" ).arg( themePath ) );
            QString themeName = config.readEntry( "Name" );
            QString themeAuthor = config.readEntry( "Author" );

            enableButtonOk( true );
            _themeBox->insertItem( i, i18n( "%1 (by %2)",themeName, themeAuthor ) );
            _themes.insert( i, themePath );
            i++;
        }
    }
    if(_themeBox->count() < 1) {
        KMessageBox::error( this, i18n("Could not find any themes - this is very likely an installation error" ) );
    }
}

int HTMLDialog::exec( const QStringList& list )
{
    _list = list;
    return KDialog::exec();
}



Setup HTMLGenerator::HTMLDialog::setup() const
{
    Setup setup;
    setup.setTitle( _title->text() );
    setup.setBaseDir( _baseDir->text() );
    setup.setBaseURL( _baseURL->text() );
    setup.setOutputDir( _outputDir->text() );
    setup.setThumbSize( _thumbSize->value() );
    setup.setDescription( _description->toPlainText() );
    setup.setNumOfCols( _numOfCols->value() );
    setup.setGenerateKimFile( _generateKimFile->isChecked() );
    setup.setThemePath( _themes[_themeBox->currentIndex()] );
    for( QMap<QString,QCheckBox*>::ConstIterator includeIt = _whatToIncludeMap.begin();
         includeIt != _whatToIncludeMap.end(); ++includeIt ) {
        setup.setIncludeCategory( includeIt.key(), includeIt.value()->isChecked() );
    }
    setup.setImageList( _list );

    setup.setResolutions( activeResolutions() );
    setup.setInlineMovies( _inlineMovies->isChecked() );
    return setup;
}

#include "HTMLDialog.moc"
