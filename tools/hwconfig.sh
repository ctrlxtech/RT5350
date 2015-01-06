#! /bin/sh
tftp -l Top -r Top -g 10.10.10.102
tftp -l server -r server -g 10.10.10.102
tftp -l ir_sensor -r ir_sensor -g 10.10.10.102
tftp -l UDP_broadcast -r UDP_broadcast -g 10.10.10.102
tftp -l hotspot.sh -r hotspot.sh -g 10.10.10.102
chmod 777 Top server ir_sensor UDP_broadcast