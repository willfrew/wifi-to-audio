#!/bin/bash

systemctl stop systemd-networkd.socket
systemctl stop systemd-networkd.service
systemctl stop wpa_supplicant@wlp2s0.service

modprobe -r ath10k_pci
modprobe -r ath10k_core

modprobe ath10k_core rawmode=1 cryptmode=1
modprobe ath10k_pci

sleep 5

airmon-ng start wlp2s0
