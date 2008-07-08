#include "ImportHandler.h"
#include "KimFileReader.h"
#include "ImportSettings.h"
#include <QDebug>
#include <QComboBox>
#include <QCheckBox>
#include <QApplication>
#include <QFile>
#include <QProgressDialog>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kio/jobuidelegate.h>
#include "MainWindow/Window.h"
#include <kmessagebox.h>
#include "DB/ImageDB.h"
#include "ImportMatcher.h"
#include "Browser/BrowserWidget.h"
#include "DB/MD5Map.h"

ImportExport::ImportHandler::ImportHandler()
    : m_finishedPressed(false), _progress(0), _reportUnreadableFiles( true )

{
}

bool ImportExport::ImportHandler::exec( const ImportSettings& settings, KimFileReader* kimFileReader )
{
    m_settings = settings;
    m_kimFileReader = kimFileReader;
    m_finishedPressed = true;
    m_nameMap = Utilities::createUniqNameMap( Utilities::infoListToStringList(m_settings.selectedImages()), true, m_settings.destination() );
    bool ok;
    if ( m_settings.externalSource() ) {
        copyFromExternal();
        ok = m_eventLoop.exec();
    }
    else {
        ok = copyFilesFromZipFile();
        if ( ok )
            updateDB();
    }
    if ( _progress )
        delete _progress;

    return ok;
}

void ImportExport::ImportHandler::copyFromExternal()
{
    _pendingCopies = m_settings.selectedImages();
    _totalCopied = 0;
    _progress = new QProgressDialog( i18n("Copying Images"), i18n("&Cancel"), 0,2 * _pendingCopies.count(), MainWindow::Window::theMainWindow() );
    _progress->setValue( 0 );
    _progress->show();
    connect( _progress, SIGNAL( canceled() ), this, SLOT( stopCopyingImages() ) );
    copyNextFromExternal();

}

void ImportExport::ImportHandler::copyNextFromExternal()
{
    // JKP - handle the situation where we should not copy, as the image already is in the DB
    DB::ImageInfoPtr info = _pendingCopies[0];
    _pendingCopies.pop_front();
    QString fileName = info->fileName( true );
    KUrl src1 = m_settings.kimFile();
    KUrl src2 = m_settings.baseURL();
    bool succeeded = false;
    QStringList tried;

    // First search for images next to the .kim file
    // Second search for images base on the image root as specified in the .kim file
    for ( int i = 0; i < 2; ++i ) {
        KUrl src = src1;
        if ( i == 1 )
            src = src2;

        src.setFileName( fileName );
        if ( KIO::NetAccess::exists( src, KIO::NetAccess::SourceSide, MainWindow::Window::theMainWindow() ) ) {
            KUrl dest;
            dest.setPath( Settings::SettingsData::instance()->imageDirectory() + m_nameMap[fileName] );
            _job = KIO::file_copy( src, dest, -1, KIO::HideProgressInfo );
            connect( _job, SIGNAL( result( KJob* ) ), this, SLOT( aCopyJobCompleted( KJob* ) ) );
            succeeded = true;
            break;
        } else
            tried << src.prettyUrl();
    }

    if (!succeeded)
        aCopyFailed( tried );
}

bool ImportExport::ImportHandler::copyFilesFromZipFile()
{
    DB::ImageInfoList images = m_settings.selectedImages();

    _totalCopied = 0;
    _progress = new QProgressDialog( i18n("Copying Images"), i18n("&Cancel"), 0,2 * images.count(), MainWindow::Window::theMainWindow() );
    _progress->setValue( 0 );
    _progress->show();

    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        if ( !isImageAlreadyInDB( *it ) ) {
            QString fileName = (*it)->fileName( true );
            QByteArray data = m_kimFileReader->loadImage( fileName );
            if ( data.isNull() )
                return false;
            QString newName = Settings::SettingsData::instance()->imageDirectory() + m_nameMap[fileName];

            QString relativeName = newName.mid( Settings::SettingsData::instance()->imageDirectory().length() );
            if ( relativeName.startsWith( QString::fromLatin1( "/" ) ) )
                relativeName= relativeName.mid(1);

            QFile out( newName );
            if ( !out.open( QIODevice::WriteOnly ) ) {
                KMessageBox::error( MainWindow::Window::theMainWindow(), i18n("Error when writing image %1", newName ) );
                return false;
            }
            out.write( data, data.size() );
            out.close();
        }

        qApp->processEvents();
        _progress->setValue( ++_totalCopied );
        if ( _progress->wasCanceled() ) {
            return false;
        }
    }
    return true;
}

void ImportExport::ImportHandler::updateDB()
{
    disconnect( _progress, SIGNAL( canceled() ), this, SLOT( stopCopyingImages() ) );
    _progress->setLabelText( i18n("Updating Database") );

    // Run though all images
    DB::ImageInfoList images = m_settings.selectedImages();
    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        DB::ImageInfoPtr info = *it;

        if ( isImageAlreadyInDB( info ) )
            updateInfo( matchingInfoFromDB( info ), info );
        else
            addNewRecord( info );

        _progress->setValue( ++_totalCopied );
        if ( _progress->wasCanceled() )
            break;
    }
    Browser::BrowserWidget::instance()->home();
}

void ImportExport::ImportHandler::stopCopyingImages()
{
    _job->kill();
}

void ImportExport::ImportHandler::aCopyFailed( QStringList files )
{
    int result = _reportUnreadableFiles ?
                 KMessageBox::warningYesNoCancelList( _progress,
                                                      i18n("Can't copy file from any of the following locations:"),
                                                      files, QString::null, KStandardGuiItem::cont(), KGuiItem( i18n("Continue without Asking") )) : KMessageBox::Yes;

    switch (result) {
    case KMessageBox::Cancel:
        // This might be late -- if we managed to copy some files, we will
        // just throw away any changes to the DB, but some new image files
        // might be in the image directory...
        m_eventLoop.exit(false);
        break;

    case KMessageBox::No:
        _reportUnreadableFiles = false;
        // fall through
    default:
        aCopyJobCompleted( 0 );
    }
}

void ImportExport::ImportHandler::aCopyJobCompleted( KJob* job )
{
    if ( job && job->error() ) {
        job->uiDelegate()->showErrorMessage();
        m_eventLoop.exit(false);
    }
    else if ( _pendingCopies.count() == 0 ) {
        updateDB();
        m_eventLoop.exit(true);
    }
    else if ( _progress->wasCanceled() ) {
        m_eventLoop.exit(false);
    }
    else {
        _progress->setValue( ++_totalCopied );
        copyNextFromExternal();
    }
}

bool ImportExport::ImportHandler::isImageAlreadyInDB( const DB::ImageInfoPtr& info )
{
    return DB::ImageDB::instance()->md5Map()->contains(info->MD5Sum());
}

DB::ImageInfoPtr ImportExport::ImportHandler::matchingInfoFromDB( const DB::ImageInfoPtr& info )
{
    const QString& name = DB::ImageDB::instance()->md5Map()->lookup(info->MD5Sum());
    return DB::ImageDB::instance()->info( name );
}

void ImportExport::ImportHandler::updateInfo( DB::ImageInfoPtr dbInfo, DB::ImageInfoPtr newInfo )
{
    if ( dbInfo->label() != newInfo->label() && m_settings.importAction("*Label*") == ImportSettings::Replace )
        dbInfo->setLabel( newInfo->label() );

    if ( dbInfo->description().simplified() != newInfo->description().simplified() ) {
        if ( m_settings.importAction("*Description*") == ImportSettings::Replace )
            dbInfo->setDescription( newInfo->description() );
        else if ( m_settings.importAction("*Description*") == ImportSettings::Merge )
            dbInfo->setDescription( dbInfo->description() + QString::fromLatin1("\n\n") + newInfo->description() );
    }
}

void ImportExport::ImportHandler::addNewRecord( DB::ImageInfoPtr info )
{
    DB::ImageInfoPtr updateInfo(new DB::ImageInfo( m_nameMap[info->fileName(true)], DB::Image, false ));
    updateInfo->setLabel( info->label() );
    updateInfo->setDescription( info->description() );
    updateInfo->setDate( info->date() );
    updateInfo->rotate( info->angle() );
    updateInfo->setMD5Sum( Utilities::MD5Sum( updateInfo->fileName(false) ) );


    DB::ImageInfoList list;
    list.append(updateInfo);
    DB::ImageDB::instance()->addImages( list );

    // Run though the categories
    const ImportMatchers matchers = m_settings.importMatchers();
    for( QList<ImportMatcher*>::ConstIterator grpIt = matchers.begin(); grpIt != matchers.end(); ++grpIt ) {
        QString otherGrp = (*grpIt)->_otherCategory;
        QString myGrp = (*grpIt)->_myCategory;

        // Run through each option
        QList<CategoryMatch*>& matcher = (*grpIt)->_matchers;
        for( QList<CategoryMatch*>::Iterator optionIt = matcher.begin(); optionIt != matcher.end(); ++optionIt ) {
            if ( !(*optionIt)->_checkbox->isChecked() )
                continue;
            QString otherOption = (*optionIt)->_text;
            QString myOption = (*optionIt)->_combobox->currentText();

            if ( info->hasCategoryInfo( otherGrp, otherOption ) ) {
                updateInfo->addCategoryInfo( myGrp, myOption );
                DB::ImageDB::instance()->categoryCollection()->categoryForName( myGrp )->addItem( myOption );
            }

        }
    }
}
