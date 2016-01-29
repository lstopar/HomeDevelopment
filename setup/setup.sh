#!/bin/bash

sudo apt-get update
sudo apt-get upgrade

# update RPi
#sudo rpi-update

sudo apt-get install git tmux htop
sudo apt-get install chkconfig
sudo apt-get install uuid-dev i2c-tools libi2c-dev

# install node 12
curl -sL https://deb.nodesource.com/setup_0.12 | sudo -E bash -
sudo apt-get install --yes nodejs
sudo ln -s /usr/bin/nodejs /usr/bin/node
# install node-gyp
sudo npm install -g node-gyp

# fix issue with rpcbind
# follow instructions setup/rpcbind-instructions.txt

# fix message "failed to execute /lib/udev/mtp-probe" at boot
sudo apt-get install libmtp-runtime

# remove the RPi network service as it is unreliable
sudo apt-get remove raspberrypi-net-mods

# setup I2C
# open /boot/config.txt
# uncomment line dtparam=i2c_arm=on

# server
sudo apt-get install apache2
# enable the necessary modules
sudo ln -s /etc/apache2/mods-available/proxy.load /etc/apache2/mods-enabled/proxy.load
sudo ln -s /etc/apache2/mods-available/proxy_http.load /etc/apache2/mods-enabled/proxy_http.load
sudo ln -s /etc/apache2/mods-available/proxy_wstunnel.load /etc/apache2/mods-enabled/proxy_wstunnel.load

# torrents
sudo apt-get install transmission-daemon

# TODO must fix rpcbind to mount the storage

# to mount the storage
# sudo mount -t nfs -rw 192.168.1.100:/nfs/Public /mnt/nfs-wd

