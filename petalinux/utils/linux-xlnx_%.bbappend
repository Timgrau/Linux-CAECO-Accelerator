FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " file://bsp.cfg"
SRC_URI:append = " file://fix_u96v2_pwrseq_simple.patch"
KERNEL_FEATURES:append = " bsp.cfg"
SRC_URI += "file://user.cfg"

