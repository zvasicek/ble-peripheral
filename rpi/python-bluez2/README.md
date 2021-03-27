# DESCRIPTION

Python uses Bluez library via bluezero (DBUS)

# PREREQUISITES
Bluez
Python 3.7+

The mandatory Python packages:
```
$ sudo pip3 install dbus-python
$ sudo pip3 install bluezero
```

Optional package for reading the real CPU temperature:
```
$ sudo pip3 install gpiozero RPi.GPIO
```

# EXAMPLE

Direct usage

```console
$ sudo python3.7 main.py
$ sudo python3 main.py
```

Python virtual environment

```
$ cd python-bluez2

$ sudo apt install libgirepository1.0-dev
$ sudo apt install libcairo2-dev

$ python3.7 -m venv ble-env
$ source ble-env/bin/activate
(ble-env) $ pip3 install dbus-python
(ble-env) $ pip3 install PyGObject
(ble-env) $ pip3 install bluezero
(ble-env) $ pip3 install gpiozero RPi.GPIO  # optional package for reading the real CPU temperature

(ble-env) $ sudo ble-env/bin/python3.7 main.py
```

# LIMITATIONS
Please see the [python-bluez/README.md](../python-bluez/README.md)
