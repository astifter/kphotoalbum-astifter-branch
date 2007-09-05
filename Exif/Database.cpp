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
#undef QT_NO_CAST_ASCII
#undef QT_CAST_NO_ASCII
#include "Exif/Database.h"
#include <qsqldatabase.h>
#include <qsqldriver.h>
//Added by qt3to4:
#include <Q3ValueList>
#include "Settings/SettingsData.h"
#include <qsqlquery.h>
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include "Exif/DatabaseElement.h"
#include "Database.h"
#include <qfile.h>
#include "MainWindow/Window.h"
#include <kmessagebox.h>
#include <klocale.h>
#include "DB/ImageDB.h"
#include <QSqlError>
#include <kdebug.h>

using namespace Exif;

static Q3ValueList<DatabaseElement*> elements()
{
    static Q3ValueList<DatabaseElement*> elms;

    if ( elms.count() == 0 ) {
        elms.append( new RationalExifElement( "Exif.Photo.FocalLength" ) );
        elms.append( new RationalExifElement( "Exif.Photo.ExposureTime" ) );

        elms.append( new RationalExifElement( "Exif.Photo.ApertureValue" ) );
        elms.append( new RationalExifElement( "Exif.Photo.FNumber" ) );
        //elms.append( new RationalExifElement( "Exif.Photo.FlashEnergy" ) );

        elms.append( new IntExifElement( "Exif.Photo.Flash" ) );
        elms.append( new IntExifElement( "Exif.Photo.Contrast" ) );
        elms.append( new IntExifElement( "Exif.Photo.Sharpness" ) );
        elms.append( new IntExifElement( "Exif.Photo.Saturation" ) );
        elms.append( new IntExifElement( "Exif.Image.Orientation" ) );
        elms.append( new IntExifElement( "Exif.Photo.MeteringMode" ) );
        elms.append( new IntExifElement( "Exif.Photo.ISOSpeedRatings" ) );
        elms.append( new IntExifElement( "Exif.Photo.ExposureProgram" ) );

        elms.append( new StringExifElement( "Exif.Image.Make" ) );
        elms.append( new StringExifElement( "Exif.Image.Model" ) );
    }

    return elms;
}

Exif::Database* Exif::Database::_instance = 0;

static void showError( QSqlQuery& query )
{
    qWarning( "Error running query: %s\nError was: %s", qPrintable(query.executedQuery()), qPrintable(query.lastError().text()));
}

Exif::Database::Database()
    : _isOpen(false)
{
}


void Exif::Database::openDatabase()
{
    _db = QSqlDatabase::addDatabase( QString::fromLatin1( "QSQLITE" ), QString::fromLatin1( "exif" ) );

    _db.setDatabaseName( exifDBFile() );

    if ( !_db.open() )
        qWarning("Couldn't open db %s", qPrintable(_db.lastError().text()) );
    else
        _isOpen = true;

    // If SQLite in Qt has Unicode feature, it will convert querys to
    // UTF-8 automatically. Otherwise we should do the conversion to
    // be able to store any Unicode character.
    _doUTF8Conversion = !_db.driver()->hasFeature(QSqlDriver::Unicode);
}

bool Exif::Database::isOpen() const
{
    return _isOpen;
}

void Exif::Database::populateDatabase()
{
    QStringList attributes;
    Q3ValueList<DatabaseElement*> elms = elements();
    for( Q3ValueList<DatabaseElement*>::Iterator tagIt = elms.begin(); tagIt != elms.end(); ++tagIt ) {
        attributes.append( (*tagIt)->createString() );
    }

    QSqlQuery query( QString::fromLatin1( "create table exif (filename string PRIMARY KEY, %1 )")
                     .arg( attributes.join( QString::fromLatin1(", ") ) ), _db );
    if ( !query.exec())
        ; // This always prints out a false error, so lets not worry about it now - //showError( query );
}

void Exif::Database::add( const QString& fileName )
{
    if ( !isUsable() )
        return;

    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fileName.toLocal8Bit().data());
        Q_ASSERT(image.get() != 0);
        image->readMetadata();
        Exiv2::ExifData &exifData = image->exifData();
        insert( fileName, exifData );
    }
    catch (...)
    {
    }
}

void Exif::Database::remove( const QString& fileName )
{
    if ( !isUsable() )
        return;

    QSqlQuery query( QString::fromLatin1( "DELETE FROM exif WHERE fileName=?" ), _db );
    query.bindValue( 0, _doUTF8Conversion ? fileName.toUtf8() : fileName );
    if ( !query.exec() )
        showError( query );
}

void Exif::Database::insert( const QString& filename, Exiv2::ExifData data )
{
    if ( !isUsable() )
        return;

    QStringList formalList;
    Q3ValueList<DatabaseElement*> elms = elements();
    for( Q3ValueList<DatabaseElement*>::Iterator tagIt = elms.begin(); tagIt != elms.end(); ++tagIt ) {
        formalList.append( (*tagIt)->queryString() );
    }

    QSqlQuery query( QString::fromLatin1( "INSERT into exif values (?, %1) " ).arg( formalList.join( QString::fromLatin1( ", " ) ) ), _db );
    query.bindValue(  0, _doUTF8Conversion ? filename.toUtf8() : filename );
    int i = 1;
    for( Q3ValueList<DatabaseElement*>::Iterator tagIt = elms.begin(); tagIt != elms.end(); ++tagIt ) {
        (*tagIt)->bindValues( &query, i, data );
    }

    if ( !query.exec() )
        showError( query );

}

Exif::Database* Exif::Database::instance()
{
    if ( !_instance ) {
        _instance = new Exif::Database();
        _instance->init();
    }

    return _instance;
}

bool Exif::Database::isAvailable()
{
#ifdef QT_NO_SQL
    return false;
#endif

    return QSqlDatabase::isDriverAvailable( QString::fromLatin1( "QSQLITE" ) );
}

bool Exif::Database::isUsable() const
{
    return (isAvailable() && isOpen());
}

QString Exif::Database::exifDBFile()
{
    return ::Settings::SettingsData::instance()->imageDirectory() + QString::fromLatin1("/exif-info.db");
}

Set<QString> Exif::Database::filesMatchingQuery( const QString& queryStr )
{
    if ( !isUsable() )
        return Set<QString>();

    Set<QString> result;
    QSqlQuery query( queryStr, _db );

    if ( !query.exec() )
        showError( query );

    else {
        if ( _doUTF8Conversion )
            while ( query.next() )
                result.insert( QString::fromUtf8( query.value(0).toByteArray() ) );
        else
            while ( query.next() )
                result.insert( query.value(0).toString() );
    }

    return result;
}

void Exif::Database::offerInitialize()
{
    int ret = KMessageBox::questionYesNo( MainWindow::Window::theMainWindow(),
                                          i18n("<p>Congratulation, your KPhotoAlbum version now supports searching "
                                               "for EXIF information.</p>"
                                               "<p>For this to work, KPhotoAlbum needs to rescan your images. "
                                               "Do you want this to happen now?</p>"),
                                          i18n("Rescan for EXIF information") );
    if ( ret == KMessageBox::Yes )
        DB::ImageDB::instance()->slotReread( DB::ImageDB::instance()->images(), EXIFMODE_DATABASE_UPDATE );

}

QList< QPair<QString,QString> > Exif::Database::cameras() const
{
    QList< QPair<QString,QString> > result;

    if ( !isUsable() )
        return result;

    QSqlQuery query( QString::fromLatin1("SELECT DISTINCT Exif_Image_Make, Exif_Image_Model FROM exif"), _db );
    if ( !query.exec() )
        showError( query );

    else {
        while ( query.next() ) {
            QString make = query.value(0).toString();
            QString model = query.value(1).toString();
            if ( !make.isEmpty() && !model.isEmpty() )
                result.append( qMakePair( make, model ) );
        }
    }

    return result;
}

void Exif::Database::init()
{
    if ( !isAvailable() )
        return;

    bool dbExists = QFile::exists( exifDBFile() );

    openDatabase();

    if ( !isOpen() )
        return;

    if ( !dbExists ) {
        populateDatabase();
        offerInitialize();
    }
}

