/* Copyright 2026 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <setupapi.h>
#include <conio.h>
#include <devguid.h>
#include <string>
#include <time.h>
#include "BitStream.h"
#include "Decoder.h"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

static void Capture(HANDLE hComPort)
{
	Decoder_t *pDecoder;

	pDecoder = DECODER_New();

	while (!_kbhit()) {
		uint8_t Buffer[128];
		DWORD bytesRead = 0;

		if (!ReadFile(hComPort, Buffer, sizeof(Buffer), &bytesRead, NULL)) {
			DWORD error = GetLastError();

			if (error != ERROR_IO_PENDING) {
				printf("Error reading from COM port (%d)\n", error);
				break;
			}
		}

		if (bytesRead > 0) {
			DECODER_AddBytes(pDecoder, Buffer, bytesRead);
			while (DECODER_Check(pDecoder)) {
				static char Text[64 + (ANYTONE_MAX_FRAME_LENGTH * 3)];
				bool bSkip = false;

				while (DECODER_GetFrameLength(pDecoder)) {
					if (DECODER_GetText(pDecoder, bSkip, Text, sizeof(Text))) {
						char Log[64];
						struct tm TimeInfo;
						time_t Now;

						Now = time(nullptr);
						localtime_s(&TimeInfo, &Now);
						strftime(Log, sizeof(Log), "[%Y-%m-%d %H:%M:%S] ", &TimeInfo);
						printf("%s%s\n", Log, Text);
					}
					bSkip = true;
				}
			}
		}

		Sleep(1);
	}
}

static void ScanComPorts(void)
{
	SP_DEVINFO_DATA devData;
	size_t Total = 0;
	DWORD i;

	// Get a list of all COM ports
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) {
		printf("Error: Failed to get device information.\n");
		return;
	}

	devData.cbSize = sizeof(SP_DEVINFO_DATA);

	// Enumerate all COM ports
	for (i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData); i++) {
		char friendlyName[256] = { 0 };
		DWORD dataType = 0;
		DWORD size = sizeof(friendlyName);

		// Get the friendly name of the device
		if (SetupDiGetDeviceRegistryProperty(hDevInfo, &devData, SPDRP_FRIENDLYNAME,
			&dataType, (PBYTE)friendlyName, size, &size)) {
			std::string portName = friendlyName;
			size_t startPos = portName.find("(COM");
			size_t endPos = portName.find(")", startPos);

			if (startPos != std::string::npos && endPos != std::string::npos) {
				std::string comPort = portName.substr(startPos + 1, endPos - startPos - 1);
				printf("-> %s\n", comPort.c_str());
				Total++;
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);

	if (!Total) {
		printf("No COM ports found.\n");
	}
}

static HANDLE StartCapture(const char *portName)
{
	DCB dcb;
	COMMTIMEOUTS timeouts;
	HANDLE hComPort;

	// Open the COM port
	std::string fullPortName = "\\\\.\\";
	fullPortName += portName;

	printf("Waiting for port...\n");
	do {
		hComPort = CreateFile(
			fullPortName.c_str(),
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	} while (hComPort == INVALID_HANDLE_VALUE && !_kbhit());

	if (hComPort == INVALID_HANDLE_VALUE) {
		if (_kbhit()) {
			printf("Exiting...\n");
		} else {
			DWORD error = GetLastError();

			printf("Error: Failed to open COM port (%d).\n", error);
		}
		return hComPort;
	}

	printf("Configuring port...\n");

	memset(&dcb, 0, sizeof(dcb));
	dcb.DCBlength = sizeof(dcb);

	if (!GetCommState(hComPort, &dcb)) {
		printf("Error: Failed to get COM port state.\n");
		CloseHandle(hComPort);
		return INVALID_HANDLE_VALUE;
	}

	dcb.BaudRate = CBR_115200;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	if (!SetCommState(hComPort, &dcb)) {
		printf("Error: Failed to set COM port state.\n");
		CloseHandle(hComPort);
		return INVALID_HANDLE_VALUE;
	}

	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutConstant = 10;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 10;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(hComPort, &timeouts)) {
		printf("Error: Failed to set COM port timeouts.\n");
		CloseHandle(hComPort);
		return INVALID_HANDLE_VALUE;
	}

	printf("Port initialised...\n");

	return hComPort;
}

static void StopCapture(HANDLE hComPort)
{
	// Close the COM port
	if (hComPort != INVALID_HANDLE_VALUE) {
		CloseHandle(hComPort);
		hComPort = INVALID_HANDLE_VALUE;
	}

	printf("Stopped capturing data.\n");
}

int main(int argc, char *argv[])
{
	HANDLE hComPort;

	printf("AnyTi3r v0.1  (c) Copyright 2026 Dual Tachyon\n\n");

	if (argc == 2 && strcmp(argv[1], "-l") == 0) {
		ScanComPorts();
		return 0;
	}

	if (argc != 3 || strcmp(argv[1], "-p")) {
		printf("Usage:\n");
		printf("    %s -l         List available COM ports.\n", argv[0]);
		printf("    %s -p COMx    Start capture on port COMx.\n", argv[0]);
		return 1;
	}

	hComPort = StartCapture(argv[2]);

	if (hComPort != INVALID_HANDLE_VALUE) {
		Capture(hComPort);
		StopCapture(hComPort);
	}

	return 0;
}
