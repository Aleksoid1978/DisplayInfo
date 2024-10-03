#pragma once

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
	static bool IsWin81OrGreater() {
		static auto bIsWin81OrGreater = osInfo.dwMajorVersion > 6 || (osInfo.dwMajorVersion == 6 && osInfo.dwMinorVersion == 3);
		return bIsWin81OrGreater;
	}

	static const bool IsWin10_1607OrGreater() {
		static auto bIsWin10_1607OrGreater = osInfo.dwMajorVersion > 10 || (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 14393);
		return bIsWin10_1607OrGreater;
	}

	static const bool IsWin11_24H2OrGreater() {
		static auto IsWin11_24H2OrGreater = osInfo.dwMajorVersion > 10 || (osInfo.dwMajorVersion == 10 && osInfo.dwBuildNumber >= 26100);
		return IsWin11_24H2OrGreater;
	}
}
