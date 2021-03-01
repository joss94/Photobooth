// ----------------------------------------------------------------------------- 
//  Copyright (c) <2017>-<2019>, IXBLUE. 
// ----------------------------------------------------------------------------- 
//  N O T I C E 
// 
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE. 
// ----------------------------------------------------------------------------- 

#pragma once

#include "JsonConfig.h"
#include "QMap"

struct photobooth_settings_t
{
	QString backgroundDir = "";
	QString picturesDir = "";
	QString modifPicturesDir = "";
	QString modelPath = "";
	QString scriptPath = "";
	QString cameraURL = "192.168.0.12:5000/takepicture";
	QString baseImagePath = "";
	int maxOccurence = 10;
	double baseOpacity = 0.3;

	void loadFromJson(const QJsonObject& json)
	{
		backgroundDir = json["backgrounds"].toString(backgroundDir);
		picturesDir = json["pictures"].toString(picturesDir);
		modifPicturesDir = json["modified_pictures"].toString(modifPicturesDir);
		modelPath = json["dnn_model"].toString(modifPicturesDir);
		scriptPath = json["script"].toString(modifPicturesDir);
		cameraURL = json["camera_url"].toString(cameraURL);
		baseImagePath = json["base_image"].toString(baseImagePath);
		maxOccurence = json["max_occurence"].toInt(maxOccurence);
		baseOpacity = json["base_opacity"].toDouble(baseOpacity);
	};

	QJsonObject toJson() const
	{
		QJsonObject v;

		v["backgrounds"] = backgroundDir;
		v["pictures"] = picturesDir;
		v["modified_pictures"] = modifPicturesDir;
		v["dnn_model"] = modelPath;
		v["script"] = scriptPath;
		v["camera_url"] = cameraURL;
		v["base_image"] = baseImagePath;
		v["max_occurence"] = maxOccurence;
		v["base_opacity"] = baseOpacity;

		return v;
	};
};
		
class PhotoboothSettings : public JsonConfig
{

public:

	/// @brief Constructor
	/// @param[in] QString name of the config file
	PhotoboothSettings(QString filename);

	/// @brief Get a constant reference to the config data structure
	/// @return const ra_config_t&
	const photobooth_settings_t& settings() const;

	void updateSettings(const photobooth_settings_t& settings);

private:

	photobooth_settings_t m_settings;
};