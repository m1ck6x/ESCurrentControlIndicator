#include "pch.h"
#include "CCISettings.h"
#include "CCI.h"
#include "CCIRadarScreen.h"
#include <Shlwapi.h>
#include <filesystem>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

CCISettings::CCISettings(const char *iniFile) {
	// This is called "cheating". But I don't care. It's C++, after all .-.
	GLOBAL::pSettings = this;

	char path[MAX_PATH + 1]{0};
	GetModuleFileNameA((HINSTANCE)&__ImageBase, path, MAX_PATH);
	PathRemoveFileSpecA(path);
	std::filesystem::path basePath = path;
	basePath.append(iniFile);

	strcpy_s(m_IniFile, CCI_MAX_PATH, basePath.string().c_str());

	const char* descClr = "; Format: R,G,B (in range 0-255, both inclusive)";
	const char* descAction = "; 0-8: 0 = No action, 1 = Assume, 2 = Release, 3 = Transfer, 4 = Set CTRL none, 5 = Set CTRL DEL, 6 = Set CTRL GND, 7 = Set CTRL TWR, 8 = Open CTRL popup";

	m_Settings = {
		{ Settings::ATCO_CLR_DEL, SettingColor{ "colors", "rgbDelivery", descClr, 0x00FFAA00 } },
		{ Settings::ATCO_CLR_GND, SettingColor{ "colors", "rgbGround", descClr, 0x0000FF00 } },
		{ Settings::ATCO_CLR_TWR, SettingColor{ "colors", "rgbTower", descClr, 0x0000CCFF } },
		{ Settings::GRP_MAX_ZOOM, SettingInt{ "grp", "maxZoom", "; Maximum zoom where the active controller indicator will still be drawn", 600 } },
		{ Settings::GRP_CLICK_RADIUS, SettingInt{ "grp", "clickRadius", "; Defines the size (in pixels) of the interactable area around an acft on the ground radar", 7 } },
		{ Settings::GRP_CCI_LMB_ACTION, SettingInt{ "grp", "lmbIndicatorAction", descAction, GLOBAL::OBJ_ACTION_ASSUME } },
		{ Settings::GRP_CCI_MMB_ACTION, SettingInt{ "grp", "mmbIndicatorAction", descAction, GLOBAL::OBJ_ACTION_TRANSFER } },
		{ Settings::GRP_CCI_RMB_ACTION, SettingInt{ "grp", "rmbIndicatorAction", descAction, GLOBAL::OBJ_ACTION_RELEASE } },
		{ Settings::GRP_CCI_LMB_DBL_ACTION, SettingInt{ "grp", "lmbIndicatorDblAction", descAction, 0 } },
		{ Settings::GRP_CCI_RMB_DBL_ACTION, SettingInt{ "grp", "rmbIndicatorDblAction", descAction, 0 } },
		{ Settings::GRP_RTARGET_LMB_ACTION, SettingInt{ "grp", "lmbPlaneAction", descAction, GLOBAL::OBJ_ACTION_ASSUME } },
		{ Settings::GRP_RTARGET_MMB_ACTION, SettingInt{ "grp", "mmbPlaneAction", descAction, GLOBAL::OBJ_ACTION_TRANSFER } },
		{ Settings::GRP_RTARGET_RMB_ACTION, SettingInt{ "grp", "rmbPlaneAction", descAction, GLOBAL::OBJ_ACTION_RELEASE } },
		{ Settings::GRP_RTARGET_LMB_DBL_ACTION, SettingInt{ "grp", "lmbPlaneDblAction", descAction, 0 } },
		{ Settings::GRP_RTARGET_RMB_DBL_ACTION, SettingInt{ "grp", "rmbPlaneDblAction", descAction, GLOBAL::OBJ_ACTION_OPEN_POPUP } },
		{ Settings::GLOBAL_VERSION, SettingString{ "global", "version", "; Please don't ever change this. Thanks :)", GLOBAL::version } }
	};

	m_Ini.SetUnicode();

	if (!PathFileExistsA(m_IniFile)) {
		saveSettings();
		SettingsUpdated();
		return;
	}

	SI_Error err = m_Ini.LoadFile(m_IniFile);
	if (err < 0) {
		GLOBAL::pPlugIn->DisplayUserMessage(GLOBAL::tag, GLOBAL::tag, "Failed to load config file!", true, true, true, true, true);
		return;
	}

	loadSettings();
}

CCISettings::~CCISettings() { saveSettings(); }

void CCISettings::saveSettings() {
	for (std::pair<const Settings, Setting>& pair : m_Settings) {
		if (SettingColor* data = std::get_if<SettingColor>(&pair.second)) {
			m_Ini.SetValue(data->section, data->key, GLOBAL::ClrToStr(data->value), data->desc);
		} else if (SettingString* data = std::get_if<SettingString>(&pair.second)) {
			m_Ini.SetValue(data->section, data->key, data->value, data->desc);
		} else if (SettingInt* data = std::get_if<SettingInt>(&pair.second)) {
			m_Ini.SetLongValue(data->section, data->key, data->value, data->desc);
		} else if (SettingLong* data = std::get_if<SettingLong>(&pair.second)) {
			m_Ini.SetLongValue(data->section, data->key, data->value, data->desc);
		}
	}

	// No failsafe. This surely works. All the time. Definetly. Yes yes .-.
	m_Ini.SaveFile(m_IniFile);

	GLOBAL::pPlugIn->DisplayUserMessage(GLOBAL::tag, GLOBAL::tag, "Saved config file", true, true, false, false, false);
}

void CCISettings::justGet(Settings setting, void *dest, int size) {
	if (!m_Settings.contains(setting))
		return;

	if (SettingColor* data = std::get_if<SettingColor>(&m_Settings.at(setting))) {
		memcpy_s(dest, size, &data->value, sizeof(COLORREF));
	} else if (SettingString* data = std::get_if<SettingString>(&m_Settings.at(setting))) {
		memcpy_s(dest, size, &data->value, strlen(data->value));
	} else if (SettingInt* data = std::get_if<SettingInt>(&m_Settings.at(setting))) {
		memcpy_s(dest, size, &data->value, sizeof(int));
	} else if (SettingLong* data = std::get_if<SettingLong>(&m_Settings.at(setting))) {
		memcpy_s(dest, size, &data->value, sizeof(long));
	}
}

COLORREF CCISettings::getColor(Settings setting) {
	if (!m_Settings.contains(setting))
		return 0x00FFFFFF;

	if (SettingColor* data = std::get_if<SettingColor>(&m_Settings.at(setting)))
		return data->value;

	return 0x00FFFFFF;
}

const char *CCISettings::getString(Settings setting) {
	if (!m_Settings.contains(setting))
		return "";

	if (SettingString *data = std::get_if<SettingString>(&m_Settings.at(setting)))
		return data->value;

	return "";
}

int CCISettings::getInt(Settings setting) {
	if (!m_Settings.contains(setting))
		return 0;

	if (SettingInt *data = std::get_if<SettingInt>(&m_Settings.at(setting)))
		return data->value;

	return 0;
}

long CCISettings::getLong(Settings setting) {
	if (!m_Settings.contains(setting))
		return 0L;

	if (SettingLong *data = std::get_if<SettingLong>(&m_Settings.at(setting)))
		return data->value;

	return 0L;
}

void CCISettings::loadSettings() {
	for (std::pair<const Settings, Setting>& pair : m_Settings) {
		if (SettingColor* data = std::get_if<SettingColor>(&pair.second)) {
			data->value = GLOBAL::StrToClr(m_Ini.GetValue(data->section, data->key, GLOBAL::ClrToStr(data->value)));
		} else if (SettingString* data = std::get_if<SettingString>(&pair.second)) {
			data->value = m_Ini.GetValue(data->section, data->key, data->value);
		} else if (SettingInt* data = std::get_if<SettingInt>(&pair.second)) {
			data->value = m_Ini.GetLongValue(data->section, data->key, data->value);
		} else if (SettingLong* data = std::get_if<SettingLong>(&pair.second)) {
			data->value = m_Ini.GetLongValue(data->section, data->key, data->value);
		}
	}

	if (SettingString *data = std::get_if<SettingString>(&m_Settings.at(Settings::GLOBAL_VERSION))) {
		if (strcmp(data->value, GLOBAL::version) != 0)
			GLOBAL::pPlugIn->DisplayUserMessage(GLOBAL::tag, GLOBAL::tag, "The plugin is outdated! Please update using the installer (or manually).", true, true, true, true, true);
	}

	SettingsUpdated();
}
