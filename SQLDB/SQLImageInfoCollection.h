/*
  Copyright (C) 2006 Tuomas Suutari <thsuut@utu.fi>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program (see the file COPYING); if not, write to the
  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
  MA 02110-1301 USA.
*/

#ifndef SQLIMAGEINFOCOLLECTION_H
#define SQLIMAGEINFOCOLLECTION_H

#include "DB/ImageInfoPtr.h"
#include "Connection.h"
#include "QueryHelper.h"
#include <qobject.h>
#include <qmap.h>
//#include <qmutex.h>
namespace DB { class Category; }

namespace SQLDB {
    class SQLImageInfoCollection: public QObject
    {
        Q_OBJECT

    public:
        explicit SQLImageInfoCollection(Connection& connection);
        ~SQLImageInfoCollection();
        DB::ImageInfoPtr getImageInfoOf(const QString& relativeFilename) const;
        void clearCache();

    public slots:
        void deleteTag(DB::Category* category, const QString& item);
        void renameTag(DB::Category* category,
                       const QString& oldName, const QString& newName);

    protected:
        QueryHelper _qh;

    private:
        mutable QMap<int, DB::ImageInfoPtr> _infoPointers;
        // mutable QMutex _mutex;
    };
}

#endif /* SQLIMAGEINFOCOLLECTION_H */
