#ifndef SQLIMAGEDATERANGECOLLECTION_H
#define SQLIMAGEDATERANGECOLLECTION_H
#include "Data/ImageDateCollection.h"

class SQLImageDateCollection :public ImageDateCollection
{
public:
    virtual ImageCount count( const ImageDate& date );
    virtual QDateTime lowerLimit() const;
    virtual QDateTime upperLimit() const;
};


#endif /* SQLIMAGEDATERANGECOLLECTION_H */

