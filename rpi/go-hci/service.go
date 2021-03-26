package main

import (
	"time"
	"log"
	"io"
	"encoding/binary"
	"github.com/paypal/gatt"

	"io/ioutil"
	"strconv"
	"strings"
	"math/rand"
)

type packet struct {
	temperature int16
	unit uint8
}

func getTemperature() float64 {
	path := "/sys/class/thermal/thermal_zone0/temp"
	raw, err := ioutil.ReadFile(path)
	if err != nil {
		log.Printf("Failed to read temperature from %q: %v", path, err)
		return 32.0 + 20.0*rand.Float64()
	}

	cpuTempStr := strings.TrimSpace(string(raw))
	cpuTempInt, err := strconv.Atoi(cpuTempStr) // e.g. 55306
	if err != nil {
		log.Fatalf("%q does not contain an integer: %v", path, err)
	}

	cpuTemp := float64(cpuTempInt) / 1000.0
	//log.Printf("CPU temperature: %.3f°C", cpuTemp)

	return cpuTemp
}

func NewThermometerService() *gatt.Service {
	s := gatt.NewService(gatt.MustParseUUID("9941f656-8e3e-11eb-8dcd-0242ac130003"))
	unit := "C"
	unitchanged := make(chan bool, 1)

	sendTempPacket := func (w io.Writer) error {

		temp := getTemperature()
		if unit == "F" {
			temp = (temp * 1.8) + 32.0
		}

		return binary.Write(w, binary.LittleEndian, packet{int16(temp*100), uint8(unit[0])})
	}

	//Temperature characteristic
	c := s.AddCharacteristic(gatt.UUID16(0x2A6E))
	c.HandleNotifyFunc(
		func(r gatt.Request, n gatt.Notifier) {
			// start a timer event which calls the update callback ever 2 seconds
			for !n.Done() {
				if err := sendTempPacket(n); err != nil {
					log.Fatal("Write failed")
				}
				select {
					case _ = <- unitchanged:
					case <-time.After(2 * time.Second):
				}
			}
		})	

	c.HandleReadFunc(
		func(rsp gatt.ResponseWriter, req *gatt.ReadRequest) {
			if err := sendTempPacket(rsp); err != nil {
				log.Fatal("Write failed")
			}
		})

	c.AddDescriptor(gatt.UUID16(0x2904)).SetValue([]byte{ 0x0E, 0xFE, //signed 16-bit
		0x2F, 0x27, //GATT Unit, temperature celsius 0x272F,  
		//0xAC, 0x27, //GATT Unit, thermodynamic temperature (degree Fahrenheit) 0x27AC
		0x01, 0x00, 0x00})

	//Temperature Unit characteristic
	c = s.AddCharacteristic(gatt.MustParseUUID("9941fb38-8e3e-11eb-8dcd-0242ac130003"))

	c.HandleWriteFunc(
		func(r gatt.Request, data []byte) (status byte) {
			newunit := "C"
			if string(data) == "F" {
				newunit = "F"
			}

			if newunit != unit {
				unit = newunit
				log.Println("change unit to:", unit)
				unitchanged <- true
			}

			return gatt.StatusSuccess
		})

	c.HandleReadFunc(
		func(rsp gatt.ResponseWriter, req *gatt.ReadRequest) {
			rsp.Write([]byte(unit))
		})

	c.AddDescriptor(gatt.UUID16(0x2901)).SetValue([]byte("Temperature Units (F or C)"))

	return s
}
