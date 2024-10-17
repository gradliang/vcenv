#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <direct.h>
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

using namespace std;

static bool needXp = false;
static string buildVer = "";
static string kitRoot = "";

static void splitString(const std::string & strtem, const char a, std::vector<std::string> & res)
{
	res.clear();
	std::string::size_type pos1, pos2;
	pos2 = strtem.find(a);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		res.push_back(strtem.substr(pos1, pos2 - pos1));
		pos1 = pos2 + 1;
		pos2 = strtem.find(a, pos1);
	}
	res.push_back(strtem.substr(pos1));
}

static string getEnvValue(const string & key)
{
	const int BUFF_SIZE = 65536;
	static char buffer[BUFF_SIZE];
	size_t size = 0;
	buffer[0] = 0;
	buffer[BUFF_SIZE - 1] = 0;
	int code = getenv_s(&size, buffer, BUFF_SIZE - 1, key.c_str());
	if (code == 0) {
		return string(buffer);
	}
	return "";
}

static vector<string> envToStrList(const string & env)
{
	vector<string> strlist;
	string envValue = getEnvValue(env);
	if (envValue.empty()) {
		return strlist;
	}
	splitString(envValue, ';', strlist);
	return strlist;
}

static void switchToNewEnvVar()
{
	// 从注册表中读取WinSDK安装目录
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD type, size = 256;
		char regValue[256];
		if (RegQueryValueExA(hKey, "KitsRoot10", NULL, &type, (LPBYTE)&regValue, &size) == ERROR_SUCCESS) {
			kitRoot = regValue;
		}
		RegCloseKey(hKey);
	}
	if (kitRoot.empty()) {
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
			DWORD type, size = 256;
			char regValue[256];
			if (RegQueryValueExA(hKey, "KitsRoot10", NULL, &type, (LPBYTE)&regValue, &size) == ERROR_SUCCESS) {
				kitRoot = regValue;
			}
			RegCloseKey(hKey);
		}
	}
	//
	if (kitRoot.empty()) {
		printf("WinSDK is not installed");
		exit(-1);
	}
	if (kitRoot[kitRoot.length() - 1] != '\\') {
		kitRoot += "\\";
	}
	//printf("WinSDK = %s\n", kitRoot.c_str());
	// 想办法得到当前使用的 WinSDK 的版本
	auto getCurrentWinSDKVersion = [&]() -> string {
		vector<string> libList = move(envToStrList("LIB"));
		for (auto dirIt = libList.begin(); dirIt != libList.end(); dirIt++) {
			const string & dir = *dirIt;
			if (0 == strnicmp(dir.c_str(), kitRoot.c_str(), kitRoot.length())) {			// starts_with(kitRoot)
				const char * p1 = &(dir.c_str())[kitRoot.length()];
				if (0 == strnicmp(p1, "lib\\", sizeof("lib\\") - 1)) {
					const char * p2 = &p1[sizeof("lib\\") - 1];
					const char * p3 = strchr(p2, '\\');
					if (p3 != nullptr) {
						return string(p2, p3 - p2);
					}
				}

			}
		}
		return "";
	};
	string currSdkVer = getCurrentWinSDKVersion();
	if (currSdkVer.empty()) {
		printf("cannot find the current version of WinSDK being used\n");
		exit(-1);
	}
	//
	printf("Current WinSDK version is %s\n", currSdkVer.c_str());
	//
	// 获取替换版本号
	vector<string> versionList;
	string replaceVersion = "";
	if (!buildVer.empty()) {
		string findText = kitRoot + "Lib\\*";
		WIN32_FIND_DATAA findData;
		HANDLE handle = FindFirstFileA(findText.c_str(), &findData);
		if (INVALID_HANDLE_VALUE != handle) {
			do {
				if (findData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
					if (0 != strcmp(findData.cFileName, ".") && 0 != strcmp(findData.cFileName, ".."))
						versionList.push_back(findData.cFileName);
				}
			} while (FindNextFileA(handle, &findData) != 0);

			FindClose(handle);
		}
		//
		auto findMatchVersion = [](const vector<string> & versionList, const string & buildVer) -> string {
			auto matchVersion = [](const string & matchVer, const string & buildVer) -> bool {
				if (matchVer == buildVer)	return true;
				if (0 == strncmp(buildVer.c_str(), "10.0.", sizeof("10.0.") - 1)) {					// starts_with
					if (0 == strncmp(buildVer.c_str(), buildVer.c_str(), buildVer.length())) {
						return true;
					}
				}
				else {

				}
			};
			//
			for (auto cit = versionList.cbegin(); cit != versionList.cend(); cit++) {
				if (matchVersion(*cit, buildVer)) {
					return *cit;
				}
			}
			return "";
		};
		replaceVersion = findMatchVersion(versionList, buildVer);
	}
	


}

static void addXpBuildEnv()
{

}

static void switchVisualCppEnv()
{
	switchToNewEnvVar();
	addXpBuildEnv();
}

int main(int argc, char ** argv)
{
	if (argc < 2) return 0;
	for (int i = 1; i < argc; i++) {
		string label(argv[i]);
		transform(label.begin(), label.end(), label.begin(), ::tolower);		// to lower
		//
		if (label == "xp" || label == "winxp") {
			needXp = true;
			continue;
		}
		//
		if (label[0] >= '0' && label[0] <= '9') {
			buildVer = label;
			continue;
		}
		printf("error: I'm not sure about your intentions, \"%s\"", label.c_str());
		return -1;
	}
	if (buildVer == "" && !needXp) {
		printf("do nothings\n");
		return 0;								// need to do nothings
	}
	switchVisualCppEnv();
	return 0;
}




