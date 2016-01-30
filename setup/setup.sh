#!/bin/bash

sudo apt-get update
sudo apt-get upgrade

#======================================================
# INSTALL LIBRARIES
#======================================================

sudo apt-get install git tmux htop
sudo apt-get install chkconfig
sudo apt-get install uuid-dev
sudo apt-get install python-smbus

#======================================================
# INSTALL NODE
#======================================================

# install node 12
curl -sL https://deb.nodesource.com/setup_0.12 | sudo -E bash -
sudo apt-get install --yes nodejs
sudo ln -s /usr/bin/nodejs /usr/bin/node
# install node-gyp
sudo npm install -g node-gyp

#======================================================
# RPi C++ libraries
#======================================================

# install wiringPi:
cd workspace/lib
git clone git://git.drogon.net/wiringPi
cd wiringPi
./build

#======================================================
# I2C
#======================================================

sudo apt-get install i2c-tools libi2c-dev
# run raspi-config and enable I2C in the advanced tab

#======================================================
# APACHE SERVER
#======================================================

# Apache server
sudo apt-get install apache2
# enable the necessary modules
sudo ln -s /etc/apache2/mods-available/proxy.load /etc/apache2/mods-enabled/proxy.load
sudo ln -s /etc/apache2/mods-available/proxy_http.load /etc/apache2/mods-enabled/proxy_http.load
sudo ln -s /etc/apache2/mods-available/proxy_wstunnel.load /etc/apache2/mods-enabled/proxy_wstunnel.load

# torrents
sudo apt-get install transmission-daemon

#======================================================
# MOUNT NFS
#======================================================

# check if rpcbind is running:
sudo status rpcbind status
# if not, follow instructions in setup/rpcbind-instructions.txt to set it up

# to automatically mount NFS, add this line to /etc/fstab
# 192.168.1.100:/nfs/Public   /mnt/nfs-wd   nfs    rw  0  0
# to mount manually:
# sudo mount -t nfs -rw 192.168.1.100:/nfs/Public /mnt/nfs-wd

#======================================================
# FIXES
#======================================================

# fix message "failed to execute /lib/udev/mtp-probe" at boot
sudo apt-get install libmtp-runtime

# remove the RPi network service as it is unreliable
sudo apt-get remove raspberrypi-net-mods

