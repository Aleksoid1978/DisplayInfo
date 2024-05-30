#include "DisplayConfig/DisplayConfig.h"

#include <shellscalingapi.h>

#include <iostream>
#include <map>
#include <cmath>

static OSVERSIONINFOEXW GetWindowVersion()
{
	OSVERSIONINFOEXW osInfo = { sizeof(osInfo) };
	typedef NTSTATUS(WINAPI* PFN_RtlGetVersion)(PRTL_OSVERSIONINFOEXW);
	static auto pRtlGetVersion = reinterpret_cast<PFN_RtlGetVersion>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
	if (pRtlGetVersion) {
		pRtlGetVersion(&osInfo);
	}

	return osInfo;
}
static auto osInfo = GetWindowVersion();

namespace SysVersion {
	static bool IsWin81orLater() {
		static auto bIsWin81orLater = osInfo.dwMajorVersion > 6 || (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3);
		return bIsWin81orLater;
	}

	static const bool IsWin10_1607orLater() {
		static auto bIsWin10_1607orLater = osInfo.dwMajorVersion > 10 || (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 14393);
		return bIsWin10_1607orLater;
	}
}

std::map<std::wstring, long> monitors;

static BOOL CALLBACK EnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
{
	MONITORINFOEXW monitorInfoEx = { sizeof(monitorInfoEx) };
	GetMonitorInfoW(hMonitor, &monitorInfoEx);

	UINT dpiX, dpiY;
	if (S_OK == GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) {
		monitors.try_emplace(monitorInfoEx.szDevice, std::lround(dpiY * 100. / 96.));
	}

	return TRUE;
}

int main()
{
	if (SysVersion::IsWin10_1607orLater()) {
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		EnumDisplayMonitors(nullptr, nullptr, EnumProc, 0);
	} else if (SysVersion::IsWin81orLater()) {
		SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
		EnumDisplayMonitors(nullptr, nullptr, EnumProc, 0);
	}

	std::vector<DisplayConfig_t> displayConfigs;
	if (GetDisplayConfigs(displayConfigs)) {
		for (const auto& config : displayConfigs) {
			auto str = L"\r\nDisplay: " + DisplayConfigToString(config);
			if (auto it = monitors.find(config.displayName); it != monitors.end()) {
				str.append(std::format(L", DPI scaling factor: {}%", it->second));
			}
			std::wcout << str << std::endl;
			if (config.bitsPerChannel) {
				const wchar_t* colenc = ColorEncodingToString(config.colorEncoding);
				if (colenc) {
					str = std::format(L"  Color: {} {}-bit", colenc, config.bitsPerChannel);
					if (config.advancedColor.advancedColorSupported) {
						str.append(L", HDR10: ");
						str.append(config.advancedColor.advancedColorEnabled ? L"on" : L"off");

						str.append(std::format(L"\r\n         Advanced Color : supported - {}, enabled - {}, wide enforced - {}, force disabled - {}",
											   config.advancedColor.advancedColorSupported, config.advancedColor.advancedColorEnabled,
											   config.advancedColor.wideColorEnforced, config.advancedColor.advancedColorForceDisabled));
					}

					std::wcout << str << std::endl;
				}
			}
		}
	}
}
