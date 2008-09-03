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

#include "ImageInfo.h"
#include <qfileinfo.h>
#include <qdom.h>
#include "Settings/SettingsData.h"
#include "Utilities/Util.h"
#include "DB/ImageDB.h"
#include "DB/CategoryCollection.h"
#include "FileInfo.h"
#include <qstringlist.h>
#include "DB/MemberMap.h"
#include <config-kpa-exiv2.h>
#include "Exif/Database.h"
#include <kdebug.h>

using namespace DB;

ImageInfo::ImageInfo() :_null( true ), _rating(-1), _stackId(0), _stackOrder(0),
#ifdef HAVE_MARBLE
       _gpsPrecision(-1),
#endif
    _locked( false ), _dirty( false ), _delaySaving( false )
{
}

ImageInfo::ImageInfo( const QString& fileName, MediaType type, bool readExifInfo )
    :  _imageOnDisk( YesOnDisk ), _null( false ), _size( -1, -1 ), _type( type ),
    _rating(-1), _stackId(0), _stackOrder(0),
#ifdef HAVE_MARBLE
       _gpsPrecision(-1),
#endif
    _locked(false), _delaySaving( true )
{
    QString fullPath = Settings::SettingsData::instance()->imageDirectory()+ fileName;
    QFileInfo fi( Settings::SettingsData::instance()->imageDirectory() + fileName );
    _label = fi.completeBaseName();
    _angle = 0;

    setFileName( fileName);

    // Read EXIF information
    if ( readExifInfo )
        readExif(fullPath, EXIFMODE_INIT);

    _dirty = false;
    _delaySaving = false;
}

/** Change delaying of saving changes.
 *
 * Will save changes when set to false.
 *
 * Use this method to set multiple attributes with only one
 * database operation.
 *
 * Example:
 * \code
 * info.delaySavingChanges(true);
 * info.setLabel("Hello");
 * info.setDescription("Hello world");
 * info.delaySavingChanges(false);
 * \endcode
 *
 * \see saveChanges()
 */
void ImageInfo::delaySavingChanges(bool b)
{
    _delaySaving = b;
    if (!b)
        saveChanges();
}

void ImageInfo::setLabel( const QString& desc )
{
    if (desc != _label)
        _dirty = true;
    _label = desc;
    saveChangesIfNotDelayed();
}

QString ImageInfo::label() const
{
    return _label;
}

void ImageInfo::setDescription( const QString& desc )
{
    if (desc != _description)
        _dirty = true;
    _description = desc;
    saveChangesIfNotDelayed();
}

QString ImageInfo::description() const
{
    return _description;
}


void ImageInfo::setCategoryInfo( const QString& key, const StringSet& value )
{
    // Don't check if really changed, because it's too slow.
    _dirty = true;
    _categoryInfomation[key] = value;
    saveChangesIfNotDelayed();
}

bool ImageInfo::hasCategoryInfo( const QString& key, const QString& value )
{
    return _categoryInfomation[key].contains(value);
}

StringSet ImageInfo::itemsOfCategory( const QString& key ) const
{
    return QStringList(_categoryInfomation[key].toList());
}

void ImageInfo::renameItem( const QString& key, const QString& oldValue, const QString& newValue )
{
    StringSet& set = _categoryInfomation[key];
    StringSet::iterator it = set.find( oldValue );
    if ( it != set.end() ) {
        _dirty = true;
        set.erase( it );
        set.insert( newValue );
        saveChangesIfNotDelayed();
    }
}

QString ImageInfo::fileName( bool relative ) const
{
    if (relative)
        return _relativeFileName;
    else
        return _absoluteFileName;
}

void ImageInfo::setFileName( const QString& relativeFileName )
{
    if (relativeFileName != _relativeFileName)
        _dirty = true;
    _relativeFileName = relativeFileName;
    setAbsoluteFileName();
    _imageOnDisk = Unchecked;
    DB::CategoryPtr folderCategory = DB::ImageDB::instance()->categoryCollection()->
        categoryForName(QString::fromLatin1("Folder"));
    if (folderCategory) {
        DB::MemberMap& map = DB::ImageDB::instance()->memberMap();
        createFolderCategoryItem( folderCategory, map );
        //ImageDB::instance()->setMemberMap( map );
    }
    saveChangesIfNotDelayed();
}


void ImageInfo::rotate( int degrees )
{
    if (degrees != 0)
        _dirty = true;
    _angle += degrees + 360;
    _angle = _angle % 360;
    saveChangesIfNotDelayed();
}

int ImageInfo::angle() const
{
    return _angle;
}

void ImageInfo::setAngle( int angle )
{
    if (angle != _angle)
        _dirty = true;
    _angle = angle;
    saveChangesIfNotDelayed();
}

short ImageInfo::rating() const
{
    return _rating;
}

void ImageInfo::setRating( short rating )
{
    if ( rating > 10 )
        rating = 10;
    if ( rating < -1 )
        rating = -1;
    if ( _rating != rating )
        _dirty = true;

    _rating = rating;
    saveChangesIfNotDelayed();
}

unsigned int ImageInfo::stackId() const
{
    return _stackId;
}

void ImageInfo::setStackId( const unsigned int stackId )
{
    if ( stackId != _stackId )
        _dirty = true;
    _stackId = stackId;
    saveChangesIfNotDelayed();
}

unsigned int ImageInfo::stackOrder() const
{
    return _stackOrder;
}

void ImageInfo::setStackOrder( const unsigned int stackOrder )
{
    if ( stackOrder != _stackOrder )
        _dirty = true;
    _stackOrder = stackOrder;
    saveChangesIfNotDelayed();
}

#ifdef HAVE_MARBLE
int ImageInfo::gpsPrecision() const
{
    return _gpsPrecision;
}

void ImageInfo::setGpsPrecision( int precision )
{
    if ( precision < -1 )
        precision = -1;
    if ( _gpsPrecision != precision )
        _dirty = true;
    _gpsPrecision = precision;
    saveChangesIfNotDelayed();
}

GeoDataCoordinates ImageInfo::gpsCoordinates() const
{
    return _gpsCoordinates;
}

void ImageInfo::setGpsCoordinates( const GeoDataCoordinates& coordinates )
{
    if ( ! ( coordinates == _gpsCoordinates ) ) // no operator!=()
        _dirty = true;
    _gpsCoordinates = coordinates;
    saveChangesIfNotDelayed();
}
#endif

void ImageInfo::setDate( const ImageDate& date )
{
    if (date != _date)
        _dirty = true;
    _date = date;
    saveChangesIfNotDelayed();
}

ImageDate& ImageInfo::date()
{
    return _date;
}

ImageDate ImageInfo::date() const
{
    return _date;
}

bool ImageInfo::operator!=( const ImageInfo& other )
{
    return !(*this == other);
}

bool ImageInfo::operator==( const ImageInfo& other )
{
    bool changed =
        ( _relativeFileName != other._relativeFileName ||
          _label != other._label ||
          ( !_description.isEmpty() && !other._description.isEmpty() && _description != other._description ) || // one might be isNull.
          _date != other._date ||
          _angle != other._angle ||
#ifdef HAVE_MARBLE
          ( _gpsPrecision != other._gpsPrecision || 
            ! ( ( _gpsPrecision == -1 ) ? true :
            ( _gpsCoordinates == other._gpsCoordinates ) ) ) ||
#endif
          _rating != other._rating ||
          ( _stackId != other._stackId || 
            ! ( ( _stackId == 0 ) ? true :
            ( _stackOrder == other._stackOrder ) ) )
           );
    if ( !changed ) {
        QStringList keys = DB::ImageDB::instance()->categoryCollection()->categoryNames();
        for( QStringList::Iterator it = keys.begin(); it != keys.end(); ++it )
            changed |= _categoryInfomation[*it] != other._categoryInfomation[*it];
    }
    return !changed;
}

void ImageInfo::renameCategory( const QString& oldName, const QString& newName )
{
    _dirty = true;
    _categoryInfomation[newName] = _categoryInfomation[oldName];
    _categoryInfomation.remove(oldName);
    saveChangesIfNotDelayed();
}

void ImageInfo::setLocked( bool locked )
{
    _locked = locked;
}

bool ImageInfo::isLocked() const
{
    return _locked;
}

void ImageInfo::readExif(const QString& fullPath, DB::ExifMode mode)
{
    DB::FileInfo exifInfo = DB::FileInfo::read( fullPath, mode );

    bool oldDelaySaving = _delaySaving;
    delaySavingChanges(true);

    // Date
    if ( updateDateInformation(mode) ) {
        setDate( exifInfo.dateTime() );
    }

    // Orientation
    if ( (mode & EXIFMODE_ORIENTATION) && Settings::SettingsData::instance()->useEXIFRotate() ) {
        setAngle( exifInfo.angle() );
    }

    // Description
    if ( (mode & EXIFMODE_DESCRIPTION) && Settings::SettingsData::instance()->useEXIFComments() ) {
        setDescription( exifInfo.description() );
    }

    delaySavingChanges(false);
    _delaySaving = oldDelaySaving;

    // Database update
    if ( mode & EXIFMODE_DATABASE_UPDATE ) {
#ifdef HAVE_EXIV2
        Exif::Database::instance()->add( fullPath );
#endif
    }
}


QStringList ImageInfo::availableCategories() const
{
    return _categoryInfomation.keys();
}

void ImageInfo::clearMatched() const
{
    _matched.clear();
}

void ImageInfo::setMatched( const QString& category, const QString& value ) const
{
    _matched[category].insert( value );
    const MemberMap& map = DB::ImageDB::instance()->memberMap();
    QStringList members = map.members( category, value, true );
    _matched[category].insert( members );
}

/**
 * Returns whether all tokens for the given image are matches by the search
 * example: returns true if all people on an image is in the search,
 * i.e. it is only true if there are no persons on the image that are not
 * explicit searched for.
 * This is used when the search includes *None*
 */
bool ImageInfo::allMatched( const QString& category )
{
    const StringSet items = itemsOfCategory( category );
    for( StringSet::const_iterator it = items.begin(); it != items.end(); ++it ) {
        if ( !_matched[category].contains( *it ) )
            return false;
    }

    return true;
}

QSize ImageInfo::size() const
{
    return _size;
}

void ImageInfo::setSize( const QSize& size )
{
    if (size != _size)
        _dirty = true;
    _size = size;
    saveChangesIfNotDelayed();
}

bool ImageInfo::imageOnDisk( const QString& fileName )
{
    QFileInfo fi( fileName );
    return fi.exists();
}

ImageInfo::ImageInfo( const QString& fileName,
                      const QString& label,
                      const QString& description,
                      const ImageDate& date,
                      int angle,
                      const MD5& md5sum,
                      const QSize& size,
                      MediaType type,
                      short rating,
                      unsigned int stackId,
                      unsigned int stackOrder )
{
    _delaySaving = true;
    _relativeFileName = fileName;
    setAbsoluteFileName();
    _label =label;
    _description =description;
    _date = date;
    _angle =angle;
    _md5sum =md5sum;
    _size = size;
    _imageOnDisk = Unchecked;
    _locked = false;
    _null = false;
    _type = type;
    _dirty = true;
    delaySavingChanges(false);

    if ( rating > 10 )
        rating = 10;
    if ( rating < -1 )
        rating = -1;
    _rating = rating;
#ifdef HAVE_MARBLE
    _gpsPrecision = -1;
#endif
    _stackId = stackId;
    _stackOrder = stackOrder;
}

ImageInfo& ImageInfo::operator=( const ImageInfo& other )
{
    _relativeFileName = other._relativeFileName;
    setAbsoluteFileName();
    _label = other._label;
    _description = other._description;
    _date = other._date;
    _categoryInfomation = other._categoryInfomation;
    _angle = other._angle;
    _imageOnDisk = other._imageOnDisk;
    _md5sum = other._md5sum;
    _null = other._null;
    _size = other._size;
    _dirty = other._dirty;
    _rating = other._rating;
    _stackId = other._stackId;
    _stackOrder = other._stackOrder;
#ifdef HAVE_MARBLE
    _gpsCoordinates = other._gpsCoordinates;
    _gpsPrecision = other._gpsPrecision;
#endif

    delaySavingChanges(false);

    return *this;
}

MediaType DB::ImageInfo::mediaType() const
{
    return _type;
}

/**
 * During profiling I found that it took almost 5% of the time during
 * categorizing when browsing, simply to calculate the absolute filename, therefore it is
 * now an instance variable rather than calculated dynamically in
 * fileName().
 */
void DB::ImageInfo::setAbsoluteFileName()
{
    _absoluteFileName = Settings::SettingsData::instance()->imageDirectory() + _relativeFileName;
}

void DB::ImageInfo::createFolderCategoryItem( DB::CategoryPtr folderCategory, DB::MemberMap& memberMap )
{
    QString folderName = Utilities::relativeFolderName( _relativeFileName );
    if ( folderName.isNull() )
        return;

    QStringList directories = folderName.split(QString::fromLatin1( "/" ) );

    QString curPath;
    for( QStringList::ConstIterator directoryIt = directories.begin(); directoryIt != directories.end(); ++directoryIt ) {
        if ( curPath.isEmpty() )
            curPath = *directoryIt;
        else {
            QString oldPath = curPath;
            curPath = curPath + QString::fromLatin1( "/" ) + *directoryIt;
            memberMap.addMemberToGroup( folderCategory->name(), oldPath, curPath );
        }
    }

    _categoryInfomation.insert( folderCategory->name() , QStringList( folderName ) );
    folderCategory->addItem( folderName );
}

void DB::ImageInfo::addCategoryInfo( const QString& category, const StringSet& values )
{
    for ( StringSet::const_iterator valueIt = values.begin(); valueIt != values.end(); ++valueIt ) {
        if (! _categoryInfomation[category].contains( *valueIt ) ) {
            _dirty = true;
            _categoryInfomation[category].insert( *valueIt );
        }
    }
    saveChangesIfNotDelayed();
}

void DB::ImageInfo::removeCategoryInfo( const QString& category, const StringSet& values )
{
    for ( StringSet::const_iterator valueIt = values.begin(); valueIt != values.end(); ++valueIt ) {
        if ( _categoryInfomation[category].contains( *valueIt ) ) {
            _dirty = true;
            _categoryInfomation[category].erase(*valueIt);
        }
    }
    saveChangesIfNotDelayed();
}

void DB::ImageInfo::addCategoryInfo( const QString& category, const QString& value )
{
    if (! _categoryInfomation[category].contains( value ) ) {
        _dirty = true;
        _categoryInfomation[category].insert( value );
    }
    saveChangesIfNotDelayed();
}

void DB::ImageInfo::removeCategoryInfo( const QString& category, const QString& value )
{
    if ( _categoryInfomation[category].contains( value ) ) {
        _dirty = true;
        _categoryInfomation[category].erase( value );
    }
    saveChangesIfNotDelayed();
}

bool DB::ImageInfo::updateDateInformation( int mode ) const
{
    if ((mode & EXIFMODE_DATE) == 0)
        return false;

    if ( (mode & EXIFMODE_FORCE) != 0 )
        return true;

#ifdef HAVE_EXIV2
    return true;
#endif

    return Settings::SettingsData::instance()->trustTimeStamps();
}

