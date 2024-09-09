#include <libusb-1.0/libusb.h>
#include <stdio.h>

int main() {
    libusb_device_handle *handle;
    libusb_context *ctx = NULL;
    int r;

    r = libusb_init(&ctx);
    if(r < 0) {
        printf("Init Error %d\n", r);
        return 1;
    }

    handle = libusb_open_device_with_vid_pid(ctx, 0x04b4, 0x000a); // Use your VID and PID
    if(handle == NULL) {
        printf("Cannot open device\n");
        return 1;
    }

    printf("Device Opened\n");

    // Here you would add code to communicate with the device

    libusb_close(handle);
    libusb_exit(ctx);
    return 0;
}