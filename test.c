#include <stdio.h>
#include "cbeacon.h"

void callback1(CBEACON_PKT *data) {
        printf("<Cbeacon : Major %u Minor %u Rssi %u Power %u>\n", data->major, data->minor, data->rssi, data->power);
}

void callback2(ADVERT_PKT *data) {
        printf("<Advert>\n");
}

void callback3(OTHER_PKT *data) {
        printf("<Other>\n");
}


int main(int argc, char **argv)
{

  if (cbeacon_init(&callback1, &callback2, &callback3) == -1 ) {
        return -1;
  }
  if (cbeacon_start() == -1 ) {
        return -1;
  }
  cbeacon_stop();
  return 0;
}
