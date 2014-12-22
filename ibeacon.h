/*
 * =====================================================================================
 *
 *       Filename:  ibeacon.h
 *
 *    Description:  Example header file for ibeacon packet
 *
 *        Version:  1.0
 *        Created:  2014年12月23日 07時39分38秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Paul Creaser
 *        Company:  
 *
 * =====================================================================================
 */

#ifndef IBEACON_H
#define IBEACON_H

#include <stdint.h>

typedef struct ibeacon_pkt {
    uint8_t 	mac[6];
    uint8_t 	spoof[6];
	uint8_t 	uuid[UUID_LEN];
    uint16_t 	major;
    uint16_t 	minor;
    uint8_t 	power;
    int8_t 		rssi;
} IBEACON_PKT;

#endif


