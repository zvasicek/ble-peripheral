# DESCRIPTION

Python uses Bluez library via bluezero (DBUS)

# PREREQUISITES
Python 3.7+

```console
$ sudo pip3 install bluezero
$ sudo pip3 install gpiozero
```

# EXAMPLE

Direct usage

```console
$ sudo python3.7 main.py
$ sudo python3 main.py
```

Python virtual environment

```console
$ cd python-bluez2

$ sudo apt install libgirepository1.0-dev
$ sudo apt install libcairo2-dev

$ python3.7 -m venv ble-env
$ source ble-env/bin/activate

(ble-env) $ pip3.7 install dbus-python

(ble-env) $ pip3.7 install PyGObject

(ble-env) $ pip3.7 install bluezero

#for CPU temperature
(ble-env) $ pip3.7 install gpiozero RPi.GPIO

(ble-env) $ sudo ble-env/bin/python3.7 main.py
```

# LIMITATIONS
Please see the [python-bluez/README.md](../python-bluez/README.md)
