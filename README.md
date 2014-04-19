laser-rfid
==========

Client software for the RFID laser odometer at Ace Monster Toys


The RFID laser odometer at Ace Monster Toys is made from:
---------------------------------------------------------

this teensy arduino clone: http://www.pjrc.com/store/teensy.html

with this RFID reader: https://www.sparkfun.com/products/11827

and a 16x2 Character LCD like this one: https://www.sparkfun.com/products/791

a generic 5v DPDT relay (used to disable the laser and lasercut license dongle)

connected to this cheapo TP-Link router: http://www.amazon.com/TP-LINK-TL-WR703N-Portable-802-11n-Wireless/dp/B0083Z54P0


TP-Link Router Configuration:
-----------------------------

the tp-link router was re-flashed using the openwrt image builder.

the openwrt image builder was invoked like this:

make image PROFILE=TLWR703 PACKAGES=base-files busybox dnsmasq dropbear firewall hotplug2 iptables kernel kmod-ath9k kmod-gpio-button-hotplug kmod-ipt-nathelper kmod-usb-core kmod-usb2 kmod-wdt-ath79 libc libgcc mtd netifd opkg swconfig uboot-envtools uci wpad-mini wget coreutils-sha256sum

some other configuration changes were made to associate with the AMT network
and to run the laser_boss.sh script upon startup and possibly add a package or
two that i forgot about when running the image builder.


teensy configuration:
---------------------

the LCD panel is connected to teensy pins 12, 13, 14, 15, 21 and 20

the Laser Firing input signal is comnected to teensy pin 0

the Laser Enable relay is connected to teensy pin 16

the Laser Ready input signal is connected to teensy pin 17

the ID12LA RFID reader is connected to RX pin of the teensy UART

the teensy is connected to the TP-Link router via USB and shows up as a serial
device at /dev/ttyACM0
