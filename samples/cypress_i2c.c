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
 * \file
 * \brief  Sample code for I2C communication via Cypress USB-Serial 
 *         Bridge Controller.
 *
 */

#include "cypress_i2c.h"
#include "CyUSBSerial.h"
#include "time.h"
#include <stdio.h>

#define REQUEST_I2C_ACCESS_GPIO    5
#define I2C_ACCESS_GRANTED_GPIO    6
#define START_I2C_TRANSACTION_GPIO 9
#define I2C_CLOCK_FREQUENCY_HZ     100000
#define DLP_I2C_SLAVE_ADDRESS      (0X36 >> 1)
#define I2C_TIMEOUT_MILLISECONDS   500   

static CY_HANDLE          s_Handle;
static CY_I2C_DATA_CONFIG s_DataConfig;

/* Gets the handle of the connected Cypress USB-Serial bridge controller */
bool GetCyI2CHandle(CY_HANDLE* Handle)
{
    CY_RETURN_STATUS Status;
    CY_DEVICE_INFO   DeviceInfo;
    uint8_t          NumDevices = 0;
    uint8_t          DeviceIdx;
    uint8_t          InterfaceIdx;

    Status = CyGetListofDevices(&NumDevices);
    DEBUG_PRINT_VARS("Number of devices found: %d\n", NumDevices);

    if ((Status != CY_SUCCESS) || (NumDevices == 0))
    {
        DEBUG_PRINT_VARS("Failed to get list of devices, Status: %d\n", Status);
        return false;
    }

    for (DeviceIdx = 0; DeviceIdx < NumDevices; DeviceIdx++)
    {
        Status = CyGetDeviceInfo(DeviceIdx, &DeviceInfo);
        if (Status != CY_SUCCESS)
        {
            DEBUG_PRINT_VARS("Failed to get device info for DeviceIdx %d, Status: %d\n", DeviceIdx, Status);
            continue;
        }

        for (InterfaceIdx = 0; InterfaceIdx < DeviceInfo.numInterfaces; InterfaceIdx++)
        {
            if (DeviceInfo.deviceType[InterfaceIdx] == CY_TYPE_I2C)
            {
                Status = CyOpen(DeviceIdx, InterfaceIdx, Handle);
                if (Status == CY_SUCCESS)
                {
                    DEBUG_PRINT_VARS("I2C handle obtained successfully for DeviceIdx %d, InterfaceIdx %d\n", DeviceIdx, InterfaceIdx);
                    return true;
                }
                else
                {
                    DEBUG_PRINT_VARS("Failed to open I2C handle, Status: %d\n", Status);
                }
            }
        }
    }

    DEBUG_PRINT_VARS("Failed to obtain I2C handle\n");
    return false;
}

bool CYPRESS_I2C_GetCyGpio(uint8_t GpioNum, uint8_t* Value) 
{
    DEBUG_PRINT_VARS("Getting GPIO Value for GpioNum %d\n", GpioNum);
    return CyGetGpioValue(s_Handle, GpioNum, Value) == CY_SUCCESS;
}

bool CYPRESS_I2C_SetCyGpio(uint8_t GpioNum, uint8_t Value)
{
    DEBUG_PRINT_VARS("Setting GPIO Value for GpioNum %d to %d\n", GpioNum, Value);
    return CySetGpioValue(s_Handle, GpioNum, Value) == CY_SUCCESS;
}

bool CYPRESS_I2C_RequestI2CBusAccess()
{
    uint8_t Value     = 0;
    time_t  StartTime = time(NULL);

    DEBUG_PRINT_VARS("Requesting I2C Bus Access\n");

    if (!CYPRESS_I2C_SetCyGpio(REQUEST_I2C_ACCESS_GPIO, 1))
    {
        DEBUG_PRINT_VARS("Failed to set GPIO for I2C access request\n");
        return false;
    }

    while ((time(NULL) - StartTime) < I2C_TIMEOUT_MILLISECONDS)
    {
        if (!CYPRESS_I2C_GetCyGpio(I2C_ACCESS_GRANTED_GPIO, &Value))
        {
            DEBUG_PRINT_VARS("Failed to get GPIO value for I2C access granted\n");
            break;
        }

        if (Value == 1)
        {
            if (!CYPRESS_I2C_SetCyGpio(START_I2C_TRANSACTION_GPIO, 1))
            {
                DEBUG_PRINT_VARS("Failed to set GPIO for starting I2C transaction\n");
                break;
            }

            CyI2cReset(s_Handle, false);
            CyI2cReset(s_Handle, true);

            DEBUG_PRINT_VARS("I2C Bus Access granted successfully\n");
            return true;
        }
    }

    DEBUG_PRINT_VARS("Request I2C Bus Access timed out or failed\n");
    return false;
}

bool CYPRESS_I2C_RelinquishI2CBusAccess()
{
    DEBUG_PRINT_VARS("Relinquishing I2C Bus Access\n");
    return CYPRESS_I2C_SetCyGpio(REQUEST_I2C_ACCESS_GPIO, 0) 
        && CYPRESS_I2C_SetCyGpio(START_I2C_TRANSACTION_GPIO, 0);
}

bool CYPRESS_I2C_WriteI2C(uint32_t WriteDataLength, uint8_t* WriteData)
{
    CY_DATA_BUFFER   WriteBuffer;
    CY_RETURN_STATUS Status;

    DEBUG_PRINT_VARS("Writing to I2C, Data Length: %d\n", WriteDataLength);

    WriteBuffer.buffer        = WriteData;
    WriteBuffer.length        = WriteDataLength;
    WriteBuffer.transferCount = 0;
    
    Status = CyI2cWrite(s_Handle, 
                        &s_DataConfig,
                        &WriteBuffer,
                        I2C_TIMEOUT_MILLISECONDS);
    if (Status != CY_SUCCESS)
    {
        DEBUG_PRINT_VARS("Write I2C Error, Status: %d\n", Status);
        CyI2cReset(s_Handle, false);
        CyI2cReset(s_Handle, true);
        return false;
    }
    
    DEBUG_PRINT_VARS("I2C Write completed successfully\n");
    return true;
}

bool CYPRESS_I2C_ReadI2C(uint32_t ReadDataLength, uint8_t* ReadData)
{
    CY_DATA_BUFFER   ReadBuffer;
    CY_RETURN_STATUS Status;

    DEBUG_PRINT_VARS("Reading from I2C, Data Length: %d\n", ReadDataLength);

    ReadBuffer.buffer        = ReadData;
    ReadBuffer.length        = ReadDataLength;
    ReadBuffer.transferCount = 0;

    Status = CyI2cRead(s_Handle,
                       &s_DataConfig,
                       &ReadBuffer,
                       I2C_TIMEOUT_MILLISECONDS);
    if ((Status != CY_SUCCESS) && (Status != CY_ERROR_IO_TIMEOUT))
    {
        DEBUG_PRINT_VARS("Read I2C Error, Status: %d\n", Status);
        CyI2cReset(s_Handle, false);
        CyI2cReset(s_Handle, true);
        return false;
    }

    DEBUG_PRINT_VARS("I2C Read completed successfully\n");
    return true;
}

bool CYPRESS_I2C_ConnectToCyI2C()
{
    CY_RETURN_STATUS Status;	
    CY_I2C_CONFIG I2CConfig;

    // Initialize the Cypress USB Serial library
    Status = CyLibraryInit();
    if (Status != CY_SUCCESS)
    {
        DEBUG_PRINT_VARS("Failed to initialize Cypress USB Serial Library, Status: %d\n", Status);
        return false;
    }
        
    if (!GetCyI2CHandle(&s_Handle))
    {
        DEBUG_PRINT_VARS("Failed to get I2C handle\n");
        return false;
    }

    I2CConfig.frequency      = I2C_CLOCK_FREQUENCY_HZ;
    I2CConfig.slaveAddress   = 0x30;
    I2CConfig.isMaster       = true;
    I2CConfig.isClockStretch = false;
    
    Status = CySetI2cConfig(s_Handle, &I2CConfig);
    if (Status != CY_SUCCESS)
    {
		// printf("Connect to I2C Error %d!!! \n", Status);
        DEBUG_PRINT_VARS("Connect to I2C Error, Status: %d\n", Status);
        return false;
    }

    s_DataConfig.isNakBit     = true;
    s_DataConfig.isStopBit    = true;
    s_DataConfig.slaveAddress = DLP_I2C_SLAVE_ADDRESS;

    return true;
}
