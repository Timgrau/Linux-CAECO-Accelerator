#!/bin/sh
# Script to set the repo of the dnf package installer.
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

DIR="/etc/yum.repos.d/"
REPO="zynqmp-generic_eg.repo"

sudo wget https://petalinux.xilinx.com/sswreleases/rel-v2020/generic/rpm/repos/zynqmp-generic_eg.repo

if [ -d "$DIR" ];
then
    echo "INFO: $DIR already exist."
else
    echo "INFO: $DIR does not exist, creating.."
    sudo mkdir $DIR
fi

echo "INFO: Moving $REPO into $DIR .."
sudo mv $REPO $DIR
sudo dnf --releasever
sudo dnf clean all

