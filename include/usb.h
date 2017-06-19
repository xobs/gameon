#ifndef PALAWAN_USB_H_
#define PALAWAN_USB_H_

#include <stdint.h>

void usbStart(void);
void usbProcess(void (*received_data)(void *data, uint32_t size));
int usbSend(const void *data, int len);

#endif /* PALAWAN_USB_H_ */
