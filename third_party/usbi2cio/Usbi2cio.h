#if !defined(__USBI2CIO_H__)
#define __USBI2CIO_H__

#include <stdint.h>  // For standard integer types
#include <stdio.h>   // For printf (or other standard I/O functions)

#define DAPI_MAX_DEVICES      127

#ifdef _DEBUG
#define DbgWrStr(sDebug) fprintf(stderr, "%s", (sDebug))
#else
#define DbgWrStr(sDebug)
#endif

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
typedef enum {
    I2C_TRANS_NOADR,
    I2C_TRANS_8ADR,
    I2C_TRANS_16ADR
} I2C_TRANS_TYPE;

//-----------------------------------------------------------------------------
// Structure Definitions
//-----------------------------------------------------------------------------
typedef struct _DEVINFO {
    uint8_t byInstance;
    uint8_t SerialId[9];
} DEVINFO, *LPDEVINFO;

#pragma pack(1)  // force byte alignment

typedef struct _I2C_TRANS {
    uint8_t byTransType;
    uint8_t bySlvDevAddr;
    uint16_t wMemoryAddr;
    uint16_t wCount;
    uint8_t Data[256];
} I2C_TRANS, *PI2C_TRANS;

//-----------------------------------------------------------------------------
// API Function Prototypes (exported)
//-----------------------------------------------------------------------------

uint16_t DAPI_GetDllVersion(void);
void* DAPI_OpenDeviceInstance(char* lpsDevName, uint8_t byDevInstance);
int DAPI_CloseDeviceInstance(void* hDevInstance);
int DAPI_DetectDevice(void* hDevInstance);
uint8_t DAPI_GetDeviceCount(char* lpsDevName);
uint8_t DAPI_GetDeviceInfo(char* lpsDevName, LPDEVINFO lpDevInfo);
void* DAPI_OpenDeviceBySerialId(char* lpsDevName, char* lpsDevSerialId);
int DAPI_GetSerialId(void* hDevInstance, char* lpsDevSerialId);
int DAPI_ConfigIoPorts(void* hDevInstance, unsigned long ulIoPortConfig);
int DAPI_GetIoConfig(void* hDevInstance, long* lpulIoPortData);
int DAPI_ReadIoPorts(void* hDevInstance, long* lpulIoPortData);
int DAPI_WriteIoPorts(void* hDevInstance, unsigned long ulIoPortData, unsigned long ulIoPortMask);
int32_t DAPI_ReadI2c(void* hDevInstance, PI2C_TRANS TransI2C);
int32_t DAPI_WriteI2c(void* hDevInstance, PI2C_TRANS TransI2C);
void DAPI_EnablePolling(void);
void DAPI_DisablePolling(void);
void DAPI_GetPolledInfo(void);
int32_t DAPI_ReadDebugBuffer(char* lpsDebugString, void* hDevInstance, int32_t ulMaxBytes);

#endif  // __USBI2CIO_H__
