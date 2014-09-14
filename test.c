#include <stdio.h>
#include "cbeacon.h"

void callback(CBEACON_PKT *data) {
        printf("<Cbeacon : Major %u Minor %u Rssi %u Power %u>\n", data->major, data->minor, data->rssi, data->power);
}

int main(int argc, char **argv)
{

  if (cbeacon_init(&callback) == -1 ) {
        return -1;
  }
  if (cbeacon_start() == -1 ) {
        return -1;
  }
  cbeacon_stop();
  return 0;
}
