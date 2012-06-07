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

#ifndef BACKGROUNDTASKS_JOBMODEL_H
#define BACKGROUNDTASKS_JOBMODEL_H

#include <QAbstractTableModel>
#include "JobInterface.h"
#include "JobInfo.h"

namespace BackgroundTasks {

class JobModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit JobModel(QObject *parent = 0);
    OVERRIDE int rowCount(const QModelIndex&) const;
    OVERRIDE int columnCount(const QModelIndex&) const;
    OVERRIDE QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    OVERRIDE QVariant headerData(int section, Qt::Orientation orientation, int role) const;

private slots:
    void jobEnded( JobInterface* job);
    void jobStarted( JobInterface* job);

private:
    enum Column { ActiveCol = 0, TitleCol = 1, DetailsCol = 2 };

    JobInfo info(int row) const;
    QString type( JobInfo::JobType type ) const;

    QList<JobInfo> m_previousJobs;
};

} // namespace BackgroundTasks

#endif // BACKGROUNDTASKS_JOBMODEL_H
