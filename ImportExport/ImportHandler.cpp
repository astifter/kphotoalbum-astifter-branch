/* Copyright (C) 2003-2010 Jesper K. Pedersen <blackie@kde.org>

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
#include "ImportHandler.h"

#include <QApplication>
#include <QFile>
#include <QProgressDialog>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kio/jobuidelegate.h>
#include <kmessagebox.h>
#include <kprogressdialog.h>
#include <QDebug>

#include "Utilities/Util.h"
#include "KimFileReader.h"
#include "ImportSettings.h"
#include "MainWindow/Window.h"
#include "DB/ImageDB.h"
#include "Browser/BrowserWidget.h"
#include "DB/MD5Map.h"
#include "DB/Category.h"
#include "DB/CategoryCollection.h"
#include "Utilities/UniqFilenameMapper.h"
#include "kio/job.h"

#ifdef DEBUG_KIM_IMPORT
# define Debug qDebug
#else
# define Debug if (0) qDebug
#endif

using namespace ImportExport;

ImportExport::ImportHandler::ImportHandler()
    : m_fileMapper(nullptr), m_finishedPressed(false), m_progress(0), m_reportUnreadableFiles( true )
    , m_eventLoop( new QEventLoop )

{
}

ImportHandler::~ImportHandler() {
    delete m_fileMapper;
    delete m_eventLoop;
}

bool ImportExport::ImportHandler::exec( const ImportSettings& settings, KimFileReader* kimFileReader )
{
    m_settings = settings;
    m_kimFileReader = kimFileReader;
    m_finishedPressed = true;
    delete m_fileMapper;
    m_fileMapper = new Utilities::UniqFilenameMapper(m_settings.destination());
    bool ok;
    // copy images
    if ( m_settings.externalSource() ) {
        copyFromExternal();

        // If none of the images were to be copied, then we flushed the loop before we got started, in that case, don't start the loop.
        if ( m_pendingCopies.count() > 0 )
            ok = m_eventLoop->exec();
        else
            ok = false;
    }
    else {
        ok = copyFilesFromZipFile();
        if ( ok )
            updateDB();
    }
    if ( m_progress )
        delete m_progress;

    return ok;
}

void ImportExport::ImportHandler::copyFromExternal()
{
    m_pendingCopies = m_settings.selectedImages();
    m_totalCopied = 0;
    m_progress = new KProgressDialog( MainWindow::Window::theMainWindow(), i18n("Copying Images") );
    m_progress->progressBar()->setMinimum( 0 );
    m_progress->progressBar()->setMaximum( 2 * m_pendingCopies.count() );
    m_progress->show();
    connect( m_progress, SIGNAL(cancelClicked()), this, SLOT(stopCopyingImages()) );
    copyNextFromExternal();

}

void ImportExport::ImportHandler::copyNextFromExternal()
{
    DB::ImageInfoPtr info = m_pendingCopies[0];
    m_pendingCopies.pop_front();

    if ( isImageAlreadyInDB( info ) ) {
        aCopyJobCompleted(0);
        return;
    }

    const DB::FileName fileName = info->fileName();
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

        src.setFileName( fileName.relative() );
        if ( KIO::NetAccess::exists( src, KIO::NetAccess::SourceSide, MainWindow::Window::theMainWindow() ) ) {
            KUrl dest;
            dest.setPath( m_fileMapper->uniqNameFor(fileName) );
            m_job = KIO::file_copy( src, dest, -1, KIO::HideProgressInfo );
            connect( m_job, SIGNAL(result(KJob*)), this, SLOT(aCopyJobCompleted(KJob*)) );
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

    m_totalCopied = 0;
    m_progress = new KProgressDialog( MainWindow::Window::theMainWindow(), i18n("Copying Images") );
    m_progress->progressBar()->setMinimum( 0 );
    m_progress->progressBar()->setMaximum( 2 * m_pendingCopies.count() );
    m_progress->show();

    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        if ( !isImageAlreadyInDB( *it ) ) {
            const DB::FileName fileName = (*it)->fileName();
            QByteArray data = m_kimFileReader->loadImage( fileName.relative() );
            if ( data.isNull() )
                return false;
            QString newName = m_fileMapper->uniqNameFor(fileName);

            QFile out( newName );
            if ( !out.open( QIODevice::WriteOnly ) ) {
                KMessageBox::error( MainWindow::Window::theMainWindow(), i18n("Error when writing image %1", newName ) );
                return false;
            }
            out.write( data, data.size() );
            out.close();
        }

        qApp->processEvents();
        m_progress->progressBar()->setValue( ++m_totalCopied );
        if ( m_progress->wasCancelled() ) {
            return false;
        }
    }
    return true;
}

void ImportExport::ImportHandler::updateDB()
{
    disconnect( m_progress, SIGNAL(cancelClicked()), this, SLOT(stopCopyingImages()) );
    m_progress->setLabelText( i18n("Updating Database") );
    int len = Settings::SettingsData::instance()->imageDirectory().length();
    // image directory is always a prefix of destination
    if ( len == m_settings.destination().length() )
        len = 0;
    else
        Debug()
            << "Re-rooting of ImageInfos from " << Settings::SettingsData::instance()->imageDirectory()
            << " to " << m_settings.destination();

    // Run though all images
    DB::ImageInfoList images = m_settings.selectedImages();
    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        DB::ImageInfoPtr info = *it;
        if ( len != 0) {
            // exchange prefix:
            QString name = m_settings.destination() + info->fileName().absolute().mid(len);
            Debug() << info->fileName().absolute() << " -> " << name;
            info->setFileName( DB::FileName::fromAbsolutePath(name) );
        }

        if ( isImageAlreadyInDB( info ) ) {
            Debug() << "Updating ImageInfo for " << info->fileName().absolute();
            updateInfo( matchingInfoFromDB( info ), info );
        } else {
            Debug() << "Adding ImageInfo for " << info->fileName().absolute();
            addNewRecord( info );
        }

        m_progress->progressBar()->setValue( ++m_totalCopied );
        if ( m_progress->wasCancelled() )
            break;
    }

    Browser::BrowserWidget::instance()->home();
}

void ImportExport::ImportHandler::stopCopyingImages()
{
    m_job->kill();
}

void ImportExport::ImportHandler::aCopyFailed( QStringList files )
{
    int result = m_reportUnreadableFiles ?
                 KMessageBox::warningYesNoCancelList( m_progress,
                                                      i18n("Cannot copy from any of the following locations:"),
                                                      files, QString(), KStandardGuiItem::cont(), KGuiItem( i18n("Continue without Asking") )) : KMessageBox::Yes;

    switch (result) {
    case KMessageBox::Cancel:
        // This might be late -- if we managed to copy some files, we will
        // just throw away any changes to the DB, but some new image files
        // might be in the image directory...
        m_eventLoop->exit(false);
        m_pendingCopies.pop_front();
        break;

    case KMessageBox::No:
        m_reportUnreadableFiles = false;
        // fall through
    default:
        aCopyJobCompleted( 0 );
    }
}

void ImportExport::ImportHandler::aCopyJobCompleted( KJob* job )
{
    if ( job && job->error() ) {
        job->uiDelegate()->showErrorMessage();
        m_eventLoop->exit(false);
    }
    else if ( m_pendingCopies.count() == 0 ) {
        updateDB();
        m_eventLoop->exit(true);
    }
    else if ( m_progress->wasCancelled() ) {
        m_eventLoop->exit(false);
    }
    else {
        m_progress->progressBar()->setValue( ++m_totalCopied );
        copyNextFromExternal();
    }
}

bool ImportExport::ImportHandler::isImageAlreadyInDB( const DB::ImageInfoPtr& info )
{
    return DB::ImageDB::instance()->md5Map()->contains(info->MD5Sum());
}

DB::ImageInfoPtr ImportExport::ImportHandler::matchingInfoFromDB( const DB::ImageInfoPtr& info )
{
    const DB::FileName name = DB::ImageDB::instance()->md5Map()->lookup(info->MD5Sum());
    return DB::ImageDB::instance()->info(name);
}

/**
 * Merge the ImageInfo data from the kim file into the existing ImageInfo.
 */
void ImportExport::ImportHandler::updateInfo( DB::ImageInfoPtr dbInfo, DB::ImageInfoPtr newInfo )
{
    if ( dbInfo->label() != newInfo->label() && m_settings.importAction(QString::fromLatin1("*Label*")) == ImportSettings::Replace )
        dbInfo->setLabel( newInfo->label() );

    if ( dbInfo->description().simplified() != newInfo->description().simplified() ) {
        if ( m_settings.importAction(QString::fromLatin1("*Description*")) == ImportSettings::Replace )
            dbInfo->setDescription( newInfo->description() );
        else if ( m_settings.importAction(QString::fromLatin1("*Description*")) == ImportSettings::Merge )
            dbInfo->setDescription( dbInfo->description() + QString::fromLatin1("<br/><br/>") + newInfo->description() );
    }


    if (dbInfo->angle() != newInfo->angle() && m_settings.importAction(QString::fromLatin1("*Orientation*")) == ImportSettings::Replace )
        dbInfo->setAngle( newInfo->angle() );

    if (dbInfo->date() != newInfo->date() && m_settings.importAction(QString::fromLatin1("*Date*")) == ImportSettings::Replace )
        dbInfo->setDate( newInfo->date() );


    updateCategories( newInfo, dbInfo, false );
}

void ImportExport::ImportHandler::addNewRecord( DB::ImageInfoPtr info )
{
    const DB::FileName importName = info->fileName();

    DB::ImageInfoPtr updateInfo(new DB::ImageInfo(importName, info->mediaType(), false /*don't read exif */));
    updateInfo->setLabel( info->label() );
    updateInfo->setDescription( info->description() );
    updateInfo->setDate( info->date() );
    updateInfo->setAngle( info->angle() );
    updateInfo->setMD5Sum( Utilities::MD5Sum( updateInfo->fileName() ) );


    DB::ImageInfoList list;
    list.append(updateInfo);
    DB::ImageDB::instance()->addImages( list );

    updateCategories( info, updateInfo, true );
}

void ImportExport::ImportHandler::updateCategories( DB::ImageInfoPtr XMLInfo, DB::ImageInfoPtr DBInfo, bool forceReplace )
{
    // Run though the categories
    const QList<CategoryMatchSetting> matches = m_settings.categoryMatchSetting();

    for ( const CategoryMatchSetting& match : matches ) {
        QString XMLCategoryName = match.XMLCategoryName();
        QString DBCategoryName = match.DBCategoryName();
        ImportSettings::ImportAction action = m_settings.importAction(DBCategoryName);

        const Utilities::StringSet items = XMLInfo->itemsOfCategory(XMLCategoryName);
        DB::CategoryPtr DBCategoryPtr =  DB::ImageDB::instance()->categoryCollection()->categoryForName( DBCategoryName );

        if ( !forceReplace && action == ImportSettings::Replace )
            DBInfo->setCategoryInfo( DBCategoryName, Utilities::StringSet() );

        if ( action == ImportSettings::Merge || action == ImportSettings::Replace || forceReplace ) {
            for ( const QString& item : items ) {
                if (match.XMLtoDB().contains( item ) ) {
                    DBInfo->addCategoryInfo( DBCategoryName, match.XMLtoDB()[item] );
                    DBCategoryPtr->addItem( match.XMLtoDB()[item] );
                }
            }
        }
    }

}
// vi:expandtab:tabstop=4 shiftwidth=4:
