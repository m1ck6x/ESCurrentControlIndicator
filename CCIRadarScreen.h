#pragma once

#include "EuroScopePlugIn.h"
#include <cmath>

using namespace EuroScopePlugIn;

void SettingsUpdated();

class CCIRadarScreen : public CRadarScreen {
public:
	void OnAsrContentToBeClosed();
	void OnRefresh(HDC hDC, int Phase);
	void OnClickScreenObject(int ObjectType, const char *sObjectId, POINT Pt, RECT Area, int Button);
	void OnDoubleClickScreenObject(int ObjectType, const char *sObjectId, POINT Pt, RECT Area, int Button);

private:
	inline double getScreenNM() {
		RECT area = GetRadarArea();

		POINT tl = { area.top, area.left };
		POINT br = { area.bottom, area.right };

		CPosition ctl = ConvertCoordFromPixelToPosition(tl);
		CPosition cbr = ConvertCoordFromPixelToPosition(br);

		return ctl.DistanceTo(cbr);
	}

	inline int getZoomLevel() { return (std::round(getScreenNM() * 100.0) / 100.0) * 100.0; }

	void executeAction(const char* callsign, int action, RECT area);
	CPosition calculateIndicatorMeterOffset(double lat, double lon, double offset, double deg);
	CPosition getIndicatorOffset(CPosition pos, double offset, double zoomScale, double deg);
};
