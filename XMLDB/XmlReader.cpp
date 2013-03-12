/* Copyright (C) 2013 Jesper K. Pedersen <blackie@kde.org>

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
#include "XmlReader.h"
#include <KLocale>

namespace XMLDB {

XmlReader::XmlReader()
{
}

QString XmlReader::attribute( const QString& name, const QString& defaultValue )
{
    QStringRef ref = attributes().value(name);
    if ( ref.isNull() )
        return defaultValue;
    else
        return ref.toString();
}

ElementInfo XmlReader::readNextStartOrStopElement(const QString& expectedStart)
{
    if (m_peek.isValid) {
        m_peek.isValid = false;
        return m_peek;
    }

    TokenType type = readNextInternal();

    if ( hasError() )
        reportError(i18n("Error reading next element"));

    if ( type != StartElement && type != EndElement )
        reportError(i18n("Expected to read a start or stop element, but read %1").arg(tokenToString(type)));

    const QString elementName = name().toString();
    if ( type == StartElement ) {
        if ( !expectedStart.isNull() && elementName != expectedStart)
            reportError(i18n("Expected to read %1, but read %2").arg(expectedStart).arg(elementName));
    }

    return ElementInfo(type == StartElement, elementName);
}

void XmlReader::readEndElement()
{
    TokenType type = readNextInternal();
    if ( type != EndElement )
        reportError(i18n("Expected to read an end element but read an %2").arg(tokenToString(type)));
}

bool XmlReader::hasAttribute(const QString &name)
{
    return attributes().hasAttribute(name);
}

ElementInfo XmlReader::peekNext()
{
    if (m_peek.isValid)
        return m_peek;
    m_peek = readNextStartOrStopElement(QString());
    return m_peek;
}

void XmlReader::complainStartElementExpected(const QString &name)
{
    reportError(i18n("Expected to read start element '%1'").arg(name));
}

void XmlReader::reportError(const QString & text)
{
    QString message = i18n("On line %1, column %2:\n%3").arg(lineNumber()).arg(columnNumber()).arg(text);
    if ( hasError() )
        message += QString::fromUtf8("\n") + errorString();

    qFatal("%s", qPrintable(message));
}

QString XmlReader::tokenToString(QXmlStreamReader::TokenType type)
{
    switch ( type ) {
    case NoToken: return QString::fromUtf8("NoToken");
    case Invalid: return QString::fromUtf8("Invalid");
    case StartDocument: return QString::fromUtf8("StartDocument");
    case EndDocument: return QString::fromUtf8("EndDocument");
    case StartElement: return QString::fromUtf8("StartElement");
    case EndElement: return QString::fromUtf8("EndElement");
    case Characters: return QString::fromUtf8("Characters");
    case Comment: return QString::fromUtf8("Comment");
    case DTD: return QString::fromUtf8("DTD");
    case EntityReference: return QString::fromUtf8("EntityReference");
    case ProcessingInstruction: return QString::fromUtf8("ProcessingInstruction");

    }
    return QString();
}

QXmlStreamReader::TokenType XmlReader::readNextInternal()
{
    forever {
        TokenType type = readNext();
        if (type == Comment || type == StartDocument)
            continue;
        else if (type == Characters ) {
            if (isWhitespace())
                continue;
        }
        else
            return type;
    }
}

}
