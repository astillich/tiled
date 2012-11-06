/*
 * objecttypes.cpp
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "objecttypes.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

namespace Tiled {
namespace Internal {

static void writeProperties(QXmlStreamWriter &writer, const Properties &properties)
{
    if (properties.isEmpty())
        return;

    writer.writeStartElement(QLatin1String("properties"));

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it) {
        writer.writeStartElement(QLatin1String("property"));
        writer.writeAttribute(QLatin1String("name"), it.key());

        const QString &value = it.value();
        if (value.contains(QLatin1Char('\n'))) {
            writer.writeCharacters(value);
        } else {
            writer.writeAttribute(QLatin1String("value"), it.value());
        }
        writer.writeEndElement();
    }

    writer.writeEndElement();
}

bool ObjectTypesWriter::writeObjectTypes(const QString &fileName,
                                         const ObjectTypes &objectTypes)
{
    mError.clear();

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        mError = QCoreApplication::translate(
                    "ObjectTypes", "Could not open file for writing.");
        return false;
    }

    QXmlStreamWriter writer(&file);

    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(1);

    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String("objecttypes"));

    foreach (const ObjectType &objectType, objectTypes) {
        writer.writeStartElement(QLatin1String("objecttype"));
        writer.writeAttribute(QLatin1String("name"), objectType.name);
        writer.writeAttribute(QLatin1String("color"), objectType.color.name());
        writeProperties(writer, objectType.properties);
        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();

    if (file.error() != QFile::NoError) {
        mError = file.errorString();
        return false;
    }

    return true;
}

static void readUnknownElement(QXmlStreamReader &reader)
{
    qDebug() << "Unknown element (fixme):" << reader.name()
             << " at line " << reader.lineNumber()
             << ", column " << reader.columnNumber();
    reader.skipCurrentElement();
}

static void readProperty(QXmlStreamReader &reader, Properties &properties)
{
    Q_ASSERT(reader.isStartElement() && reader.name() == QLatin1String("property"));

    const QXmlStreamAttributes atts = reader.attributes();
    QString propertyName = atts.value(QLatin1String("name")).toString();
    QString propertyValue = atts.value(QLatin1String("value")).toString();

    while (reader.readNext() != QXmlStreamReader::Invalid) {

        if (reader.isEndElement()) {
            break;
        } else if (reader.isCharacters() && !reader.isWhitespace()) {
            if (propertyValue.isEmpty())
                propertyValue = reader.text().toString();
        } else if (reader.isStartElement()) {
            readUnknownElement(reader);
        }
    }

    properties.insert(propertyName, propertyValue);
}

static void readProperties(QXmlStreamReader &reader, Properties &properties)
{
    Q_ASSERT(reader.isStartElement() && reader.name() == QLatin1String("properties"));

    while (reader.readNextStartElement()) {

        if (reader.name() == QLatin1String("property"))
            readProperty(reader, properties);
        else
            readUnknownElement(reader);
    }
}

ObjectTypes ObjectTypesReader::readObjectTypes(const QString &fileName)
{
    mError.clear();

    ObjectTypes objectTypes;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        mError = QCoreApplication::translate(
                    "ObjectTypes", "Could not open file.");
        return objectTypes;
    }

    QXmlStreamReader reader(&file);

    if (!reader.readNextStartElement() || reader.name() != QLatin1String("objecttypes")) {
        mError = QCoreApplication::translate(
                    "ObjectTypes", "File doesn't contain object types.");
        return objectTypes;
    }

    ObjectType type;
    while (reader.readNextStartElement()) {

        if (reader.name() == QLatin1String("objecttype")) {

            const QXmlStreamAttributes atts = reader.attributes();
            type.name = atts.value(QLatin1String("name")).toString();
            type.color = atts.value(QLatin1String("color")).toString();

            while (reader.readNextStartElement()) {

                if (reader.name() == QLatin1String("properties"))
                {
                    readProperties(reader, type.properties);
                    break;
                }
            }

            objectTypes.append(type);
        }

        reader.skipCurrentElement();
    }

    if (reader.hasError()) {
        mError = QCoreApplication::translate("ObjectTypes",
                                             "%3\n\nLine %1, column %2")
                .arg(reader.lineNumber())
                .arg(reader.columnNumber())
                .arg(reader.errorString());
        return objectTypes;
    }

    return objectTypes;
}

} // namespace Internal
} // namespace Tiled
