#ifndef CLIBUSBCONTROL_H
#define CLIBUSBCONTROL_H

#include <libusb.h>
#include <stdio.h>

typedef struct usbid
{
    uint16_t vid; /* VID */
    uint16_t pid; /* PID */
} usbid_t;

typedef struct usbdev
{
    libusb_device_handle* handle;
    uint8_t endpoint_in;   /* in 端点*/
    uint8_t endpoint_out;  /* out 端点 */
    uint8_t bulk_interface;/* 接口号 */
    int nb_ifaces;
} usbdev_t ;


class CLibusbControl
{
private:
    usbid_t m_usbid;
    usbdev_t m_usbdev;

    int get_interface_attr(libusb_device_handle* handle);

    void bulkusb_init();
    void bulkusb_exit();
public:
    CLibusbControl();
    ~CLibusbControl();

    void setusbid(uint16_t vid, uint16_t pid);


    int bulkusb_write(void* buffer, int len, int ms);
    int bulkusb_read(void* buffer, int len, int ms);
    int control_write( uint8_t bRequest, void *data, int len, int ms);
    int control_read(uint8_t bRequest, void **data, int len, int ms);
};

#endif // CLIBUSBCONTROL_H
