#include "libusbcontrol.h"
#include <libusb.h>

CLibusbControl::CLibusbControl()
{
    m_usbid.vid = 0x046d;
    m_usbid.pid = 0xC200;
    bulkusb_init();

}


CLibusbControl::~CLibusbControl()
{
    bulkusb_exit();
}

void CLibusbControl::setusbid(uint16_t vid, uint16_t pid)
{
    m_usbid.vid = vid;
    m_usbid.pid = pid;
}

/**
 * @name: bulkusb_init
 * @brief: libusb 初始化
 * @return：no
 */
void CLibusbControl::bulkusb_init()
{
    uint16_t pid = 0;
    uint16_t vid = 0;
    int ret = -1;

    /*Step1: 初始化 */
    libusb_init(NULL);

    vid = m_usbid.vid;
    pid = m_usbid.pid;

    /*Step2：打开设备 */
    libusb_device_handle* pHandle = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (pHandle == NULL)
    {
        printf("libusb_open_device_with_vid_pid(%d, %d) failed \n", vid, pid);
        libusb_exit(NULL);
        return;
    }

    m_usbdev.handle = pHandle;

    /*Step3: 声明某个接口*/
    int iface = get_interface_attr(pHandle);
    if (iface < 0) {
        libusb_close(pHandle);
        libusb_exit(NULL);
    }

    libusb_set_auto_detach_kernel_driver(pHandle, 1);

#if 0
    ret = libusb_claim_interface(pHandle, iface);
    if (ret != LIBUSB_SUCCESS) {
        printf("   Failed.\n");
    }

#endif

    libusb_reset_device(pHandle);

}

/**
 * @name: bulkusb_exit
 * @brief: libusb 初始化
 * @return：no
 */
void CLibusbControl::bulkusb_exit()
{
    //libusb_release_interface(m_usbdev.handle, m_usbdev.bulk_interface);
    libusb_close(m_usbdev.handle);
    libusb_exit(NULL);
}

int CLibusbControl::get_interface_attr(libusb_device_handle* handle)
{
    int ret = -1;

    /*Step1: 获取设备描述符 */
    libusb_device* dev = NULL;
    dev = libusb_get_device(handle);
    if (dev == NULL) {
        printf("libusb_get_device error\n");
        return -1;
    }

    struct libusb_device_descriptor desc;
    ret = libusb_get_device_descriptor(dev, &desc);
    if (ret < 0)
    {
        fprintf(stderr, "failed to get device descriptor");
        return -1;
    }

    /* Step2: 获取配置描述符 */
    struct libusb_config_descriptor* conf_desc;
    const struct libusb_endpoint_descriptor* endpoint;
    uint8_t endpoint_in = 0, endpoint_out = 0;	// default IN and OUT endpoints
    int nb_ifaces;
    uint8_t interfaceClass;

    ret = libusb_get_config_descriptor(dev, 0, &conf_desc);
    if (ret < 0)
    {
        fprintf(stderr, "failed to get config descriptor");
        return -1;
    }
    /* Step3：获取接口描述符 */
    nb_ifaces = conf_desc->bNumInterfaces;
    m_usbdev.nb_ifaces = nb_ifaces;

    for(int i = 0; i < nb_ifaces; i++) {
        for (int j = 0; j < conf_desc->interface[i].num_altsetting; j++) {
            //只获取接口类别为：厂商自定义类（0xFF）和CDC数据类（0xA）
            interfaceClass = conf_desc->interface[i].altsetting[j].bInterfaceClass;
            if (interfaceClass != 0xFF) { //0xA CDC
                continue;
            }
            m_usbdev.bulk_interface = i;
            for (int k = 0; k < conf_desc->interface[i].altsetting[j].bNumEndpoints; k++) {
                endpoint = &conf_desc->interface[i].altsetting[j].endpoint[k];
                // Use the first bulk IN/OUT endpoints as default for testing
                if ((endpoint->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) & (LIBUSB_TRANSFER_TYPE_BULK)) {//只获取批量传输端点
                    if (endpoint->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
                        if (!endpoint_in)
                        {
                            endpoint_in = endpoint->bEndpointAddress;
                            m_usbdev.endpoint_in = endpoint_in;//赋值
                          //  libusb_clear_halt(handle, bdev->endpoint_in);//清除暂停标志
                        }
                    }
                    else {
                        if (!endpoint_out)
                        {
                            endpoint_out = endpoint->bEndpointAddress;
                            m_usbdev.endpoint_out = endpoint_out;//赋值
                           // libusb_clear_halt(handle, bdev->endpoint_out);
                        }
                    }
                }
            }

            break;
        }
    }

    libusb_free_config_descriptor(conf_desc);
    return m_usbdev.bulk_interface;
}


/**
 * @brief bulkusb_write
 * @param dev
 * @param buffer
 * @param len
 * @param ms
 * @return
 */
int CLibusbControl::bulkusb_write(void* buffer, int len, int ms)
{
    int size, errcode;
    libusb_device_handle* handle = m_usbdev.handle;
    uint8_t endpoint_out = m_usbdev.endpoint_out;

    libusb_claim_interface(handle, m_usbdev.bulk_interface);

    errcode = libusb_bulk_transfer(handle, endpoint_out, (unsigned char*)buffer, len, &size, ms);
    if (errcode<0)
    {
        printf("write:   %s\n", libusb_strerror((enum libusb_error)errcode));
        return -1;
    }

    int offset = 0, rsize = 0;
    if (size != len) {
        offset = len - size;
        errcode = libusb_bulk_transfer(handle, endpoint_out,(unsigned char*) buffer + offset, offset, &rsize, ms);
        if (errcode < 0)
        {
            printf("write:   %s\n", libusb_strerror((enum libusb_error)errcode));
            return -1;
        }
    }

    libusb_release_interface(handle, m_usbdev.bulk_interface);

    return size+offset;
}

/**
 * @brief bulkusb_read
 * @param dev
 * @param buffer
 * @param len
 * @param ms
 * @return
 */
int CLibusbControl::bulkusb_read(void* buffer, int len, int ms)
{
    int size, errcode;
    libusb_device_handle* handle = m_usbdev.handle;
    uint8_t endpoint_in = m_usbdev.endpoint_in;

    libusb_claim_interface(handle, m_usbdev.bulk_interface);

    errcode = libusb_bulk_transfer(handle, endpoint_in,(unsigned char*) buffer, len, &size, ms);
    if (errcode < 0)
    {
        printf("read:   %s\n", libusb_strerror((enum libusb_error)errcode));
        return -1;
    }

    int offset = 0, rsize = 0;
    if (size != len) {
        offset = len - size;
        errcode = libusb_bulk_transfer(handle, endpoint_in,(unsigned char*) buffer + offset, offset, &rsize, ms);
        if (errcode < 0)
        {
            printf("read:   %s\n", libusb_strerror((enum libusb_error)errcode));
            return -1;
        }
    }

    libusb_release_interface(handle, m_usbdev.bulk_interface);

    return size + offset;
}

int CLibusbControl::control_write(uint8_t bRequest, void *data, int len, int ms)
{
    libusb_device_handle* handle = m_usbdev.handle;
    int status = libusb_control_transfer(
            handle,
            /* bmRequestType */ LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ bRequest,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ (unsigned char *)data,
            /* wLength       */ len,
            ms);

    return status;
}

int CLibusbControl::control_read(uint8_t bRequest, void **data, int len, int ms)
{
    libusb_device_handle* handle = m_usbdev.handle;
    int status = libusb_control_transfer(
            handle,
            /* bmRequestType */ LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE,
            /* bRequest      */ bRequest,
            /* wValue        */ 0,
            /* wIndex        */ 0,
            /* Data          */ (unsigned char*)(*data),
            /* wLength       */ len,
            ms);
    return status;
}
