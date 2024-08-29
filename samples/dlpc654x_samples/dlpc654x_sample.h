#ifndef DLPC654X_SAMPLE_H
#define DLPC654X_SAMPLE_H

#include <stdio.h>
#include "../../api/dlpc654x.h"

#ifdef _WIN32
#include <Windows.h> // sleep()
#include <winusb.h>
#include <usb100.h>
#include <SetupAPI.h>
#include <shlwapi.h> // StrStrI
#define IN_STDERR stdout
#else
#include <unistd.h> // sleep()
#define IN_STDERR stderr
#endif

#ifdef _WIN32
#  define EXPORTFUNC __declspec( dllexport )
#else
#  define EXPORTFUNC
#endif

#define MAX_BLOCK_SIZE 500
#define FLASH_TABLE_SIGNATURE_LENGTH 8

// use of bitfields here ( field : length) makes the full header 1 byte long
typedef struct
{
	uint8_t destination : 3;
	uint8_t opcodeLen : 1; // 1 == two bytes, 0 == 1 byte
	uint8_t hasDataLen : 1;
	uint8_t hasChecksum : 1;
	uint8_t replyReq : 1;
	uint8_t isRead : 1;
} _MessageHeader;

// so we can read the header struct as an integer without the C compiler complaining
union MessageHeader
{
	_MessageHeader headerStruct;
	uint8_t        headerInt;
};

typedef struct
{
	uint32_t size;
	uint32_t startAddress;
} SectorAddressAndSize;

typedef struct
{
	uint32_t simpleCheckSum;
	uint32_t sumOfSumChecksum;
} Checksum;

typedef struct
{
	uint32_t startAddress;
	Checksum checksum;
} AddressChecksum;

enum FlashType
{
	NONE = 0,
	BOOTLOADER = 1,
	APPLICATION = 2,
	ALL = 3
};

/*
 * @brief writes data to WinUSB pipe. Should be registered as a callback with DLPC_COMMON_InitCommandLibrary
*/
EXPORTFUNC uint32_t doWrite(uint16_t           writeDataLength,
	uint8_t*                           writeData,
	DLPC_COMMON_CommandProtocolData_s* protocolData);

/*
 * @brief reads/writes data to WinUSB pipe. Should be registered as a callback with DLPC_COMMON_InitCommandLibrary
*/
EXPORTFUNC uint32_t doRead(uint16_t            WriteDataLength,
	uint8_t*                           WriteData,
	uint16_t                           ReadDataLength,
	uint8_t*                           ReadData,
	DLPC_COMMON_CommandProtocolData_s* ProtocolData);

/*
 * @brief initializes DLPC Common InitCommandLibrary and connects to projector
 * @return uint32_t indicating failure(>0)/success(0)
*/
EXPORTFUNC uint32_t init();

/*
 * @brief attempts to change the projector to bootloader mode, if not in it already
 * @param doChange toggles whether or not we should write the change mode command,
                   or just check if the mode changed has occured.
 * @return 0 if the projector is in bootloader mode, 1 otherwise
*/
EXPORTFUNC uint32_t changeMode(uint8_t doChange);

/*
 * @brief uses changeMode() to switch to bootloader mode, periodically checking
          if the change has finished for up to MODE_SWITCH_TIMEOUT_S seconds
 * @return  0 if the projector ends in bootloader mode, 1 otherwise
*/
EXPORTFUNC uint32_t switchToBootloaderMode();

/*
 * @brief reads sector information from the projector
 * @param numSectors [OUT] the number of sectors on the chip
 * @return an array representing each sector on the chip
*/
EXPORTFUNC SectorAddressAndSize* getSectorStartAddressesAndSize(/*out*/ uint32_t *numSectors);

/*
 * @brief reads how many differently-sized sectors there are on the chip
 * @return a value indicating how many SectorInfo structs the chip indicated it has
*/
EXPORTFUNC uint32_t getSectorInfoCount();

/*
 * @brief looks for the start of the flash table
 * @param allSectors an array with the information for each sector (from getSectorStartAddressAndSize)
 * @param numSectors the length of allSectors
 * @param flashImgFile the path to a flash image file
 * @return the address of the start of the flash table, or UINT32_MAX if not found
 */
EXPORTFUNC uint32_t getStartOfFlashTable(SectorAddressAndSize *allSectors, uint32_t numSectors, char* flashImgFile);

/*
 * @brief ensures that the flash image is not too large for the device
 * @param allSectors an array with the information for each sector (from getSectorStartAddressAndSize)
 * @param numSectors the length of allSectors
 * @param startOfFlashTable the address of the start of the flash table
 * @param flashImgFile the path to a flash image file
 * @return uint32_t indicating whether or not the flash image is valid for the projector
*/
EXPORTFUNC uint32_t validateFlashImage(SectorAddressAndSize* allSectors, uint32_t numSectors, uint32_t startOfFlashTable, char* flashImgFile);

/*
 * @brief gets the checksum for the given sector of the flash image
 * @param imgFile the opened FILE* handle to the flash image file
 * @param sector the sector on the chip relating to the address in the image file
          (currently only used for the address)
 * @return the checksum of the sector from the flash image file
*/
EXPORTFUNC Checksum getChecksum(FILE* imgFile, SectorAddressAndSize sector);

/*
 * @brief gets the checksums for all sectors in the flash image file
 * @param allSectors an array with the information for each sector (from getSectorStartAddressAndSize)
 * @param numSectors the length of allSectors
 * @param flashImgFile the path to the flash image file
 * @param numFlashImgChecksums [OUT] the number of checksums calculated
 * @param an array of AddressChecksums containing the checksums of each sector from the flash image file
*/
EXPORTFUNC AddressChecksum* getFlashImageChecksums(SectorAddressAndSize* allSectors, uint32_t numSectors, char* flashImgFile, /*out*/ uint32_t *numFlashImgChecksums);

/*
 * @brief gets the checksum for the given sector on the projector
 * @param sector the sector to get the checksum for
 * @return the Checksum of the sector from the projector
 */
EXPORTFUNC Checksum getFlashSectorChecksum(SectorAddressAndSize sector);

/*
 * @brief gets a list of sectors that need to be programmed
 * @param allSectors an array with the information for each sector
 * @param numSectors the length of allSectors
 * @param flashImgChecksums an array of checksums for the flash image file
 * @param numFlashImgChecksums the length flashImgChecksums
 * @param startOfFlashTable address of the flash table in flash image
 * @param updateModifiedSectorsOnly only flash sectors that are changing
 * @param numSectorsToProgram the number of sectors in the return array
 * @param flashType the type of flash to perform
 * @return the sectors that need to be programmed
*/
EXPORTFUNC SectorAddressAndSize* getSectorsToProgram(SectorAddressAndSize* allSectors,
	uint32_t numSectors,
	AddressChecksum *flashImgChecksums,
	uint32_t numFlashImgChecksums,
	uint32_t startOfFlashTable,
	uint8_t updateModifiedSectorsOnly,
	uint32_t *numSectorsToProgram,
	enum FlashType flashType);

/*
 * @brief toggles whether or not the chip is locked for programming
 * @param locked whether to lock or unlock
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t setLockedForFlashProgramming(uint8_t locked);

/*
 * @brief erase the given sectors on the chip
 * @param allSectors an array with the information for each sector (from getSectorStartAddressAndSize)
 * @param numSectors the length of allSectors
 * @param startOfFlashTable the address of the start of the flash table
 * @param flashType which sectors to erase
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t eraseSectors(SectorAddressAndSize* sectorsToProgram, uint32_t numSectors,
		                       	 uint32_t startOfFlashTable, enum FlashType flashType);

/*
 * @brief programs a sector on the chip with the sector from the flash image
 * @param imgFile the opened FILE* handle to the flash image file
 * @param sector the sector to flash
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t programSector(FILE* imgFile, SectorAddressAndSize sector);

/*
 * @brief programs the given sectors on the chip
 * @param sectorsToProgram the sectors to program
 * @param numSectorsToProgram the length of sectorsToProgram
 * @param startOfFlashTable the address of the start of the flash table
 * @param flashImgFile the path to the flash image file
 * @param flashType which sectors to flash
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t programSectors(SectorAddressAndSize* sectorsToProgram, uint32_t numSectorsToProgram,
							   	   uint32_t startOfFlashTable, char* flashImgFile, enum FlashType flashType);

/*
 * @brief programs the flash table signature
 * @param startOfFlashTable the address of the start of the flash table
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t programFlashTableSignature(uint32_t startOfFlashTable);

/*
 * @brief compares the checksums on the chip to the checksums of the flash image file
 * @param sectorsToProgram the sectors to program
 * @param numSectorsToProgram the length of sectorsToProgram
 * @param flashImgChecksums the checksums of the flash image
 * @param numFlashImgChecksums the length of flashImgChecksums
 * @param startOfFlashTable address of the flash table in flash image
 * @param flashType the type of flash to perform
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t verifyFlashData(SectorAddressAndSize* sectorsToProgram,
	uint32_t numSectorsToProgram,
	AddressChecksum* flashImgCheckSums,
	uint32_t numFlashImgChecksums,
	uint32_t startOfFlashTable,
	enum FlashType flashType);

/*
 * @brief switches to the main application via reset
 * @return 0 if successful or >0 on error
*/
EXPORTFUNC uint32_t switchToMainApp();

/*
 * @brief main entry point to the app. Updates the system with the data pointed to by file `filePath`
 * @param filePath path to flash image file
 * @param flashModifiedSectorsOnly 1 = flash only modified sectors, 0 = flash all sectors
 * @param flashType the type of flash to perform
 * @return 0 if successful, >0 on error
*/
EXPORTFUNC int doFlashUpdate(char* filePath, uint8_t flashModifiedSectorsOnly, enum FlashType flashType);

#endif
