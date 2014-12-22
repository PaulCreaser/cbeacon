#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <stdio.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>

#include "cbeacon.h"


#define SNAP_LEN        HCI_MAX_FRAME_SIZE

struct hcidump_hdr {
        uint16_t        len;
        uint8_t         in;
        uint8_t         pad;
        uint32_t        ts_sec;
        uint32_t        ts_usec;
} __attribute__ ((packed));

#define HCIDUMP_HDR_SIZE (sizeof(struct hcidump_hdr))

/*****************************************************************************************************************************
 * Global variables
 *******************************************************************************************************************************/
static int 				device_handle=-1;
static cbeaconCallBack  cbeacon_cb  = NULL; //
static bdaddr_t 		g_mac_address;
static int				g_sd=-1;

/*****************************************************************************************************************************
 * Prototypes
 *******************************************************************************************************************************/
static int open_socket(int dev);
static int start_lescan(int device_id);
static int stop_lescan(int device_handle);
static int process_frames(int dev, bool nb);
/***********************************************************************************************************/

/**
* @brief   Open Socket 
*
* @param dev Device handle
*
* @return 
*/
static int open_socket(int dev)
{
        struct sockaddr_hci addr;
        struct hci_filter flt;
        struct hci_dev_info di;
        int sk, opt;

        if (dev != HCI_DEV_NONE) {
                int dd = hci_open_dev(dev);
                if (dd < 0) {
                        perror("Can't open device");
                        return -1;
                }

                if (hci_devinfo(dev, &di) < 0) {
                        perror("Can't get device info");
                        return -1;
                }

				if (hci_read_bd_addr(dd, &g_mac_address, 1000) < 0 ) {
						perror("Can't get device mac address");
                        return -1;
				}

                opt = hci_test_bit(HCI_RAW, &di.flags);
                if (ioctl(dd, HCISETRAW, opt) < 0) {
                        if (errno == EACCES) {
                                perror("Can't access device");
                                return -1;
                        }
                }
                hci_close_dev(dd);
        }

        /* Create HCI socket */
        sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
        if (sk < 0) {
                perror("Can't create raw socket");
                return -1;
        }

        opt = 1;
        if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
                perror("Can't enable data direction info");
                return -1;
        }

        opt = 1;
        if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0) {
                perror("Can't enable time stamp");
                return -1;
        }

        /* Setup filter */
        hci_filter_clear(&flt);
        hci_filter_all_ptypes(&flt);
        hci_filter_all_events(&flt);
        if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
                perror("Can't set filter");
                return -1;
        }

        /* Bind socket to the HCI device */
        memset(&addr, 0, sizeof(addr));
        addr.hci_family = AF_BLUETOOTH;
        addr.hci_dev = dev;
        if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                fprintf(stderr, "Can't attach to device hci%d. %s(%d)\n",
                                        dev, strerror(errno), errno);
                return -1;
        }

        return sk;
}

/**
* @brief  Stop low energy scan 
*
* @param device_handle
*
* @return 
*/
static int stop_lescan(int device_handle)
{
  uint8_t filter_dup = 0;
  int err = hci_le_set_scan_enable(device_handle, 0x00, filter_dup, 1000);
  if (err < 0) {
        perror("Disable scan failed");
	return -1;
  }
  hci_close_dev(device_handle);
  return 0;
}

/**
* @brief Start low energy scan
*
* @param device_id
*
* @return 
*/
static int start_lescan(int device_id)
{
  int device_handle = 0;
  if((device_handle = hci_open_dev(device_id)) < 0)
  {
    perror("Could not open device");
    return -1;
  }
  uint8_t filter_policy = 0x00;
  uint16_t interval = htobs(0x0010);
  uint16_t window = htobs(0x0010);

  int err = hci_le_set_scan_parameters(device_handle, 0x01, interval, window, 0x00, filter_policy, 1000);
  if (err < 0) {
  	perror("Set scan parameters failed");
	return -1;
  }

  err = hci_le_set_scan_enable(device_handle, 0x01, 0, 1000);
  if (err < 0) {
 	perror("Enable scan failed");
	return -1;
  }
  return device_handle;
}

/**
* @brief  Process frames
*
* @param dev   Device handle
* @param sock  Socket number
* @param nb    Non blocking flag
*
* @return 
*/
static int process_frames(int dev, bool nb)
{
    if (g_sd < 0) return -1;
    char buf[SNAP_LEN + HCIDUMP_HDR_SIZE];//  = malloc(snap_len + HCIDUMP_HDR_SIZE);
    while (device_handle!=-1) // Loop in blocking mode
	{
		void *data = buf + HCIDUMP_HDR_SIZE;
		fd_set set;
		FD_ZERO(&set);
		FD_SET(g_sd, &set);
		struct timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec=100000;
		int rv = select (g_sd+1, &set, NULL, NULL, &timeout);
		struct iovec  iv;
        iv.iov_base = data;
        iv.iov_len  = SNAP_LEN;
		struct msghdr msg;
		char ctrl[128];
		memset(&msg, 0, sizeof(msg));
                msg.msg_iov = &iv;
                msg.msg_iovlen = 1;
                msg.msg_control = ctrl;
 		msg.msg_controllen = 100;
		if (rv > 0) 
		{ 
        	int len = recvmsg(g_sd, &msg, MSG_DONTWAIT);
            if (len <= 0) 
			{
            	if (errno == EAGAIN || errno == EINTR)
                	continue;
                perror("Receive failed");
                return -1;
            } else {
				if ( cbeacon_cb != NULL ) cbeacon_cb( (int8_t*)data, len);
			}
		}
		if (nb) break;
    }
    return 0;
}

/**
* @brief 
*
* @return 
*/
int cbeacon_init()
{
  	int device_id = hci_get_route(NULL); // Get device
  	device_handle = start_lescan(device_id);
	g_sd = open_socket(0);
	return  device_handle;
}

/**
* @brief 
*
* @param cb
*
* @return 
*/
int cbeacon_setcb(cbeaconCallBack  cb)
{
	cbeacon_cb = cb;
	return 0;
}

/**
* @brief  Start beacon scan
*
* @return 
*/
int cbeacon_start(bool nb)
{
    return process_frames(0, nb);
}

/**
* @brief   Get mac address of BLE device
*
* @param mac_address[6]
*/
void cbeacon_get_mac_address(int8_t mac_address[6])
{
	memcpy(mac_address, (int8_t *)&g_mac_address, 6);
}

/**
* @brief Stop beacon scan
*/
void cbeacon_stop()
{
	if (device_handle != -1) stop_lescan(device_handle);
	cbeacon_cb = NULL;
}
