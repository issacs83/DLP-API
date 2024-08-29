/*------------------------------------------------------------------------------
 * Copyright (c) 2019 Texas Instruments Incorporated - http://www.ti.com/
 *------------------------------------------------------------------------------
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file   devasys_i2c
 * \brief  Sample code for I2C communication via Cypress USB-Serial 
 *         Bridge Controller.
 *
 */

#include "windows.h"
#include "devasys_i2c.h"
#include "usbi2cio/Usbi2cio.h"
#include "stdio.h"
#include "time.h"

HINSTANCE hDLL;
typedef HANDLE(CALLBACK* OPENDEVICEINSTANCE)(LPSTR, BYTE);
typedef BOOL(CALLBACK* CLOSEDEVICEINSTANCE)(HANDLE);
typedef BOOL(CALLBACK* DETECTDEVICE)(HANDLE);
typedef BYTE(CALLBACK* GETDEVICECOUNT)(LPSTR);
typedef BYTE(CALLBACK* GETDEVICEINFO)(LPSTR, LPDEVINFO);
typedef HANDLE(CALLBACK* OPENDEVICEBYSERIALID)(LPSTR, LPSTR);
typedef BOOL(CALLBACK* GETSERIALID)(HANDLE, LPSTR);
typedef BOOL(CALLBACK* READI2C)(HANDLE, PI2C_TRANS);
typedef BOOL(CALLBACK* WRITEI2C)(HANDLE, PI2C_TRANS);
OPENDEVICEINSTANCE OpenDeviceInstance;
CLOSEDEVICEINSTANCE CloseDeviceInstance;
DETECTDEVICE DetectDevice;
GETDEVICECOUNT GetDeviceCount;
GETDEVICEINFO GetDeviceInfo;
OPENDEVICEBYSERIALID OpenDeviceBySerialId;
GETSERIALID GetSerialId;
READI2C ReadI2c;
WRITEI2C WriteI2c;

HANDLE hDevInstance = INVALID_HANDLE_VALUE;		// handle to UsbI2cIo device
I2C_TRANS TransI2c;								// I2C transaction structure
DEVINFO DeviceInfo;

bool DEVASYS_I2C_ConnectToDevI2C()
{
	int NumDevices = 0;
	BOOL Status = false;

	TransI2c.bySlvDevAddr = 0x36;
	TransI2c.byTransType = I2C_TRANS_NOADR;		// device does not use sub-address
	TransI2c.wMemoryAddr = 0;					// unused for I2C_TRANS_NOADR

	// Get a handle to the DLL module.
	hDLL = LoadLibrary("UsbI2cIo.dll");			// attempt load dll
	if (hDLL != NULL) {
		// got handle to dll, now initialize API function pointers
		OpenDeviceInstance = (OPENDEVICEINSTANCE)GetProcAddress(hDLL, "DAPI_OpenDeviceInstance");
		CloseDeviceInstance = (CLOSEDEVICEINSTANCE)GetProcAddress(hDLL, "DAPI_CloseDeviceInstance");
		DetectDevice = (DETECTDEVICE)GetProcAddress(hDLL, "DAPI_DetectDevice");
		GetDeviceCount = (GETDEVICECOUNT)GetProcAddress(hDLL, "DAPI_GetDeviceCount");
		GetDeviceInfo = (GETDEVICEINFO)GetProcAddress(hDLL, "DAPI_GetDeviceInfo");
		OpenDeviceBySerialId = (OPENDEVICEBYSERIALID)GetProcAddress(hDLL, "DAPI_OpenDeviceBySerialId");
		GetSerialId = (GETSERIALID)GetProcAddress(hDLL, "DAPI_GetSerialId");
		ReadI2c = (READI2C)GetProcAddress(hDLL, "DAPI_ReadI2c");
		WriteI2c = (WRITEI2C)GetProcAddress(hDLL, "DAPI_WriteI2c");
	}
	else
	{
		// unable to get handle to dll, no sense continuing, exit application
		exit(0);
	}

#if 1
	//NumDevices = DAPI_GetDeviceInfo("UsbI2cIo", &DeviceInfo);
	NumDevices = GetDeviceInfo("UsbI2cIo", &DeviceInfo);
	if (NumDevices > 0)
	{
		hDevInstance = OpenDeviceBySerialId("UsbI2cIo", (LPSTR)(DeviceInfo.SerialId));
		Status = DetectDevice(hDevInstance);
	}
#else

	NumDevices = GetDeviceCount("UsbI2cIo");
	if (NumDevices > 0)
	{
		hDevInstance = OpenDeviceInstance("UsbI2cIo", 0);
		if (hDevInstance != INVALID_HANDLE_VALUE)
		{
			bool Status = GetSerialId(hDevInstance, (LPSTR)DeviceId);
			CloseDeviceInstance(hDevInstance);
			//hDevInstance = DAPI_OpenDeviceBySerialId("UsbI2cIo", DeviceId);
		}
	}
#endif
	if ((NumDevices == 0) || (hDevInstance == INVALID_HANDLE_VALUE) || (Status != true))
	{
		printf("No DeVaSys I2C available for communication!! \n");
		return false;
	}

	return true;
}

bool DEVASYS_I2C_WriteI2C(uint32_t WriteDataLength, uint8_t* WriteData)
{
	long lBytesWritten = 0;

    if (WriteDataLength > 64)
	{
		printf("The maximum number of bytes per transfer is limited to 64!!! \n");
	}

	// Setup for I2C communication
	TransI2c.wCount = WriteDataLength;
	for (uint32_t Index = 0; Index < WriteDataLength; Index++)
	{
		TransI2c.Data[Index] = WriteData[Index];
	}

	// Make sure the I2C device is still attached
	BOOL Status = DetectDevice(hDevInstance);
	if (!Status)
	{
		hDevInstance = OpenDeviceBySerialId("UsbI2cIo", (LPSTR)(DeviceInfo.SerialId));
	}

	lBytesWritten = WriteI2c(hDevInstance, &TransI2c);
	if (lBytesWritten != TransI2c.wCount)
	{
		printf("Number of bytes written does not match Data Length!!! \n");
		return false;
	}

	return true;
}

bool DEVASYS_I2C_ReadI2C(uint32_t ReadDataLength, uint8_t* ReadData)
{
	long lBytesRead = 0;

	if (ReadDataLength > 64)
	{
		printf("The maximum number of bytes per transfer is limited to 64!!! \n");
	}

	// Make sure the I2C device is still attached
	BOOL Status = DetectDevice(hDevInstance);
	if (!Status)
	{
		hDevInstance = OpenDeviceInstance("UsbI2cIo", (BYTE)(DeviceInfo.SerialId));
	}

	// Setup for I2C communication
	TransI2c.wCount = ReadDataLength;
	lBytesRead = ReadI2c(hDevInstance, &TransI2c);
	if (lBytesRead != TransI2c.wCount)
	{
		printf("Number of bytes read does not match Data Length!!! \n");
		return false;
	}

	for (uint32_t Index = 0; Index < ReadDataLength; Index++)
	{
		ReadData[Index] = TransI2c.Data[Index];
	}
	return true;
}
