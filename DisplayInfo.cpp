#include <iostream>
#include "DisplayConfig/DisplayConfig.h"

int main()
{
	std::vector<DisplayConfig_t> displayConfigs;
	if (GetDisplayConfigs(displayConfigs)) {
		for (const auto& config : displayConfigs) {
			std::wcout << "\r\nDisplay: " << DisplayConfigToString(config) << std::endl;
			if (config.bitsPerChannel) {
				const wchar_t* colenc = ColorEncodingToString(config.colorEncoding);
				if (colenc) {
					auto str = std::format(L"  Color: {} {}-bit", colenc, config.bitsPerChannel);
					if (config.advancedColor.advancedColorSupported) {
						str.append(L" HDR10: ");
						str.append(config.advancedColor.advancedColorEnabled ? L"on" : L"off");
					}

					str.append(std::format(L"\r\n         Advanced Color : supported - {}, enabled - {}, wide enforced - {}, force disabled - {}",
										   config.advancedColor.advancedColorSupported, config.advancedColor.advancedColorEnabled,
										   config.advancedColor.wideColorEnforced, config.advancedColor.advancedColorForceDisabled));

					std::wcout << str << std::endl;
				}
			}
		}
	}
}
