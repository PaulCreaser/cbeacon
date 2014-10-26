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

typedef struct advert_pkt {
        uint8_t mac[6];
        uint8_t spoof[6];
        uint8_t uuid[16];
	uint8_t payload[5];
	int8_t rssi;
} __attribute__ ((packed))
ADVERT_PKT;

typedef struct other_pkt {
        uint8_t mac[6];
} __attribute__ ((packed))
OTHER_PKT;


// Callback for beacon message
typedef void (*CbeaconCallBack)(CBEACON_PKT *packet);
// Callback for advert message
typedef void (*CadvertCallBack)(ADVERT_PKT *packet);
// Callback for other packets
typedef void (*CotherCallBack)(OTHER_PKT *packet);

int cbeacon_init(CbeaconCallBack bcb, CadvertCallBack acb, CotherCallBack ocb);
int cbeacon_start();
void cbeacon_stop();

#endif /* __CBEACON_H */
