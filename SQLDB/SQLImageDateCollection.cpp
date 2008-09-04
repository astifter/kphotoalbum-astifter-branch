#include "SQLImageDateCollection.h"
#include <qvariant.h>
#include <qmap.h>
#include "QueryHelper.h"

using namespace SQLDB;

SQLImageDateCollection::SQLImageDateCollection(QueryHelper& queryHelper):
    _qh(queryHelper)
{
}

DB::ImageCount SQLImageDateCollection::count( const DB::ImageDate& range )
{
    // In a perfect world, we should check that the db hasn't changed, but
    // as we will get a new instance of this class each time the search
    // changes, it is really not that important, esp. because it is only
    // for the datebar, where a bit out of sync doens't matter too much.

    static QMap<DB::ImageDate, DB::ImageCount> cache;
    if ( cache.contains( range ) )
        return cache[range];

    int exact =
        _qh.executeQuery(QString::fromLatin1("SELECT COUNT(*) FROM file "
                                             "WHERE ?<=time_start AND time_end<=?"),
                         QueryHelper::Bindings() <<
                         range.start() << range.end()
                         ).firstItem().toInt();
    int rng =
        _qh.executeQuery(QString::fromLatin1("SELECT COUNT(*) FROM file "
                                             "WHERE ?<=time_end AND time_start<=?"),
                         QueryHelper::Bindings() <<
                         range.start() << range.end()
                         ).firstItem().toInt() - exact;
    DB::ImageCount result( exact, rng );
    cache.insert( range, result );
    return result;
}

QDateTime SQLImageDateCollection::lowerLimit() const
{
    static QDateTime cachedLower;
    if (cachedLower.isNull())
        cachedLower = _qh.executeQuery(QString::fromLatin1("SELECT MIN(time_start) FROM file")
                                       ).firstItem().toDateTime();
    return cachedLower;
}

QDateTime SQLImageDateCollection::upperLimit() const
{
    static QDateTime cachedUpper;
    if (cachedUpper.isNull())
        cachedUpper = _qh.executeQuery(QString::fromLatin1("SELECT MAX(time_end) FROM file")
                                       ).firstItem().toDateTime();
    return cachedUpper;
}
