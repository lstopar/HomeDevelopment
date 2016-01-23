#!/bin/bash

sudo apt-get update
sudo apt-get upgrade

sudo rpi-update

sudo apt-get install uuid-dev
sudo apt-get install i2c-tools
sudo apt-get install libi2c-dev

# fix message "failed to execute /lib/udev/mtp-probe" at boot
sudo apt-get install libmtp-runtime
# remove the RPi network service as it is unreliable
sudo apt-get remove raspberrypi-net-mods

# server and torrents
sudo apt-get install apache2
# enable the necessary modules
sudo ln -s /etc/apache2/mods-available/proxy.load /etc/apache2/mods-enabled/proxy.load
sudo ln -s /etc/apache2/mods-available/proxy_http.load /etc/apache2/mods-enabled/proxy_http.load
sudo ln -s /etc/apache2/mods-available/proxy_wstunnel.load /etc/apache2/mods-enabled/proxy_wstunnel.load
