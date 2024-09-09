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
  * \file   dlpc343x_samples
  * \brief  Sample code for communicating with a DLPC34xx Display controller.
  */

#include "dlpc_common.h"
#include "dlpc34xx.h"
#include "cypress_i2c.h"
#include "devasys_i2c.h"
#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include "time.h"

#define CYPRESS_I2C_COMMUNICATION         1		/* set to 0 for DeVaSys I2C Communication */

#define MAX_WIDTH                         DLP2010_WIDTH
#define MAX_HEIGHT                        DLP2010_HEIGHT

#define FLASH_WRITE_BLOCK_SIZE            1024
#define FLASH_READ_BLOCK_SIZE             256

#define MAX_WRITE_CMD_PAYLOAD             (FLASH_WRITE_BLOCK_SIZE + 8)
#define MAX_READ_CMD_PAYLOAD              (FLASH_READ_BLOCK_SIZE  + 8)

static uint8_t                            s_WriteBuffer[MAX_WRITE_CMD_PAYLOAD];
static uint8_t                            s_ReadBuffer[MAX_READ_CMD_PAYLOAD];

bool I2CCommSucceed = false;

static FILE*								s_FilePointer;

/**
 * Implement the I2C write transaction here. The sample code here sends
 * data to the controller via the Cypress USB-Serial adapter.
 */
uint32_t WriteI2C(uint16_t             WriteDataLength,
	uint8_t*                           WriteData,
	DLPC_COMMON_CommandProtocolData_s* ProtocolData)
{
	bool Status = true;
	//printf("Write I2C Starts, length %d!!! \n", WriteDataLength);

	if (CYPRESS_I2C_COMMUNICATION)
	{
		Status = CYPRESS_I2C_WriteI2C(WriteDataLength, WriteData);
	}
	else
	{
		Status = DEVASYS_I2C_WriteI2C(WriteDataLength, WriteData);
	}

	if (Status != true)
	{
		printf("Write I2C Error!!! \n");
		return FAIL;
	}

	return SUCCESS;
}

/**
 * Implement the I2C write/read transaction here. The sample code here
 * receives data from the controller via the Cypress USB-Serial adapter.
 */
uint32_t ReadI2C(uint16_t              WriteDataLength,
	uint8_t*                           WriteData,
	uint16_t                           ReadDataLength,
	uint8_t*                           ReadData,
	DLPC_COMMON_CommandProtocolData_s* ProtocolData)
{
	bool Status = 0;
	//printf("Read/Write I2C Starts%d!!! \n", WriteDataLength);

	if (CYPRESS_I2C_COMMUNICATION)
	{
		Status = CYPRESS_I2C_WriteI2C(WriteDataLength, WriteData);
	}
	else
	{
		Status = DEVASYS_I2C_WriteI2C(WriteDataLength, WriteData);
	}

	if (Status != true)
	{
		printf("Write I2C Error!!! \n");
		return FAIL;
	}

	if (CYPRESS_I2C_COMMUNICATION)
	{
		Status = CYPRESS_I2C_ReadI2C(ReadDataLength, ReadData);
	}
	else
	{
		Status = DEVASYS_I2C_ReadI2C(ReadDataLength, ReadData);
	}

	if (Status != true)
	{
		printf("Read I2C Error!!! \n");
		return FAIL;
	}

	return SUCCESS;
}

/**
 * Initialize the command layer by setting up the read/write buffers and
 * callbacks.
 */
void InitConnectionAndCommandLayer()
{
	DLPC_COMMON_InitCommandLibrary(s_WriteBuffer,
		sizeof(s_WriteBuffer),
		s_ReadBuffer,
		sizeof(s_ReadBuffer),
		WriteI2C,
		ReadI2C);

	if (CYPRESS_I2C_COMMUNICATION)
	{
		I2CCommSucceed = CYPRESS_I2C_ConnectToCyI2C();
	}
	else
	{
		I2CCommSucceed = DEVASYS_I2C_ConnectToDevI2C();
	}
}

void WaitForSeconds(uint32_t Seconds)
{
	uint32_t retTime = (uint32_t)(time(0)) + Seconds;	// Get finishing time.
	while (time(0) < retTime);					        // Loop until it arrives.
}

void WriteTestPatternGridLines()
{
	/* Write Input Image Size */
	DLPC34XX_WriteInputImageSize(MAX_WIDTH, MAX_HEIGHT);

	/* Write Image Crop */
	DLPC34XX_WriteImageCrop(0, 0, MAX_WIDTH, MAX_HEIGHT);

	/* Write Display Size */
	DLPC34XX_WriteDisplaySize(0, 0, MAX_WIDTH, MAX_HEIGHT);

	/* Write Grid Lines */
	DLPC34XX_GridLines_s GridLines;
	GridLines.Border = DLPC34XX_BE_ENABLE;
	GridLines.BackgroundColor = DLPC34XX_C_GREEN;
	GridLines.ForegroundColor = DLPC34XX_C_MAGENTA;
	GridLines.HorizontalForegroundLineWidth = 0xF;
	GridLines.HorizontalBackgroundLineWidth = 0xF;
	GridLines.VerticalForegroundLineWidth = 0xF;
	GridLines.VerticalBackgroundLineWidth = 0xF;
	DLPC34XX_WriteGridLines(&GridLines);

	DLPC34XX_WriteOperatingModeSelect(DLPC34XX_OM_TEST_PATTERN_GENERATOR);
	WaitForSeconds(10);
}

void WriteTestPatternColorBar()
{
	/* Write Input Image Size */
	DLPC34XX_WriteInputImageSize(MAX_WIDTH, MAX_HEIGHT);

	/* Write Image Crop */
	DLPC34XX_WriteImageCrop(0, 0, MAX_WIDTH, MAX_HEIGHT);

	/* Write Display Size */
	DLPC34XX_WriteDisplaySize(0, 0, MAX_WIDTH, MAX_HEIGHT);

	/* Write Color Bar & Select Test Pattern */
	DLPC34XX_WriteColorbars(DLPC34XX_BE_ENABLE);

	DLPC34XX_WriteOperatingModeSelect(DLPC34XX_OM_TEST_PATTERN_GENERATOR);
	WaitForSeconds(10);
}

void WriteLookSelect(uint8_t LookNumber)
{
	/* Read Current Operating Mode Selected */
	DLPC34XX_OperatingMode_e OperatingMode;
	DLPC34XX_ReadOperatingModeSelect(&OperatingMode);

	/* Write RGB LED Current (based on Flash data) */
	DLPC34XX_WriteRgbLedCurrent(0x03E8, 0x03E8, 0x03E8);

	/* Select Look */
	DLPC34XX_WriteLookSelect(LookNumber);
	WaitForSeconds(5);

	/* Submit Write Splash Screen Execute if in Splash Mode */
	if ((OperatingMode == DLPC34XX_OM_SPLASH_SCREEN) ||
		(OperatingMode == DLPC34XX_OM_SENS_SPLASH_PATTERN))
	{
		DLPC34XX_WriteSplashScreenExecute();
		WaitForSeconds(5);
	}
	WaitForSeconds(10);
}

void WriteLabbCaic()
{
	//SetIntelliBright(bool EnableLabb, uint8_t LabbStrength, uint8_t LabbSharpness,
	//                 bool EnableCaic, double CaicGain, bool EnableCaicDisplay,
	//                 double *MaxPower, uint16_t *CaicRed, uint16_t *CaicGreen, uint16_t *CaicBlue)

	DLPC34XX_WriteLocalAreaBrightnessBoostControl(DLPC34XX_LC_MANUAL, 5, 60);		/* Enable LABB */
    DLPC34XX_WriteCaicImageProcessingControl(DLPC34XX_CGDS_P1024, true, 2.5, 0);	/* Enable CAIC */

	DLPC34XX_WriteLedOutputControlMethod(DLPC34XX_LCM_AUTOMATIC);

	double MaxPower;
	DLPC34XX_ReadCaicLedMaxAvailablePower(&MaxPower);

	uint16_t CaicRedCurrent, CaicGreenCurrent, CaicBlueCurrent;
	DLPC34XX_ReadCaicRgbLedCurrent(&CaicRedCurrent, &CaicGreenCurrent, &CaicBlueCurrent);

	DLPC34XX_OperatingMode_e CurrentOperatingMode;
	DLPC34XX_ReadOperatingModeSelect(&CurrentOperatingMode);

	if ((CurrentOperatingMode == DLPC34XX_OM_SPLASH_SCREEN) ||
		(CurrentOperatingMode == DLPC34XX_OM_SENS_SPLASH_PATTERN))
	{
		DLPC34XX_WriteSplashScreenExecute();
		WaitForSeconds(5);
	}
	WaitForSeconds(10);
}

void LoadFirmware()
{
	/* write up to 1024 bytes of data */
	uint8_t FlashDataArray[1024];

	/* Pattern File assumes to be in the \build\vs2017\dlpc343x folder */
	s_FilePointer = fopen("dlpc3470_7.4.0.img", "rb");
	if (!s_FilePointer)
	{
		printf("Error opening the flash image file!");
		return;
	}
	fseek(s_FilePointer, 0, SEEK_END);
	uint32_t FlashDataSize = ftell(s_FilePointer);
	fseek(s_FilePointer, 0, SEEK_SET);

	/* Select Flash Data Block and Erase the Block */
	DLPC34XX_WriteFlashDataTypeSelect(DLPC34XX_FDTS_ENTIRE_FLASH);
	DLPC34XX_WriteFlashErase();

	/* Read Short Status to make sure Erase is completed */
	DLPC34XX_ShortStatus_s ShortStatus;
	do
	{
		WaitForSeconds(1);
		DLPC34XX_ReadShortStatus(&ShortStatus);
	} while (ShortStatus.FlashEraseComplete == DLPC34XX_FE_NOT_COMPLETE);

	DLPC34XX_WriteFlashDataLength(1024);
	fread(FlashDataArray, sizeof(FlashDataArray), 1, s_FilePointer);
	DLPC34XX_WriteFlashStart(1024, FlashDataArray);

	int32_t BytesLeft = FlashDataSize - 1024;
	do
	{
		fread(FlashDataArray, sizeof(FlashDataArray), 1, s_FilePointer);
		DLPC34XX_WriteFlashContinue(1024, FlashDataArray);

		BytesLeft = BytesLeft - 1024;
	} while (BytesLeft > 0);

	fclose(s_FilePointer);
}

void main()
{
	DEBUG_PRINT("Starting the DLPC347x Sample Program...\n", "");

	InitConnectionAndCommandLayer();
	DEBUG_PRINT("Initialized Connection and Command Layer. I2CCommSucceed = %d\n", I2CCommSucceed);

	if (!I2CCommSucceed)
	{
		// printf("Error Init Connection & Command Layer!!!");
		DEBUG_PRINT("Error: Initialization of Connection & Command Layer failed! I2CCommSucceed = %d\n", I2CCommSucceed);
		return;
	}

	/* TI DLP Pico EVMs use a GPIO handshake scheme for Cypress I2C bus arbitration.
	 * Call to request/relinquish I2C Bus Access if using a TI EVM; remove otherwise.
	 * For DeVaSys I2C; need to manually request/relinquish before running the sample.
	 */
	if (CYPRESS_I2C_COMMUNICATION)
	{
		bool Status = CYPRESS_I2C_RequestI2CBusAccess();
		if (Status != true)
		{
			printf("Error Request I2C Bus ACCESS!!!");
			return;
		}
	}

	DLPC34XX_ControllerDeviceId_e DeviceId = 0;
	DLPC34XX_ReadControllerDeviceId(&DeviceId);
	printf("Controller Devicde Id = %d \n", DeviceId);

	uint16_t PixelsPerLine, LinesPerFrame;
	DLPC34XX_ReadInputImageSize(&PixelsPerLine, &LinesPerFrame);
	printf("Input Image Size = 0x%x, 0x%x \n", PixelsPerLine, LinesPerFrame);

	/* ***** Write Test Patterns ***** */
	WriteTestPatternGridLines();
	WriteTestPatternColorBar();

	WriteLabbCaic();

	bool LoadFromFirmware = true;
	if (LoadFromFirmware)
	{
		LoadFirmware();
	}

	if (CYPRESS_I2C_COMMUNICATION)
	{
		CYPRESS_I2C_RelinquishI2CBusAccess();
	}
}
