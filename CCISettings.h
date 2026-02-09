#pragma once

#include "SimpleIni.h"
#include <unordered_map>
#include <variant>

#define CCI_MAX_PATH MAX_PATH * 2

struct SettingString {
	const char* section;
	const char* key;
	const char* desc;
	const char* value;
};

struct SettingColor {
	const char* section;
	const char* key;
	const char* desc;
	COLORREF value;
};

struct SettingInt {
	const char* section;
	const char* key;
	const char* desc;
	int value;
};

struct SettingLong {
	const char* section;
	const char* key;
	const char* desc;
	long value;
};

/*
template<DataType dt>
struct SettingInfo;

template<>
struct SettingInfo<STRING> {
	const char* section;
	const char* key;
	const char* value;
};

template<>
struct SettingInfo<COLOR> {
	const char* section;
	const char* key;
	COLORREF value;
};

template<>
struct SettingInfo<INTEGER> {
	const char* section;
	const char* key;
	int value;
};

template<>
struct SettingInfo<TYPE_LONG> {
	const char* section;
	const char* key;
	long value;
};

namespace CCISettings {
	inline const SettingInfo<COLOR> ATCO_CLR_DEL = { "colors", "rgbDelivery", GLOBAL::ATCO_CLR_DEL };
	inline const SettingInfo<COLOR> ATCO_CLR_GND = { "colors", "rgbGround", GLOBAL::ATCO_CLR_GND };
	inline const SettingInfo<COLOR> ATCO_CLR_TWR = { "colors", "rgbTower", GLOBAL::ATCO_CLR_TWR };
	inline const SettingInfo<INTEGER> GRP_MAX_ZOOM = { "grp", "maxZoom", 600 };
	inline const SettingInfo<INTEGER> GRP_CLICK_RADIUS = { "grp", "clickRadius", 7 };
	inline const SettingInfo<INTEGER> GRP_LMB_ACTION = { "grp", "lmbAction", 0 };
	inline const SettingInfo<INTEGER> GRP_MMB_ACTION = { "grp", "mmbAction", 0 };
	inline const SettingInfo<INTEGER> GRP_RMB_ACTION = { "grp", "rmbAction", 0 };
	inline const SettingInfo<INTEGER> GRP_LMB_DBL_ACTION = { "grp", "lmbDblAction", 0 };
	inline const SettingInfo<INTEGER> GRP_MMB_DBL_ACTION = { "grp", "mmbDblAction", 0 };
	inline const SettingInfo<INTEGER> GRP_RMB_DBL_ACTION = { "grp", "rmbDblAction", 0 };
} // namespace CCISettings
*/

typedef enum class Settings {
	ATCO_CLR_DEL = 0,
	ATCO_CLR_GND,
	ATCO_CLR_TWR,
	GRP_MAX_ZOOM,
	GRP_CLICK_RADIUS,
	GRP_CCI_LMB_ACTION,
	GRP_CCI_MMB_ACTION,
	GRP_CCI_RMB_ACTION,
	GRP_CCI_LMB_DBL_ACTION,
	GRP_CCI_RMB_DBL_ACTION,
	GRP_RTARGET_LMB_ACTION,
	GRP_RTARGET_MMB_ACTION,
	GRP_RTARGET_RMB_ACTION,
	GRP_RTARGET_LMB_DBL_ACTION,
	GRP_RTARGET_RMB_DBL_ACTION,
	GLOBAL_VERSION
};

class CCISettings {
public:
	CCISettings(const char* iniFile);
	~CCISettings();

	void saveSettings();
	void justGet(Settings setting, void *dest, int size);
	COLORREF getColor(Settings setting);
	const char *getString(Settings setting);
	int getInt(Settings setting);
	long getLong(Settings setting);

protected:
	using Setting = std::variant<SettingColor, SettingString, SettingInt, SettingLong>;

	CSimpleIniA m_Ini;
	char m_IniFile[CCI_MAX_PATH];
	std::unordered_map<Settings, Setting> m_Settings;

	void loadSettings();
};
