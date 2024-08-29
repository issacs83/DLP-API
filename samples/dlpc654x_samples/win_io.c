#include "win_io.h"

#define IN_STDERR stdout

PWINUSB_INTERFACE_HANDLE winUsbHandle;
HANDLE driverHandle;

uint8_t variableLengthRead;

uint32_t ioRead(PVOID pBuffer, uint32_t dwSize)
{
	ULONG bytesRead = 0xFFFF;
	if (winUsbHandle)
	{
		// ORing with 0x80 sets the direction bit to "read"
		if (!WinUsb_ReadPipe(winUsbHandle, (UCHAR)1 | 0x80, (PUCHAR)pBuffer, dwSize, &bytesRead, 0))
		{
			printf("FAILED TO READ. ERR: %d\n", GetLastError());
			bytesRead = 0xFFFF;
		}
	}
    return bytesRead;
}

uint32_t ioWrite(PVOID pBuffer, uint32_t dwSize)
{
    ULONG bytesWritten = 0xFFFFFFFF;
	if (winUsbHandle)
	{
		if (!WinUsb_WritePipe(winUsbHandle, (UCHAR)1, (PUCHAR)pBuffer, dwSize, &bytesWritten, 0))
		{
			int err = GetLastError();
			printf("FAILED TO WRITE. ERR: %d\n", err);
			bytesWritten = 0xFFFFFFFF;
		}
	}
    if (bytesWritten > 0)
    {
        /*printf("WROTE: ");
        for (uint32_t i = 0; i < bytesWritten; ++i)
        {
            printf("0x%02X ", ((uint8_t*)(pBuffer))[i]);
        }
        printf("\n");*/
    }
    return bytesWritten;
}

uint32_t ioInit()
{
	LPSTR devicePath = getDevicePath(VID_DEVICE, PID_DEVICE);
	return connectDevice(devicePath);
}

void parseDevicePath(const char* pStr, uint32_t* vid, uint32_t* pid, uint32_t* mi)
{
    // it would be nice to use regex, but I don't want to add the
    // dependency to this dll, so, time to do it 'the old fashioned way...'
    char temp[5] = {0,0,0,0,0};

    const char *p = StrStrI(pStr, "vid_");
    if (p)
    {
        memcpy(temp, p + 4, 4);
        *vid = strtoul(temp, 0, 16);
    }
    p = StrStrI(pStr, "pid_");
    if (p)
    {
        memcpy(temp, p + 4, 4);
        *pid = strtoul(temp, 0, 16);
    }
    p = StrStrI(pStr, "mi_");
    if (p)
    {
        memcpy(temp, p + 3, 2);
        temp[2] = 0;
        *mi = strtoul(temp, 0, 16);
    }
}

LPSTR getDevicePath(DWORD vid, DWORD pid)
{
    BOOL   bResult      = FALSE;
    LPSTR  lpDevicePath = NULL;

    // Get information about all the installed devices for the specified
    // device interface class.
    HDEVINFO hDeviceInfo;
    hDeviceInfo = SetupDiGetClassDevs(&GUID_DLP_DEVICE, NULL, NULL, 
                                      /*DIGCF_PRESENT |*/ DIGCF_DEVICEINTERFACE);
    //hDeviceInfo = SetupDiGetClassDevs(NULL, NULL, NULL, 
                                      ///*DIGCF_PRESENT |*/ DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);

    //hDeviceInfo = SetupDiGetClassDevs(&GUID_DLP_DEVICE, NULL, NULL, 
                                      //DIGCF_PRESENT | DIGCF_ALLCLASSES | DIGCF_DEFAULT);

    if (hDeviceInfo == INVALID_HANDLE_VALUE) 
    { 
        DWORD lastError = GetLastError();
        printf("Error SetupDiGetClassDevs: %d\n", lastError);
        goto ErrorExit;
    }

    // Enumerate all the device interfaces in the device information set.
    SP_DEVINFO_DATA DeviceInfoData;
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	DWORD devIndex2 = 0;
	int res = SetupDiEnumDeviceInfo(hDeviceInfo, devIndex2, &DeviceInfoData);

    for (DWORD devIndex = 0; !lpDevicePath && SetupDiEnumDeviceInfo(hDeviceInfo, devIndex, &DeviceInfoData); devIndex++)
    {
        PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = NULL;
        for (DWORD infIndex = 0; !lpDevicePath; infIndex++)
        {
            // Reset for this iteration
            LocalFree(pInterfaceDetailData);
            pInterfaceDetailData = NULL;

            SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
            deviceInterfaceData.cbSize = sizeof(SP_INTERFACE_DEVICE_DATA);

            // Get information about the device interface.
            bResult = SetupDiEnumDeviceInterfaces(hDeviceInfo, /*&DeviceInfoData*/ NULL,
                                                  &GUID_DLP_DEVICE, infIndex, 
                                                  &deviceInterfaceData);

            // Check if last interface
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
                break; // try next device?

            // Check for other errors
            if (!bResult) 
            {
                DWORD lastError = GetLastError();
                printf("Error SetupDiEnumDeviceInterfaces: %d\n", lastError);
                break; // try next device?
            }

            // Interface data is returned in SP_DEVICE_INTERFACE_DETAIL_DATA
            // which we need to allocate, so we have to call this function twice.
            // First to get the size so that we know how much to allocate
            // Second, the actual call with the allocated buffer

            ULONG requiredLength = 0;
            bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData,
                                                    NULL, 0, &requiredLength, NULL);
            // Check for error
            if (!bResult) 
            {
                if ((ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (requiredLength > 0))
                {
                    // we got the size, allocate buffer
                    pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LPTR, requiredLength);

                    if (!pInterfaceDetailData) 
                    { 
                        // ERROR 
                        printf("Error allocating memory for the device detail buffer.\n");
                        goto ErrorExit;
                    }
                }
                else
                {
                    DWORD lastError = GetLastError();
                    printf("Error SetupDiEnumDeviceInterfaces: %d\n", lastError);
                    break; // try next device?
                }
            }

            // get the interface detail data
            pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA); //requiredLength?

            // Now call it with the correct size and allocated buffer
            bResult = SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData,
                                                    pInterfaceDetailData, requiredLength,
                                                    NULL, &DeviceInfoData);

            // Check for some other error
            if (!bResult) 
            {
                DWORD lastError = GetLastError();
                printf("Error SetupDiGetDeviceInterfaceDetail: %d \n", lastError);
                break; // try next device?
            }

            // check VID & PID
            DWORD deviceVID = 0, devicePID = 0, deviceMI = 0;
            parseDevicePath(pInterfaceDetailData->DevicePath, &deviceVID, &devicePID, &deviceMI);
            if (deviceVID == vid && devicePID == pid && deviceMI == 0)
            {
                HANDLE dummyDevice = openDevicePath(pInterfaceDetailData->DevicePath);
                if (dummyDevice != INVALID_HANDLE_VALUE)
                {
					printf("Found device path: %s\n", pInterfaceDetailData->DevicePath);
                    // we be re-opened later, but we know it works!
                    CloseHandle(dummyDevice);
                    lpDevicePath = _strdup(pInterfaceDetailData->DevicePath);
                    break; // found device!
                }
            }
        }
        LocalFree(pInterfaceDetailData);
        pInterfaceDetailData = NULL;
    }

    if (!lpDevicePath)
    {
        // Error.
        DWORD lastError = GetLastError();
        printf("Error: Device not found or in use. Ensure no other software is using the device. Error code: %d\n", lastError);
        goto ErrorExit;
    }

ErrorExit:
    SetupDiDestroyDeviceInfoList(hDeviceInfo);
    return lpDevicePath;
}

HANDLE openDevicePath(LPSTR devicePath)
{
    HANDLE h = INVALID_HANDLE_VALUE;
    // Open the device
    h = CreateFile (devicePath, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                    OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (h == INVALID_HANDLE_VALUE)
    {
        // Error.
        DWORD lastError = GetLastError();

		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		LPTSTR lpszFunction = "";
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);
		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			lpszFunction, lastError, lpMsgBuf);
		//MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
        //fprintf(IN_STDERR, "Failed with error %d: %s", lastError, (char*)lpMsgBuf);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
	}

	return h;
}

uint32_t connectDevice(LPSTR devicePath)
{
	ULONG timeout = 2000;

    // open device driver handle
    driverHandle = openDevicePath(devicePath);
    if (driverHandle != INVALID_HANDLE_VALUE)
    {
        // initialize WinUsb library with driver handle
        if (initializeWinUsb(&winUsbHandle))
        {
            WinUsb_SetPipePolicy(winUsbHandle, 0x01, PIPE_TRANSFER_TIMEOUT, sizeof(ULONG), &timeout);
            WinUsb_SetPipePolicy(winUsbHandle, 0x81, PIPE_TRANSFER_TIMEOUT, sizeof(ULONG), &timeout);
			printf("Connected to device\n");
            return 0;
        }
        CloseHandle(driverHandle);
        driverHandle = INVALID_HANDLE_VALUE;
    }
    return 1;
}

void disconnectDevice()
{
    if (winUsbHandle)
    {
        WinUsb_Free(winUsbHandle);
        winUsbHandle = NULL;
    }
    if (driverHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(driverHandle);
        driverHandle = INVALID_HANDLE_VALUE;
    }
}

BOOL initializeWinUsb(PWINUSB_INTERFACE_HANDLE winUsbHandle)
{
    if (driverHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    BOOL bResult = WinUsb_Initialize(driverHandle, winUsbHandle);
    if (!bResult)
    {
        // Error
        DWORD lastError = GetLastError();
        fprintf(IN_STDERR, "Error: WinUsb_Initialize Error: %d\n", lastError);
    }

    return bResult;
}
