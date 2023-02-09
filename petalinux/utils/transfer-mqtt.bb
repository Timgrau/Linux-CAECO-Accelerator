# This file is the transfer-mqtt recipe.
#

SUMMARY = "Simple transfer-mqtt application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://transfer-mqtt.c \
	   file://axidma_transfer.c \
	   file://mqtt_pal.c \
	   file://libaxidma.c \
	   file://mqtt.c \
	   file://libaxidma.h \
	   file://axidma_ioctl.h \
	   file://mqtt.h \
	   file://mqtt_pal.h \
	   file://posix_sockets.h \
	   file://Makefile \
	   "

S = "${WORKDIR}"

do_compile() {
	     oe_runmake
}

do_install() {
	     install -d ${D}${bindir}
	     install -m 0755 transfer-mqtt ${D}${bindir}
}