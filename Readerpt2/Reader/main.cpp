#include "main.h"

// Libraries
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "kernel32.lib")

using namespace std;
std::vector<std::wstring> textLines;
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hWndMain = NULL;
NOTIFYICONDATA nid;
HANDLE hSerial;

//SimpleIni 
std::vector<std::string> sliderMappingArray;
std::vector<std::string> settingArray;
std::vector<std::string> buttonsArray;
std::vector<std::string> buttonMappingArray;

void stripWhitespace(char* str) {
    size_t len = strlen(str);
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (!isspace(str[i]) && str[i] != '\0') {
            str[j++] = str[i];
        }
    }

    str[j] = '\0';
}

void runCommand(const std::wstring& command) {
    const wchar_t* cmd = command.c_str();
    ShellExecute(NULL, L"open", cmd, NULL, NULL, SW_SHOWNORMAL);
}

void appendToTextBox(const std::wstring& newText) {
    textLines.push_back(newText);
    if (textLines.size() > 28) {
        textLines.erase(textLines.begin());
    }
    std::wstring fullText;
    for (const auto& line : textLines) {
        fullText += line + L"\r\n";
    }
    HWND hTextBox = FindWindowEx(hWndMain, nullptr, L"EDIT", nullptr);
    SetWindowText(hTextBox, fullText.c_str());
    SendMessage(hTextBox, EM_SCROLL, SB_BOTTOM, 0);
}

std::wstring getProcessNameByProcessId(DWORD processId) {
    HANDLE hProcess = nullptr;
    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (hProcess && hProcess != INVALID_HANDLE_VALUE) {
        wchar_t processName[MAX_PATH] = { 0 };
        DWORD bufferSize = sizeof(processName) / sizeof(processName[0]);
        if (QueryFullProcessImageNameW(hProcess, 0, processName, &bufferSize)) {
            CloseHandle(hProcess);
            wchar_t* fileName = wcsrchr(processName, L'\\');
            if (fileName) {
                return fileName + 1;
            }
        }
    }
    return L"";
}

DWORD getProcessIdByName(const wchar_t* processName) {
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        if (Process32First(hSnapshot, &processEntry)) {
            do {
                if (!_wcsicmp(processEntry.szExeFile, processName)) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &processEntry));
        }
        CloseHandle(hSnapshot);
    }
    return processId;
}

void adjustVolumeForProcess(const wchar_t* processName, float volumeLevel) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to initialize COM.");
        return;
    }

    IMMDeviceEnumerator* enumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to create device enumerator.");
        CoUninitialize();
        return;
    }

    IMMDevice* device = NULL;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get default audio endpoint.");
        enumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioSessionManager2* sessionManager = NULL;
    hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to activate audio session manager.");
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioSessionEnumerator* sessionEnumerator = NULL;
    hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get session enumerator.");
        sessionManager->Release();
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    int sessionCount = 0;
    hr = sessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get session count.");
        sessionEnumerator->Release();
        sessionManager->Release();
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    for (int i = 0; i < sessionCount; i++) {
        IAudioSessionControl* sessionControl = NULL;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get session control.");
            continue;
        }

        IAudioSessionControl2* sessionControl2 = NULL;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get session control 2.");
            sessionControl->Release();
            continue;
        }

        DWORD sessionId = 0;
        hr = sessionControl2->GetProcessId(&sessionId);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get process ID for the session.");
            sessionControl2->Release();
            sessionControl->Release();
            continue;
        }

        sessionControl2->Release();

        std::wstring sessionProcessName = getProcessNameByProcessId(sessionId);
        if (_wcsicmp(sessionProcessName.c_str(), processName) == 0) {
            ISimpleAudioVolume* simpleVolume = NULL;
            hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
            if (SUCCEEDED(hr) && simpleVolume != NULL) {
                hr = simpleVolume->SetMasterVolume(volumeLevel, NULL);
                if (FAILED(hr)) {
                    appendToTextBox(L"Failed to set volume level.");
                }
                simpleVolume->Release();
            }
            else {
                appendToTextBox(L"Failed to get simple audio volume interface.");
            }
        }

        sessionControl->Release();
    }

    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    enumerator->Release();

    CoUninitialize();

    std::wstring message = L"Volume adjusted successfully for ";
    message += processName;
    message += L".";
    appendToTextBox(message);
}

void changeSystemVolume(float volumeLevel) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to initialize COM.");
        return;
    }

    IMMDeviceEnumerator* enumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to create device enumerator.");
        CoUninitialize();
        return;
    }

    IMMDevice* device = NULL;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get default audio endpoint.");
        enumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioEndpointVolume* endpointVolume = NULL;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&endpointVolume);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to activate endpoint volume.");
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    hr = endpointVolume->SetMasterVolumeLevelScalar(volumeLevel, NULL);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to set master volume level.");
    }

    endpointVolume->Release();
    device->Release();
    enumerator->Release();

    CoUninitialize();
}

void changeGlobalVolume(float volumeLevel) {
    HRESULT hr;
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDeviceCollection* deviceCollection = NULL;
    IMMDevice* device = NULL;
    IAudioSessionManager2* sessionManager = NULL;
    IAudioSessionEnumerator* sessionEnumerator = NULL;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to initialize COM.");
        return;
    }

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to create device enumerator.");
        CoUninitialize();
        return;
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get default audio endpoint.");
        enumerator->Release();
        CoUninitialize();
        return;
    }

    hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to activate audio session manager.");
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get session enumerator.");
        sessionManager->Release();
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    int sessionCount = 0;
    hr = sessionEnumerator->GetCount(&sessionCount);
    if (FAILED(hr)) {
        appendToTextBox(L"Failed to get session count.");
        sessionEnumerator->Release();
        sessionManager->Release();
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    std::vector<std::string> processNamesList;
    CSimpleIniA::TNamesDepend processNames;
    for (int i = 0; i < 5; ++i) {
        std::string processNameStr;
        if (i < sliderMappingArray.size()) {
            processNameStr = sliderMappingArray[i];
        }
        std::transform(processNameStr.begin(), processNameStr.end(), processNameStr.begin(), ::tolower);
        processNamesList.push_back(processNameStr);
        std::wstring message = L"Added processNames: ";
        message += std::wstring(processNameStr.begin(), processNameStr.end());
        message += L".";
        appendToTextBox(message);
    }

    for (int i = 0; i < sessionCount; i++) {
        IAudioSessionControl* sessionControl = NULL;
        hr = sessionEnumerator->GetSession(i, &sessionControl);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get session control.");
            continue;
        }

        IAudioSessionControl2* sessionControl2 = NULL;
        hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get session control 2.");
            sessionControl->Release();
            continue;
        }

        DWORD sessionProcessId = 0;
        hr = sessionControl2->GetProcessId(&sessionProcessId);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get process ID.");
            sessionControl2->Release();
            sessionControl->Release();
            continue;
        }

        if (sessionProcessId != 0) {
            std::wstring processName = getProcessNameByProcessId(sessionProcessId);
            std::string processNameStr(processName.begin(), processName.end());
            std::transform(processNameStr.begin(), processNameStr.end(), processNameStr.begin(), ::tolower);
            std::wstring message = L"Process: ";
            message += processName;
            message += L".";
            appendToTextBox(message);

            if (std::find(processNamesList.begin(), processNamesList.end(), processNameStr) != processNamesList.end()) {
                std::wstring message = L"Skipping volume adjustment for process: ";
                message += processName;
                message += L".";
                appendToTextBox(message);
                sessionControl2->Release();
                sessionControl->Release();
                continue;
            }
        }

        ISimpleAudioVolume* simpleVolume = NULL;

        hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to get simple audio volume interface.");
            sessionControl2->Release();
            sessionControl->Release();
            continue;
        }

        hr = simpleVolume->SetMasterVolume(volumeLevel, NULL);
        if (FAILED(hr)) {
            appendToTextBox(L"Failed to set volume level.");
        }

        simpleVolume->Release();
        sessionControl2->Release();
        sessionControl->Release();
    }

    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    enumerator->Release();

    CoUninitialize();

    appendToTextBox(L"Global volume adjusted successfully.");
}

void readIniFile(const std::string& filePath) {
    CSimpleIniA ini;
    SI_Error loadResult = ini.LoadFile(filePath.c_str());
    if (loadResult < 0) {
        std::cerr << "Error loading INI file: " << loadResult << std::endl;
        return;
    }

    sliderMappingArray.clear();
    settingArray.clear();
    buttonsArray.clear();
    buttonMappingArray.clear();

    const CSimpleIniA::TKeyVal* pSection = ini.GetSection("slider_mapping");
    if (pSection != nullptr) {
        for (auto& entry : *pSection) {
            sliderMappingArray.push_back(entry.second);
        }
    }

    pSection = ini.GetSection("Setting");
    if (pSection != nullptr) {
        for (auto& entry : *pSection) {
            settingArray.push_back(entry.second);
        }
    }

    pSection = ini.GetSection("Buttons");
    if (pSection != nullptr) {
        for (size_t i = 0; i <= 11; ++i) {
            const char* value = ini.GetValue("Buttons", std::to_string(i).c_str());
            if (value == nullptr) {
                buttonsArray.push_back("NULL");
            }
            else {
                buttonsArray.push_back(value);
            }
        }
    }

    pSection = ini.GetSection("Button_Mapping");
    if (pSection != nullptr) {
        for (size_t i = 0; i <= 11; ++i) {
            const char* value = ini.GetValue("Button_Mapping", std::to_string(i).c_str());
            if (value == nullptr) {
                buttonMappingArray.push_back("NULL");
            }
            else {
                buttonMappingArray.push_back(value);
            }
        }
    }
}

int calculateChecksum(const std::string& message) {
    int sum = 0;
    for (char c : message) {
        sum += static_cast<int>(c);
    }
    return sum;
}

void updateButtons(HANDLE hSerial) {
    readIniFile("config.ini");
    std::string buttonsMessage;
    for (const auto& button : buttonsArray) {
        buttonsMessage += button + ",";
    }
    if (!buttonsMessage.empty()) {
        buttonsMessage.pop_back(); 
    }

    std::string setupPrefix = "Setup:";
    std::string completeMessage = buttonsMessage;
    int checksum = calculateChecksum(completeMessage);
    std::string setupMessage = setupPrefix + std::to_string(checksum) + ":" + buttonsMessage + "\n";

    DWORD bytesWritten;
    if (!WriteFile(hSerial, setupMessage.c_str(), static_cast<DWORD>(setupMessage.length()), &bytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        std::wstring errorMessage = L"Error writing setup message to serial port. Error code: " + std::to_wstring(dwError);
        appendToTextBox(errorMessage);

        if (dwError == ERROR_IO_PENDING) {
            appendToTextBox(L"IO operation is pending. Try again later.");
        }
        else if (dwError == ERROR_INVALID_HANDLE) {
            appendToTextBox(L"Invalid handle. Check the serial port connection.");
        }
        else {
            appendToTextBox(L"Unknown error occurred.");
        }
    }
    else {
        std::wstring wideSetupMessage(setupMessage.begin(), setupMessage.end());
        std::wstring message = L"Setup message sent successfully: ";
        message += wideSetupMessage;
        message += L".";
        appendToTextBox(message);
    }
}

bool initializeSerialPort(HANDLE& hSerial) {
    std::string comPortStr = settingArray[1];
    std::string comPortName = "\\\\.\\COM" + comPortStr;

    hSerial = CreateFileA(comPortName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        appendToTextBox(L"Error opening serial port. Error code: " + std::to_wstring(dwError));
        return false;
    }
    else {
        appendToTextBox(L"Serial port opened successfully.");
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        DWORD dwError = GetLastError();
        appendToTextBox(L"Error getting serial port state. Error code: " + std::to_wstring(dwError));
        CloseHandle(hSerial);
        return false;
    }
    dcbSerialParams.BaudRate = CBR_9600;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        DWORD dwError = GetLastError();
        appendToTextBox(L"Error setting serial port state. Error code: " + std::to_wstring(dwError));
        CloseHandle(hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        DWORD dwError = GetLastError();
        appendToTextBox(L"Error setting serial port timeouts. Error code: " + std::to_wstring(dwError));
        CloseHandle(hSerial);
        return false;
    }

    return true;
}

std::wstring getKeyFromValue(const CSimpleIniW& ini, const std::wstring& section, const std::wstring& searchValue) {
    CSimpleIniW::TNamesDepend keys;
    ini.GetAllKeys(section.c_str(), keys);
    std::locale loc;
    for (const auto& key : keys) {
        const wchar_t* value = ini.GetValue(section.c_str(), key.pItem);
        if (value) {
            std::wstring uppercaseValue = value;
            std::wstring uppercaseSearchValue = searchValue;
            for (wchar_t& c : uppercaseValue) {
                c = std::toupper(c, loc);
            }
            for (wchar_t& c : uppercaseSearchValue) {
                c = std::toupper(c, loc);
            }

            if (uppercaseValue == uppercaseSearchValue) {
                return key.pItem;
            }
        }
    }
    std::wstring uppercaseResult = searchValue;
    for (wchar_t& c : uppercaseResult) {
        c = std::toupper(c, loc);
    }
    return uppercaseResult;
}

int getKeyFromValueFromArray(const std::vector<std::string>& array, const std::wstring& value) {
    std::wstring uppercaseValue;
    std::transform(value.begin(), value.end(), std::back_inserter(uppercaseValue), [](wchar_t c) {
        return std::toupper(c);
        });

    for (size_t i = 0; i < array.size(); ++i) {
        std::wstring uppercaseArrayItem(array[i].begin(), array[i].end());
        std::transform(uppercaseArrayItem.begin(), uppercaseArrayItem.end(), uppercaseArrayItem.begin(), [](wchar_t c) {
            return std::toupper(c);
            });

        if (uppercaseArrayItem == uppercaseValue) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void handleButtonClick(const std::string& incomingMessage) {
    if (incomingMessage.substr(0, 15) == "BUTTON CLICKED:") {
        std::string buttonData = incomingMessage.substr(16);
        buttonData.erase(std::remove_if(buttonData.begin(), buttonData.end(), ::isspace), buttonData.end());
        int key = getKeyFromValueFromArray(buttonsArray, std::wstring(buttonData.begin(), buttonData.end()));
        if (key != -1) {
            try {
                std::wstring command = std::wstring(buttonMappingArray[static_cast<size_t>(key)].begin(), buttonMappingArray[static_cast<size_t>(key)].end());
                std::wstring WideButton = std::wstring(buttonData.begin(), buttonData.end());
                if (!command.empty()) {
                    runCommand(command);
                    appendToTextBox(L"Command executed for button: ");
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
                else {
                    appendToTextBox(L"No command found for button: ");
                }
            }
            catch (const std::exception& ex) {
                std::string errorMessage = ex.what();
                std::wstring wideErrorMessage(errorMessage.begin(), errorMessage.end());
                appendToTextBox(L"Error executing command: ");
                appendToTextBox(wideErrorMessage);
            }
        }
        else {
            appendToTextBox(L"Button data not found in the array");
        }
    }
}

void showContextMenu(HWND hWnd, POINT pt)
{
    HMENU hMenu = CreatePopupMenu();
    if (IsWindowVisible(hWnd))
        AppendMenu(hMenu, MF_STRING, IDM_SHOWHIDE, L"Hide Window");
    else
        AppendMenu(hMenu, MF_STRING, IDM_SHOWHIDE, L"Show Window");
    AppendMenu(hMenu, MF_STRING, IDM_UPDATEBUTTONS, L"Update Buttons");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 255, 0));
        SetBkColor(hdcStatic, RGB(0, 0, 0));
        return (LRESULT)CreateSolidBrush(RGB(0, 0, 0)); 
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_SHOWHIDE:
            ShowWindow(hWnd, IsWindowVisible(hWnd) ? SW_HIDE : SW_SHOW);
            break;
        case IDM_UPDATEBUTTONS:
            updateButtons(hSerial);
            break;
        case IDM_EXIT:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        break;
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            showContextMenu(hWnd, pt);
        }
        break;
    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM registerClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TEST));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

void addTrayIcon()
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWndMain;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    LoadString(hInst, IDS_APP_TITLE, nid.szTip, 32);
    Shell_NotifyIcon(NIM_ADD, &nid);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    hWndMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, 0, 400, 500, nullptr, nullptr, hInstance, nullptr);
    if (!hWndMain)
        return FALSE;

    HWND hTextBox = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"Beans", WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE,
        -2, -2, 400, 500, hWndMain, NULL, hInstance, NULL);

    LONG_PTR style = GetWindowLongPtr(hTextBox, GWL_STYLE);
    SetWindowLongPtr(hTextBox, GWL_STYLE, style & ~WS_BORDER);

    ShowWindow(hWndMain, SW_HIDE);
    addTrayIcon();
    return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TEST, szWindowClass, MAX_LOADSTRING);
    registerClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TEST));
    MSG msg;

    readIniFile("config.ini");

    if (!initializeSerialPort(hSerial)) {
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    updateButtons(hSerial);


    DWORD bytesRead;
    char incomingData[256];
    int prevValues[5] = { 0 };

    bool bRunning = true;
    while (bRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                bRunning = false;
                break;
            }
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        if (!ReadFile(hSerial, incomingData, sizeof(incomingData) - 1, &bytesRead, NULL)) {
            DWORD dwError = GetLastError();
            std::wstring wideMessage = std::to_wstring(dwError);
            std::wstring message = L"Error reading from serial port. Error code: ";
            message += wideMessage;
            message += L".";
            appendToTextBox(message);
            break;
        }
        if (bytesRead > 0) {
            incomingData[bytesRead] = '\0';
            std::string incomingMessage = incomingData;

            std::string buttonSetting = settingArray[0];
            std::transform(buttonSetting.begin(), buttonSetting.end(), buttonSetting.begin(), ::toupper);
            if (buttonSetting == "TRUE") {
                if (incomingMessage.substr(0, 15) == "BUTTON CLICKED:" && incomingMessage.find("|") == std::string::npos) {
                    if (incomingMessage.length() > 17) {
                        std::string buttonData = incomingMessage.substr(16);
                        if (!buttonData.empty()) {
                            buttonData.erase(std::remove_if(buttonData.begin(), buttonData.end(), ::isspace), buttonData.end());
                        }
                        std::wstring wideMessage(buttonData.begin(), buttonData.end());
                        std::wstring message = L"BUTTON CLICKED:";
                        message += wideMessage;
                        std::wcout << message << std::endl;
                        handleButtonClick(incomingMessage);
                    }
                }
            }

            stripWhitespace(incomingData);
            int values[5];
            if (sscanf_s(incomingData, "V0:%d|V1:%d|V2:%d|V3:%d|V4:%d|", &values[0], &values[1], &values[2], &values[3], &values[4]) == 5) {
                for (int i = 0; i < 5; i++) {
                    std::string key = std::to_string(i);
                    const char* value = sliderMappingArray[i].c_str();
                    int difference = values[i] - prevValues[i];
                    if (value) {
                        float mappedValue = 0.0f;
                        std::string inverseSetting = settingArray[2];
                        std::transform(inverseSetting.begin(), inverseSetting.end(), inverseSetting.begin(), ::toupper);
                        if (inverseSetting == "TRUE") {
                            mappedValue = 1.0f - (static_cast<float>(values[i]) / 1023.0f);
                        }
                        else {
                            mappedValue = static_cast<float>(values[i]) / 1024.0f;
                        }

                        std::wstring StringMappedValue = std::to_wstring(mappedValue);
                        if (std::string(value) != "Master" && std::string(value) != "unmapped" ) {
                            std::string processNameStr = sliderMappingArray[i];
                            std::wstring processName(processNameStr.begin(), processNameStr.end());
                            DWORD processId = getProcessIdByName(processName.c_str());
                            if (processId != 0) {
                                adjustVolumeForProcess(processName.c_str(), mappedValue);
                                std::wstring message = L"Volume adjusted to mapped value for process: ";
                                message += StringMappedValue;
                                message += L".";
                                //appendToTextBox(message);
                            }
                            else {
                                appendToTextBox(L"Failed to find the target process.");
                            }
                        }
                        else if (std::string(value) == "Master") {
                            changeSystemVolume(mappedValue);
                            std::wstring message = L"System Volume set to: ";
                            message += StringMappedValue;
                            message += L".";
                            appendToTextBox(message);
                        }
                        else if (std::string(value) == "unmapped" ) {
                            changeGlobalVolume(mappedValue);
                            //memset(prevValues, 0, sizeof(prevValues));
                        }
                    }

                    prevValues[i] = values[i];
                }
            }
        }
    }
    CloseHandle(hSerial);
    return (int)msg.wParam;
}
