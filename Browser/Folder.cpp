/* Copyright (C) 2003-2005 Jesper K. Pedersen <blackie@kde.org>

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

#include "Folder.h"
#include <klocale.h>
#include "Settings/Settings.h"
#include "imagedb.h"

int Browser::Folder::_idCount = 0;

Browser::Folder::Folder( const ImageSearchInfo& info, Browser* parent )
    : _index(_idCount++), _browser( parent ), _info( info ), _enabled( true )
{
}


Browser::FolderAction::FolderAction(const ImageSearchInfo& info, Browser* browser )
    :  _browser( browser ), _info( info )
{
}

QString Browser::FolderAction::path() const
{
    return _info.toString();
}

int Browser::Folder::compare( Folder* other, int col, bool asc ) const
{
    Folder* o = static_cast<Folder*>( other );
    if ( !_browser->allowSort() ) {
        Folder* o = static_cast<Folder*>( other );
        if ( _index < o->_index )
            return (asc ? -1 : 1);
        else if ( _index > o->_index )
            return (asc ? 1 : -1);
        else
            return 0;
    }

    else if ( col == 0 )
        return text().compare( o->text() );

    else if ( col == 1 ) {
        if ( _count < o->_count )
            return -1;
        else
            return ( _count != o->_count);
    }
    else
        Q_ASSERT( false );
    return 0;
}

bool Browser::FolderAction::allowSort() const
{
    return true;
}

QString Browser::FolderAction::title() const
{
    return QString::fromLatin1( "" );
}

QString Browser::FolderAction::category() const
{
    return QString::null;
}

void Browser::Folder::setEnabled( bool b )
{
    _enabled = b;
}
