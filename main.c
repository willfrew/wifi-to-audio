#include <endian.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pcap.h>
#include <pcap/pcap.h>
#include <pulse/mainloop.h>

// TODO remove
#include <pulse/util.h>

#include "./depends/radiotap/radiotap.h"
#include "./depends/radiotap/radiotap_iter.h"
#include "./def.h"
#include "./pulse.h"
#include "./drop-root.h"

#define LOOP_FOREVER 0
#define CAPTURE_USER ""

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

struct ieee80211_hdr {
	unsigned short frame_control;
	unsigned short duration_id;
	unsigned char addr1[6];
	unsigned char addr2[6];
	unsigned char addr3[6];
	unsigned short seq_ctrl;
	unsigned short addr4[6];
} __attribute__ ((packed));

static const struct ieee80211_radiotap_namespace ns[] = {};

static const struct ieee80211_radiotap_vendor_namespaces vns = {
  .ns = ns,
  .n_ns = 0,
};

// Please excuse this filthyness
void print_channel_flags(u_short flags) {
  char enabled_flags[8][128];
  int i = 0;

  if (flags & 0x0010) {
    strcpy(enabled_flags[i], "Turbo channel");
    i++;
  }
  if (flags & 0x0020) {
    strcpy(enabled_flags[i], "CCK channel");
    i++;
  }
  if (flags & 0x0040) {
    strcpy(enabled_flags[i], "OFDM channel");
    i++;
  }
  if (flags & 0x0080) {
    strcpy(enabled_flags[i], "2 GHz spectrum channel");
    i++;
  }
  if (flags & 0x0100) {
    strcpy(enabled_flags[i], "5 GHz spectrum channel");
    i++;
  }
  if (flags & 0x0200) {
    strcpy(enabled_flags[i], "Only passive scan allowed");
    i++;
  }
  if (flags & 0x0400) {
    strcpy(enabled_flags[i], "Dynamic CCK-OFDM channel");
    i++;
  }
  if (flags & 0x0800) {
    strcpy(enabled_flags[i], "GFSK channel (FHSS PHY)");
    i++;
  }

  printf("Channel flags: ");
  if (i == 0) {
    printf("-\n");
  } else {
    for(int j = 0; j < i; j++) {
      printf("%s", enabled_flags[j]);

      // If not last flag, print a comma and spacing
      if (j != i - 1) {
        printf(", ");
      }
    }
    printf("\n");
  }
}

void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *bytes) {
  struct ieee80211_radiotap_iterator *iter = malloc(sizeof *iter);
  int err;

  printf(
    "-----\nTimestamp:\t\t%ld\nCapture length:\t\t%lu\nRaw packet length:\t%lu\n",
    header->ts.tv_sec,
    header->caplen,
    header->len
  );

  err = ieee80211_radiotap_iterator_init(iter, (struct ieee80211_radiotap_header*) bytes, header->len, &vns);
  if (err != 0) {
    printf("Unable to initialise radiotap parser\n");
    return;
  }

  /* Iterate through all the radiotap fields and print */
  int i = 0;
  while (i != -ENOENT && i != -EINVAL) {
    i = ieee80211_radiotap_iterator_next(iter);
    switch (iter->this_arg_index) {
      case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
        printf("Antenna signal: %hhd dBm\n", *(char*) iter->this_arg);
        break;
      case IEEE80211_RADIOTAP_DBM_ANTNOISE:
        printf("Antenna noise: %hhd dBm\n", *(char*) iter->this_arg);
        break;
      case IEEE80211_RADIOTAP_CHANNEL:
        printf("Channel Frequency: %hu MHz\n", le16toh(*(u_short*) iter->this_arg));
        print_channel_flags(le16toh(*(u_short*) iter->this_arg+2));
        break;
      default:
        break;
    }
  }

  unsigned int header_length = ((struct ieee80211_radiotap_header*)bytes)->it_len;
  printf("Radiotap header length: %u", header_length);

  if (i == -EINVAL) {
    printf("Something bad happened while parsing radiotap packet");
  }

  printf("\n");

  // ******* play with rest of the packet

  u_char *payload = bytes + header_length;
  int payload_length = header->len - header_length;

  //printf("Radio tap header:\n");
  //fwrite(bytes, 1, header_length, stdout);
  //printf("\n");
  //printf("802.11 frame:\n");
  //fwrite(payload, 1, payload_length, stdout);
  //printf("\n");

  struct ieee80211_hdr *frame_header = (struct ieee80211_hdr*) payload;
  printf("Frame Control: %04hx\n", frame_header->frame_control);
  printf("Frame Control: %04hu us\n", frame_header->duration_id);
  printf("Source address: "MACSTR"\n", MAC2STR(frame_header->addr2));
  printf("Destination address: "MACSTR"\n", MAC2STR(frame_header->addr1));
  printf("Bssid: "MACSTR"\n", MAC2STR(frame_header->addr3));
  printf("\n");

  return;
}

void generate_audio_callback(void* buffer, size_t num_bytes) {
  printf("Audio data requested!\n");
}

void *run_audio_thread(void *arg) {
  app_audio_context *audio_context = (app_audio_context *) arg;

  pa_mainloop* audio_mainloop = setup_pulseaudio_connection(audio_context);

  if (pa_mainloop_run(audio_mainloop, NULL) < 0) {
    fprintf(stderr, "Failed to start the main loop running.\n");
    exit(1);
  }
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

  if (drop_root_privileges() != 0) {
    fprintf(stderr, "Couldn't drop root privileges");
  }

  app_audio_context audio_context = {
    audio_generation_callback: generate_audio_callback,
  };

  pthread_t *audio_thread = malloc(sizeof(pthread_t));

  pthread_create(audio_thread, NULL, run_audio_thread, &audio_context);

  // TODO switch to either manually driving the pcap & pulse loops or go multiprocess/threaded.
  pcap_loop(capture_handle, LOOP_FOREVER, &packet_handler, CAPTURE_USER);

  return(0);
}

