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
#include "DatabaseElement.h"
#include <qsqlquery.h>
#include <exiv2/exif.hpp>
#include <QVariant>
#include <QDebug>

#ifdef DEBUG_EXIF
#define Debug qDebug
#else
#define Debug if(0) qDebug
#endif

static QString replaceDotWithUnderscore( const char* cstr )
{
    QString str( QString::fromLatin1( cstr ) );
    return str.replace( QString::fromLatin1( "." ), QString::fromLatin1( "_" ) );
}

Exif::DatabaseElement::DatabaseElement()
    : m_value()
{
}

QVariant Exif::DatabaseElement::value() const
{
    return m_value;
}
void Exif::DatabaseElement::setValue( QVariant val )
{
    m_value = val;
}

Exif::StringExifElement::StringExifElement( const char* tag )
    : m_tag( tag )
{
}

QString Exif::StringExifElement::columnName() const
{
    return replaceDotWithUnderscore( m_tag );
}

QString Exif::StringExifElement::createString() const
{
    return QString::fromLatin1( "%1 string" ).arg( replaceDotWithUnderscore( m_tag ) );
}


QString Exif::StringExifElement::queryString() const
{
    return QString::fromLatin1( "?" );
}


void Exif::StringExifElement::bindValues( QSqlQuery* query, int& counter, Exiv2::ExifData& data ) const
{
    query->bindValue( counter++, QLatin1String(data[m_tag].toString().c_str() ) );
}

void Exif::StringExifElement::bindValues(QSqlQuery* query , int& counter)
{
    query->bindValue( counter++, 0, QSql::Out );
}


Exif::IntExifElement::IntExifElement( const char* tag )
    : m_tag( tag )
{
}

QString Exif::IntExifElement::columnName() const
{
    return replaceDotWithUnderscore( m_tag );
}

QString Exif::IntExifElement::createString() const
{
    return QString::fromLatin1( "%1 int" ).arg( replaceDotWithUnderscore( m_tag ) );
}


QString Exif::IntExifElement::queryString() const
{
    return QString::fromLatin1( "?" );
}


void Exif::IntExifElement::bindValues( QSqlQuery* query, int& counter, Exiv2::ExifData& data ) const
{
    if (data[m_tag].count() > 0)
        query->bindValue( counter++, (int) data[m_tag].toLong() );
    else
        query->bindValue( counter++, (int) 0 );
}

void Exif::IntExifElement::bindValues(QSqlQuery* query , int& counter)
{
    query->bindValue( counter++, 0, QSql::Out );
}


Exif::RationalExifElement::RationalExifElement( const char* tag )
    : m_tag( tag )
{
}

QString Exif::RationalExifElement::columnName() const
{
    return replaceDotWithUnderscore( m_tag );
}

QString Exif::RationalExifElement::createString() const
{
    return QString::fromLatin1( "%1 float" ).arg( replaceDotWithUnderscore( m_tag ) );
}

QString Exif::RationalExifElement::queryString() const
{
    return QString::fromLatin1( "?" );
}


void Exif::RationalExifElement::bindValues( QSqlQuery* query, int& counter, Exiv2::ExifData& data ) const
{
    double value;
    Exiv2::Exifdatum &tagDatum = data[m_tag];
    switch ( tagDatum.count() )
    {
    case 0: // empty
        value = -1.0;
    case 1: // "normal" rational
        value = 1.0 * tagDatum.toRational().first / tagDatum.toRational().second;
        break;
    case 3: // GPS lat/lon data:
    {
        value = 0.0;
        double divisor = 1.0;
        // hour / minute / second:
        for (int i=0 ; i < 4 ; i++ )
        {
            double nom = tagDatum.toRational(i).first;
            double denom = tagDatum.toRational(i).second;
            if ( denom == 0 )
                value += 0;
            else
                value += (nom / denom)/ divisor;
            divisor *= 60.0;
        }
    }
        break;
    default:
        // FIXME: there are at least the following other rational types:
        // whitepoints -> 2 components
        // YCbCrCoefficients -> 3 components (Coefficients for transformation from RGB to YCbCr image data. )
        // chromaticities -> 6 components
        qWarning() << "Exif rational data with " << tagDatum.count() << " components is not handled, yet!";
        value = -1.0;
    }
    query->bindValue( counter++, value);
}

void Exif::RationalExifElement::bindValues(QSqlQuery* query , int& counter)
{
    query->bindValue( counter++, 0, QSql::Out );
}

Exif::LensExifElement::LensExifElement()
    : m_tag("Exif.Photo.LensModel")
{
}

QString Exif::LensExifElement::columnName() const
{
    return replaceDotWithUnderscore( m_tag );
}

QString Exif::LensExifElement::createString() const
{
    return QString::fromLatin1( "%1 string" ).arg( replaceDotWithUnderscore( m_tag ) );
}


QString Exif::LensExifElement::queryString() const
{
    return QString::fromLatin1( "?" );
}


void Exif::LensExifElement::bindValues(QSqlQuery* query , int& counter)
{
    query->bindValue( counter++, 0, QSql::Out );
}

void Exif::LensExifElement::bindValues( QSqlQuery* query, int& counter, Exiv2::ExifData& data ) const
{
    QString value;
    for (Exiv2::ExifData::const_iterator it = data.begin(); it != data.end(); ++it)
    {
        const QString datum = QString::fromLatin1(it->key().c_str());

        // Exif.Photo.LensModel [Ascii]
        // Exif.Canon.LensModel [Ascii]
        // Exif.OlympusEq.LensModel [Ascii]
        if (datum.endsWith(QString::fromLatin1(".LensModel")))
        {
            Debug() << datum << ": " << it->toString().c_str();
            value = QString::fromUtf8(it->toString().c_str());
            // we can break here since Exif.Photo.LensModel should be bound first
            break;
        }

        // Exif.NikonLd3.LensIDNumber [Byte]
        // on Nikon cameras, this seems to provide better results than .Lens and .LensType
        // (i.e. it includes the lens manufacturer).
        if (datum.endsWith(QString::fromLatin1(".LensIDNumber")))
        {
            // ExifDatum::print() returns the interpreted value
            Debug() << datum << ": " << it->print(&data).c_str();
            value = QString::fromUtf8(it->print(&data).c_str());
            continue;
        }

        // Exif.Nikon3.LensType [Byte]
        // Exif.OlympusEq.LensType [Byte]
        // Exif.Panasonic.LensType [Ascii]
        // Exif.Pentax.LensType [Byte]
        // Exif.Samsung2.LensType [Short]
        if (datum.endsWith(QString::fromLatin1(".LensType")))
        {
            // ExifDatum::print() returns the interpreted value
            Debug() << datum << ": " << it->print(&data).c_str();
            // make sure this cannot overwrite LensIDNumber
            if (value.isEmpty())
            {
                value = QString::fromUtf8(it->print(&data).c_str());
            }
        }
    }

    Debug() << "bind value " << value;
    query->bindValue( counter++, value );
}

// vi:expandtab:tabstop=4 shiftwidth=4:
