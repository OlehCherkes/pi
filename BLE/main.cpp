#include <stdlib.h>
#include <errno.h>
#include <curses.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluetooth/l2cap.h>

#include <set>
#include <string>
#include <chrono>
#include <thread>

#define ATT_CID 4 


struct hci_request ble_hci_request(uint16_t ocf, int clen, void* status, void* cparam)
{
	struct hci_request rq;
	memset(&rq, 0, sizeof(rq));
	rq.ogf = OGF_LE_CTL;
	rq.ocf = ocf;
	rq.cparam = cparam;
	rq.clen = clen;
	rq.rparam = status;
	rq.rlen = 1;
	return rq;
}

int main()
{
	int ret, status;

	// Get HCI device.
	const int device = hci_open_dev(hci_get_route(NULL)); // returns the adapter number
	if (device < 0) {
		perror("Failed to open HCI device.");
		return 0;
	}

	// Set BLE scan parameters.
	le_set_scan_parameters_cp scan_params_cp;
	memset(&scan_params_cp, 0, sizeof(scan_params_cp));
	scan_params_cp.type = 0x00;
	scan_params_cp.interval = htobs(0x0010);
	scan_params_cp.window = htobs(0x0010);
	scan_params_cp.own_bdaddr_type = 0x00; // Public Device Address (default).
	scan_params_cp.filter = 0x00; // Accept all.

	struct hci_request scan_params_rq = ble_hci_request(OCF_LE_SET_SCAN_PARAMETERS, LE_SET_SCAN_PARAMETERS_CP_SIZE, &status, &scan_params_cp);

	ret = hci_send_req(device, &scan_params_rq, 1000);
	if (ret < 0) {
		hci_close_dev(device);
		perror("Failed to set scan parameters data.");
		return 0;
	}

	// Set BLE events report mask.
	le_set_event_mask_cp event_mask_cp;
	memset(&event_mask_cp, 0, sizeof(le_set_event_mask_cp));
	int i = 0;
	for (i = 0; i < 8; i++) event_mask_cp.mask[i] = 0xFF;

	struct hci_request set_mask_rq = ble_hci_request(OCF_LE_SET_EVENT_MASK, LE_SET_EVENT_MASK_CP_SIZE, &status, &event_mask_cp);
	ret = hci_send_req(device, &set_mask_rq, 1000);
	if (ret < 0) {
		hci_close_dev(device);
		perror("Failed to set event mask.");
		return 0;
	}

	// Enable scanning.
	le_set_scan_enable_cp scan_cp;
	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable = 0x01;	// Enable flag.
	scan_cp.filter_dup = 0x00; // Filtering disabled.

	struct hci_request enable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);

	ret = hci_send_req(device, &enable_adv_rq, 1000);
	if (ret < 0) {
		hci_close_dev(device);
		perror("Failed to enable scan.");
		return 0;
	}

	// Get Results.
	struct hci_filter nf;
	hci_filter_clear(&nf);
	hci_filter_set_ptype(HCI_EVENT_PKT, &nf);
	hci_filter_set_event(EVT_LE_META_EVENT, &nf);
	if (setsockopt(device, SOL_HCI, HCI_FILTER, &nf, sizeof(nf)) < 0) {
		hci_close_dev(device);
		perror("Could not set socket options\n");
		return 0;
	}

	printf("Scanning....\n");

	uint8_t buf[HCI_MAX_EVENT_SIZE];
	evt_le_meta_event* meta_event;
	le_advertising_info* info;
	int len;

	std::set<std::string> seenAddresses;
	auto start_time = std::chrono::high_resolution_clock::now();




	//------------------------------------------------------------
	
	//int l2cap_socket = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	//if (l2cap_socket < 0) {
	//	perror("Failed to create L2CAP socket.");
	//	hci_close_dev(device);
	//	return 0;
	//}

	//// Set the Bluetooth address of the remote device
	//struct sockaddr_l2 addr;
	//memset(&addr, 0, sizeof(addr));
	//addr.l2_family = AF_BLUETOOTH;
	//addr.l2_psm = htobs(0xFFE1);
	//str2ba("E0:E2:E6:3B:31:62", &addr.l2_bdaddr);

	//// Connect to the remote device
	//int rets = connect(l2cap_socket, (struct sockaddr*)&addr, sizeof(addr));
	//if (rets < 0) {
	//	perror("Failed to connect to the remote device.");
	//	close(l2cap_socket);
	//	hci_close_dev(device);
	//	return 0;
	//}

	//------------------------------------------------------------

	while (1) {
		len = read(device, buf, sizeof(buf));

	#if 0
		// Calculate elapsed time
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed_seconds = current_time - start_time;

		// Break the loop after 5 seconds
		if (elapsed_seconds.count() >= 5.0) {
			break;
		}
#endif

		if (len >= HCI_EVENT_HDR_SIZE) {
			meta_event = (evt_le_meta_event*)(buf + HCI_EVENT_HDR_SIZE + 1);

			if (meta_event->subevent == EVT_LE_ADVERTISING_REPORT) {
				uint8_t reports_count = meta_event->data[0];
				void* offset = meta_event->data + 1;

				while (reports_count--) {
					info = (le_advertising_info*)offset;
					char addr[18];
					ba2str(&(info->bdaddr), addr);

					// Check if the address is not in the set of seen addresses
					if (seenAddresses.find(addr) == seenAddresses.end()) {
						// Add the address to the set
						seenAddresses.insert(addr);

						// Print device information
						printf("%s - RSSI %d\n", addr, (char)info->data[info->length]);
					}

					offset = info->data + info->length + 2;
				}
			}
		}
	}
	// Disable scanning.

	memset(&scan_cp, 0, sizeof(scan_cp));
	scan_cp.enable = 0x00;	// Disable flag.

	struct hci_request disable_adv_rq = ble_hci_request(OCF_LE_SET_SCAN_ENABLE, LE_SET_SCAN_ENABLE_CP_SIZE, &status, &scan_cp);
	ret = hci_send_req(device, &disable_adv_rq, 1000);
	if (ret < 0) {
		hci_close_dev(device);
		perror("Failed to disable scan.");
		return 0;
	}

	hci_close_dev(device);

	return 0;
}