#include "unix_io.h"

#include <stdio.h>
#include <libusb.h>

#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 1)   /* endpoint address */
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2)   /* endpoint address */
#define USB_TIMEOUT	        2000        /* Connection timeout (in ms) */

static libusb_context *ctx = NULL;
static libusb_device_handle *handle;

// ioRead fakes read operations, necessitating the use of a second buffer
static uint8_t receiveBuf[UINT16_MAX];
int totalReceive = 0;
uint16_t totalRead = 0;
uint8_t  lastTransferWasWrite = 0;

uint8_t variableLengthRead;

/*
 * libusb (on Linux, at least) can't handle fine-grained read sizes (i.e., try to read 3, get 4 returned)
 * Because of this, I had to do things kind of weird to make it work with the current business logic of:
 *      Step 1: Read header
 *      Step 2: Read data
 * Basically, my solution is to just read as much as possible in step 1, and then "fake" the read in step 2
 *
 * Since each read operation is preceeded by a write operation, we can use whether or not the previous
 * operation was a write to determine whether or not to do a real read operation or to fake one
*/
uint32_t ioRead(char* buffer, uint32_t dwSize)
{
    int ret = 0;

    if (lastTransferWasWrite)
    {
        totalRead = 0;
        totalReceive = 0;
        ret = libusb_bulk_transfer(handle, 0x81, receiveBuf, UINT16_MAX, &totalReceive, 0);
    }

    lastTransferWasWrite = 0;

    for (int i = 0; i < dwSize; ++i)
    {
        buffer[i] = receiveBuf[i + totalRead];
    }

    totalRead += dwSize;

    switch(ret){
        case 0:
            break;
        case LIBUSB_ERROR_TIMEOUT:
            printf("ERROR in bulk read: %d Timeout\n", ret);
            break;
        case LIBUSB_ERROR_PIPE:
            printf("ERROR in bulk read: %d Pipe\n", ret);
            break;
        case LIBUSB_ERROR_OVERFLOW:
            printf("ERROR in bulk read: %d Overflow\n", ret);
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            printf("ERROR in bulk read: %d No Device\n", ret);
            break;
        case LIBUSB_ERROR_BUSY:
            printf("ERROR in bulk read: %d Busy\n", ret);
            break;
        case LIBUSB_ERROR_INVALID_PARAM:
            printf("ERROR in bulk read: %d Invalid param\n", ret);
            break;
        default:
            printf("ERROR in bulk read: %d\n", ret);
            break;

    }
    return totalReceive;
}

uint32_t ioWrite(char* buffer, uint32_t dwSize)
{
    int ret = 0;
    int bytesWritten = 0xFFFFFFFF;

    lastTransferWasWrite = 1;

    ret = libusb_bulk_transfer(handle, 0x01, buffer, dwSize, &bytesWritten, USB_TIMEOUT);

    switch(ret){
        case 0:
            return 0;
            break;
        case LIBUSB_ERROR_TIMEOUT:
            printf("ERROR in bulk write: %d Timeout\n", ret);
            break;
        case LIBUSB_ERROR_PIPE:
            printf("ERROR in bulk write: %d Pipe\n", ret);
            break;
        case LIBUSB_ERROR_OVERFLOW:
            printf("ERROR in bulk write: %d Overflow\n", ret);
            break;
        case LIBUSB_ERROR_NO_DEVICE:
            printf("ERROR in bulk write: %d No Device\n", ret);
            break;
        case LIBUSB_ERROR_BUSY:
            printf("ERROR in bulk write: %d Busy\n", ret);
            break;
        case LIBUSB_ERROR_INVALID_PARAM:
            printf("ERROR in bulk write: %d Invalid param\n", ret);
            break;
        default:
            printf("ERROR in bulk write: %d\n", ret);
            break;

    }
    return -1;
}

uint32_t ioInit()
{
    libusb_init(&ctx);
    handle = libusb_open_device_with_vid_pid(ctx, VID_DEVICE, PID_DEVICE);

    if (!handle) {
        printf("ERROR: Could not connect to device\n");
        return 1;
    }

    libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, 3);

    int ret = libusb_claim_interface(handle, 0);

    if (ret < 0) {
        fprintf(stderr, "usb_claim_interface error %d\n", ret);
        return 1;
    }

    printf("Device connected\n");

    return 0;
}

void disconnectDevice()
{
    libusb_release_interface(handle, 0);
    libusb_close(handle);
}
