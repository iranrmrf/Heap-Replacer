#include "settings.h"


settings::settings(const char* path)
{
	this->path = path;
	this->ini.LoadFile(this->path);
}

settings::~settings()
{
	this->save();
}

void settings::save()
{
	this->ini.SaveFile(this->path);
}

bool settings::load_bool(const char* section, const char* key, bool def_value)
{
	return ini.GetBoolValue(section, key, def_value);
}

long settings::load_long(const char* section, const char* key, long def_value)
{
	return ini.GetLongValue(section, key, def_value);
}

double settings::load_double(const char* section, const char* key, double def_value)
{
	return ini.GetDoubleValue(section, key, def_value);
}

const char* settings::load_str(const char* section, const char* key, const char* def_value)
{
	return ini.GetValue(section, key, def_value);
}

void settings::save_bool(const char* section, const char* key, bool value)
{
	ini.SetBoolValue(section, key, value);
}

void settings::save_long(const char* section, const char* key, long value)
{
	ini.SetLongValue(section, key, value);
}

void settings::save_double(const char* section, const char* key, double value)
{
	ini.SetDoubleValue(section, key, value);
}

void settings::save_str(const char* section, const char* key, const char* value)
{
	ini.SetValue(section, key, value);
}
