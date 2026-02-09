#include "pch.h"
#include "CCI.h"
#include "CCIRadarScreen.h"
#include <string>
#include <format>

CCI::CCI(void) : CPlugIn(COMPATIBILITY_CODE, GLOBAL::name, GLOBAL::version, GLOBAL::authors, GLOBAL::license) {
	RegisterTagItemType(GLOBAL::TAG_ITEM_CCI_NAME, GLOBAL::TAG_ITEM_CCI);

	RegisterTagItemFunction(GLOBAL::TAG_FUNC_XFER_NAME, GLOBAL::TAG_FUNC_XFER);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_ASSUME_NAME, GLOBAL::TAG_FUNC_ASSUME);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_RELEASE_NAME, GLOBAL::TAG_FUNC_RELEASE);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_POPUP_NAME, GLOBAL::TAG_FUNC_POPUP);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_ATCO_NONE_NAME, GLOBAL::TAG_FUNC_ATCO_NONE);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_ATCO_DEL_NAME, GLOBAL::TAG_FUNC_ATCO_DEL);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_ATCO_GND_NAME, GLOBAL::TAG_FUNC_ATCO_GND);
	RegisterTagItemFunction(GLOBAL::TAG_FUNC_ATCO_TWR_NAME, GLOBAL::TAG_FUNC_ATCO_TWR);

	// Registering actively controlled acft list with callsign and active ground controller.
	// When left-clicking, the control shall be transferred by default.
	// When right-clicking, the control shall be released by default.
	m_FpList = RegisterFpList(GLOBAL::fpListName);
	if (m_FpList.GetColumnNumber() == 0) {
		m_FpList.DeleteAllColumns();

		m_FpList.AddColumnDefinition("C/S", 10, false, NULL, TAG_ITEM_TYPE_CALLSIGN, NULL, TAG_ITEM_FUNCTION_NO, NULL,
									 TAG_ITEM_FUNCTION_NO);
		m_FpList.AddColumnDefinition("ATC", 3, false, GLOBAL::name, GLOBAL::TAG_ITEM_CCI, GLOBAL::name,
									 GLOBAL::TAG_FUNC_XFER, GLOBAL::name, GLOBAL::TAG_FUNC_RELEASE);
	}

	DisplayUserMessage(GLOBAL::tag, GLOBAL::tag, std::format("Current Control Indicator ({}) loaded successfully!", GLOBAL::version).c_str(), true, true, false, false,
					   false);
}

void __declspec(dllexport) EuroScopePlugInInit(CPlugIn **ppPlugInInstance) {
	*ppPlugInInstance = GLOBAL::pPlugIn = new CCI();
	GLOBAL::pSettings = new CCISettings("cciConfig.ini");
}

void __declspec(dllexport) EuroScopePlugInExit(void) {
	delete GLOBAL::pSettings;
	delete GLOBAL::pPlugIn;
}

void CCI::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData,
							 char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize) {
	if (ItemCode == GLOBAL::TAG_ITEM_CCI) {
		auto activeStation = GLOBAL::m_ATCMap.find(RadarTarget.GetCallsign());
		if (activeStation != GLOBAL::m_ATCMap.end()) {
			GLOBAL::GndStationToStr(activeStation->second, sItemString);

			if (GLOBAL::GndStationToClr(activeStation->second, pRGB))
				*pColorCode = TAG_COLOR_RGB_DEFINED;
		}
	}
}

void CCI::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area) {
	if (
		FunctionId == GLOBAL::TAG_FUNC_ATCO_NONE
		|| FunctionId == GLOBAL::TAG_FUNC_ATCO_DEL
		|| FunctionId == GLOBAL::TAG_FUNC_ATCO_GND
		|| FunctionId == GLOBAL::TAG_FUNC_ATCO_TWR
	) {
		if (strnlen_s(m_ModCs, 10) == 0) {
			CRadarTarget rTarget = RadarTargetSelectASEL();
			if (!rTarget.IsValid() || !rTarget.GetCorrelatedFlightPlan().IsValid())
				return;

			CFlightPlan fpln = rTarget.GetCorrelatedFlightPlan();
			if (fpln.IsValid())
				setActiveATCO(fpln, static_cast<GndStations>(FunctionId - GLOBAL::TAG_FUNC_ATCO_NONE));
		} else {
			CFlightPlan fpln = FlightPlanSelect(m_ModCs);
			if (fpln.IsValid())
				setActiveATCO(fpln, static_cast<GndStations>(FunctionId - GLOBAL::TAG_FUNC_ATCO_NONE));

			memset(m_ModCs, 0, 10);
		}

		return;
	}

	CRadarTarget rTarget = RadarTargetSelectASEL();
	if (!rTarget.IsValid() || !rTarget.GetCorrelatedFlightPlan().IsValid())
		return;

	switch (FunctionId) {
		case GLOBAL::TAG_FUNC_ASSUME: {
			if (m_MyStation != NONE)
				setActiveATCO(rTarget.GetCorrelatedFlightPlan(), m_MyStation);

			break;
		}

		case GLOBAL::TAG_FUNC_RELEASE: {
			setActiveATCO(rTarget.GetCorrelatedFlightPlan(), NONE);
			break;
		}

		case GLOBAL::TAG_FUNC_XFER: {
			setActiveATCO(rTarget.GetCorrelatedFlightPlan(), static_cast<GndStations>((m_MyStation + 1) % 4));
			break;
		}

		case GLOBAL::TAG_FUNC_POPUP: {
			GndStations activeStation = NONE;

			auto entry = GLOBAL::m_ATCMap.find(rTarget.GetCallsign());
			if (entry != GLOBAL::m_ATCMap.end())
				activeStation = entry->second;

			openATCPopup(activeStation, Area);

			break;
		}
	}
}

void CCI::OnControllerPositionUpdate(CController Controller) {
	GndStations oldStation = m_MyStation;

	if (Controller.IsValid() && Controller.IsController() &&
		strcmp(Controller.GetCallsign(), ControllerMyself().GetCallsign()) == 0) {
		int fac = Controller.GetFacility();

		if (fac < 2 || fac > 4) {
			m_MyStation = NONE;
			return;
		}

		m_MyStation = static_cast<GndStations>(fac - 1);

		char atcBuf[4];
		GLOBAL::GndStationToStr(m_MyStation, atcBuf);
	}

	if (m_MyStation != oldStation)
		GLOBAL::m_ATCMap.clear();
}

void CCI::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	if (GLOBAL::m_ATCMap.contains(FlightPlan.GetCallsign())) {
		m_FpList.RemoveFpFromTheList(FlightPlan);
		GLOBAL::m_ATCMap.erase(FlightPlan.GetCallsign());
	}
}

bool CCI::OnCompileCommand(const char *sCommandLine) {
	if (strcmp(sCommandLine, ".gndctrllist") == 0) {
		m_FpList.ShowFpList(true);
		return true;
	}

	return false;
}

CRadarScreen *CCI::OnRadarScreenCreated(const char *sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) {
	return new CCIRadarScreen();
}

void CCI::setActiveATCO(const CFlightPlan &fpln, GndStations station) {
	if (!fpln.IsValid())
		return;

	GLOBAL::m_ATCMap.erase(fpln.GetCallsign());
	m_FpList.RemoveFpFromTheList(fpln);

	switch (station) {
		case DELIVERY:
		case GROUND:
		case TOWER: {
			if (station == m_MyStation)
				m_FpList.AddFpToTheList(fpln);

			GLOBAL::m_ATCMap.insert(std::make_pair(fpln.GetCallsign(), station));

			break;
		}

		case NONE:
		default: {
			break;
		}
	}
}

void CCI::setActiveATCO(const char* callsign, GndStations station) {
	setActiveATCO(FlightPlanSelect(callsign), station);
}

void CCI::openATCPopup(GndStations activeStation, RECT area) {
	char dBuf[4], gBuf[4], tBuf[4];
	GLOBAL::GndStationToStr(DELIVERY, dBuf);
	GLOBAL::GndStationToStr(GROUND, gBuf);
	GLOBAL::GndStationToStr(TOWER, tBuf);

	OpenPopupList(area, GLOBAL::popupName, 3);

	AddPopupListElement("   ", "", GLOBAL::TAG_FUNC_ATCO_NONE, true, POPUP_ELEMENT_NO_CHECKBOX, activeStation == NONE);

	AddPopupListElement(dBuf, "", GLOBAL::TAG_FUNC_ATCO_DEL, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == DELIVERY);

	AddPopupListElement(gBuf, "", GLOBAL::TAG_FUNC_ATCO_GND, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == GROUND);

	AddPopupListElement(tBuf, "", GLOBAL::TAG_FUNC_ATCO_TWR, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == TOWER);
}

void CCI::openATCPopup(GndStations activeStation, RECT area, const char* callsign) {
	strcpy_s(m_ModCs, 10, callsign);
	openATCPopup(activeStation, area);
}
