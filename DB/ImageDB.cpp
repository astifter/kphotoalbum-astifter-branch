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
#include "ImageDB.h"
#include "XMLDB/Database.h"
#include <kinputdialog.h>
#include <klocale.h>
#include <qfileinfo.h>
//Added by qt3to4:
#include <Q3ValueList>
#include "Browser/BrowserWidget.h"
#include "DB/CategoryCollection.h"
#include <QProgressBar>
#include <qapplication.h>
#include "NewImageFinder.h"
#include <DB/MediaCount.h>
#include <kdebug.h>
#include <config-kpa-sqldb.h>
#ifdef SQLDB_SUPPORT
#include "SQLDB/Database.h"
#include "SQLDB/DatabaseAddress.h"
#endif
#include "MainWindow/DirtyIndicator.h"
#include <QProgressDialog>

using namespace DB;

ImageDB* ImageDB::_instance = 0;

ImageDB* DB::ImageDB::instance()
{
    if ( _instance == 0 )
        qFatal("ImageDB::instance must not be called before ImageDB::setup");
    return _instance;
}

void ImageDB::setupXMLDB( const QString& configFile )
{
    if (_instance)
        qFatal("ImageDB::setupXMLDB: Setup must be called only once.");
    _instance = new XMLDB::Database( configFile );
    connectSlots();
}

#ifdef SQLDB_SUPPORT
void ImageDB::setupSQLDB( const SQLDB::DatabaseAddress& address )
{
    if (_instance)
        qFatal("ImageDB::setupSQLDB: Setup must be called only once.");
    _instance = new SQLDB::Database(address);

    connectSlots();
}
#endif /* SQLDB_SUPPORT */

void ImageDB::connectSlots()
{
    connect( Settings::SettingsData::instance(), SIGNAL( locked( bool, bool ) ), _instance, SLOT( lockDB( bool, bool ) ) );
}

QString ImageDB::NONE()
{
    return i18n("**NONE**");
}

QStringList ImageDB::currentScope( bool requireOnDisk ) const
{
    return search( Browser::BrowserWidget::instance()->currentContext(), requireOnDisk );
}

void ImageDB::setDateRange( const ImageDate& range, bool includeFuzzyCounts )
{
    _selectionRange = range;
    _includeFuzzyCounts = includeFuzzyCounts;
}

void ImageDB::clearDateRange()
{
    _selectionRange = ImageDate();
}

void ImageDB::slotRescan()
{
    bool newImages = NewImageFinder().findImages();
    if ( newImages )
        MainWindow::DirtyIndicator::markDirty();

    emit totalChanged( totalCount() );
}

void ImageDB::slotRecalcCheckSums( QStringList list )
{
    if ( list.isEmpty() ) {
        list = images();
        md5Map()->clear();
    }

    bool d = NewImageFinder().calculateMD5sums( list, md5Map() );
    if ( d )
        MainWindow::DirtyIndicator::markDirty();

    // To avoid deciding if the new images are shown in a given thumbnail view or in a given search
    // we rather just go to home.
    Browser::BrowserWidget::instance()->home();

    emit totalChanged( totalCount() );
}

StringSet DB::ImageDB::imagesWithMD5Changed()
{
    MD5Map map;
    bool wasCanceled;
    QStringList imageList = images();
    (void) NewImageFinder().calculateMD5sums( imageList, &map, &wasCanceled );
    if ( wasCanceled )
        return StringSet();

    StringSet changes =  md5Map()->diff( map );
    StringSet res;
    for ( StringSet::ConstIterator it = changes.begin(); it != changes.end(); ++it )
        res.insert( Settings::SettingsData::instance()->imageDirectory() + *it );
    return res;

}


ImageDB::ImageDB()
{
}

DB::MediaCount ImageDB::count( const ImageSearchInfo& searchInfo )
{
    QStringList list = search( searchInfo );
    uint images = 0;
    uint videos = 0;
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
        ImageInfoPtr inf = info( *it );
        if ( inf->mediaType() == Image )
            ++images;
        else
            ++videos;
    }
    return MediaCount( images, videos );
}

void ImageDB::convertBackend(ImageDB* newBackend, QProgressBar* progressBar)
{
    QStringList allImages = images();

    CategoryCollection* origCategories = categoryCollection();
    CategoryCollection* newCategories = newBackend->categoryCollection();

    Q3ValueList<CategoryPtr> categories = origCategories->categories();

    if (progressBar) {
        progressBar->setMaximum(categories.count() + allImages.count());
        progressBar->setValue(0);
    }

    uint n = 0;

    // Convert the Category info
    for( Q3ValueList<CategoryPtr>::ConstIterator it = categories.begin(); it != categories.end(); ++it ) {
        newCategories->addCategory( (*it)->name(), (*it)->iconName(), (*it)->viewType(),
                                    (*it)->thumbnailSize(), (*it)->doShow() );
        newCategories->categoryForName( (*it)->name() )->addOrReorderItems( (*it)->items() );

        if (progressBar) {
            progressBar->setValue(n++);
            qApp->processEvents();
        }
    }

    // Convert member map
    newBackend->memberMap() = memberMap();

    // Convert all images to the new back end
    uint count = 0;
    ImageInfoList list;
    for( QStringList::ConstIterator it = allImages.begin(); it != allImages.end(); ++it ) {
        list.append( info(*it) );
        if (++count % 100 == 0) {
            newBackend->addImages( list );
            list.clear();
        }
        if (progressBar) {
            progressBar->setValue(n++);
            qApp->processEvents();
        }
    }
    newBackend->addImages(list);
    if (progressBar)
        progressBar->setValue(n);
}

void ImageDB::slotReread( const QStringList& list, int mode)
{
// Do here a reread of the exif info and change the info correctly in the database without loss of previous added data
    QProgressDialog  dialog( i18n("Loading information from images"),
                             i18n("Cancel"), 0, list.count() );

    uint count=0;
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it, ++count  ) {
        if ( count % 10 == 0 ) {
            dialog.setValue( count ); // ensure to call setProgress(0)
            qApp->processEvents( QEventLoop::AllEvents );

            if ( dialog.wasCanceled() )
                return;
        }

        QFileInfo fi( *it );

        if (fi.exists())
            info(*it)->readExif(*it, mode);
        MainWindow::DirtyIndicator::markDirty();
    }
}

QString
ImageDB::findFirstItemInRange(const ImageDate& range,
                              bool includeRanges,
                              const Q3ValueVector<QString>& images) const
{
    QString candidate;
    QDateTime candidateDateStart;
    for (Q3ValueVector<QString>::const_iterator i = images.begin();
         i != images.end(); ++i) {
        ImageInfoPtr iInfo = info(*i);

        ImageDate::MatchType match = iInfo->date().isIncludedIn(range);
        if (match == DB::ImageDate::ExactMatch ||
            (includeRanges && match == DB::ImageDate::RangeMatch)) {
            if (candidate.isNull() ||
                iInfo->date().start() < candidateDateStart) {
                candidate = *i;
                candidateDateStart = info(candidate)->date().start();
            }
        }
    }
    return candidate;
}

/** \fn void ImageDB::renameCategory( const QString& oldName, const QString newName )
 * \brief Rename category in media items stored in database.
 */

#include "ImageDB.moc"
