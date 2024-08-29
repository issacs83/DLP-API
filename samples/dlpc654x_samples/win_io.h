#pragma once

#include <stdbool.h> // bool
#include <stdio.h>
#include <stdint.h>
#ifdef _WIN32
#include <Windows.h>
#include <winusb.h>
#include <usb100.h>
#include <SetupAPI.h>
#include <shlwapi.h> // StrStrI
#include <strsafe.h>
#endif

#ifdef _WIN32
#  define EXPORTFUNC __declspec( dllexport )
#else
#  define EXPORTFUNC
#endif

#include "../../api/dlpc_common_private.h"

// 754x hardware identifiers
// {ecceff35-1463-4ff3-acd9-8f992d09acdd}
static const GUID GUID_DLP_DEVICE = { 0xecceff35, 0x1463, 0x4ff3, {0xac, 0xd9, 0x8f, 0x99, 0x2d, 0x09, 0xac, 0xdd} };

static const DWORD VID_DEVICE = 0x0451;
static const DWORD PID_DEVICE = 0x7540;

extern uint8_t variableLengthRead;

// WinUSB
EXPORTFUNC uint32_t ioInit();

EXPORTFUNC HANDLE openDevicePath(LPSTR devicePath);

EXPORTFUNC uint32_t connectDevice(LPSTR devicePath);

EXPORTFUNC void disconnectDevice();

EXPORTFUNC BOOL initializeWinUsb(PWINUSB_INTERFACE_HANDLE winUsbHandle);

EXPORTFUNC uint32_t ioRead(PVOID pBuffer, uint32_t dwSize);

EXPORTFUNC uint32_t ioWrite(PVOID pBuffer, uint32_t dwSize);

EXPORTFUNC void parseDevicePath(const char* pStr, uint32_t* vid, uint32_t* pid, uint32_t* mi);

EXPORTFUNC LPSTR getDevicePath(DWORD vid, DWORD pid);
