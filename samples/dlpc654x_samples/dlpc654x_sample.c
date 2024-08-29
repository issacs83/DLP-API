#include "dlpc654x_sample.h"
#include "../../api/dlpc_common.h"
#include "../../api/dlpc_common_private.h"

#ifdef _WIN32
#include "win_io.h"
#include <Windows.h> // Sleep()
#else
#include "unix_io.h"
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#include <time.h>
#include <string.h>

#define MODE_SWITCH_TIMEOUT_S 30

#define MAX_WIDTH                         DLP2010_WIDTH
#define MAX_HEIGHT                        DLP2010_HEIGHT

#define FLASH_WRITE_BLOCK_SIZE            1024
#define FLASH_READ_BLOCK_SIZE             256

#define MAX_WRITE_CMD_PAYLOAD             (FLASH_WRITE_BLOCK_SIZE + 8)
#define MAX_READ_CMD_PAYLOAD              (FLASH_READ_BLOCK_SIZE  + 8)

#define HEADER_LENGTH					  3

static uint8_t                            s_WriteBuffer[MAX_WRITE_CMD_PAYLOAD];
static uint8_t                            s_ReadBuffer[UINT16_MAX];

uint8_t doLog = 0;

char flashTableSignature[] = { 0xF7, 0xA5, 0x47, 0xAB, 0x7E, 0x51, 0x62, 0xA7 };

extern uint8_t variableLengthRead;

uint32_t doWrite(uint16_t writeDataLength,
	uint8_t* writeData,
	DLPC_COMMON_CommandProtocolData_s* protocolData)
{
	union MessageHeader header;
	header.headerStruct.destination = (uint8_t)protocolData->CommandDestination;
	header.headerStruct.opcodeLen = 0;
	header.headerStruct.hasDataLen = 1;
	header.headerStruct.hasChecksum = 0;
	header.headerStruct.replyReq = 1; // require ACK
	header.headerStruct.isRead = 0;
	// 0x51

	uint32_t fullWriteLength = HEADER_LENGTH + writeDataLength;

	if (fullWriteLength < 4)
	{
		return 1;
	}

	unsigned char* fullWriteData = (unsigned char*)malloc(fullWriteLength);

	if (!fullWriteData)
	{
		return 1;
	}

	fullWriteData[0] = header.headerInt;
	fullWriteData[1] = writeData[0];
	fullWriteData[2] = (writeDataLength - 1) & 0xFF; // remove opcode from length
	fullWriteData[3] = (writeDataLength - 1) >> 8;

	for (int32_t i = 0; i < writeDataLength - 1; ++i)
	{
		fullWriteData[i + 4] = writeData[i + 1];
	}

	if (doLog)
	{
		printf("WRITE ");

		for (uint32_t i = 0; i < fullWriteLength; i++)
		{
			printf("0x%02X ", (unsigned char)fullWriteData[i]);
		}
		printf("\n");
	}

	ioWrite(fullWriteData, fullWriteLength);
	
	if (header.headerStruct.replyReq)
	{
		// make room for 2 length bits
		uint8_t readLength = 2 + (header.headerStruct.hasDataLen * 2);
		char* readData = (char*)malloc(sizeof(char) * readLength);

		ioRead(readData, readLength);
	}

	return 0;
}

uint32_t doRead(uint16_t               writeDataLength,
	uint8_t*                           writeData,
	uint16_t                           readDataLength,
	uint8_t*                           readData,
	DLPC_COMMON_CommandProtocolData_s* protocolData)
{
	union MessageHeader header;
	header.headerStruct.destination = (uint8_t)protocolData->CommandDestination;
	header.headerStruct.opcodeLen = 0;
	header.headerStruct.hasDataLen = 1;
	header.headerStruct.hasChecksum = 0;
	header.headerStruct.replyReq = 1; // require ACK
	header.headerStruct.isRead = 1;
	// 0xD1 || 0b11010001

	uint32_t fullWriteLength = HEADER_LENGTH + writeDataLength;

	if (fullWriteLength < 4)
	{
		return 1;
	}

	unsigned char* fullWriteData = (unsigned char*)malloc(fullWriteLength);

	if (!fullWriteData)
	{
		return 1;
	}

	fullWriteData[0] = header.headerInt;
	fullWriteData[1] = writeData[0];
	fullWriteData[2] = (writeDataLength - 1) & 0xFF; // remove opcode from length
	fullWriteData[3] = (writeDataLength - 1) >> 8;

	for (int32_t i = 0; i < writeDataLength - 1; ++i)
	{
		fullWriteData[i + 4] = writeData[i + 1];
	}

	if (doLog)
	{
		printf("READ WRITE ");

		for (uint32_t i = 0; i < fullWriteLength; i++)
		{
			printf("0x%02X ", (unsigned char)fullWriteData[i]);
		}
		printf("\n");
	}

	ioWrite(fullWriteData, fullWriteLength);

	if (variableLengthRead)
	{
		protocolData->BytesRead = ioRead(readData, HEADER_LENGTH);

		uint16_t responseLength = readData[2];
		responseLength = responseLength << 8;
		responseLength += readData[1];

		if (doLog)
		{
			printf("Variable read. %d bytes\n", responseLength);
		}
		if (responseLength > 0)
		{
			for (int32_t i = 0; i < readDataLength - HEADER_LENGTH; ++i)
			{
				// "shift" out the header
				readData[i] = readData[i + HEADER_LENGTH];
			}

			uint32_t readDataLength = responseLength < MAX_READ_CMD_PAYLOAD - HEADER_LENGTH ? responseLength : MAX_READ_CMD_PAYLOAD - HEADER_LENGTH;

			uint32_t remainingDataLength = responseLength;
			if (remainingDataLength > 1)
			{
				uint8_t* readBuffer = (uint8_t*)malloc(remainingDataLength);

				if (!readBuffer)
				{
					return 1;
				}

				protocolData->BytesRead = ioRead(readBuffer, remainingDataLength);

				for (uint16_t i = 0; i < remainingDataLength; ++i)
				{
					readData[i] = readBuffer[i];
				}
			}
		}
	}
	else //fixed-length read
	{
		protocolData->BytesRead = ioRead(readData, HEADER_LENGTH);
		if (protocolData->BytesRead != 0xFFFF) // if first read didn't fail
		{
			if (doLog)
			{
				printf("\nGOT: ");
				for (uint32_t i = 0; i < protocolData->BytesRead; ++i)
				{
					printf("0x%02X ", readData[i]);
				}
			}
			protocolData->BytesRead = ioRead(readData, readDataLength);
			if (protocolData->BytesRead != 0xFFFF) // if first read didn't fail
			{
				if (doLog)
				{
					for (uint32_t i = 0; i < protocolData->BytesRead; ++i)
					{
						printf("0x%02X ", readData[i]);
					}
				}
			}
			if (doLog)
			{
				printf("\n");
			}
		}
	}

	if (doLog && variableLengthRead && protocolData->BytesRead > 0 && protocolData->BytesRead != 0xFFFF)
	{
		printf("\nGOT ");
		for (uint32_t i = 0; i < protocolData->BytesRead; ++i)
		{
			printf("0x%02X ", readData[i]);
		}
		printf("\n");
	}

	variableLengthRead = 0;
	return 0;
}

uint32_t init()
{
	variableLengthRead = 0;
	DLPC_COMMON_InitCommandLibrary(s_WriteBuffer,
		sizeof(s_WriteBuffer),
		s_ReadBuffer,
		sizeof(s_ReadBuffer),
		doWrite,
		doRead);

	return ioInit();
}

SectorAddressAndSize* getSectorStartAddressesAndSize(/*out*/ uint32_t *numSectors)
{
	uint32_t numSectorInfos;
	if (numSectors == NULL)
	{
		return NULL;
	}

	variableLengthRead = 1;

	// This sends the same opcode to the projector (0x21) as the upcoming call to DLPC654X_ReadGetFlashSectorInformation.
	// From this first call, we only check the number of sector infos so we know how much space to allocate ahead of time
	// since the latter call expects space to be allocated already
	numSectorInfos = getSectorInfoCount();

    printf("numSectorInfos: %d\n", numSectorInfos);

	// getSectorInfoCount returns the number of each n-byte sector. For example, 5 2-byte sectors and 10 4-byte sectors
	DLPC654X_SectorInfo_s** sectorInfo = malloc(numSectorInfos * sizeof(DLPC654X_SectorInfo_s));

	SectorAddressAndSize* sectorAddressesAndSize = NULL;

	if (sectorInfo == NULL)
	{
		fprintf(IN_STDERR, "Failed to get number of sectors\n");
		return NULL;
	}

	for (uint32_t i = 0; i < numSectorInfos; ++i)
	{
		sectorInfo[i] = (DLPC654X_SectorInfo_s*)malloc(sizeof(DLPC654X_SectorInfo_s));
	}

	variableLengthRead = 1;
	DLPC654X_ReadGetFlashSectorInformation(sectorInfo);

	// calculate needed size of array
	for (uint32_t i = 0; i < numSectorInfos; ++i)
	{
        printf("numSectors in sectorInfo: %d\n", sectorInfo[i]->NumSectors);
		if (sectorInfo[i]->NumSectors <= 0)
		{
			break;
		}
		for (uint32_t sectorNum = 0; sectorNum < sectorInfo[i]->NumSectors; sectorNum++)
		{
			++(*numSectors);
		}
	}

	sectorAddressesAndSize = (SectorAddressAndSize*)malloc(*numSectors * sizeof(SectorAddressAndSize));

	if (sectorAddressesAndSize == NULL)
	{
		return NULL;
	}

	uint32_t nextSector = 0;
	uint32_t offset = 0;

	// populate array
	for (uint32_t i = 0; i < numSectorInfos; ++i)
	{
		for (uint32_t sectorNum = 0; sectorNum < sectorInfo[i]->NumSectors; sectorNum++)
		{
			sectorAddressesAndSize[nextSector].startAddress = offset;
			sectorAddressesAndSize[nextSector].size = sectorInfo[i]->SectorSize;
			offset += sectorInfo[i]->SectorSize;
			++nextSector;
		}
	}

	if (sectorInfo != NULL)
	{
		free(sectorInfo);
	}
	return sectorAddressesAndSize;
}

uint32_t getSectorInfoCount()
{
    uint32_t Status = 0;

    DLPC_COMMON_ClearWriteBuffer();
    DLPC_COMMON_ClearReadBuffer();

    DLPC_COMMON_PackOpcode(1, 0x21);

    DLPC_COMMON_SetCommandDestination(1);

    Status = DLPC_COMMON_SendRead(0xFFFF);

    if (Status == 0)
    {
		return (uint32_t)(DLPC_COMMON_GetBytesRead() / 6);
    }
	return 1;
}

uint32_t getStartOfFlashTable(SectorAddressAndSize* allSectors, uint32_t numSectors, char* flashImgFile)
{
	FILE* imgFile = fopen(flashImgFile, "rb");
	
	if (imgFile != NULL)
	{
		char c;
		for (uint32_t i = 0; i < numSectors; ++i)
		{
			uint8_t foundSignature = 1;
			fseek(imgFile, allSectors[i].startAddress, SEEK_SET);

			for (uint8_t j = 0; j < FLASH_TABLE_SIGNATURE_LENGTH; ++j)
			{
				c = fgetc(imgFile);
				if (c != flashTableSignature[j])
				{
					foundSignature = 0;
					break;
				}
			}

			if (foundSignature)
			{
				fclose(imgFile);
				return allSectors[i].startAddress;
			}
		}

		fclose(imgFile);
	}
	else
	{
		fprintf(IN_STDERR, "Failed to open file %s for reading\n", flashImgFile);
	}
	return UINT32_MAX;
}

uint32_t validateFlashImage(SectorAddressAndSize* allSectors, uint32_t numSectors, uint32_t startOfFlashTable, char* flashImgFile)
{
	FILE* imgFile = fopen(flashImgFile, "rb");
	if (imgFile == NULL)
	{
		fprintf(IN_STDERR, "Failed to open file %s for reading\n", flashImgFile);
		return 1;
	}

	fseek(imgFile, 0, SEEK_END);
	uint64_t flashImgSize = ftell(imgFile);
	fclose(imgFile);

	uint64_t totalFlashSize = 0;

	for (uint32_t i = 0; i < numSectors; ++i)
	{
		totalFlashSize += allSectors[i].size;
	}

	if (flashImgSize > totalFlashSize)
	{
		fprintf(IN_STDERR, "Flash image file size (%lu bytes) exceeds the device capacity (%lu bytes)\n", flashImgSize, totalFlashSize);
	}

	// TODO: handle "skip bootloader" case
	if (flashImgSize <= totalFlashSize)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

uint32_t changeMode(uint8_t doChange)
{
	DLPC654X_CmdModeT_e mode = 1;
	DLPC654X_CmdControllerConfigT_e config;

	uint32_t err = DLPC654X_ReadMode(&mode, &config);
	printf("Mode: ");
	if (mode == DLPC654X_CMT_BOOTLOADER)
	{
		printf("bootloader\n");
	}
	else
	{
		printf("main application\n");
	}

	if (err == 0)
	{
		if (doChange && mode != DLPC654X_CMT_BOOTLOADER)
		{
			printf("Changing mode\n");
			DLPC654X_CmdSwitchTypeT_e switchType = DLPC654X_CSTT_TO_BOOT;
			err = DLPC654X_WriteSwitchMode(switchType);
			if (err != 0)
			{
				fprintf(IN_STDERR, "Got error when switching to bootloader mode. Error code %u\n", err);
			}
		}
	}
	else
	{
		fprintf(IN_STDERR, "Got error when reading device mode. Error code %u\n", err);
	}


	if (!err && mode == DLPC654X_CMT_BOOTLOADER)
	{
		return 0;
	}
	else
	{
		return 1;
	}

}

uint32_t switchToBootloaderMode()
{
	Sleep(1000); // Wait for the system to boot up

	struct timespec startTime;
	struct timespec currentTime;
	timespec_get(&startTime, TIME_UTC);

	printf("Switching modes...\n");
	uint32_t notInBootloaderMode = changeMode(1);

	uint8_t reconnected = 0;

	while (notInBootloaderMode)
	{

		// 654x required reconnecting after switching to bootloader mode
		if (!reconnected)
		{
            printf("\nReconnecting...\n");
			reconnected = 1;
			Sleep(1000);
			disconnectDevice();
			Sleep(3250);
			ioInit();
			Sleep(250);
		}

		notInBootloaderMode = changeMode(0);

		timespec_get(&currentTime, TIME_UTC);

		if (currentTime.tv_sec - startTime.tv_sec > MODE_SWITCH_TIMEOUT_S)
		{
			fprintf(IN_STDERR, "Timeout reached. Could not switch to bootloader mode\n");
			break;
		}
		Sleep(500);
	}

	return notInBootloaderMode;
}

Checksum getChecksum(FILE* imgFile, SectorAddressAndSize sector)
{
	fseek(imgFile, sector.startAddress, SEEK_SET);
	Checksum checksum;
	checksum.simpleCheckSum = 0;
	checksum.sumOfSumChecksum = 0;

	unsigned char c;
	for (uint32_t i = 0; i < sector.size; ++i)
	{
		c = (unsigned char)fgetc(imgFile);
		checksum.simpleCheckSum += c;
		checksum.sumOfSumChecksum += checksum.simpleCheckSum;
	}

	return checksum;
}

AddressChecksum* getFlashImageChecksums(SectorAddressAndSize* allSectors, uint32_t numSectors, char* flashImgFile, /*out*/ uint32_t *numFlashImgSectors)
{
	FILE* imgFile = fopen(flashImgFile, "rb");

	AddressChecksum* addressChecksums = (AddressChecksum*)malloc(numSectors * sizeof(AddressChecksum));

	if (!addressChecksums)
	{
		return NULL;
	}

	fseek(imgFile, 0, SEEK_END);
	uint64_t flashImgSize = ftell(imgFile);

	fseek(imgFile, 0, SEEK_SET);

	*numFlashImgSectors = 0;

	for (uint32_t i = 0; i < numSectors; ++i)
	{
		if (allSectors[i].startAddress > flashImgSize)
		{
			break;
		}

		addressChecksums[i].checksum = getChecksum(imgFile, allSectors[i]);
		addressChecksums[i].startAddress = allSectors[i].startAddress;

		++(*numFlashImgSectors);
	}
	fclose(imgFile);

	return addressChecksums;
}

Checksum getFlashSectorChecksum(SectorAddressAndSize sector)
{
	Checksum checksum;
	checksum.simpleCheckSum = 0;
	checksum.sumOfSumChecksum = 0;
	DLPC654X_ReadChecksum(sector.startAddress, sector.size, &(checksum.simpleCheckSum), &(checksum.sumOfSumChecksum));

	return checksum;
}

SectorAddressAndSize* getSectorsToProgram(SectorAddressAndSize* allSectors,
	uint32_t numSectors,
	AddressChecksum* flashImgChecksums,
	uint32_t numFlashImgChecksums,
	uint32_t startOfFlashTable,
	uint8_t updateModifiedSectorsOnly,
	uint32_t *numSectorsToProgram,
	enum FlashType flashType)
{
	*numSectorsToProgram = 0;

	SectorAddressAndSize* sectorsToProgram = (SectorAddressAndSize*)malloc(numSectors * sizeof(SectorAddressAndSize));
	SectorAddressAndSize flashSector = allSectors[0];

	if (!sectorsToProgram)
	{
		return NULL;
	}

	uint8_t addedFlashSector = 0;

	for (uint32_t i = 0; i < numFlashImgChecksums; ++i)
	{
		uint8_t isFlashSector = 0;
		SectorAddressAndSize sector = allSectors[i];
		if (sector.startAddress == startOfFlashTable)
		{
			isFlashSector = 1;
			addedFlashSector = 1;
			flashSector = sector;
		}

		if (!isFlashSector
			&& ((sector.startAddress < startOfFlashTable && (flashType & BOOTLOADER) == 0)
			|| sector.startAddress >= startOfFlashTable && (flashType & APPLICATION) == 0))
		{
			continue;
		}

		if (updateModifiedSectorsOnly)
		{
			printf("Building the list of sectors to program (%d of %d)\n", i + 1, numFlashImgChecksums);

			Checksum flashSectorChecksum = getFlashSectorChecksum(sector);

			if (!isFlashSector // always include flash sector
				&& flashSectorChecksum.simpleCheckSum == flashImgChecksums[i].checksum.simpleCheckSum
				&& flashSectorChecksum.sumOfSumChecksum == flashImgChecksums[i].checksum.sumOfSumChecksum)
			{
				printf("Skipping sector (%d)\n", i + 1);
				continue;
			}
			else
			{
				printf("Adding sector (%d)\n", i + 1);
			}

		}
		sectorsToProgram[(*numSectorsToProgram)++] = sector;
	}

	// if we only added the flash sector, don't update anything
	if (addedFlashSector && (*numSectorsToProgram) == 1 || (*numSectorsToProgram) == 0)
	{
		free(sectorsToProgram);
		sectorsToProgram = NULL;
	}

	return sectorsToProgram;
}

uint32_t setLockedForFlashProgramming(uint8_t locked)
{
	DLPC654X_CmdFlashUpdateT_e lockState;
	if (locked)
	{
		lockState = DLPC654X_CFUT_LOCK;
	}
	else // unlock
	{
		lockState = DLPC654X_CFUT_UNLOCK;
	}
	return DLPC654X_WriteUnlockFlashForUpdate(lockState);
}

uint32_t eraseSectors(SectorAddressAndSize* sectorsToProgram, uint32_t numSectors,
	uint32_t startOfFlashTable, enum FlashType flashType)
{
	for (uint32_t i = 0; i < numSectors; ++i)
	{
		uint32_t retVal = 0;
		if ((flashType & BOOTLOADER) == BOOTLOADER && sectorsToProgram[i].startAddress < startOfFlashTable)
		{
			printf("Erasing boot sector (%d of %d)\n", i + 1, numSectors);
			retVal = DLPC654X_WriteEraseSector(sectorsToProgram[i].startAddress);
		}
		if ((flashType & APPLICATION) == APPLICATION && sectorsToProgram[i].startAddress >= startOfFlashTable)
		{
			printf("Erasing app  sector (%d of %d)\n", i + 1, numSectors);
			retVal = DLPC654X_WriteEraseSector(sectorsToProgram[i].startAddress);
		}

		if (retVal)
		{
			return retVal;
		}
	}
	return 0;
}

uint32_t programSector(FILE* imgFile, SectorAddressAndSize sector)
{
	char buffer[MAX_BLOCK_SIZE];
	uint32_t bufferLimit = MAX_BLOCK_SIZE; // to avoid reallocations, track the end of the usable section of the buffer

	fseek(imgFile, 0, SEEK_END);
	uint64_t flashImgSize = ftell(imgFile);

	fseek(imgFile, sector.startAddress, SEEK_SET);

	uint32_t bytesToProgram = sector.size < (flashImgSize - sector.startAddress) ? sector.size : (flashImgSize - sector.startAddress);

	// force even writes
	if (bytesToProgram % 2 != 0)
	{
		++bytesToProgram;
		printf("Resizing bytesToProgram\n");
	}

	uint32_t retVal = DLPC654X_WriteInitializeFlashReadWriteSettings(sector.startAddress, bytesToProgram);
	if (retVal)
	{
		return retVal;
	}

	for (int64_t remainingBytes = bytesToProgram, offset = sector.startAddress;
		remainingBytes > 0;
		remainingBytes -= MAX_BLOCK_SIZE, offset += MAX_BLOCK_SIZE)
	{
		if (remainingBytes < MAX_BLOCK_SIZE)
		{
			bufferLimit = remainingBytes;
		}

		size_t read = fread(buffer, 1, bufferLimit, imgFile);

		if (read > 0)
		{
			if (read % 2 != 0)
			{
				buffer[read] = 0xFF;
				++read;
				printf("Resizing read\n");
			}
		}
		else
		{
			continue;
		}

		if (read == bufferLimit)
		{
			DLPC654X_WriteFlashWrite(bufferLimit, buffer);
		}
		else
		{
			DLPC654X_WriteFlashWrite(read, buffer);
		}
	}
	return 0;
}

uint32_t programSectors(SectorAddressAndSize* sectorsToProgram,
	uint32_t numSectorsToProgram,
	uint32_t startOfFlashTable,
	char* flashImgFile,
	enum FlashType flashType)
{
	uint32_t dataBytesToProgram = 0;
	for (uint32_t i = 0; i < numSectorsToProgram; ++i)
	{
		dataBytesToProgram += sectorsToProgram[i].size;
	}

	FILE* imgFile = fopen(flashImgFile, "rb");
	
	if (imgFile != NULL)
	{
		for (uint32_t i = 0; i < numSectorsToProgram; ++i)
		{
			SectorAddressAndSize sector = sectorsToProgram[i];
			if ((flashType & BOOTLOADER) == BOOTLOADER && sectorsToProgram[i].startAddress < startOfFlashTable)
			{
				printf("Flashing boot sector (%d of %d)\n", i + 1, numSectorsToProgram);
				programSector(imgFile, sector);
			}
			if ((flashType & APPLICATION) == APPLICATION && sectorsToProgram[i].startAddress >= startOfFlashTable)
			{
				printf("Flashing app  sector (%d of %d)\n", i + 1, numSectorsToProgram);
				// don't write the flash header
				if (sector.startAddress == startOfFlashTable)
				{
					SectorAddressAndSize modifiedSector;
					modifiedSector.startAddress = sector.startAddress + FLASH_TABLE_SIGNATURE_LENGTH;
					modifiedSector.size = sector.size - FLASH_TABLE_SIGNATURE_LENGTH;
					programSector(imgFile, modifiedSector);
				}
				else
				{
					programSector(imgFile, sector);
				}
			}

		}
		fclose(imgFile);
	}
	return 0;
}

uint32_t programFlashTableSignature(uint32_t startOfFlashTable)
{
	DLPC654X_WriteInitializeFlashReadWriteSettings(startOfFlashTable, FLASH_TABLE_SIGNATURE_LENGTH);
	DLPC654X_WriteFlashWrite(FLASH_TABLE_SIGNATURE_LENGTH, flashTableSignature);
	return 0;
}

uint32_t verifyFlashData(SectorAddressAndSize* allSectors,
	uint32_t numSectors,
	AddressChecksum* flashImgCheckSums,
	uint32_t numFlashImgChecksums,
	uint32_t startOfFlashTable,
	enum FlashType flashType)
{ 

	uint8_t mismatchCount = 0;
	for (uint32_t i = 0; i < numFlashImgChecksums; ++i)
	{
		SectorAddressAndSize sector = allSectors[i];

		uint8_t isFlashSector = 0;
		if (sector.startAddress == startOfFlashTable)
		{
			isFlashSector = 1;
		}

		if ((sector.startAddress < startOfFlashTable && (flashType & BOOTLOADER) != BOOTLOADER)
			|| sector.startAddress >= startOfFlashTable && (flashType & APPLICATION) != APPLICATION)
		{
			continue;
		}

		printf("Verifying sector (%d of %d)\n", i + 1, numFlashImgChecksums);

		Checksum flashSectorChecksum = getFlashSectorChecksum(sector);

		if (!isFlashSector 
			&& flashSectorChecksum.simpleCheckSum != flashImgCheckSums[i].checksum.simpleCheckSum
			|| flashSectorChecksum.sumOfSumChecksum != flashImgCheckSums[i].checksum.sumOfSumChecksum)
		{
			printf("Sector (%d of %d) checksum mismatch!\n", i + 1, numFlashImgChecksums);
			++mismatchCount;
		}
	}
	return mismatchCount;
}

uint32_t switchToMainApp()
{
	DLPC654X_CmdSwitchTypeT_e switchType = DLPC654X_CSTT_TO_APP_VIA_RESET;
	return DLPC654X_WriteSwitchMode(switchType);
}

int doFlashUpdate(char* filePath, uint8_t flashModifiedSectorsOnly, enum FlashType flashType)
{
	if (!filePath)
	{
		printf("Usage: dlpc654x_sample.exe /path/to/img/file\n");
		return 1;
	}

	uint32_t err = init();

	if (!err)
	{
		err = switchToBootloaderMode();
		if (err)
		{
			fprintf(IN_STDERR, "Could not switch to bootloader mode\n");
		}
	}

	uint32_t numSectors = 0;
	SectorAddressAndSize* allSectors = NULL;
	if (!err)
	{
		allSectors = getSectorStartAddressesAndSize(&numSectors);
		if (allSectors == NULL)
		{
			fprintf(IN_STDERR, "Could not read sector information\n");
			err = 1;
		}
		printf("numSectors: %u\n", numSectors);
	}

	uint32_t startOfFlashTable;

	if (!err)
	{
		startOfFlashTable = getStartOfFlashTable(allSectors, numSectors, filePath);

		printf("startOfFlashTable: %u\n", startOfFlashTable);

		if (validateFlashImage(allSectors, numSectors, startOfFlashTable, filePath))
		{
			printf("Flash image failed validation\n");
			err = 1;
		}
		else
		{
			printf("Flash image passed validation\n");
		}
	}

	uint32_t numFlashImgChecksums = 0;
	AddressChecksum* flashImgChecksums = NULL;
	
	if (!err)
	{
		flashImgChecksums = getFlashImageChecksums(allSectors, numSectors, filePath, &numFlashImgChecksums);

		if (flashImgChecksums == NULL)
		{
			fprintf(IN_STDERR, "Could not gather flash image checksums\n");
			err = 1;
		}

		printf("numFlashImgChecksums: %u\n", numFlashImgChecksums);
	}

	uint32_t numSectorsToProgram = 0;
	SectorAddressAndSize* sectorsToProgram = NULL;
	
	if (!err)
	{
		sectorsToProgram = getSectorsToProgram(allSectors,
			numSectors,
			flashImgChecksums,
			numFlashImgChecksums,
			startOfFlashTable,
			flashModifiedSectorsOnly,
			&numSectorsToProgram,
			flashType);

		if (sectorsToProgram == NULL)
		{
			fprintf(IN_STDERR, "No sectors to program\n");
			err = 1;
		}
	}

	if (!err)
	{
		if (setLockedForFlashProgramming(0))
		{
			fprintf(IN_STDERR, "Could not unlock for programming\n");
			err = 1;
		}
	}

	if (!err)
	{
		if ((flashType & BOOTLOADER) == BOOTLOADER)
		{
			eraseSectors(sectorsToProgram, numSectorsToProgram, startOfFlashTable, BOOTLOADER);

			programSectors(sectorsToProgram, numSectorsToProgram, startOfFlashTable, filePath, BOOTLOADER);
		}

		if ((flashType & APPLICATION) == APPLICATION)
		{
			eraseSectors(sectorsToProgram, numSectorsToProgram, startOfFlashTable, APPLICATION);

			programSectors(sectorsToProgram, numSectorsToProgram, startOfFlashTable, filePath, APPLICATION);
		}

		if (flashType > 0)
		{
			programFlashTableSignature(startOfFlashTable);
		}

		setLockedForFlashProgramming(1);

		// Sometimes, bootloader sector gets incorrectly reported as being incorrect.
		// The cause has not been identified, so let's skip the verification step if
		// we only program the bootloader
		if ((flashType & APPLICATION) == APPLICATION)
		{
			verifyFlashData(allSectors, numSectors, flashImgChecksums, numFlashImgChecksums, startOfFlashTable, flashType);
		}

		switchToMainApp();
	}

	if (allSectors != NULL)
	{
		free(allSectors);
	}
	if (flashImgChecksums != NULL)
	{
		free(flashImgChecksums);
	}

	return 0;
}

int doFastBootloaderUpdate(uint8_t* bootloaderData, uint32_t bootloaderSize)
{
	if (!bootloaderData)
	{
		printf("Error: Invalid bootloader data\n");
		return 1;
	}

	uint32_t err = init();

	if (!err)
	{
		err = switchToBootloaderMode();
		if (err)
		{
			fprintf(IN_STDERR, "Could not switch to bootloader mode\n");
		}
	}

	uint32_t numSectors = 0;
	SectorAddressAndSize* allSectors = NULL;
	if (!err)
	{
		allSectors = getSectorStartAddressesAndSize(&numSectors);
		if (allSectors == NULL)
		{
			fprintf(IN_STDERR, "Could not read sector information\n");
			err = 1;
		}
		printf("numSectors: %u\n", numSectors);
	}

	uint32_t startOfFlashTable;

	if (!err)
	{
		startOfFlashTable = 0x20000;

		printf("startOfFlashTable: %u\n", startOfFlashTable);
	}

	uint32_t numFlashImgChecksums = 0;
	AddressChecksum* flashImgChecksums = NULL;
	
	uint32_t numSectorsToProgram = 0;
	SectorAddressAndSize* sectorsToProgram = NULL;
	
	if (!err)
	{
		if (setLockedForFlashProgramming(0))
		{
			fprintf(IN_STDERR, "Could not unlock for programming\n");
			err = 1;
		}
	}

	if (!err)
	{
		DLPC654X_WriteEraseSector(0x00000000);

		FILE* imgFile = tmpfile();
		SectorAddressAndSize sector = { 0x20000, 0 };

		uint32_t written = 0;

		while (written < sector.size)
		{
			int toWrite;

			if (written < bootloaderSize)
			{
				toWrite = bootloaderData[written];
			}
			else
			{
				toWrite = 0xFF;
			}

			fputc(toWrite, imgFile);
			++written;
		}

		rewind(imgFile);

		programSector(imgFile, sector);

		fclose(imgFile);

		programFlashTableSignature(startOfFlashTable);

		setLockedForFlashProgramming(1);

		switchToMainApp();
	}

	if (allSectors != NULL)
	{
		free(allSectors);
	}
	if (flashImgChecksums != NULL)
	{
		free(flashImgChecksums);
	}

	return 0;
}
