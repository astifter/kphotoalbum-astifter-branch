#ifndef OVERVIEWMODEL_H
#define OVERVIEWMODEL_H
#include "BrowserAction.h"
#include <DB/ImageSearchInfo.h>
#include <DB/Category.h>
#include <QAbstractListModel>
#include <DB/MediaCount.h>

namespace Browser {
class BrowserWidget;

class OverviewModel :public QAbstractListModel, public BrowserAction
{
public:
    OverviewModel( const DB::ImageSearchInfo& info, Browser::BrowserWidget* );
    OVERRIDE int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
    OVERRIDE QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    OVERRIDE void activate();
    OVERRIDE BrowserAction* generateChildAction( const QModelIndex& );
    OVERRIDE Qt::ItemFlags flags ( const QModelIndex & index ) const;

private:
    QList<DB::CategoryPtr> categories() const;

    bool isCategoryIndex( int row ) const;
    bool isExivIndex( int row ) const;
    bool isSearchIndex( int row ) const;
    bool isImageIndex( int row ) const;

    QVariant categoryInfo( int row, int role ) const;
    QVariant exivInfo( int role ) const;
    QVariant searchInfo( int role ) const;
    QVariant imageInfo( int role ) const;

private:
    DB::ImageSearchInfo _info;
    QMap<int,DB::MediaCount> _count;
};

}


#endif /* OVERVIEWMODEL_H */
