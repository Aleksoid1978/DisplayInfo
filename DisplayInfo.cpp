#include "DisplayConfig/DisplayConfig.h"

#include <shellscalingapi.h>

#include <iostream>
#include <map>
#include <cmath>

#include <dxgi.h>
#include <dxgi1_6.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

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

static std::wstring ColorModeToStr(DISPLAYCONFIG_ADVANCED_COLOR_MODE ColorMode)
{
	std::wstring str;
#define UNPACK_VALUE(VALUE) case VALUE: str = L"" #VALUE; break;
	switch (ColorMode) {
		UNPACK_VALUE(DISPLAYCONFIG_ADVANCED_COLOR_MODE_SDR);
		UNPACK_VALUE(DISPLAYCONFIG_ADVANCED_COLOR_MODE_WCG);
		UNPACK_VALUE(DISPLAYCONFIG_ADVANCED_COLOR_MODE_HDR);
		default:
			str = std::to_wstring(static_cast<int>(ColorMode));
	};
#undef UNPACK_VALUE

	return str;
}

int main()
{
	if (SysVersion::IsWin10_1607OrGreater()) {
		SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
		EnumDisplayMonitors(nullptr, nullptr, EnumProc, 0);
	} else if (SysVersion::IsWin81OrGreater()) {
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
					if (config.HDRSupported()) {
						str.append(L", HDR10: ");
						str.append(config.HDREnabled() ? L"on" : L"off");

						if (SysVersion::IsWin11_24H2OrGreater()) {
							auto& colors = config.windows1124H2Colors;
							str.append(std::format(L"\r\n         Advanced Color: Supported: {}, Active: {}, Limited by OS policy: {}, HDR is supported: {}",
												   colors.advancedColorSupported, colors.advancedColorActive,
												   colors.advancedColorLimitedByPolicy, colors.highDynamicRangeSupported));
							str.append(std::format(L"\r\n                         HDR enabled: {}, Wide supported: {}, Wide enabled: {}",
												   colors.highDynamicRangeUserEnabled, colors.wideColorSupported,
												   colors.wideColorUserEnabled));
							str.append(std::format(L"\r\n                         Display color mode: {}", ColorModeToStr(colors.activeColorMode)));
						} else {
							auto& colors = config.advancedColor;
							str.append(std::format(L"\r\n         Advanced Color: Supported: {}, Enabled: {}, Wide forced: {}, Force disabled: {}",
												   colors.advancedColorSupported, colors.advancedColorEnabled,
												   colors.wideColorEnforced, colors.advancedColorForceDisabled));
						}
					}

					std::wcout << str << std::endl;
				}
			}
		}
	}
}
