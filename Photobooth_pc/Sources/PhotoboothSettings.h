// ----------------------------------------------------------------------------- 
//  Copyright (c) <2017>-<2019>, IXBLUE. 
// ----------------------------------------------------------------------------- 
//  N O T I C E 
// 
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE. 
// ----------------------------------------------------------------------------- 

#pragma once

#include "QApplication"

#include "JsonConfig.h"
#include "QMap"

struct photobooth_settings_t
{
	QString modelPath = QCoreApplication::applicationDirPath() + "/people_detector/frozen_inference_graph.pb";
	QString scriptPath = QCoreApplication::applicationDirPath() + "/people_detector/inference/inference.exe";
	QString cameraURL = "192.168.0.12:5000/takepicture";
	bool autoIp = true;
	QString baseImagePath = "";
	int maxOccurence = 10;
	double baseOpacity = 0.3;
	int mosaicWidth = 86;
	int mosaicHeight = 52;
	int displaySpeed = 10;
	int delayBeforeMosaic = 10000;
	bool secondButton = false;
	bool showConsole = false;

	void loadFromJson(const QJsonObject& json)
	{
		cameraURL = json["camera_url"].toString(cameraURL);
		autoIp = json["auto_ip"].toBool(autoIp);
		baseImagePath = json["base_image"].toString(baseImagePath);
		maxOccurence = json["max_occurence"].toInt(maxOccurence);
		baseOpacity = json["base_opacity"].toDouble(baseOpacity);
		mosaicWidth = json["mosaic_width"].toInt(mosaicWidth);
		mosaicHeight = json["mosaic_height"].toInt(mosaicHeight);
		displaySpeed = json["display_speed"].toInt(displaySpeed);
		delayBeforeMosaic = json["delay_mosaic"].toInt(delayBeforeMosaic);
		secondButton = json["second_button"].toBool(secondButton);
		showConsole = json["show_console"].toBool(showConsole);
	};

	QJsonObject toJson() const
	{
		QJsonObject v;

		v["camera_url"] = cameraURL;
		v["auto_ip"] = autoIp;
		v["base_image"] = baseImagePath;
		v["max_occurence"] = maxOccurence;
		v["base_opacity"] = baseOpacity;
		v["mosaic_width"] = mosaicWidth;
		v["mosaic_height"] = mosaicHeight;
		v["display_speed"] = displaySpeed;
		v["delay_mosaic"] = delayBeforeMosaic;
		v["second_button"] = secondButton;
		v["show_console"] = showConsole;

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