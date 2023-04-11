# Entwicklung eines Embedded-Linux-Systems für Multisensor-Datenverarbeitung

Dieses Repository enthält die Quelldateien der oben genannten Bachelorarbeit.
- Autor: Timo Grautstück
- Datum: 13.01.2023
- Abschluss: B.Sc.

## Kompilieren der Applikationen:
```bash
$ make 
```

- Zum Crosskompilieren, installiere den GNU C Compiler für die arm64-Architektur:

    ```bash 
    $ sudo apt-get install gcc-aarch64-linux-gnu
    $ make CROSS_COMPILE=aarch64-linux-gnu-
    ```

## Linux-Image bauen:
```bash
$ cd petalinux/ 
$ ./petalinux_build.sh
```
- Um das PetaLinux-Script ausführen zu können, wird PetaLinux v2022.1 benötigt:

    https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/embedded-design-tools/2022-1.html
## SD-Karte partitionieren
1.  Finde das richtige Blockgerät, das formatiert werden soll:

	```bash
	$ lsblk -I 8
	# oder
	$ sudo fdisk -l
	```
2.  Hänge Dateisysteme aus, **sdd ist ein Beispiel, wähle die richtige SD-Karte aus 1.**:
   
	```bash
	$ sudo umount /dev/sdd*
	```
3.  Formatiere die SD-Karte mit fdisk:
   
    ```bash
    $ sudo fdisk /dev/sdd
        # In fdisk folgende Befehle
        o   # clear the in-memory partition table
        n   # new partition
        <Enter>
        <Enter>
        <Enter>
        +100M   # Boot-Partition
        # Falls nach Signatur entfernen gefragt wird -> [J]
        t
        b
        n
        <Enter>
        <Enter>
        <Enter>
        <Enter>   # Root-Partition
        p
        w
	```
4.  Dateisysteme anlegen:

	```bash
	# Boot-Partition
    $ sudo mkfs.vfat /dev/sdd1 -n BOOT
		
	# Root-Partition
    $ sudo mkfs.ext4 /dev/sdd2 -L "rootfs"
	```


## SD-Karte flashen:
- Nachdem das PetaLinux-Script durchgelaufen ist:

    ```bash
    $ cd return/
    $ cp BOOT.BIN boot.scr image.ub /media/<user>/BOOT
    $ sudo tar xvf rootfs.tar.gz -C /media/<user>/roootfs
    $ sync ; 
    ```

## System nutzen:
- Auf dem Ultra96v2-Board, nachdem der Kernel gebootet wurde:
    - Username: petalinux
    
- Wi-Fi Verbindung herstellen
    ```bash
    $ cd /etc/conf
    $ ./wifi.sh
    ```
- Kernelmodule laden:
    ```bash
    # AXI-DMA Treiber
    $ sudo modprobe xilinx-axdima
    # CAECO Treiber
    $ sudo modprobe caeco
    ```

- Accelerometerdaten transferieren:
    ```bash
    $ sudo transfer-mqtt
    ```
 **NOTE**: *Diese Anwendung funktioniert nur, wenn das Board im Foresight-Netzwerk eingebunden ist. Es kann aber die test-transfer Applikation genutzt, oder der Source-Code angepasst werden.*
- OpenSSH-Daemon starten:
    ```bash
    $ sudo systemctl start sshd
    $ sudo systemctl enable sshd
    ```

- Test-Transfer-Applikation auf das Board übertragen:
    ```bash
    $ cd bin/
    $ scp test-transfer-caeco petalinux@<IP-ADDR>:/home/petalinux/
    ```
