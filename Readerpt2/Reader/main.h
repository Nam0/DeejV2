#pragma once

// Resource IDs
#define IDS_APP_TITLE   103
#define IDI_TEST        107
#define IDI_SMALL       108
#define IDC_TEST        109
#define MAX_LOADSTRING  100
#define WM_TRAYICON     (WM_USER + 1)
#define IDM_SHOWHIDE    1001
#define IDM_EXIT        1002
#define IDM_UPDATEBUTTONS   101

#include "SimpleIni.h"
#include <Psapi.h>
#include <audiopolicy.h>
#include <cctype>
#include <endpointvolume.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <tlhelp32.h>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <locale>
#include <numeric>
