#include "pch.h"
#include "CCIRadarScreen.h"
#include "CCI.h"
#include <afxwin.h>
#include <cmath>

int ACTIONS[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void SettingsUpdated() {
	ACTIONS[0] = GLOBAL::pSettings->getInt(Settings::GRP_CCI_LMB_ACTION);
	ACTIONS[1] = GLOBAL::pSettings->getInt(Settings::GRP_CCI_MMB_ACTION);
	ACTIONS[2] = GLOBAL::pSettings->getInt(Settings::GRP_CCI_RMB_ACTION);
	ACTIONS[3] = GLOBAL::pSettings->getInt(Settings::GRP_CCI_LMB_DBL_ACTION);
	ACTIONS[4] = 0;
	ACTIONS[5] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_RMB_DBL_ACTION);
	ACTIONS[6] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_LMB_ACTION);
	ACTIONS[7] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_MMB_ACTION);
	ACTIONS[8] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_RMB_ACTION);
	ACTIONS[9] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_LMB_DBL_ACTION);
	ACTIONS[10] = 0;
	ACTIONS[11] = GLOBAL::pSettings->getInt(Settings::GRP_RTARGET_RMB_DBL_ACTION);
}

void CCIRadarScreen::OnAsrContentToBeClosed() {
	// TODO: Nothing.
}

void CCIRadarScreen::OnRefresh(HDC hDC, int Phase) {
	if (
		Phase != REFRESH_PHASE_AFTER_LISTS
		|| getZoomLevel() >= 600
	) return;

	CDC dc;
	dc.Attach(hDC);

	CRadarTarget rTarget = GLOBAL::pPlugIn->RadarTargetSelectFirst();

	while (rTarget.IsValid()) {
		POINT p = ConvertCoordFromPositionToPixel(rTarget.GetPosition().GetPosition());

		CRect areas[4] = {
			// Format: LEFT, TOP, RIGHT, BOTTOM

			// Top area
			{ p.x - 14, p.y - 14, p.x + 14, p.y - 5 },

			// Right area
			{ p.x + 5, p.y - 5, p.x + 14, p.y + 5 },

			// Bottom area
			{ p.x - 14, p.y + 5, p.x + 14, p.y + 14 },

			// Left area
			{ p.x - 14, p.y - 5, p.x - 5, p.y + 5 }
		};

		for (int i = 0; i < 4; ++i) {
			CRect& area = areas[i];

			/*dc.MoveTo(area.left, area.top);
			dc.LineTo(area.right, area.top);
			dc.LineTo(area.right, area.bottom);
			dc.LineTo(area.left, area.bottom);
			dc.LineTo(area.left, area.top);*/

			char buffer[16];
			sprintf_s(buffer, 16, "%d:%s", i, rTarget.GetCallsign());

			AddScreenObject(GLOBAL::OBJ_TYPE_RTARGET, buffer, area, false, buffer);
		}

		rTarget = GLOBAL::pPlugIn->RadarTargetSelectNext(rTarget);
	}

	if (GLOBAL::m_ATCMap.empty()) {
		dc.Detach();
		return;
	}

	LOGFONT lgfont;
	memset(&lgfont, 0, sizeof LOGFONT);
	strcpy_s(lgfont.lfFaceName, LF_FACESIZE, _TEXT("EuroScope"));
	lgfont.lfHeight = 15;
	lgfont.lfWeight = FW_NORMAL;
	lgfont.lfOutPrecision = OUT_TT_ONLY_PRECIS;

	CFont font;
	font.CreateFontIndirectA(&lgfont);
	CFont *oldFont = dc.SelectObject(&font);

	for (const auto &[callsign, station] : GLOBAL::m_ATCMap) {
		CRadarTarget target = GLOBAL::pPlugIn->RadarTargetSelect(callsign.data());
		CPosition targetPos = target.GetPosition().GetPosition();

		CPosition offset = getIndicatorOffset(targetPos, 20, 0.5, 180.0);
		POINT offsetPx = ConvertCoordFromPositionToPixel(offset);

		dc.SelectObject(&font);

		COLORREF clr = 0x00FFFFFF;
		// char buf[4] = "NON";

		GLOBAL::GndStationToClr(station, &clr);
		// GLOBAL::GndStationToStr(station, buf);

		// CSize txtSize = dc.GetTextExtent(buf);
		CSize txtSize = dc.GetTextExtent("A");

		CRect area;
		area.top = offsetPx.y;
		area.bottom = area.top + txtSize.cy;
		area.left = offsetPx.x - txtSize.cx;
		area.right = area.left + txtSize.cx;

		// dc.SetTextColor(clr);
		// dc.DrawText(buf, &area, DT_BOTTOM);
		dc.FillSolidRect(&area, clr);

		/*CPen pen(PS_SOLID, 1, clr);
		CPen *oldPen = dc.SelectObject(&pen);

		dc.MoveTo(area.left - 1, area.top - 1);
		dc.LineTo(area.right + 1, area.top - 1);
		dc.LineTo(area.right + 1, area.bottom + 1);
		dc.LineTo(area.left - 1, area.bottom + 1);
		dc.LineTo(area.left - 1, area.top - 1);

		dc.SelectObject(oldPen);*/

		AddScreenObject(
			GLOBAL::OBJ_TYPE_CCI,
			callsign.data(),
			area,
			true,
			callsign.data()
		);
	}

	dc.SelectObject(&oldFont);
	dc.Detach();
}

void CCIRadarScreen::OnClickScreenObject(int ObjectType, const char *sObjectId, POINT Pt, RECT Area, int Button) {
	if (ObjectType != GLOBAL::OBJ_TYPE_CCI && ObjectType != GLOBAL::OBJ_TYPE_RTARGET)
		return;

	const char*& callsign = sObjectId;
	if (ObjectType == GLOBAL::OBJ_TYPE_RTARGET)
		callsign += 2 * sizeof(char);

	int offset = ObjectType == GLOBAL::OBJ_TYPE_RTARGET ? 6 : 0;

	executeAction(callsign, ACTIONS[Button - 1 + offset], Area);
}

void CCIRadarScreen::OnDoubleClickScreenObject(int ObjectType, const char *sObjectId, POINT Pt, RECT Area, int Button) {
	if (ObjectType != GLOBAL::OBJ_TYPE_CCI && ObjectType != GLOBAL::OBJ_TYPE_RTARGET)
		return;

	const char*& callsign = sObjectId;
	if (ObjectType == GLOBAL::OBJ_TYPE_RTARGET)
		callsign += 2 * sizeof(char);

	int offset = 3 + (ObjectType == GLOBAL::OBJ_TYPE_RTARGET ? 6 : 0);

	executeAction(callsign, ACTIONS[Button - 1 + offset], Area);
}

void CCIRadarScreen::executeAction(const char* callsign, int action, RECT area) {
	switch (action) {
		case GLOBAL::OBJ_ACTION_ASSUME: {
			GLOBAL::pPlugIn->setActiveATCO(callsign, GLOBAL::pPlugIn->m_MyStation);
			break;
		}
									  
		case GLOBAL::OBJ_ACTION_RELEASE: {
			GLOBAL::pPlugIn->setActiveATCO(callsign, NONE);
			break;
		}
									   
		case GLOBAL::OBJ_ACTION_TRANSFER: {
			GLOBAL::pPlugIn->setActiveATCO(callsign, static_cast<GndStations>((GLOBAL::pPlugIn->m_MyStation + 1) % 4));
			break;
		}

		case GLOBAL::OBJ_ACTION_OPEN_POPUP: {
			GndStations activeStation = NONE;

			auto entry = GLOBAL::m_ATCMap.find(callsign);
			if (entry != GLOBAL::m_ATCMap.end())
				activeStation = entry->second;

			GLOBAL::pPlugIn->openATCPopup(activeStation, area, callsign);

			break;
		}
										
		case GLOBAL::OBJ_ACTION_SET_NONE:
		case GLOBAL::OBJ_ACTION_SET_DEL:					
		case GLOBAL::OBJ_ACTION_SET_GND:					
		case GLOBAL::OBJ_ACTION_SET_TWR: {
			GLOBAL::pPlugIn->setActiveATCO(callsign, static_cast<GndStations>((action - OBJ_ACTION_SET_NONE) + GLOBAL::TAG_FUNC_ATCO_NONE));
			break;
		}

		default:
			break;
	}
}

CPosition CCIRadarScreen::calculateIndicatorMeterOffset(double lat, double lon, double offset, double deg) {
	const double radius = 6378137.0;
	auto toRad = [](double num) -> double { return num * 3.1415926535 / 180; };
	auto toDeg = [](double num) -> double { return num * 180 / 3.1415926535; };

	const double latRad = toRad(lat);
	const double lonRad = toRad(lon);
	const double degRad = toRad(deg);

	const double delta = offset / radius;

	const double sinLatRad = std::sin(latRad);
	const double cosLatRad = std::cos(latRad);
	const double dSin = std::sin(delta);
	const double dCos = std::cos(delta);

	const double newLat = sinLatRad * dCos + cosLatRad * dSin * std::cos(degRad);
	const double asinNewLat = std::asin(newLat);

	const double y = std::sin(degRad) * dSin * cosLatRad;
	const double x = dCos - sinLatRad * newLat;
	double newLon = lonRad + std::atan2(y, x);

	newLon = std::fmod(toDeg(newLon) + 540, 360) - 180;

	CPosition newPos;
	newPos.m_Latitude = toDeg(asinNewLat);
	newPos.m_Longitude = newLon;

	return newPos;
}

CPosition CCIRadarScreen::getIndicatorOffset(CPosition pos, double offset, double zoomScale, double deg) {
	double probe = 10.0;
	const double eps = 1e-9;

	POINT p0 = ConvertCoordFromPositionToPixel(pos);

	CPosition nProbe = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, 0.0);
	CPosition eProbe = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, 90.0);
	CPosition sProbe = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, 180.0);
	CPosition wProbe = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, 270.0);

	POINT pN = ConvertCoordFromPositionToPixel(nProbe);
	POINT pE = ConvertCoordFromPositionToPixel(eProbe);
	POINT pS = ConvertCoordFromPositionToPixel(sProbe);
	POINT pW = ConvertCoordFromPositionToPixel(wProbe);

	const double ex = (static_cast<double>(pE.x) - static_cast<double>(pW.x)) / (2.0 * probe);
	const double ey = (static_cast<double>(pE.y) - static_cast<double>(pW.y)) / (2.0 * probe);
	const double nx = (static_cast<double>(pN.x) - static_cast<double>(pS.x)) / (2.0 * probe);
	const double ny = (static_cast<double>(pN.y) - static_cast<double>(pS.y)) / (2.0 * probe);

	// I really don't have time for that shit...
	// const double rad = deg * std::numbers::pi / 180.0;
	// This should be precise enough.
	const double rad = deg * 3.1415926535 / 180.0;
	const double vx = std::sin(rad);
	const double vy = -std::cos(rad);

	const double me = vx * ex + vy * ey;
	const double mn = vx * nx + vy * ny;

	double bearingDeg = std::atan2(me, mn) * 180.0 / 3.1415926535;
	bearingDeg = std::fmod(bearingDeg + 360.0, 360.0);

	CPosition probePos = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, bearingDeg);

	POINT p1 = ConvertCoordFromPositionToPixel(probePos);

	double dx = static_cast<double>(p1.x) - static_cast<double>(p0.x);
	double dy = static_cast<double>(p1.y) - static_cast<double>(p0.y);
	double alignment = dx * vx + dy * vy;

	const double flipThresholdPx = 1.0;
	if (alignment < -flipThresholdPx) {
		bearingDeg = std::fmod(bearingDeg + 180.0, 360.0);
		probePos = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, probe, bearingDeg);
		p1 = ConvertCoordFromPositionToPixel(probePos);
		double dx = static_cast<double>(p1.x) - static_cast<double>(p0.x);
		double dy = static_cast<double>(p1.y) - static_cast<double>(p0.y);
		double alignment = dx * vx + dy * vy;
	}

	alignment = max(alignment, eps);

	double pxPerMeter = alignment / probe;

	const double baseScale = max(pxPerMeter, eps);
	const double sharpness = 0.5;
	const double smoothScale = std::tanh(std::log(baseScale) / sharpness);

	offset *= std::exp(zoomScale * smoothScale);

	double targetMeters = (pxPerMeter > eps) ? (offset / pxPerMeter) : 0.0;

	CPosition tempPos = calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, targetMeters, bearingDeg);

	POINT p2 = ConvertCoordFromPositionToPixel(tempPos);
	const double dtx = static_cast<double>(p2.x) - static_cast<double>(p0.x);
	const double dty = static_cast<double>(p2.y) - static_cast<double>(p0.y);

	const double pxDist = dtx * vx + dty * vy;

	if (std::abs(pxDist) > eps) targetMeters *= (offset / pxDist);

	return calculateIndicatorMeterOffset(pos.m_Latitude, pos.m_Longitude, targetMeters, bearingDeg);
}
