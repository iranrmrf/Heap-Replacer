#pragma once

#include "simpleini/SimpleIni.h"

class settings
{

private:

	CSimpleIniA ini;
	const char* path;

public:
	
	settings(const char* path);
	~settings();

	void save();

	bool load_bool(const char* section, const char* key, bool def_value = false);
	long load_long(const char* section, const char* key, long def_value = 0);
	double load_double(const char* section, const char* key, double def_value = 0.0);
	const char* load_str(const char* section, const char* key, const char* def_value = nullptr);

	void save_bool(const char* section, const char* key, bool value);
	void save_long(const char* section, const char* key, long value);
	void save_double(const char* section, const char* key, double value);
	void save_str(const char* section, const char* key, const char* value);

};
