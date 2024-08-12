#include "DisplayConfig/DisplayConfig.h"

#include <shellscalingapi.h>

#include <iostream>
#include <map>
#include <cmath>

#include <dxgi.h>
#include <dxgi1_6.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

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

std::map<std::wstring, std::pair<long, DXGI_COLOR_SPACE_TYPE>> monitors;

static BOOL CALLBACK EnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
{
	MONITORINFOEXW monitorInfoEx = { sizeof(monitorInfoEx) };
	GetMonitorInfoW(hMonitor, &monitorInfoEx);

	UINT dpiX, dpiY;
	if (S_OK == GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) {
		monitors.try_emplace(monitorInfoEx.szDevice, std::lround(dpiY * 100. / 96.), DXGI_COLOR_SPACE_RESERVED);
	}

	return TRUE;
}

static std::wstring ColorSpaceToStr(DXGI_COLOR_SPACE_TYPE ColorSpace)
{
	std::wstring str;
#define UNPACK_VALUE(VALUE) case VALUE: str = L"" #VALUE; break;
	switch (ColorSpace) {
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_FULL_G22_NONE_P709_X601);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 );
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_TOPLEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_TOPLEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_RGB_STUDIO_G24_NONE_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P709);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_LEFT_P2020);
		UNPACK_VALUE(DXGI_COLOR_SPACE_YCBCR_STUDIO_G24_TOPLEFT_P2020);
		default:
			str = std::to_wstring(static_cast<int>(ColorSpace));
	};
#undef UNPACK_VALUE

	return str;
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

	ComPtr<IDXGIFactory> pIDXGIFactory;
	if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(pIDXGIFactory.GetAddressOf())))) {
		ComPtr<IDXGIAdapter> pIDXGIAdapter;
		for (UINT adapter = 0; pIDXGIFactory->EnumAdapters(adapter, pIDXGIAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++adapter) {
			ComPtr<IDXGIOutput> pIDXGIOutput;
			for (UINT output = 0; pIDXGIAdapter->EnumOutputs(output, pIDXGIOutput.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++output) {
				ComPtr<IDXGIOutput6> output6;
				if (SUCCEEDED(pIDXGIOutput.As(&output6))) {
					DXGI_OUTPUT_DESC1 desc;
					if (SUCCEEDED(output6->GetDesc1(&desc))) {
						if (auto it = monitors.find(desc.DeviceName); it != monitors.end()) {
							it->second.second = desc.ColorSpace;
						} else {
							monitors.try_emplace(desc.DeviceName, 0, desc.ColorSpace);
						}
					}
				}
			}
		}
	}

	std::vector<DisplayConfig_t> displayConfigs;
	if (GetDisplayConfigs(displayConfigs)) {
		for (const auto& config : displayConfigs) {
			auto str = L"\r\nDisplay: " + DisplayConfigToString(config);
			if (auto it = monitors.find(config.displayName); it != monitors.end()) {
				if (it->second.second != DXGI_COLOR_SPACE_RESERVED) {
					str.append(std::format(L", Color Space: {}", ColorSpaceToStr(it->second.second)));
				}
				if (it->second.first) {
					str.append(std::format(L"\n         DPI scaling factor: {}%", it->second.first));
				}
			}
			std::wcout << str << std::endl;
			if (config.bitsPerChannel) {
				const wchar_t* colenc = ColorEncodingToString(config.colorEncoding);
				if (colenc) {
					str = std::format(L"  Color: {} {}-bit", colenc, config.bitsPerChannel);
					if (config.advancedColor.HDRSupported()) {
						str.append(L", HDR10: ");
						str.append(config.advancedColor.HDREnabled() ? L"on" : L"off");

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
