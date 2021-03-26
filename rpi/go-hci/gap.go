package main

import (
	"log"
	"github.com/paypal/gatt"
)

var (
	attrGAPUUID = gatt.UUID16(0x1800)

	attrDeviceNameUUID        = gatt.UUID16(0x2A00)
	attrAppearanceUUID        = gatt.UUID16(0x2A01)
	attrPeripheralPrivacyUUID = gatt.UUID16(0x2A02)
	attrReconnectionAddrUUID  = gatt.UUID16(0x2A03)
	attrPeferredParamsUUID    = gatt.UUID16(0x2A04)
)

// https://www.bluetooth.com/specifications/assigned-numbers/
var gapCharAppearanceUnknown = []byte{0x00, 0x00}
var gapCharAppearanceGenericComputer = []byte{0x80, 0x00}
var gapCharAppearanceGenericSensor = []byte{0x40, 0x05} //1344 - A generic sensor


// NOTE: OS X provides GAP and GATT services, and they can't be customized.
// For Linux/Embedded, however, this is something we want to fully control.
func NewGapService(name string) *gatt.Service {
	s := gatt.NewService(attrGAPUUID)
	c := s.AddCharacteristic(attrDeviceNameUUID)
	c.SetValue([]byte(name))
	c.HandleNotifyFunc(
		func(r gatt.Request, n gatt.Notifier) {
			go func() {
				log.Printf("TODO: indicate client when the name changes")
			}()
		})
//	c.props = c.props & !CharIndicate

	s.AddCharacteristic(attrAppearanceUUID).SetValue(gapCharAppearanceGenericSensor)
//	s.AddCharacteristic(attrPeripheralPrivacyUUID).SetValue([]byte{0x00})
//	s.AddCharacteristic(attrReconnectionAddrUUID).SetValue([]byte{0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
//	s.AddCharacteristic(attrPeferredParamsUUID).SetValue([]byte{0x06, 0x00, 0x06, 0x00, 0x00, 0x00, 0xd0, 0x07})
	return s
}
