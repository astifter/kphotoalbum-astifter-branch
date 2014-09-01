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
#include "CategoryItem.h"
#include "DB/ImageDB.h"
#include "DB/MemberMap.h"
#include <qdir.h>
#include <klocale.h>
#include <kmessagebox.h>
#include "MainWindow/Window.h"
#include "DB/CategoryCollection.h"
#include <QDebug>

Settings::CategoryItem::CategoryItem( const QString& category, const QString& text, const QString& icon,
                                      DB::Category::ViewType type, int thumbnailSize, QListWidget* parent,
                                      bool positionable )
    :QListWidgetItem( text, parent ),
     m_categoryOrig( category ), m_iconOrig( icon ),
     m_positionable( positionable ), m_positionableOrig( positionable ),
     m_category( category ), m_text(text), m_icon( icon ), m_type( type ), m_typeOrig( type ),
     m_thumbnailSize( thumbnailSize ), m_thumbnailSizeOrig( thumbnailSize )
{
    m_cToLocale = DB::ImageDB::instance()->categoryCollection()->categoryForName(category)->standardCategories();
    QMap<QString, QString>::iterator i;
    for (i = m_cToLocale.begin(); i != m_cToLocale.end(); ++i) {
        // "Persons" and "Locations" just exist for compatibility with older versions of KPA
        if (i.key() != QString::fromUtf8("Persons") and i.key() != QString::fromUtf8("Locations")) {
            m_localeToC[i.value()] = i.key();
        }
    }
}

void Settings::CategoryItem::setLabel( const QString& label )
{
    setText( label );
    m_category = label;
}

void Settings::CategoryItem::submit( DB::MemberMap* memberMap )
{
    if ( m_categoryOrig.isNull() ) {
        // New Item
        DB::ImageDB::instance()->categoryCollection()->addCategory( m_category, m_icon, m_type, m_thumbnailSize, true );
    } else {
        DB::CategoryPtr category = DB::ImageDB::instance()->categoryCollection()->categoryForName( m_categoryOrig );

        if (m_category != m_categoryOrig) {
            if (m_localeToC.keys().contains(m_category)) {
                if (m_localeToC[m_category] != m_categoryOrig) {
                    renameCategory(memberMap);
                }
            } else {
                renameCategory(memberMap);
            }
        }

        if ( m_positionable != m_positionableOrig )
            category->setPositionable( m_positionable );

        if ( m_icon != m_iconOrig )
            category->setIconName( m_icon );

        if ( m_type != m_typeOrig )
            category->setViewType( m_type );

        if ( m_thumbnailSize != m_thumbnailSizeOrig )
            category->setThumbnailSize( m_thumbnailSize );
    }

    m_categoryOrig = m_category;
    m_iconOrig = m_icon;
    m_typeOrig = m_typeOrig;
    m_thumbnailSizeOrig = m_thumbnailSize;
    m_positionableOrig = m_positionable;
}

void Settings::CategoryItem::removeFromDatabase()
{
    if ( !m_categoryOrig.isNull() ) {
        // the database knows about the item.
        DB::ImageDB::instance()->categoryCollection()->removeCategory( m_categoryOrig );
    }
}

QString Settings::CategoryItem::text() const
{
    return m_text;
}

bool Settings::CategoryItem::positionable() const
{
    return m_positionable;
}

void Settings::CategoryItem::setPositionable(bool positionable)
{
    m_positionable = positionable;
}

QString Settings::CategoryItem::icon() const
{
    return m_icon;
}

int Settings::CategoryItem::thumbnailSize() const
{
    return m_thumbnailSize;
}

DB::Category::ViewType Settings::CategoryItem::viewType() const
{
    return m_type;
}

void Settings::CategoryItem::setIcon( const QString& icon )
{
    m_icon = icon;
}

void Settings::CategoryItem::setThumbnailSize( int size )
{
    m_thumbnailSize = size;
}

void Settings::CategoryItem::setViewType( DB::Category::ViewType type )
{
    m_type = type;
}

void Settings::CategoryItem::renameCategory( DB::MemberMap* memberMap )
{
    QString txt = i18n("<p>Due to a shortcoming in KPhotoAlbum, you need to save your database after renaming categories; "
                       "otherwise all the filenames of the category thumbnails will be wrong, and thus lost.</p>"
                       "<p>So either press Cancel now (and it will not be renamed), or press Continue, and as your next "
                       "step save the database.</p>" );


    if ( KMessageBox::warningContinueCancel( ::MainWindow::Window::theMainWindow(), txt ) == KMessageBox::Cancel )
        return;

    QString categoryName = m_category;
    if (m_localeToC.keys().contains(categoryName)) {
        categoryName = m_localeToC[categoryName];
    }

    QDir dir( QString::fromLatin1("%1/CategoryImages" ).arg( Settings::SettingsData::instance()->imageDirectory() ) );
    const QStringList files = dir.entryList( QStringList() << QString::fromLatin1("%1*" ).arg( m_categoryOrig ) );
    for( QStringList::ConstIterator fileNameIt = files.begin(); fileNameIt != files.end(); ++fileNameIt ) {
        QString newName = categoryName + (*fileNameIt).mid( m_categoryOrig.length() );
        dir.rename( *fileNameIt, newName );
    }

    Settings::SettingsData* settings = Settings::SettingsData::instance();
    DB::ImageSearchInfo info = settings->currentLock();
    const bool exclude = settings->lockExcludes();
    info.renameCategory( m_categoryOrig, categoryName );
    settings->setCurrentLock( info, exclude );

    DB::ImageDB::instance()->categoryCollection()->rename(  m_categoryOrig, categoryName );
    memberMap->renameCategory(  m_categoryOrig, categoryName );
    m_categoryOrig = categoryName;
}


// vi:expandtab:tabstop=4 shiftwidth=4:
