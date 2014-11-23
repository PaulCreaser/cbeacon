#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/poll.h>
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

static int  snap_len = SNAP_LEN;
static int device_handle=-1;
static CbeaconCallBack  cbeacon_cb  = NULL; // Ibeacon Advert
static CadvertCallBack  abeacon_cb  = NULL; // Generic beacon advert
static CotherCallBack   obeacon_cb  = NULL; //
static CsCallBack	sbeacon_cb  = NULL; // Special Advert
static CsaCallBack 	sabeacon_cb = NULL; // Service Advert
static DCallBack       dbeacon_cb  = NULL; // Data
static BEACON_TYPE	g_type = BEACON_TYPE_NONE;

/***********************************************************************************************************/
// Open Blue Tooth Device
static int open_socket(int dev);
static int start_lescan(int device_id);
static int stop_lescan(int device_handle);
static int parse_cbeacondata(unsigned char *buf, int len);
static int process_frames(int dev, int sock, int fd);
static void sigint_handler(int sig);
/***********************************************************************************************************/

/** Open hci
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

/*
 * Ensure clean shutdown on kill
 */
static void sigint_handler(int sig)
{
       	if (device_handle!=-1) stop_lescan(device_handle);
	exit(0);
}

/* 
 * Stop le scan
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

// Start LE scan
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
* Extract data from buffer
*/
static int parse_cbeacondata(unsigned char *buf, int len)
{
	if (len==45) {
		// Ibeacon
		if ( ( buf[21] == 0x02 && buf[22] == 0x15) && (g_type & BEACON_TYPE_I) )  // Check if ibeacon packet
		{
			CBEACON_PKT cbeacon;
        		memcpy(cbeacon.mac, buf + 8, 6);
        		memcpy(cbeacon.spoof, buf+15, 6);
        		memcpy(cbeacon.uuid, buf+23, 16);
			cbeacon.major = buf[39]*256 + buf[40];
        		cbeacon.minor = buf[41]*256 + buf[42];
			cbeacon.power= buf[43];
			int rssi = (int)buf[44];
			rssi=256-rssi;
        		cbeacon.rssi= (int8_t)rssi;
			if ( cbeacon_cb != NULL ) cbeacon_cb(&cbeacon);
		} else if (g_type & BEACON_TYPE_G)  { // Generic Advert
			ADVERT_PKT abeacon;
			memcpy(abeacon.mac, buf + 8, 6);
			memcpy(abeacon.spoof, buf+15, 6);
			memcpy(abeacon.uuid, buf+23, 16);
			memcpy(abeacon.payload, buf+39, 5);
			int rssi = (int)buf[44];
                        rssi=256-rssi;
                        abeacon.rssi= (int8_t)rssi;
			if ( abeacon_cb != NULL ) abeacon_cb(&abeacon);
		}
	} else if ( (len == 25) && (g_type & BEACON_TYPE_S)  ) {
		// Basic Advert message which also includes ASCII device name
		S_PKT  sbeacon;
		memcpy(sbeacon.mac, buf + 7, 6);
		memcpy(sbeacon.dev_name, buf+16, 5);
		memcpy(sbeacon.data, buf, len);
		sbeacon.len = len;
		int rssi = (int)buf[len-1];
		rssi=256-rssi;
                sbeacon.rssi= (int8_t)rssi;
		if ( sbeacon_cb != NULL ) sbeacon_cb(&sbeacon);
	} else if ( len==33 && (g_type & BEACON_TYPE_SA) ) {
		SA_PKT sabeacon;
		memcpy(sabeacon.mac, buf + 7, 6);
		sabeacon.len = len;
		memcpy(sabeacon.uuid, buf+16, UUID_LEN);
		int rssi = (int)buf[len-1];
                rssi=256-rssi;
                sabeacon.rssi= (int8_t)rssi;
		if (sabeacon_cb != NULL ) sabeacon_cb(&sabeacon);
	} else if ( len==28 && (g_type & BEACON_TYPE_D)) {
                D_PKT dbeacon;
		memcpy(dbeacon.mac, buf + 7, 6);
                memcpy(dbeacon.data, buf+12, 16);
                dbeacon.len = len;
                int rssi = (int)buf[len-1];
                rssi=256-rssi;
                dbeacon.rssi= (int8_t)rssi;
                if ( dbeacon_cb != NULL ) dbeacon_cb(&dbeacon);
	} else if (g_type & BEACON_TYPE_O) {
		OTHER_PKT obeacon;
		memcpy(obeacon.data, buf, len);
		obeacon.len = len;
		int rssi = (int)buf[len-1];
		rssi=256-rssi;
                obeacon.rssi= (int8_t)rssi;
		if ( obeacon_cb != NULL ) obeacon_cb(&obeacon);
	}
	return 0;
}

/**
* Process data from ble
*/
static int process_frames(int dev, int sock, int fd)
{
	char ctrl[100];
        if (sock < 0) return -1;
        if (snap_len < SNAP_LEN) snap_len = SNAP_LEN;
        char *buf = malloc(snap_len + HCIDUMP_HDR_SIZE);
        if (!buf) {
                perror("Can't allocate data buffer");
                return -1;
        }
        void *data = buf + HCIDUMP_HDR_SIZE;

        if (dev != HCI_DEV_NONE) printf("device: hci%d ", dev);

        while (device_handle!=-1) {
		struct pollfd fds[1];
		fds[0].fd = sock;
        	fds[0].events = POLLIN;
        	fds[0].revents = 0;
		poll(fds, 1, -1);
                if (fds[0].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                  if (fds[0].fd == sock) {
                    fprintf(stderr, "Device: disconnected\n");
                    break;
                  }
                  else {
                    fprintf(stderr, "Client: disconnect\n");
                    break;
                  }
                }
		struct iovec  iv;
                iv.iov_base = data;
                iv.iov_len  = snap_len;
		struct msghdr msg;
		memset(&msg, 0, sizeof(msg));
                msg.msg_iov = &iv;
                msg.msg_iovlen = 1;
                msg.msg_control = ctrl;
 		msg.msg_controllen = 100;

                int len = recvmsg(sock, &msg, MSG_DONTWAIT);
                if (len < 0) {
                        if (errno == EAGAIN || errno == EINTR)
                                continue;
                        perror("Receive failed");
                        return -1;
                }
                parse_cbeacondata(data, len);
       }
       return 0;
}

/*
 * Initialization
 */
int cbeacon_init()
{
	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        	fprintf(stderr, "Can't catch SIGINT\n");
        	return -1;
  	}
  	int device_id = hci_get_route(NULL); // Get device
  	device_handle = start_lescan(device_id);
	return  device_handle;
}

/*
 * Add Callbacks
 */
int cbeacon_setcb_i(BEACON_TYPE type, CbeaconCallBack bcb)
{
	g_type = g_type | type;
	cbeacon_cb = bcb;
	return 0;
}

int cbeacon_setcb_g(BEACON_TYPE type, CadvertCallBack acb)
{
	g_type = g_type | type;
	abeacon_cb = acb;
	return 0;
}

int cbeacon_setcb_o(BEACON_TYPE type, CotherCallBack  ocb)
{
	g_type = g_type | type;
	obeacon_cb = ocb;
	return 0;
}

int cbeacon_setcb_s(BEACON_TYPE type, CsCallBack  scb)
{
        g_type = g_type | type;
        sbeacon_cb = scb;
        return 0;
}

int cbeacon_setcb_sa(BEACON_TYPE type, CsaCallBack  sacb)
{
        g_type = g_type | type;
        sabeacon_cb = sacb;
        return 0;
}

int cbeacon_setcb_d(BEACON_TYPE type, DCallBack  dcb)
{
        g_type = g_type | type;
        dbeacon_cb = dcb;
        return 0;
}

/*
 * Start beacon
 */
int cbeacon_start()
{
	int device = 0;
	if (device_handle!=-1) {
        	return process_frames(device, open_socket(device), -1);
  	} else return -1;
}

/*
 * Stop Beacon
 */
void cbeacon_stop()
{
	if (device_handle != -1) stop_lescan(device_handle);
	cbeacon_cb = NULL;
}
