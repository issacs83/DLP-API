#pragma once
#include <stdint.h>

#ifdef _WIN32
#  define EXPORTFUNC __declspec( dllexport )
#else
#  define EXPORTFUNC
#endif

#define MAX_STR 255

// 654x/754x hardware identifiers
static const unsigned short VID_DEVICE = 0x0451;
static const unsigned short PID_DEVICE = 0x7540;

extern uint8_t variableLengthRead;

EXPORTFUNC uint32_t ioInit();

EXPORTFUNC uint32_t ioRead(char* buffer, uint32_t dwSize);

EXPORTFUNC uint32_t ioWrite(char* buffer, uint32_t dwSize);

EXPORTFUNC void disconnectDevice();
