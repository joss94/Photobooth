// ----------------------------------------------------------------------------- 
//  Copyright (c) <2017>-<2019>, IXBLUE. 
// ----------------------------------------------------------------------------- 
//  N O T I C E 
// 
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE. 
// ----------------------------------------------------------------------------- 

#pragma once

#include "QString"
#include "QJsonDocument"
#include "QJsonObject"
#include "QJsonArray"

/// \class JsonConfig
		///
		/// @brief Base class for config files based on JSON format
class JsonConfig
{

public:

	/// @brief Constructor
	/// @param[in] QString filename Name of the config file
	JsonConfig(const QString& filename);

	~JsonConfig();

protected:

	/// @brief Reads the config file and stores values in local attributes
	/// @return bool True if config file was properly parsed
	bool readConfig();

	/// @brief Saves the current JSON document in the config file. 
	void saveConfig() const;

	/// @brief Invalidates the config file. Method generally called during the parse() of the file. 
	/// @param[in] error The error responsible for the invalidating of the file
	void invalidate(const QString& error);

	///Warns user about some odd parameters in config file
	///@param[in] error The reason of the warning
	void warning(const QString& error) const;

	/// @brief Checks validity of the file
	/// @return bool True if file is valid
	bool isValid() const;

protected:

	QString _filename;
	QJsonObject _root;

	bool _valid = true;
};