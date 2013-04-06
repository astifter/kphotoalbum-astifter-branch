/* Copyright 2012 Jesper K. Pedersen <blackie@kde.org>
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLUGINS_UPLOADWIDGET_H
#define PLUGINS_UPLOADWIDGET_H

#include <libkipi/imagecollection.h>
#include <libkipi/uploadwidget.h>
class QFileSystemModel;
class QModelIndex;

namespace Plugins {

class UploadWidget : public KIPI::UploadWidget
{
    Q_OBJECT

public:
    UploadWidget(QWidget* parent);
    KIPI::ImageCollection selectedImageCollection() const;

private slots:
    void newIndexSelected(const QModelIndex& index);

private:
    QFileSystemModel* m_model;
    QString m_path;
};

} // namespace Plugins

#endif // PLUGINS_UPLOADWIDGET_H
// vi:expandtab:tabstop=4 shiftwidth=4:
