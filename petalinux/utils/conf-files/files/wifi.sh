#!/bin/sh
# Script to setup a wifi connection.
# Copyright (C) 2023  Timo Grautstueck

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

RUN_WPA="/var/run/wpa_supplicant"
ETC_WPA="/etc/wpa_supplicant.conf"

echo "INFO: You want to setup wifi."
read -p "SSID of your network: " SSID
read -p "PSK of your network: " PSK

if [ -d $RUN_WPA ];
then
    echo "INFO: Need to remove $RUN_WPA to setup wifi properly"
    sudo rm -r $RUN_WPA
fi
if [ -f $ETC_WPA ];
then
    echo "INFO: Need to remove $ETC_WPA to setup wifi properly"
    sudo rm $ETC_WPA
fi

echo "INFO: Setting SSID and PSK in wpa_supplicant.conf"

sudo sed -i "s|ssid=.*|ssid=\"${SSID}\"|g" wpa_supplicant.conf
sudo sed -i "s|psk=.*|psk=\"${PSK}\"|g" wpa_supplicant.conf

sudo cp -rf ./wpa_supplicant.conf /etc/.
sudo modprobe wilc-sdio
sudo ifconfig wlan0 up
sudo wpa_supplicant -dd -Dnl80211 -iwlan0 -c /etc/wpa_supplicant.conf -B
sudo busybox udhcpc -i wlan0

if ! ping -c1 8.8.8.8 &>/dev/null 
then
    echo "INFO: Something went wrong, could not ping dns server, try again."
    sudo rm -r $RUN_WPA
    sudo rm $ETC_WPA
    sudo rmmod wilc-sdio
else
    echo "INFO: Sucessfully set up wifi!"
fi
