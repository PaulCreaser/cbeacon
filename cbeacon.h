#ifndef __CBEACON_H
#define __CBEACON_H

#include <stdio.h>
#include <stdint.h>

typedef struct cbeacon_pkt {
        uint8_t mac[6];
        uint8_t spoof[6];
        uint8_t uuid[16];
        uint16_t major;
        uint16_t minor;
        uint8_t power;
        int8_t rssi;
} __attribute__ ((packed))
CBEACON_PKT;

// Call back
typedef void (*CbeaconCallBack)(CBEACON_PKT *packet);

int cbeacon_init(CbeaconCallBack cb);
int cbeacon_start();
void cbeacon_stop();

#endif /* __CBEACON_H */
