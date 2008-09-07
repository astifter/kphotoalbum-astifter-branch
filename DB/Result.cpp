#include "Result.h"
#include "ResultId.h"

#include <QDebug>

DB::Result::ConstIterator::ConstIterator( const Result* result, int id )
    :_result(result), _id(id)
{
}

DB::Result::ConstIterator& DB::Result::ConstIterator::operator++()
{
    ++_id;
    return *this;
}

DB::ResultId DB::Result::ConstIterator::operator*()
{
    return _result->item(_id);
}

DB::Result::ConstIterator DB::Result::begin() const
{
    return DB::Result::ConstIterator(this,0);
}

DB::Result::ConstIterator DB::Result::end() const
{
    return DB::Result::ConstIterator( this, count() );
}

bool DB::Result::ConstIterator::operator!=( const ConstIterator& other )
{
    return _id != other._id;
}

DB::ResultId DB::Result::item(int index) const
{
    return DB::ResultId(_items[index], this );
}

int DB::Result::size() const
{
    return _items.size();
}

DB::Result::Result( const QList<int>& ids)
    :_items(ids)
{
}

DB::Result::Result()
{
}

DB::Result::~Result() 
{
}

void DB::Result::debug()
{
    qDebug() << "Count: " << count();
    qDebug() << _items;
}

void DB::Result::append( const DB::ResultId& id)
{
    _items.append(id.fileId());
}

void DB::Result::appendAll( const DB::Result& result)
{
    _items += result._items;
}


void DB::Result::prepend( const DB::ResultId& id)
{
    _items.prepend(id.fileId());
}

bool DB::Result::isEmpty() const
{
    return _items.isEmpty();
}

int DB::ResultPtr::count() const
{
    // This is a debug function from the KSharedPtr class
    Q_ASSERT( false );
    return -1;
}

DB::ResultPtr::ResultPtr( Result* ptr )
    : KSharedPtr<Result>( ptr )
{
}

DB::ConstResultPtr::ConstResultPtr( const Result* ptr )
    : KSharedPtr<const Result>( ptr )
{
}

