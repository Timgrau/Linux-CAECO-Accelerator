#!/bin/bash
# Script to build the Linux-System with PetaLinux.
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

DEV_TREE_PL="components/plnx_workspace/device-tree/device-tree/pl.dtsi"
DEV_TREE_SU="project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi"
APPS="project-spec/meta-user/recipes-apps/"
MODULES="project-spec/meta-user/recipes-modules/"
KERNEL="project-spec/meta-user/recipes-kernel/linux/"
T_MQTT=${APPS}transfer-mqtt/files/
AXIDMA_B=${APPS}axidma-benchmark/files/

RET="../return"
UTILS="../utils"
SRC="../../src"

create_t_mqtt() {
    # Create the transfer-mqtt binary in petalinux under /usr/bin/
    petalinux-create -t apps -n transfer-mqtt --enable
    
    cp $SRC/mqtt-c/include/* $T_MQTT
    cp $SRC/mqtt-c/src/* $T_MQTT
    cp $SRC/mqtt-c/templates/posix_sockets.h $T_MQTT
    cp $SRC/xilinx_axidma/include/* $T_MQTT
    cp $SRC/xilinx_axidma/library/libaxidma.c $T_MQTT
    cp $SRC/axidma_transfer.c $T_MQTT
    cp $SRC/transfer-accelero-caeco.c ${T_MQTT}transfer-mqtt.c
    cp $UTILS/Makefile_tmqtt ${T_MQTT}Makefile
    cp $UTILS/transfer-mqtt.bb ${APPS}transfer-mqtt

    sed -i -e 's=#include <mqtt.h>=#include "mqtt.h"=g' ${T_MQTT}transfer-mqtt.c ${T_MQTT}mqtt_pal.c ${T_MQTT}mqtt.c
    sed -i -e 's=#include <mqtt_pal.h>=#include "mqtt_pal.h"=g' ${T_MQTT}transfer-mqtt.c ${T_MQTT}mqtt.h
    sed -i -e 's=#include "mqtt-c/templates/posix_sockets.h"=#include "posix_sockets.h"=g' ${T_MQTT}transfer-mqtt.c
}

create_axidma_b() {
    # Create the axidma-benchmark binary in petalinux (/usr/bin/)
    petalinux-create -t apps -n axidma-benchmark --enable
    
    cp $SRC/xilinx_axidma/include/* $AXIDMA_B
    cp $SRC/xilinx_axidma/library/libaxidma.c $AXIDMA_B
    cp $SRC/xilinx_axidma/examples/* $AXIDMA_B
    cp $UTILS/axidma-benchmark.bb ${APPS}axidma-benchmark/
    cp $UTILS/Makefile_axib ${AXIDMA_B}Makefile    
}

create_conf() {
    # Create bash scripts for setting up wifi etc.
    petalinux-create -t apps -n conf-files --enable    
    cp -r $UTILS/conf-files/ $APPS
}

create_ceaco_mod() {
    # Create the caeco module
    petalinux-create -t modules -n caeco --enable
    cp ../../caeco/caeco.c  ${MODULES}caeco/files/
}

create_wilc_mod() {
    # Create the wifi module and firmware
    petalinux-create -t apps -n wilc3000-fw --enable
    petalinux-create -t modules -n wilc --enable
    
    cp $UTILS/wilc3000-fw.bb ${APPS}wilc3000-fw/
    cp -r $UTILS/wilc/ $MODULES
}

create_axidma_mod() {
    # Create the xilinx-axidma 
    petalinux-create -t modules -n xilinx-axidma --enable
    cp -r $UTILS/xilinx-axidma/ $MODULES
}

###############################################
#### Start --> Main function of the script ####
###############################################
read -p "Path of your xsa-file: e.g. (utils/caeco_design.xsa): " XSA_FILE
if [ -f $XSA_FILE ] && [ "${XSA_FILE: -4}" == ".xsa" ];
then
    # Remove all till the last /  
    PROJECT_PATH=${XSA_FILE##*/}
    # Extract all in front of .xsa
    PROJECT_PATH=${PROJECT_PATH%.xsa}_linux
else
    echo "INFO: $XSA_FILE does not exists or does not end with .xsa, exiting."
    exit
fi

petalinux-create -t project --template zynqMP -n $PROJECT_PATH
cd $PROJECT_PATH
petalinux-config --get-hw-description=../$XSA_FILE --silentconfig

cp $UTILS/config $UTILS/rootfs_config project-spec/configs/
cp $UTILS/fix_u96v2_pwrseq_simple.patch $UTILS/user.cfg ${KERNEL}linux-xlnx/
cp $UTILS/linux-xlnx_%.bbappend $KERNEL

create_wilc_mod
create_axidma_mod
create_conf

petalinux-config --silentconfig
petalinux-build -c rootfs

# Check if the DT contains caeco-ipc
if grep -q "caeco" $DEV_TREE_PL
then
    echo "INFO: Using device tree includes for caeco."
    cp $UTILS/system-user_caeco.dtsi $DEV_TREE_SU
    
    create_t_mqtt
    create_ceaco_mod
else
    echo "INFO: Using device tree includes for benchmark."
    cp $UTILS/system-user_benchmark.dtsi $DEV_TREE_SU
    
    create_axidma_b
fi

petalinux-build
petalinux-package --boot --fsbl images/linux/zynqmp_fsbl.elf --fpga images/linux/system.bit --u-boot --force

if [ ! -d $RET ];
then
    mkdir $RET
fi

cd images/linux/
cp BOOT.BIN image.ub boot.scr rootfs.tar.gz ../../$RET

## TODO: If $BOOT, $ROOTFS or $ROOT is mounted copy files into it
## RETURN: cd /return
## RETURN: cp BOOT.BIN image.ub boot.scr /media/<user>/BOOT
## RETURN: sudo tar xvf rootfs.tar.gz -C /media/<user>/root
