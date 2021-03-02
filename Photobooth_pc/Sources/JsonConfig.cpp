// ----------------------------------------------------------------------------- 
//  Copyright (c) <2017>-<2019>, IXBLUE. 
// ----------------------------------------------------------------------------- 
//  N O T I C E 
// 
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE. 
// ----------------------------------------------------------------------------- 

#include "JsonConfig.h"
#include <fstream>
#include <iostream>
#include "QFile"

JsonConfig::JsonConfig(const QString& filename) : _filename(filename)
{
    readConfig();
}


JsonConfig::~JsonConfig()
{
	int i = _filename.lastIndexOf(".");
	QString tempName = _filename.left(i) + ".tmp";
    if (QFile::exists(tempName))
    {
        QFile::remove(tempName);
    }
}

bool JsonConfig::readConfig()
{
	QFile file(_filename);
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray content = file.readAll();

		QJsonParseError err;
		QJsonDocument doc = QJsonDocument::fromJson(content, &err);

		if (err.error != QJsonParseError::NoError)
		{
			invalidate(QString("Error parsing file : ") + _filename + " - " + err.errorString() + QString("   ") + QString::number(err.offset));
		}

		_root = doc.object();

		file.close();
	}
	else
	{
		invalidate(QString("Error opening file : ") + _filename);
	}

    return _valid;
}

void JsonConfig::saveConfig() const
{
    int i = _filename.lastIndexOf(".");
    QString tempName = _filename.left(i) + ".tmp";
	QFile tempFile(tempName);
	if (tempFile.open(QIODevice::ReadWrite))
	{
		QJsonDocument doc;
		doc.setObject(_root);

        tempFile.resize(0);
        tempFile.write(doc.toJson());
        tempFile.close();

        QFile::remove(_filename);
        QFile::rename(tempName, _filename);
	}
}

void JsonConfig::invalidate(const QString& error)
{
    warning(error);
    _valid = false;
}

void JsonConfig::warning(const QString& error) const
{
    qDebug() << error;
}

bool JsonConfig::isValid() const
{
    return _valid;
}