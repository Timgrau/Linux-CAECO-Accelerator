#!/bin/sh
# Script to setup a user.
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

echo "INFO: You want to create a user."
read -p "Type the name of the user:  " USERNAME

echo "INFO: User will be created and added to wheel and sudo group."
sudo useradd $USERNAME
sudo usermod -aG wheel $USERNAME
sudo usermod -aG sudo $USERNAME

sudo sed -i 's/# %sudo	ALL=(ALL) ALL/%sudo  ALL=(ALL) ALL/g' /etc/sudoers

echo "INFO: Setting up a password for the fresh created user"
sudo passwd $USERNAME

echo "INFO: Log into the created user, exit with 'exit'"
sudo login $USERNAME
