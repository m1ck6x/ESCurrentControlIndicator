#pragma once

#include "EuroScopePlugIn.h"
#include "CCISettings.h"
#include <cstdio>
#include <cstring>
#include <string_view>
#include <unordered_map>

using namespace EuroScopePlugIn;

enum GndStations { NONE, DELIVERY, GROUND, TOWER };

class CCI : public CPlugIn {
public:
	CCI(void);

	CFlightPlanList m_FpList;
	GndStations m_MyStation = NONE;
	char m_ModCs[10]{0};

	void OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize);

	void OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area);

	void OnControllerPositionUpdate(CController Controller);

	void OnFlightPlanDisconnect(CFlightPlan FlightPlan);

	bool OnCompileCommand(const char *sCommandLine);

	CRadarScreen *OnRadarScreenCreated(const char *sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

	void setActiveATCO(const CFlightPlan &fpln, GndStations station);
	void setActiveATCO(const char* callsign, GndStations station);
	void openATCPopup(GndStations activeStation, RECT area);
	void openATCPopup(GndStations activeStation, RECT area, const char* callsign);
};

// Constant definitions
// Also some not-so-constant definitions .-.
inline namespace GLOBAL {
	inline CCI *pPlugIn;
	inline CCISettings *pSettings;

	inline const char *name = "Current Control Indicator";
	inline const char *version = "rc1.1.0";
	inline const char *authors = "Michael Ott";
	inline const char *license = "GPL v3";
	inline const char *tag = "CCI";

	inline const char *fpListName = "Actively controlling";
	inline const char *screenName = "Active Control Display";
	inline const char *popupName = "ATC";

	// Radar screen object types & actions
	inline const int OBJ_TYPE_CCI = 0xA000;
	inline const int OBJ_TYPE_RTARGET = 0xAA00;
	inline const int OBJ_ACTION_ASSUME = 1;
	inline const int OBJ_ACTION_RELEASE = 2;
	inline const int OBJ_ACTION_TRANSFER = 3;
	inline const int OBJ_ACTION_SET_NONE = 4;
	inline const int OBJ_ACTION_SET_DEL = 5;
	inline const int OBJ_ACTION_SET_GND = 6;
	inline const int OBJ_ACTION_SET_TWR = 7;
	inline const int OBJ_ACTION_OPEN_POPUP = 8;

	// Tag item definitions
	inline const int TAG_ITEM_CCI = 0xA000;
	inline const char *TAG_ITEM_CCI_NAME = "Active Controller";

	// Tag item functions
	inline const int TAG_FUNC_XFER = 0xA000;
	inline const char *TAG_FUNC_XFER_NAME = "Transfer GND ctrl";

	inline const int TAG_FUNC_ASSUME = 0xAA00;
	inline const char *TAG_FUNC_ASSUME_NAME = "Assume GND ctrl";

	inline const int TAG_FUNC_RELEASE = 0xAAA0;
	inline const char *TAG_FUNC_RELEASE_NAME = "Release GND ctrl";

	inline const int TAG_FUNC_POPUP = 0xAAAA;
	inline const char *TAG_FUNC_POPUP_NAME = "Open GND ctrl popup";

	inline const int TAG_FUNC_ATCO_NONE = 0xA001;
	inline const char *TAG_FUNC_ATCO_NONE_NAME = "Set GND ctrl: NONE";

	inline const int TAG_FUNC_ATCO_DEL = 0xA002;
	inline const char *TAG_FUNC_ATCO_DEL_NAME = "Set GND ctrl: DELIVERY";

	inline const int TAG_FUNC_ATCO_GND = 0xA003;
	inline const char *TAG_FUNC_ATCO_GND_NAME = "Set GND ctrl: GROUND";

	inline const int TAG_FUNC_ATCO_TWR = 0xA004;
	inline const char *TAG_FUNC_ATCO_TWR_NAME = "Set GND ctrl: TOWER";

	// TODO: If this EVER for some reson becomes anything sorted (like a vector or sth), make sure to NOT DO (2048) and
	// instead use .reserve!
	inline std::unordered_map<std::string_view, GndStations> m_ATCMap = std::unordered_map<std::string_view, GndStations>(2048);

	inline const char *ClrToStr(COLORREF clr) {
		static char buffer[12];

		sprintf_s(buffer, sizeof(buffer), "%hhu,%hhu,%hhu", GetRValue(clr), GetGValue(clr), GetBValue(clr));

		return buffer;
	}

	inline COLORREF StrToClr(const char *str) {
		if (!str)
			return 0x00000000;

		if (str[0] == '#')
			++str;

		BYTE r = 0, g = 0, b = 0;
		sscanf_s(str, "%hhu,%hhu,%hhu", &r, &g, &b);

		return RGB(r, g, b);
	}

	inline void GndStationToStr(GndStations station, char dest[4]) {
		switch (station) {
			case DELIVERY: {
				strcpy_s(dest, 4, "DEL");
				break;
			}

			case GROUND: {
				strcpy_s(dest, 4, "GND");
				break;
			}

			case TOWER: {
				strcpy_s(dest, 4, "TWR");
				break;
			}

			case NONE:
			default:
				break;
			}
	}

	/// <returns>True if 'dest' was modified, false otherwise</returns>
	inline bool GndStationToClr(GndStations station, COLORREF *dest) {
		switch (station) {
			case DELIVERY: {
				*dest = GLOBAL::pSettings->getColor(Settings::ATCO_CLR_DEL);
				return true;
			}

			case GROUND: {
				*dest = GLOBAL::pSettings->getColor(Settings::ATCO_CLR_GND);
				return true;
			}

			case TOWER: {
				*dest = GLOBAL::pSettings->getColor(Settings::ATCO_CLR_TWR);
				return true;
			}

			case NONE:
			default:
				return false;
			}
	}
} // namespace GLOBAL
