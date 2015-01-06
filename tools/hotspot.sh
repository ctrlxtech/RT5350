#! /bin/sh
ifconfig eth2 down
iwpriv ra0 set NetworkType=Infra
iwpriv ra0 set AuthMode=WPA2PSK
iwpriv ra0 set EncrypType=AES
iwpriv ra0 set WPAPSK="12345678"
iwpriv ra0 set SSID="Hotspot"