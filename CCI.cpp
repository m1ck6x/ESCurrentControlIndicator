#include "pch.h"
#include "CCI.h"
#include <string>
#include <format>

HHOOK hHook;

LRESULT CALLBACK llKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (wParam == WM_KEYDOWN) {
		KBDLLHOOKSTRUCT key = (*(KBDLLHOOKSTRUCT*)lParam);

		if (key.vkCode == VK_END) {
			// TODO: Find a way to execute a command here...
			// Maybe just typing it out using keybd_event or SendInput?
			GLOBAL::pPlugIn->DisplayUserMessage(
				GLOBAL::tag, GLOBAL::tag,
				GLOBAL::pPlugIn->RadarTargetSelectASEL().GetCallsign(),
				true, false, false, false, false
			);
			
			GLOBAL::pPlugIn->DisplayUserMessage(
				GLOBAL::tag, GLOBAL::tag,
				GLOBAL::pPlugIn->FlightPlanSelectASEL().GetCallsign(),
				true, false, false, false, false
			);
			/*INPUT inputs[6] = {};
			memset(inputs, 0, sizeof inputs);

			inputs[0].type = INPUT_KEYBOARD;
			inputs[0].ki.wVk = VK_OEM_PERIOD; // VK_OEM_PERIOD in case '.' doesn't work

			inputs[1].type = INPUT_KEYBOARD;
			inputs[1].ki.wVk = VK_OEM_PERIOD;
			inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

			inputs[2].type = INPUT_KEYBOARD;
			inputs[2].ki.wVk = 'C';

			inputs[3].type = INPUT_KEYBOARD;
			inputs[3].ki.wVk = 'C';
			inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

			inputs[4].type = INPUT_KEYBOARD;
			inputs[4].ki.wVk = 'A';

			inputs[5].type = INPUT_KEYBOARD;
			inputs[5].ki.wVk = 'A';
			inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

			SendInput(ARRAYSIZE(inputs), inputs, sizeof INPUT);*/
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

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

	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, llKeyboardProc, NULL, NULL);

	DisplayUserMessage(GLOBAL::tag, GLOBAL::tag, std::format("Current Control Indicator ({}) loaded successfully!", GLOBAL::version).c_str(), true, true, false, false,
					   false);
}

void __declspec(dllexport) EuroScopePlugInInit(CPlugIn **ppPlugInInstance) {
	*ppPlugInInstance = GLOBAL::pPlugIn = new CCI();

	const char* delClr = GLOBAL::pPlugIn->GetDataFromSettings(GLOBAL::SETTING_ATCO_DEL_CLR);
	const char* gndClr = GLOBAL::pPlugIn->GetDataFromSettings(GLOBAL::SETTING_ATCO_GND_CLR);
	const char* twrClr = GLOBAL::pPlugIn->GetDataFromSettings(GLOBAL::SETTING_ATCO_TWR_CLR);

	if (delClr)
		GLOBAL::ATCO_CLR_DEL = strToClr(delClr);

	if (gndClr)
		GLOBAL::ATCO_CLR_GND = strToClr(gndClr);

	if (twrClr)
		GLOBAL::ATCO_CLR_TWR = strToClr(twrClr);
}

void __declspec(dllexport) EuroScopePlugInExit(void) {
	if (hHook)
		UnhookWindowsHookEx(hHook);

	GLOBAL::pPlugIn->SaveDataToSettings(GLOBAL::SETTING_ATCO_DEL_CLR, "Color of DELIVERY GND ctrl", clrToStr(GLOBAL::ATCO_CLR_DEL));
	GLOBAL::pPlugIn->SaveDataToSettings(GLOBAL::SETTING_ATCO_GND_CLR, "Color of GROUND GND ctrl", clrToStr(GLOBAL::ATCO_CLR_GND));
	GLOBAL::pPlugIn->SaveDataToSettings(GLOBAL::SETTING_ATCO_TWR_CLR, "Color of TOWER GND ctrl", clrToStr(GLOBAL::ATCO_CLR_TWR));

	delete GLOBAL::pPlugIn;
}

void CCI::OnGetTagItem(CFlightPlan FlightPlan, CRadarTarget RadarTarget, int ItemCode, int TagData,
							 char sItemString[16], int *pColorCode, COLORREF *pRGB, double *pFontSize) {
	if (ItemCode == GLOBAL::TAG_ITEM_CCI) {
		auto activeStation = m_ATCMap.find(RadarTarget.GetCallsign());
		if (activeStation != m_ATCMap.end()) {
			gndStationToStr(activeStation->second, sItemString);

			if (gndStationToClr(activeStation->second, pRGB))
				*pColorCode = TAG_COLOR_RGB_DEFINED;
		}
	}
}

void CCI::OnFunctionCall(int FunctionId, const char *sItemString, POINT Pt, RECT Area) {
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

			auto entry = m_ATCMap.find(rTarget.GetCallsign());
			if (entry != m_ATCMap.end())
				activeStation = entry->second;

			openATCPopup(activeStation, Area);

			break;
		}

		case GLOBAL::TAG_FUNC_ATCO_NONE:
		case GLOBAL::TAG_FUNC_ATCO_DEL:
		case GLOBAL::TAG_FUNC_ATCO_GND:
		case GLOBAL::TAG_FUNC_ATCO_TWR: {
			setActiveATCO(rTarget.GetCorrelatedFlightPlan(), static_cast<GndStations>(FunctionId - 0xA001));
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
		gndStationToStr(m_MyStation, atcBuf);
	}

	if (m_MyStation != oldStation)
		m_ATCMap.clear();
}

void CCI::OnFlightPlanDisconnect(CFlightPlan FlightPlan) {
	if (m_ATCMap.contains(FlightPlan.GetCallsign())) {
		m_FpList.RemoveFpFromTheList(FlightPlan);
		m_ATCMap.erase(FlightPlan.GetCallsign());
	}
}

bool CCI::OnCompileCommand(const char *sCommandLine) {
	if (strcmp(sCommandLine, ".gndctrllist") == 0) {
		m_FpList.ShowFpList(true);
		return true;
	}

	return false;
}

void CCI::setActiveATCO(const CFlightPlan &fpln, GndStations station) {
	m_ATCMap.erase(fpln.GetCallsign());
	m_FpList.RemoveFpFromTheList(fpln);

	switch (station) {
		case DELIVERY:
		case GROUND:
		case TOWER: {
			if (station == m_MyStation)
				m_FpList.AddFpToTheList(fpln);

			m_ATCMap.insert(std::make_pair(fpln.GetCallsign(), station));

			break;
		}

		case NONE:
		default: {
			break;
		}
	}
}

void CCI::openATCPopup(GndStations activeStation, RECT area) {
	char dBuf[4], gBuf[4], tBuf[4];
	gndStationToStr(DELIVERY, dBuf);
	gndStationToStr(GROUND, gBuf);
	gndStationToStr(TOWER, tBuf);

	OpenPopupList(area, GLOBAL::popupName, 3);

	AddPopupListElement("   ", "", GLOBAL::TAG_FUNC_ATCO_NONE, true, POPUP_ELEMENT_NO_CHECKBOX, activeStation == NONE);

	AddPopupListElement(dBuf, "", GLOBAL::TAG_FUNC_ATCO_DEL, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == DELIVERY);

	AddPopupListElement(gBuf, "", GLOBAL::TAG_FUNC_ATCO_GND, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == GROUND);

	AddPopupListElement(tBuf, "", GLOBAL::TAG_FUNC_ATCO_TWR, false, POPUP_ELEMENT_NO_CHECKBOX, activeStation == TOWER);
}

void CCI::gndStationToStr(GndStations station, char dest[4]) {
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

bool CCI::gndStationToClr(GndStations station, COLORREF *dest) {
	switch (station) {
	case DELIVERY: {
		*dest = GLOBAL::ATCO_CLR_DEL;
		return true;
	}

	case GROUND: {
		*dest = GLOBAL::ATCO_CLR_GND;
		return true;
	}

	case TOWER: {
		*dest = GLOBAL::ATCO_CLR_TWR;
		return true;
	}

	case NONE:
	default:
		return false;
	}
}
