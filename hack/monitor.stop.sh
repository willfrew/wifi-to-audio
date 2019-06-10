#!/bin/bash

airmon-ng stop wlp2s0mon

modprobe -r ath10k_pci
modprobe -r ath10k_core

modprobe ath10k_core
modprobe ath10k_pci

systemctl start systemd-networkd.socket
systemctl start systemd-networkd.service
systemctl start wpa_supplicant@wlp2s0.service
