// ----------------------------------------------------------------------------- 
//  Copyright (c) <2017>-<2019>, IXBLUE. 
// ----------------------------------------------------------------------------- 
//  N O T I C E 
// 
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE. 
// ----------------------------------------------------------------------------- 

#include "PhotoboothSettings.h"
#include <Windows.h>
#include <iostream>

PhotoboothSettings::PhotoboothSettings(QString filename) : JsonConfig(filename)
{
	if (_valid)
	{
		m_settings.loadFromJson(_root);
	}
}

const photobooth_settings_t& PhotoboothSettings::settings() const
{
	return m_settings;
}

void PhotoboothSettings::updateSettings(const photobooth_settings_t& newConf)
{
	_root = newConf.toJson();
	m_settings = newConf;
	saveConfig();
}