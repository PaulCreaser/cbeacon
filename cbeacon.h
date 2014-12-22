#ifndef __CBEACON_H
#define __CBEACON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// Callback for other packets
typedef void (*cbeaconCallBack)(int8_t *data, int len);

int 	cbeacon_init();
int 	cbeacon_setcb(cbeaconCallBack cb); 
void 	cbeacon_get_mac_address(int8_t mac_address[6]);
int  	cbeacon_start(bool nb);
void 	cbeacon_stop();

#endif /* __CBEACON_H */
