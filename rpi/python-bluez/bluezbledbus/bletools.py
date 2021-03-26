import dbus

BLUEZ_SERVICE_NAME = "org.bluez"
LE_ADVERTISING_MANAGER_IFACE = "org.bluez.LEAdvertisingManager1"
DBUS_OM_IFACE = "org.freedesktop.DBus.ObjectManager"

class BleTools(object):
    @classmethod
    def get_bus(self):
         bus = dbus.SystemBus()

         return bus

    @classmethod
    def find_adapter(self, bus):
        remote_om = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, "/"),
                               DBUS_OM_IFACE)
        objects = remote_om.GetManagedObjects()

        for o, props in objects.items():
            #print(o, props)
            if LE_ADVERTISING_MANAGER_IFACE in props:
                return o

        return None

    @classmethod
    def power_adapter(self, bus, adapter):
        adapter_props = dbus.Interface(bus.get_object(BLUEZ_SERVICE_NAME, adapter),
                "org.freedesktop.DBus.Properties")
        pwr = adapter_props.Get("org.bluez.Adapter1", "Powered")
        print("current power state:","ON" if pwr == dbus.Boolean(1) else "OFF")
        if pwr != dbus.Boolean(1):
            adapter_props.Set("org.bluez.Adapter1", "Powered", dbus.Boolean(1))
