#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pcap.h>
#include <pcap/pcap.h>

#define LOOP_FOREVER 0
#define CAPTURE_USER ""

void packet_handler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
  printf(
    "-----\ntimestamp:\t\t%d\ncapture length:\t\t%u\nraw packet length:\t%u\n",
    h->ts,
    h->caplen,
    h->len
  );

  fwrite(bytes, 1, h->len, stdout);
  return;
}

int main(int argc, char *argv[]) {
  int err;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t **devices = malloc(sizeof *devices);
  pcap_t *capture_handle;

  err = pcap_findalldevs(devices, errbuf);
  if(err == -1) {
    fprintf(stderr, "Couldnt find default device: %s\n", errbuf);
    return(1);
  }

  printf("Device found: %s\n", (*devices)->name);

  capture_handle = pcap_create((*devices)->name, errbuf);
  if (capture_handle == NULL) {
    fprintf(stderr, "Couldn't create capture handle: %s\n", errbuf);
    return(1);
  }

  pcap_set_promisc(capture_handle, true);
  pcap_set_datalink(capture_handle, DLT_IEEE802_11_RADIO);
  err = pcap_activate(capture_handle);

  if (err != 0) {
    char* errdesc = pcap_geterr(capture_handle);
    fprintf(stderr, "Couldn't activate capture handle: %s\n", errdesc);
    return(1);
  }

  pcap_loop(capture_handle, LOOP_FOREVER, &packet_handler, CAPTURE_USER);

  return(0);
}

