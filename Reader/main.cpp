#include <iostream>
#include <Windows.h>
#include "SimpleIni.h"

void stripWhitespace(char* str) {
    int len = strlen(str);
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (!isspace(str[i]) && str[i] != '\0') {
            str[j++] = str[i];
        }
    }

    str[j] = '\0'; 
}

int main() {
    HANDLE hSerial = CreateFile(TEXT("\\\\.\\COM11"), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening serial port. Error code: " << GetLastError() << std::endl;
        return 1;
    }
    else {
        std::cout << "Serial port opened successfully." << std::endl;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting serial port state. Error code: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return 1;
    }
    dcbSerialParams.BaudRate = CBR_9600;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial port state. Error code: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "Error setting serial port timeouts. Error code: " << GetLastError() << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    DWORD bytesRead;
    char incomingData[256];
    while (true) {
        if (!ReadFile(hSerial, incomingData, sizeof(incomingData) - 1, &bytesRead, NULL)) {
            std::cerr << "Error reading from serial port. Error code: " << GetLastError() << std::endl;
            break; 
        }
        if (bytesRead > 0) {
            incomingData[bytesRead] = '\0';

            stripWhitespace(incomingData);

            int values[5];
            sscanf(incomingData, "%d|%d|%d|%d|%d", &values[0], &values[1], &values[2], &values[3], &values[4]);

            CSimpleIniA ini;
            ini.LoadFile("config.ini");

            std::cout << "Mapping:" << std::endl;
            for (int i = 0; i < 5; i++) {
                std::string key = std::to_string(i);
                const char* value = ini.GetValue("slider_mapping", key.c_str());
                if (value) {
                    std::cout << i << ":" << values[i] << " (" << value << ")" << std::endl;
                }
            }
        }
    }

    CloseHandle(hSerial);

    return 0;
}
