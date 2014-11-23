#include <stdio.h>
#include "cbeacon.h"

void callback1(CBEACON_PKT *data) {
        printf("<Cbeacon : Major %u Minor %u Rssi %u Power %u>\n", data->major, data->minor, data->rssi, data->power);
}

void callback2(ADVERT_PKT *data) {
        printf("<Advert>\n");
}

void callback3(OTHER_PKT *data) {
        //if ( 0xda != data->mac[0] ) return;
	//if ( 0xa0 == data->mac[0] ) return;
        printf("<Other MAC:");
	int i=0;
	for (i=0; i<data->len; i++) printf("%c ", data->data[i]);
	printf(">\n");
}

void callback4(S_PKT *data) {
	int i=0;
	printf("<");
	for (i=5; i>=0; i--) printf("%x ", data->mac[i]);
	printf(" Name: ");
	for (i=0; i<5; i++) printf("%c ", data->dev_name[i]);
	printf("RSSI %d", data->rssi);
	printf(">\n");
}

void callback5(SA_PKT *data) {
        int i=0;
        printf("<");
	for (i=5; i>=0; i--) printf("%x ", data->mac[i]);
        printf(" UUID: ");
        for (i=0; i<UUID_LEN; i++) printf("%x ", data->uuid[i]);
	printf("RSSI %d", data->rssi);
        printf(">\n");
}

int main(int argc, char **argv)
{

  if (cbeacon_init(&callback1, &callback2, &callback3) == -1 ) {
        return -1;
  }
  cbeacon_setcb_i(BEACON_TYPE_I, &callback1);
  cbeacon_setcb_g(BEACON_TYPE_G, &callback2);
  cbeacon_setcb_o(BEACON_TYPE_D, &callback3);
  cbeacon_setcb_s(BEACON_TYPE_S, &callback4);
  cbeacon_setcb_sa(BEACON_TYPE_SA, &callback5);
  if (cbeacon_start() == -1 ) {
        return -1;
  }
  cbeacon_stop();
  return 0;
}
