#pragma once

#include "EuroScopePlugIn.h"
#include <string_view>
#include <unordered_map>

using namespace EuroScopePlugIn;

enum GndStations { NONE, DELIVERY, GROUND, TOWER };

// Constant definitions
// Also some not-so-constant definitions .-.
namespace GLOBAL {
	CPlugIn* pPlugIn;

	const char* name = "Current Control Indicator";
	const char* version = "b1.0.5";
	const char* authors = "Michael Ott";
	const char* license = "GPL v3";
	const char* tag = "CCI";

	const char* fpListName = "Actively controlling";
	const char* screenName = "Active Control Display";
	const char* popupName = "ATC";

	// Tag item definitions
	const int TAG_ITEM_CCI = 0xA000;
	const char* TAG_ITEM_CCI_NAME = "Active Controller";

	// Tag item functions
	const int TAG_FUNC_XFER = 0xA000;
	const char* TAG_FUNC_XFER_NAME = "Transfer GND ctrl";

	const int TAG_FUNC_ASSUME = 0xAA00;
	const char* TAG_FUNC_ASSUME_NAME = "Assume GND ctrl";

	const int TAG_FUNC_RELEASE = 0xAAA0;
	const char* TAG_FUNC_RELEASE_NAME = "Release GND ctrl";

	const int TAG_FUNC_POPUP = 0xAAAA;
	const char* TAG_FUNC_POPUP_NAME = "Open GND ctrl popup";

	const int TAG_FUNC_ATCO_NONE = 0xA001;
	const char* TAG_FUNC_ATCO_NONE_NAME = "Set GND ctrl: NONE";

	const int TAG_FUNC_ATCO_DEL = 0xA002;
	const char* TAG_FUNC_ATCO_DEL_NAME = "Set GND ctrl: DELIVERY";

	const int TAG_FUNC_ATCO_GND = 0xA003;
	const char* TAG_FUNC_ATCO_GND_NAME = "Set GND ctrl: GROUND";

	const int TAG_FUNC_ATCO_TWR = 0xA004;
	const char* TAG_FUNC_ATCO_TWR_NAME = "Set GND ctrl: TOWER";

	const char* SETTING_ATCO_DEL_CLR = "atco_del_clr";
	const char* SETTING_ATCO_GND_CLR = "atco_gnd_clr";
	const char* SETTING_ATCO_TWR_CLR = "atco_twr_clr";

	COLORREF ATCO_CLR_DEL = 0x00FFAA00;
	COLORREF ATCO_CLR_GND = 0x0000FF00;
	COLORREF ATCO_CLR_TWR = 0x0000CCFF;
}

class CCI : public CPlugIn {
  public:
	CCI(void);

	void OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16],
					  int *pColorCode, COLORREF *pRGB, double *pFontSize);

	void OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area);

	void OnControllerPositionUpdate(CController Controller);

	void OnFlightPlanDisconnect(CFlightPlan FlightPlan);

	bool OnCompileCommand(const char *sCommandLine);

  protected:
	// TODO: If this EVER for some reson becomes anything sorted (like a vector or sth), make sure to NOT DO (2048) and instead use .reserve!
	std::unordered_map<std::string_view, GndStations> m_ATCMap = std::unordered_map<std::string_view, GndStations>(2048);
	CFlightPlanList m_FpList;
	GndStations m_MyStation = NONE;

	void setActiveATCO(const CFlightPlan &fpln, GndStations station);
	void openATCPopup(GndStations activeStation, RECT area);
	void gndStationToStr(GndStations station, char dest[4]);

	/// <returns>True if 'dest' was modified, false otherwise</returns>
	bool gndStationToClr(GndStations station, COLORREF *dest);
};

inline const char *clrToStr(COLORREF clr) {
	static char buffer[12];

	sprintf_s(buffer, sizeof(buffer), "%hhu,%hhu,%hhu", GetRValue(clr), GetGValue(clr), GetBValue(clr));

	return buffer;
}

inline COLORREF strToClr(const char *str) {
	if (!str)
		return 0x00000000;

	if (str[0] == '#')
		++str;

	BYTE r = 0, g = 0, b = 0;
	sscanf_s(str, "%hhu,%hhu,%hhu", &r, &g, &b);

	return RGB(r, g, b);
}
