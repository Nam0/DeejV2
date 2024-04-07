#include <iostream>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <tlhelp32.h>
#include <string>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowsapp.lib")
#pragma comment(lib, "kernel32.lib")

using namespace std;

DWORD GetProcessIdByName(const wchar_t* processName) {
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

void ChangeSystemVolume(float volumeLevel) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        cerr << "Failed to initialize COM." << endl;
        return;
    }

    IMMDeviceEnumerator* enumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
    if (FAILED(hr)) {
        cerr << "Failed to create device enumerator." << endl;
        CoUninitialize();
        return;
    }

    IMMDevice* device = NULL;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) {
        cerr << "Failed to get default audio endpoint." << endl;
        enumerator->Release();
        CoUninitialize();
        return;
    }

    IAudioEndpointVolume* endpointVolume = NULL;
    hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&endpointVolume);
    if (FAILED(hr)) {
        cerr << "Failed to activate endpoint volume." << endl;
        device->Release();
        enumerator->Release();
        CoUninitialize();
        return;
    }

    hr = endpointVolume->SetMasterVolumeLevelScalar(volumeLevel, NULL);
    if (FAILED(hr)) {
        cerr << "Failed to set master volume level." << endl;
    }

    endpointVolume->Release();
    device->Release();
    enumerator->Release();

    CoUninitialize();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Silly It's Actually: VolumeChanger.exe \"ProgramName.exe\" \"VolumeLevel\"" << endl;
        return 1;
    }

    string command = argv[1];
    if (command == "Master") {
        float volumeLevel = static_cast <float> (atof(argv[2]));
        ChangeSystemVolume(volumeLevel);
        cout << "System volume changed successfully." << endl;
    }
    else {
        const wchar_t* targetProcessName = L"debug.exe";
        float volumeLevel = 0.5f;

        int argLength = strlen(argv[1]) + 1;
        wchar_t* wideTargetProcessName = new wchar_t[argLength];
        MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wideTargetProcessName, argLength);
        targetProcessName = wideTargetProcessName;
        volumeLevel = static_cast <float> (atof(argv[2]));

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) {
            cerr << "Failed to initialize COM." << endl;
            return 1;
        }

        DWORD targetProcessId = GetProcessIdByName(targetProcessName);
        if (targetProcessId == 0) {
            cerr << "Failed to find the target process." << endl;
            CoUninitialize();
            return 1;
        }

        IMMDeviceEnumerator* enumerator = NULL;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator);
        if (FAILED(hr)) {
            cerr << "Failed to create device enumerator." << endl;
            CoUninitialize();
            return 1;
        }

        IMMDevice* device = NULL;
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (FAILED(hr)) {
            cerr << "Failed to get default audio endpoint." << endl;
            enumerator->Release();
            CoUninitialize();
            return 1;
        }

        IAudioSessionManager2* sessionManager = NULL;
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&sessionManager);
        if (FAILED(hr)) {
            cerr << "Failed to activate audio session manager." << endl;
            device->Release();
            enumerator->Release();
            CoUninitialize();
            return 1;
        }

        IAudioSessionEnumerator* sessionEnumerator = NULL;
        hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
        if (FAILED(hr)) {
            cerr << "Failed to get session enumerator." << endl;
            sessionManager->Release();
            device->Release();
            enumerator->Release();
            CoUninitialize();
            return 1;
        }

        int sessionCount = 0;
        hr = sessionEnumerator->GetCount(&sessionCount);
        if (FAILED(hr)) {
            cerr << "Failed to get session count." << endl;
            sessionEnumerator->Release();
            sessionManager->Release();
            device->Release();
            enumerator->Release();
            CoUninitialize();
            return 1;
        }

        for (int i = 0; i < sessionCount; i++) {
            IAudioSessionControl* sessionControl = NULL;
            hr = sessionEnumerator->GetSession(i, &sessionControl);
            if (FAILED(hr)) {
                cerr << "Failed to get session control." << endl;
                continue;
            }

            IAudioSessionControl2* sessionControl2 = NULL;
            hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&sessionControl2);
            if (FAILED(hr)) {
                cerr << "Failed to get session control 2." << endl;
                sessionControl->Release();
                continue;
            }

            DWORD processId = 0;
            hr = sessionControl2->GetProcessId(&processId);
            if (FAILED(hr)) {
                cerr << "Failed to get process ID for the session." << endl;
                sessionControl2->Release();
                sessionControl->Release();
                continue;
            }

            sessionControl2->Release();

            if (processId == targetProcessId) {
                ISimpleAudioVolume* simpleVolume = NULL;
                hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVolume);
                if (SUCCEEDED(hr) && simpleVolume != NULL) {
                    hr = simpleVolume->SetMasterVolume(volumeLevel, NULL);
                    if (FAILED(hr)) {
                        cerr << "Failed to set volume level." << endl;
                    }
                    simpleVolume->Release();
                }
                else {
                    cerr << "Failed to get simple audio volume interface." << endl;
                }
            }

            sessionControl->Release();
        }

        sessionEnumerator->Release();
        sessionManager->Release();
        device->Release();
        enumerator->Release();

        CoUninitialize();

        cout << "Volume adjusted successfully for " << targetProcessName << "." << endl;
    }

    return 0;
}