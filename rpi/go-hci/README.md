# DESCRIPTION
GO implementation uses Host Controller Interface (HCI), it does not depend either on the availability, or a particular version of Bluez stack.

# PREREQUISITES
GO language 

```
$ sudo apt-get install golang
$ go version
go version go1.16.2 linux/arm
```

# EXAMPLE

Compilation of the application:

```
$ cd go-hci
$ go mod download github.com/paypal/gatt
$ go build -o ble . 
```

The resulting executable must either be run as root, 

```
$ sudo ./ble
```

or be granted appropriate capabilities:

```
$ sudo setcap 'cap_net_raw,cap_net_admin=eip' ./ble
$ ./ble
```
