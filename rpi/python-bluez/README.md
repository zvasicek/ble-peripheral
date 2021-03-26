# DESCRIPTION
Python uses Bluez library via DBUS

# PREREQUISITES
Bluez 

$ sudo pip3 install dbus-python

# EXAMPLE
$ sudo python3 main.py

# LIMITATIONS
Raspberry PI stretch:

$ bluetoothd -v
    5.43
    - does not advertise local name (a bug in bluez / not implemented?)
  - advertises as dual mode device (can't be changed)

  You need to enable experimental features (-E flag) of bluez, 
  sudo vi /lib/systemd/system/bluetooth.service 
  ExecStart=/usr/lib/bluetooth/bluetoothd -E

Raspberry PI buster:
$ bluetoothd -v
  5.50
  - advertises as dual mode device (can't be changed)


# SOURCES:
- compiled from https://github.com/hadess/bluez/tree/master/test

bluez:
https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc

bluez GATT API:
https://git.kernel.org/pub/scm/bluetooth/bluez.git/tree/doc/gatt-api.txt


https://github.com/luetzel/bluez/blob/master/unit/doc/advertising-api.txt