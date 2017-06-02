#include <string.h>
#include "grainuum.h"
#include "kl17.h"

enum usb_strings
{
  product_name = 1,
  serial_number = 2,
  manufacturer_name = 3,
  interface_name = 4,
  configuration_name = 5,
};

#define EP_INTERVAL_MS 6 /* Request that we get polled every 6 ms */

/* These define how many USB Rx buffers we keep */
#define NUM_BUFFERS 4
#define BUFFER_SIZE 8

static struct GrainuumUSB defaultUsbPhy = {
    /* PTB6 */
    .usbdnIAddr = (uint32_t)&FGPIOB->PDIR,
    .usbdnSAddr = (uint32_t)&FGPIOB->PSOR,
    .usbdnCAddr = (uint32_t)&FGPIOB->PCOR,
    .usbdnDAddr = (uint32_t)&FGPIOB->PDDR,
    .usbdnMask = (1 << 6),
    .usbdnShift = 6,

    /* PTB5 */
    .usbdpIAddr = (uint32_t)&FGPIOB->PDIR,
    .usbdpSAddr = (uint32_t)&FGPIOB->PSOR,
    .usbdpCAddr = (uint32_t)&FGPIOB->PCOR,
    .usbdpDAddr = (uint32_t)&FGPIOB->PDDR,
    .usbdpMask = (1 << 5),
    .usbdpShift = 5,
};

void set_usb_config_num(struct GrainuumUSB *usb, int configNum)
{
  (void)usb;
  (void)configNum;
  ;
}

static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01, // (GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page
    0x09, 0x06, // (LOCAL)  USAGE              0x00010006 Keyboard (CA=Application Collection)
    0xA1, 0x01, // (MAIN)   COLLECTION         0x01 Application (Usage=0x00010006: Page=Generic Desktop Page, Usage=Keyboard, Type=CA)
    0x05, 0x07, //   (GLOBAL) USAGE_PAGE         0x0007 Keyboard/Keypad Page
    0x19, 0xE0, //   (LOCAL)  USAGE_MINIMUM      0x000700E0 Keyboard Left Control (DV=Dynamic Value)
    0x29, 0xE7, //   (LOCAL)  USAGE_MAXIMUM      0x000700E7 Keyboard Right GUI (DV=Dynamic Value)
    0x25, 0x01, //   (GLOBAL) LOGICAL_MAXIMUM    0x01 (1)
    0x75, 0x01, //   (GLOBAL) REPORT_SIZE        0x01 (1) Number of bits per field
    0x95, 0x08, //   (GLOBAL) REPORT_COUNT       0x08 (8) Number of fields
    0x81, 0x02, //   (MAIN)   INPUT              0x00000002 (8 fields x 1 bit) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x95, 0x01, //   (GLOBAL) REPORT_COUNT       0x01 (1) Number of fields
    0x75, 0x08, //   (GLOBAL) REPORT_SIZE        0x08 (8) Number of bits per field
    0x81, 0x03, //   (MAIN)   INPUT              0x00000003 (1 field x 8 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x95, 0x05, //   (GLOBAL) REPORT_COUNT       0x05 (5) Number of fields
    0x75, 0x01, //   (GLOBAL) REPORT_SIZE        0x01 (1) Number of bits per field
    0x05, 0x08, //   (GLOBAL) USAGE_PAGE         0x0008 LED Indicator Page
    0x19, 0x01, //   (LOCAL)  USAGE_MINIMUM      0x00080001 Num Lock (OOC=On/Off Control)
    0x29, 0x05, //   (LOCAL)  USAGE_MAXIMUM      0x00080005 Kana (OOC=On/Off Control)
    0x91, 0x02, //   (MAIN)   OUTPUT             0x00000002 (5 fields x 1 bit) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x95, 0x01, //   (GLOBAL) REPORT_COUNT       0x01 (1) Number of fields
    0x75, 0x03, //   (GLOBAL) REPORT_SIZE        0x03 (3) Number of bits per field
    0x91, 0x03, //   (MAIN)   OUTPUT             0x00000003 (1 field x 3 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x95, 0x06, //   (GLOBAL) REPORT_COUNT       0x06 (6) Number of fields
    0x75, 0x08, //   (GLOBAL) REPORT_SIZE        0x08 (8) Number of bits per field
    0x25, 0x65, //   (GLOBAL) LOGICAL_MAXIMUM    0x65 (101)
    0x05, 0x07, //   (GLOBAL) USAGE_PAGE         0x0007 Keyboard/Keypad Page
    0x19, 0x00, //   (LOCAL)  USAGE_MINIMUM      0x00070000 Keyboard No event indicated (Sel=Selector)
    0x29, 0x65, //   (LOCAL)  USAGE_MAXIMUM      0x00070065 Keyboard Application (Sel=Selector)
    0x81, 0x00, //   (MAIN)   INPUT              0x00000000 (6 fields x 8 bits) 0=Data 0=Array 0=Absolute 0=Ignored 0=Ignored 0=PrefState 0=NoNull
    0xC0,       // (MAIN)   END_COLLECTION     Application
};

static const struct usb_device_descriptor device_descriptor = {
    .bLength = 18,                //sizeof(struct usb_device_descriptor),
    .bDescriptorType = DT_DEVICE, /* DEVICE */
    .bcdUSB = 0x0200,             /* USB 2.0 */
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 0x08, /* 8-byte packets max */
    .idVendor = 0x1209,
    .idProduct = 0x9317,
    .bcdDevice = 0xa014,                /* Device release 1.0 */
    .iManufacturer = manufacturer_name, /* No manufacturer string */
    .iProduct = product_name,           /* Product name in string #2 */
    .iSerialNumber = serial_number,     /* No serial number */
    .bNumConfigurations = 0x01,
};

static const struct usb_configuration_descriptor configuration_descriptor = {
    .bLength = 9, //sizeof(struct usb_configuration_descriptor),
    .bDescriptorType = DT_CONFIGURATION,
    .wTotalLength = (9 + 9 + 9 + 7 + 7) /*
                  (sizeof(struct usb_configuration_descriptor)
                + sizeof(struct usb_interface_descriptor)
                + sizeof(struct usb_hid_descriptor)
                + sizeof(struct usb_endpoint_descriptor)*/,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = configuration_name,
    .bmAttributes = 0x80, /* Remote wakeup not supported */
    .bMaxPower = 100 / 2, /* 100 mA (in 2-mA units) */
    .data = {
        /* struct usb_interface_descriptor { */
        /*  uint8_t bLength;            */ 9,
        /*  uint8_t bDescriptorType;    */ DT_INTERFACE,
        /*  uint8_t bInterfaceNumber;   */ 0,
        /*  uint8_t bAlternateSetting;  */ 0,
        /*  uint8_t bNumEndpoints;      */ 2,              /* Two extra EPs */
        /*  uint8_t bInterfaceClass;    */ 3,              /* HID class */
        /*  uint8_t bInterfaceSubclass; */ 0,              /* Boot Device subclass */
        /*  uint8_t bInterfaceProtocol; */ 0,              /* 1 == keyboard, 2 == mouse */
        /*  uint8_t iInterface;         */ interface_name, /* String index #4 */
                                                           /* }*/

        /* struct usb_hid_descriptor {        */
        /*  uint8_t  bLength;                 */ 9,
        /*  uint8_t  bDescriptorType;         */ DT_HID,
        /*  uint16_t bcdHID;                  */ 0x11, 0x01,
        /*  uint8_t  bCountryCode;            */ 0,
        /*  uint8_t  bNumDescriptors;         */ 1, /* We have only one REPORT */
        /*  uint8_t  bReportDescriptorType;   */ DT_HID_REPORT,
        /*  uint16_t wReportDescriptorLength; */ sizeof(hid_report_descriptor),
        sizeof(hid_report_descriptor) >> 8,
        /* }                                  */

        /* struct usb_endpoint_descriptor { */
        /*  uint8_t  bLength;             */ 7,
        /*  uint8_t  bDescriptorType;     */ DT_ENDPOINT,
        /*  uint8_t  bEndpointAddress;    */ 0x81, /* EP1 (IN) */
        /*  uint8_t  bmAttributes;        */ 3,    /* Interrupt */
        /*  uint16_t wMaxPacketSize;      */ 0x08, 0x00,
        /*  uint8_t  bInterval;           */ EP_INTERVAL_MS, /* Every 6 ms */
                                                             /* }                              */

        /* struct usb_endpoint_descriptor { */
        /*  uint8_t  bLength;             */ 7,
        /*  uint8_t  bDescriptorType;     */ DT_ENDPOINT,
        /*  uint8_t  bEndpointAddress;    */ 0x01, /* EP1 (OUT) */
        /*  uint8_t  bmAttributes;        */ 3,    /* Interrupt */
        /*  uint16_t wMaxPacketSize;      */ 0x08, 0x00,
        /*  uint8_t  bInterval;           */ EP_INTERVAL_MS, /* Every 6 ms */
                                                             /* }                              */
    },
};

#define USB_STR_BUF_LEN 64

uint32_t str_buf_storage[USB_STR_BUF_LEN / sizeof(uint32_t)];
static int send_string_descriptor(const char *str, const void **data)
{
  int len;
  int max_len;
  uint8_t *str_buf = (uint8_t *)str_buf_storage;
  uint8_t *str_offset = str_buf;

  len = strlen(str);
  max_len = (USB_STR_BUF_LEN / 2) - 2;

  if (len > max_len)
    len = max_len;

  *str_offset++ = (len * 2) + 2; // Two bytes for length count
  *str_offset++ = DT_STRING;     // Sending a string descriptor

  while (len--)
  {
    *str_offset++ = *str++;
    *str_offset++ = 0;
  }

  *data = str_buf;

  // Return the size, which is stored in the first byte of the output data.
  return str_buf[0];
}

static int get_string_descriptor(struct GrainuumUSB *usb,
                                 uint32_t num,
                                 const void **data)
{

  static const uint8_t en_us[] = {0x04, DT_STRING, 0x09, 0x04};

  (void)usb;

  if (num == 0)
  {
    *data = en_us;
    return sizeof(en_us);
  }

  if (num == product_name)
    return send_string_descriptor("Palawan Keyboard", data);

  if (num == serial_number)
    return send_string_descriptor("0203040203040", data);

  if (num == manufacturer_name)
    return send_string_descriptor("Kosagi PTE Ltd.", data);

  if (num == interface_name)
    return send_string_descriptor("Default Keyboard", data);

  if (num == configuration_name)
    return send_string_descriptor("Default configuration", data);

  return 0;
}

static int get_device_descriptor(struct GrainuumUSB *usb,
                                 uint32_t num,
                                 const void **data)
{

  (void)usb;

  if (num == 0)
  {
    *data = &device_descriptor;
    return sizeof(device_descriptor);
  }
  return 0;
}

static int get_hid_report_descriptor(struct GrainuumUSB *usb,
                                     uint32_t num,
                                     const void **data)
{

  (void)usb;

  if (num == 0)
  {
    *data = &hid_report_descriptor;
    return sizeof(hid_report_descriptor);
  }

  return 0;
}

static int get_configuration_descriptor(struct GrainuumUSB *usb,
                                        uint32_t num,
                                        const void **data)
{

  (void)usb;

  if (num == 0)
  {
    *data = &configuration_descriptor;
    return configuration_descriptor.wTotalLength;
  }
  return 0;
}

static int get_descriptor(struct GrainuumUSB *usb,
                          const void *packet,
                          const void **response)
{

  const struct usb_setup_packet *setup = packet;

  switch (setup->wValueH)
  {
  case DT_DEVICE:
    return get_device_descriptor(usb, setup->wValueL, response);

  case DT_STRING:
    return get_string_descriptor(usb, setup->wValueL, response);

  case DT_CONFIGURATION:
    return get_configuration_descriptor(usb, setup->wValueL, response);

  case DT_HID_REPORT:
    return get_hid_report_descriptor(usb, setup->wValueL, response);
  }

  return 0;
}

static uint32_t ep1_buffer[NUM_BUFFERS][BUFFER_SIZE / sizeof(uint32_t)];
static uint8_t ep1_buffer_sizes[NUM_BUFFERS];
static uint8_t ep1_buffer_head;
static uint8_t ep1_buffer_tail;
static void *get_usb_ep1_buffer(struct GrainuumUSB *usb,
                                uint8_t epNum,
                                int32_t *size)
{
  (void)usb;
  (void)epNum;

  if (size)
    *size = sizeof(ep1_buffer[0]);
  return ep1_buffer[ep1_buffer_head];
}

static int received_data(struct GrainuumUSB *usb,
                         uint8_t epNum,
                         uint32_t bytes,
                         const void *data)
{
  (void)usb;
  (void)epNum;
  (void)data;

  /* If we receive data for EP1, advance the EP1 buffer */
  if (epNum == 1)
  {
    ep1_buffer_sizes[ep1_buffer_head] = bytes;
    ep1_buffer_head = (ep1_buffer_head + 1) & (NUM_BUFFERS - 1);
  }

  /* Return 0, indicating this packet is complete. */
  return 0;
}

static int send_data_finished(struct GrainuumUSB *usb, int result)
{
  (void)usb;
  (void)result;

  return 0;
}

static struct GrainuumConfig hid_link = {
    .getDescriptor = get_descriptor,
    .getReceiveBuffer = get_usb_ep1_buffer,
    .receiveData = received_data,
    .sendDataFinished = send_data_finished,
    .setConfigNum = set_usb_config_num,
};

static GRAINUUM_BUFFER(phy_queue, 4);

void VectorBC(void)
{
  grainuumCaptureI(&defaultUsbPhy, GRAINUUM_BUFFER_ENTRY(phy_queue));

  /* Clear all pending interrupts on this port. */
  PORTB->ISFR = 0xFFFFFFFF;
}

void grainuumReceivePacket(struct GrainuumUSB *usb)
{
  (void)usb;
  GRAINUUM_BUFFER_ADVANCE(phy_queue);
}

void grainuumInitPre(struct GrainuumUSB *usb)
{
  (void)usb;
  GRAINUUM_BUFFER_INIT(phy_queue);
}

static void process_next_usb_event(struct GrainuumUSB *usb)
{
  if (!GRAINUUM_BUFFER_IS_EMPTY(phy_queue))
  {
    uint8_t *in_ptr = (uint8_t *)GRAINUUM_BUFFER_TOP(phy_queue);

    // Advance to the next packet (allowing us to be reentrant)
    GRAINUUM_BUFFER_REMOVE(phy_queue);

    // Process the current packet
    grainuumProcess(usb, in_ptr);

    return;
  }
}

void usbProcess(void (*received_data)(void *data, uint32_t size))
{
  process_next_usb_event(&defaultUsbPhy);
  if (ep1_buffer_head != ep1_buffer_tail)
  {
    received_data(ep1_buffer[ep1_buffer_tail], ep1_buffer_sizes[ep1_buffer_tail]);
    ep1_buffer_tail = (ep1_buffer_tail + 1) & (NUM_BUFFERS - 1);
  }
}

void usbStart(void)
{
  /* Unlock PORTA and PORTB */
  SIM->SCGC5 |= SIM_SCGC5_PORTA | SIM_SCGC5_PORTB;

  /* Set up D+ and D- as slow-slew GPIOs (pin mux type 1), and enable IRQs */
  PORTB->PCR[5] = (1 << 8) | (0xb << 16) | (1 << 2);
  PORTB->PCR[6] = (1 << 8) | (1 << 2);

  grainuumInit(&defaultUsbPhy, &hid_link);
  grainuumDisconnect(&defaultUsbPhy);

  {
    int i;
    for (i = 0; i < 1000; i++)
    {
      int j;
      for (j = 0; j < 77; j++)
      {
        asm("");
      }
    }
  }

  /* Enable PORTB IRQ */
  NVIC_EnableIRQ(PINB_IRQn);
  __enable_irq();

  grainuumConnect(&defaultUsbPhy);
}
