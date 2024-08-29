// the following ifndef is for preventing double includes of this header file
#if !defined(__USBI2CIO_H__)
#define __USBI2CIO_H__


#define DAPI_MAX_DEVICES      127


#ifdef _DEBUG
#define DbgWrStr(sDebug) OutputDebugString((sDebug))
#else
#define DbgWrStr(sDebug)
#endif



//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
typedef enum {
    // supported transaction types
    I2C_TRANS_NOADR,          // read or write with no address cycle
    I2C_TRANS_8ADR,           // read or write with 8 bit address cycle
    I2C_TRANS_16ADR           // read or write with 16 bit address cycle
} I2C_TRANS_TYPE;


//-----------------------------------------------------------------------------
// Structure Definitions
//-----------------------------------------------------------------------------
typedef struct _DEVINFO {             // structure for device information
    BYTE byInstance;
    BYTE SerialId[9];
} DEVINFO, *LPDEVINFO;


#pragma pack(1)                       // force byte alignment

typedef struct _I2C_TRANS {
    BYTE byTransType;
    BYTE bySlvDevAddr;
    WORD wMemoryAddr;
    WORD wCount;
    BYTE Data[256];
} I2C_TRANS, *PI2C_TRANS;


//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// API Function Prototypes (exported)
//-----------------------------------------------------------------------------

WORD _stdcall DAPI_GetDllVersion(void);
HANDLE _stdcall DAPI_OpenDeviceInstance(LPSTR lpsDevName, BYTE byDevInstance);
BOOL _stdcall DAPI_CloseDeviceInstance(HANDLE hDevInstance);
BOOL _stdcall DAPI_DetectDevice(HANDLE hDevInstance);
BYTE _stdcall DAPI_GetDeviceCount(LPSTR lpsDevName);
BYTE _stdcall DAPI_GetDeviceInfo(LPSTR lpsDevName, LPDEVINFO lpDevInfo);
HANDLE _stdcall DAPI_OpenDeviceBySerialId(LPSTR lpsDevName, LPSTR lpsDevSerialId);
BOOL _stdcall DAPI_GetSerialId(HANDLE hDevInstance, LPSTR lpsDevSerialId);
BOOL _stdcall DAPI_ConfigIoPorts(HANDLE hDevInstance, ULONG ulIoPortConfig);
BOOL _stdcall DAPI_GetIoConfig(HANDLE hDevInstance, LPLONG lpulIoPortData);
BOOL _stdcall DAPI_ReadIoPorts(HANDLE hDevInstance, LPLONG lpulIoPortData);
BOOL _stdcall DAPI_WriteIoPorts(HANDLE hDevInstance, ULONG ulIoPortData, ULONG ulIoPortMask);
LONG _stdcall DAPI_ReadI2c(HANDLE hDevInstance, PI2C_TRANS TransI2C);
LONG _stdcall DAPI_WriteI2c(HANDLE hDevInstance, PI2C_TRANS TransI2C);
void _stdcall DAPI_EnablePolling(void);
void _stdcall DAPI_DisablePolling(void);
void _stdcall DAPI_GetPolledInfo(void);
LONG _stdcall DAPI_ReadDebugBuffer(LPSTR lpsDebugString, HANDLE hDevInstance, LONG ulMaxBytes);


// the following #endif is for preventing double includes of this header file
#endif