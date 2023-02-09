# BitBake-File to built bash-scripts into the rootfs.
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

SUMMARY = "Simple conf-files application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://wifi.sh \
	   file://setup-user.sh \
	   file://wpa_supplicant.conf \
		  "

S = "${WORKDIR}"

do_install() {
	     install -d ${D}${sysconfdir_native}/conf
	     
	     install -m 0755 ${S}/wifi.sh ${D}${sysconfdir_native}/conf
	     install -m 0755 ${S}/setup-user.sh ${D}${sysconfdir_native}/conf
	     cp ${S}/wpa_supplicant.conf ${D}${sysconfdir_native}/conf
}

FILES:${PN} = "${sysconfdir_native}/conf/*"


