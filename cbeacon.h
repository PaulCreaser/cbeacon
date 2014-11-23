#ifndef __CBEACON_H
#define __CBEACON_H

#include <stdio.h>
#include <stdint.h>

#define UUID_LEN 16

typedef enum {
    BEACON_TYPE_NONE =0,
    BEACON_TYPE_I=0x1,     	// Ibeacon
    BEACON_TYPE_G=0x2,		// Generic Beacon
    BEACON_TYPE_S=0x4,  	// Special Advert
    BEACON_TYPE_SA=0x8,		// Service Advert
    BEACON_TYPE_D=0xF,		// Data
    BEACON_TYPE_O=0x1F,
    BEACON_TYPE_ALL=0xFF
} BEACON_TYPE;

typedef struct cbeacon_pkt {
        uint8_t mac[6];
        uint8_t spoof[6];
	uint8_t uuid[UUID_LEN];
        uint16_t major;
        uint16_t minor;
        uint8_t power;
        int8_t rssi;
} CBEACON_PKT;

typedef struct advert_pkt {
        uint8_t mac[6];
        uint8_t spoof[6];
	uint8_t payload[5];
	int8_t rssi;
	uint8_t uuid[UUID_LEN];
} ADVERT_PKT;

typedef struct s_pkt {
        uint8_t mac[6];
        uint8_t dev_name[6];
        uint8_t data[1024]; // Length not known
        int8_t  rssi;
        int     len; // Without mac, spoof, uuid
} S_PKT;


typedef struct sa_pkt {
        uint8_t mac[6];
        uint8_t uuid[16]; // UUID
        int     len; // Without mac, spoof, uuid
	int8_t  rssi;
} SA_PKT;

typedef struct d_pkt {
        uint8_t mac[6];
        uint8_t data[16]; // UUID
        int     len; // Without mac, spoof, uuid
        int8_t  rssi;
} D_PKT;


typedef struct other_pkt {
        uint8_t mac[6];
	uint8_t data[1024]; // Length not known
	int8_t  rssi;
	int     len; // Without mac, spoof, uuid
} OTHER_PKT;


// Callback for ibeacon message
typedef void (*CbeaconCallBack)(CBEACON_PKT *packet);
// Callback for generic advert message
typedef void (*CadvertCallBack)(ADVERT_PKT *packet);
// Callback for special packet
typedef void (*CsCallBack)(S_PKT *packet);
// Callback for service advert
typedef void (*CsaCallBack)(SA_PKT *packet);
// Data Message
typedef void (*DCallBack)(D_PKT *packet);
// Callback for other packets
typedef void (*CotherCallBack)(OTHER_PKT *packet);

int cbeacon_init();
int cbeacon_setcb_i(BEACON_TYPE type, CbeaconCallBack bcb); 
int cbeacon_setcb_g(BEACON_TYPE type, CadvertCallBack acb);
int cbeacon_setcb_s(BEACON_TYPE type, CsCallBack scb);
int cbeacon_setcb_sa(BEACON_TYPE type, CsaCallBack sacb);
int cbeacon_setcb_d(BEACON_TYPE type, DCallBack  dcb);
int cbeacon_setcb_o(BEACON_TYPE type, CotherCallBack  ocb); 
int cbeacon_start();
void cbeacon_stop();

#endif /* __CBEACON_H */
