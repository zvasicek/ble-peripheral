"""Example of how to create a Peripheral device/GATT Server with several characteristics"""
import logging, random, time

# Bluezero modules
from bluezero import async_tools
from bluezero import adapter
from bluezero import peripheral

try:
    from gpiozero import CPUTemperature as CPUTemp
    CPUTemperature = lambda : (CPUTemp()).temperature
except:
    CPUTemperature = lambda : random.randrange(3200,7000,1)/100.0
    pass


# Custom 128-bit service uuid (can be generated at https://www.uuidgenerator.net/)
CPU_TMP_SRVC = '9941f656-8e3e-11eb-8dcd-0242ac130003'

# Bluetooth SIG adopted UUID for Characteristic Presentation Format
CPU_FMT_DSCP = '2904'
# Bluetooth SIG adopted UUID for Characteristic User Description
CPU_UD_DSCP = '2901'
# https://www.bluetooth.com/specifications/assigned-numbers/

class TempCharacteristic:
    """
    Characteristic that enables read-only access to the current temperature measurement. The value takes into account the preferred unit.
    """
    # Bluetooth SIG adopted UUID for Temperature characteristic
    UUID = '2A6E'

    def __init__(self, unit):
        #Characteristic for notifications
        self.characteristic = None
        self.unit = unit

    def get_temp(self):
        """
        This routine gets CPU temperature
        """
        temp = CPUTemperature()
        if self.unit.is_farenheit():
            temp = (temp * 1.8) + 32
        return temp

    def read_value(self):
        """
        Example read callback. Value returned needs to a list of bytes/integers
        in little endian format.

        Return list of integer values.
        Bluetooth expects the values to be in little endian format and the
        temperature characteristic to be an sint16 (signed & 2 octets) and that
        is what dictates the values to be used in the int.to_bytes method call.

        :return: list of uint8 values
        """
        return list(int(self.get_temp() * 100).to_bytes(2, byteorder='little', signed=True)) + self.unit.read_value()

    def notify(self, notifying, characteristic):
        """
        Notificaton callback example. In this case used to start a timer event
        which calls the update callback ever 2 seconds

        :param notifying: boolean for start or stop of notifications
        :param characteristic: The python object for this characteristic
        """
        if notifying:
            self.characteristic = characteristic
            async_tools.add_timer_seconds(2, self.send_notification)
        else:
            self.characteristic = None

    def send_notification(self):
        """
        Routine which delivers the updated value to the connected device

        :param characteristic:
        :return: boolean to indicate if timer should continue
        """
        if not self.characteristic:
            return

        # read/calculate new value.
        new_value = self.read_value()
        # Causes characteristic to be updated and send notification
        self.characteristic.set_value(new_value)
        # Return True to continue notifying. Return a False will stop notifications
        # Getting the value from the characteristic of if it is notifying
        return self.characteristic.is_notifying

class UnitCharacteristic:
    """
    Characteristic that enables read-write access to the preffered temperature unit.
    """
    # Custom 128-bit UUID
    UUID = "9941fb38-8e3e-11eb-8dcd-0242ac130003"

    def __init__(self, notification_cb=None):
        self.unit = 'C'
        self.notification_cb = notification_cb
       
    def is_farenheit(self):
        return self.unit == 'F'

    def read_value(self):
        return [ord(self.unit)]

    def write_value(self, value, options):
        value = bytes(value).decode('utf-8')
        unit = 'F' if len(value) and value[0] == 'F' else 'C'
        if unit != self.unit:
            self.unit = unit
            if self.notification_cb:
                self.notification_cb()
            #send notification

def main(adapter_address):
    """Creation of peripheral"""
    logger = logging.getLogger('localGATT')
    logger.setLevel(logging.DEBUG)

    unit = UnitCharacteristic()
    temp = TempCharacteristic(unit)
    unit.notification_cb = temp.send_notification

    # Example of the output from read_value
    print('Temperature is {}\u00B0C'.format(
        int.from_bytes(temp.read_value()[:2], byteorder='little', signed=True)/100))


    # Create peripheral
    cpu_monitor = peripheral.Peripheral(adapter_address,
                                        local_name='Thermometer',
                                        appearance=1344)

    # Add service
    cpu_monitor.add_service(srv_id=1, uuid=CPU_TMP_SRVC, primary=True)
    # Add characteristic
    cpu_monitor.add_characteristic(srv_id=1, chr_id=1, uuid=TempCharacteristic.UUID,
                                   value=[], notifying=False,
                                   flags=['read', 'notify'],
                                   read_callback=temp.read_value,
                                   write_callback=None,
                                   notify_callback=temp.notify
                                   )
    # Add descriptor (this step is optional)
    if 0:
      cpu_monitor.add_descriptor(srv_id=1, chr_id=1, dsc_id=1, uuid=CPU_FMT_DSCP,
                               value=[0x0E, 0xFE, #signed 16-bit
                                      0x2F, 0x27, #GATT Unit, temperature celsius 0x272F,  
                                      #0xAC, 0x27, #GATT Unit,0x27AC thermodynamic temperature (degree Fahrenheit)
                                      0x01, 0x00, 0x00],
                               flags=['read'])


    # Add characteristic
    cpu_monitor.add_characteristic(srv_id=1, chr_id=2, uuid=UnitCharacteristic.UUID,
                                   value=[], notifying=False,
                                   #flags=['read', 'notify', 'write'],
                                   flags=['read', 'write'],
                                   read_callback=unit.read_value,
                                   write_callback=unit.write_value,
                                   #notify_callback=unit.notify,
                                   )
    
    #cpu_monitor.on_connect = unit.on_connect
    #cpu_monitor.on_disconnect = unit.on_disconnect

    # Add descriptor
    cpu_monitor.add_descriptor(srv_id=1, chr_id=2, dsc_id=1, uuid=CPU_UD_DSCP,
                               value=[ord(a) for a in "Temperature Units (F or C)"],
                               flags=['read'])


    # Publish peripheral and start event loop
    cpu_monitor.publish()

if __name__ == '__main__':
    adapter = list(adapter.Adapter.available())
    if len(adapter):
        adapter = adapter[0]
        adapter.powered = False
        time.sleep(.5)
        adapter.powered = True
        main(adapter.address)
    else:
        print("No BLE adapter found")
