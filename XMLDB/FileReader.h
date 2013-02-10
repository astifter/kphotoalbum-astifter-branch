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
#ifndef XMLDB_FILEREADER_H
#define XMLDB_FILEREADER_H

#include <qdom.h>
#include "DB/ImageInfoPtr.h"
#include "DB/ImageInfo.h"
#include <QSharedPointer>
#include "XmlReader.h"


class QXmlStreamReader;

namespace XMLDB
{
class Database;
typedef QSharedPointer<XmlReader> ReaderPtr;

class FileReader
{

public:
    FileReader( Database* db ) : _db( db ), _nextStackId(1) {}
    void read( const QString& configFile );
    static QString unescape( const QString& );
    DB::StackID nextStackId() const { return _nextStackId; };

protected:
    void readTopNodeInConfigDocument( const QString& configFile, QDomElement top, QDomElement* options, QDomElement* images,
                                      QDomElement* blockList, QDomElement* memberGroups );
    void loadCategories( ReaderPtr reader );
    void loadImages( ReaderPtr reader );
    void loadBlockList( ReaderPtr reader );
    void loadMemberGroups( ReaderPtr reader );
    DB::ImageInfoPtr load( const DB::FileName& filename, ReaderPtr reader );
    ReaderPtr readConfigFile( const QString& configFile );

    void createSpecialCategories();

    void checkIfImagesAreSorted();
    void checkIfAllImagesHasSizeAttributes();

    // The parent widget information dialogs are displayed in.
    QWidget *messageParent();

private:
    Database* const _db;
    int _fileVersion;
    DB::StackID _nextStackId;

    // During profilation I found that it was rather expensive to look this up over and over again (once for each image)
    DB::CategoryPtr _folderCategory;
};

}

#endif /* XMLDB_FILEREADER_H */

