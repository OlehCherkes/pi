#include <stdio.h>
#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <unistd.h>

int main() {
  inquiry_info* devices = NULL;
  int max_rsp = 255;
  int num_rsp = 0;
  int dev_id, sock;

  char addr[19] = { 0 };
  char name[248] = { 0 };

  dev_id = hci_get_route(NULL);
  sock = hci_open_dev(dev_id);

  if (dev_id < 0 || sock < 0) {
    perror("Error opening socket");
    exit(1);
  }

  devices = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
  num_rsp = hci_inquiry(dev_id, 10, max_rsp, NULL, &devices, IREQ_CACHE_FLUSH);
  
  if (num_rsp < 0) {
    perror("Error during inquiry");
    exit(1);
  }

  printf("Number of devices found: %d\n", num_rsp);

  for (int i = 0; i < num_rsp; i++) {
    char addr[19] = { 0 };
    ba2str(&(devices[i].bdaddr), addr);
    printf("Device found: %s\n", addr);
  }

  //inquiry_info* ii = NULL;

  //for (int i = 0; i < num_rsp; i++) {
  //  ba2str(&(devices[i].bdaddr), addr);
  //  memset(name, 0, sizeof(name));
  //  if (hci_read_remote_name(sock, &(ii + i)->bdaddr, sizeof(name),name, 0) < 0) strcpy(name, "[unknown]");
  //  printf("%s  %s\n", addr, name);
  //}

  free(devices);
  close(sock);

  return 0;
}