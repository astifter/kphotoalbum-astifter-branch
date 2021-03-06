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

#include "CategoryImageConfig.h"
#include <qlabel.h>
#include <qlayout.h>
#include <QPixmap>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QList>
#include <klocale.h>
#include <KComboBox>
#include "Settings/SettingsData.h"
#include "DB/CategoryCollection.h"
#include "DB/ImageInfo.h"
#include "DB/ImageDB.h"
#include "DB/MemberMap.h"
#include "Utilities/Util.h"

using Utilities::StringSet;

Viewer::CategoryImageConfig* Viewer::CategoryImageConfig::s_instance = nullptr;

Viewer::CategoryImageConfig::CategoryImageConfig()
    : m_image( QImage() )
{
    setWindowTitle( i18nc("@title:window","Configure Category Image") );
    setButtons( User1 | Close );
    setButtonText( User1, i18nc("@action:button As in 'Set the category image'","Set") );

    QWidget* top = new QWidget;
    setMainWidget( top );

    QVBoxLayout* lay1 = new QVBoxLayout( top );
    QGridLayout* lay2 = new QGridLayout;
    lay1->addLayout( lay2 );

    // Group
    QLabel* label = new QLabel( i18nc("@label:listbox As in 'select the tag category'","Category:" ), top );
    lay2->addWidget( label, 0, 0 );
    m_group = new KComboBox( top );
    lay2->addWidget( m_group, 0, 1 );
    connect( m_group, SIGNAL(activated(int)), this, SLOT(groupChanged()) );

    // Member
    label = new QLabel( i18nc("@label:listbox As in 'select a tag'", "Tag:" ), top );
    lay2->addWidget( label, 1, 0 );
    m_member = new KComboBox( top );
    lay2->addWidget( m_member, 1, 1 );
    connect( m_member, SIGNAL(activated(int)), this, SLOT(memberChanged()) );

    // Current Value
    QGridLayout* lay3 = new QGridLayout;
    lay1->addLayout( lay3 );
    label = new QLabel( i18nc("@label The current category image","Current image:"), top );
    lay3->addWidget( label, 0, 0 );

    m_current = new QLabel( top );
    m_current->setFixedSize( 128, 128 );
    lay3->addWidget( m_current, 0, 1 );

    // New Value
    m_imageLabel = new QLabel( i18nc("@label Preview of the new category imape", "New image:"), top );
    lay3->addWidget( m_imageLabel, 1, 0 );

    m_imageLabel = new QLabel( top );
    m_imageLabel->setFixedSize( 128, 128 );
    lay3->addWidget( m_imageLabel, 1, 1 );

    connect( this, SIGNAL(user1Clicked()), this, SLOT(slotSet()) );
}

void Viewer::CategoryImageConfig::groupChanged()
{
    QString categoryName = currentGroup();
    if (categoryName.isNull())
        return;

    QString currentText = m_member->currentText();
    m_member->clear();
    StringSet directMembers = m_info->itemsOfCategory(categoryName);

    StringSet set = directMembers;
    QMap<QString,StringSet> map =
        DB::ImageDB::instance()->memberMap().inverseMap(categoryName);
    for( StringSet::const_iterator directMembersIt = directMembers.begin();
         directMembersIt != directMembers.end(); ++directMembersIt ) {
        set += map[*directMembersIt];
    }

    QStringList list = set.toList();

    list.sort();
    m_member->addItems( list );
    int index = list.indexOf( currentText );
    if ( index != -1 )
        m_member->setCurrentIndex( index );

    memberChanged();
}

void Viewer::CategoryImageConfig::memberChanged()
{
    QString categoryName = currentGroup();
    if (categoryName.isNull())
        return;
    QPixmap pix =
        DB::ImageDB::instance()->categoryCollection()->categoryForName( categoryName )->
        categoryImage(categoryName, m_member->currentText(), 128, 128);
    m_current->setPixmap( pix );
}

void Viewer::CategoryImageConfig::slotSet()
{
    QString categoryName = currentGroup();
    if (categoryName.isNull())
        return;
    DB::ImageDB::instance()->categoryCollection()->categoryForName( categoryName )->
        setCategoryImage(categoryName, m_member->currentText(), m_image);
    memberChanged();
}

QString Viewer::CategoryImageConfig::currentGroup()
{
    int index = m_group->currentIndex();
    if (index == -1)
        return QString();
    return m_categoryNames[index];
}

void Viewer::CategoryImageConfig::setCurrentImage( const QImage& image, const DB::ImageInfoPtr& info )
{
    m_image = image;
    m_imageLabel->setPixmap( QPixmap::fromImage(image) );
    m_info = info;
    groupChanged();
}

Viewer::CategoryImageConfig* Viewer::CategoryImageConfig::instance()
{
    if ( !s_instance )
        s_instance = new CategoryImageConfig();
    return s_instance;
}

void Viewer::CategoryImageConfig::show()
{
    QString currentCategory = m_group->currentText();
    m_group->clear();
    m_categoryNames.clear();
     QList<DB::CategoryPtr> categories = DB::ImageDB::instance()->categoryCollection()->categories();
    int index = 0;
    int currentIndex = -1;
     for ( QList<DB::CategoryPtr>::ConstIterator categoryIt = categories.constBegin(); categoryIt != categories.constEnd(); ++categoryIt ) {
        if ( !(*categoryIt)->isSpecialCategory() ) {
            m_group->addItem((*categoryIt)->name());
            m_categoryNames.push_back((*categoryIt)->name());
            if ((*categoryIt)->name() == currentCategory)
                currentIndex = index;
            ++index;
        }
    }

    if ( currentIndex != -1 )
        m_group->setCurrentIndex( currentIndex );
    groupChanged();


    KDialog::show();
}

#include "CategoryImageConfig.moc"
// vi:expandtab:tabstop=4 shiftwidth=4:
