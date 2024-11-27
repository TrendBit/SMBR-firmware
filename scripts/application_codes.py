import can

module_types = {
    "Undefined"      : 0x00,
    "All"            : 0x01,
    "Any"            : 0x02,
    "Test_module"    : 0x03,
    "Core_module"    : 0x04,
    "Control_module" : 0x05,
    "Sensor_module"  : 0x06,
}

module_instances = {
    "Undefined"   : 0x00,
    "Exclusive"   : 0x01,
    "All"         : 0x02,
    "Reserved"    : 0x03,
    "Instance_1"  : 0x04,
    "Instance_2"  : 0x05,
    "Instance_3"  : 0x06,
    "Instance_4"  : 0x07,
    "Instance_5"  : 0x08,
    "Instance_6"  : 0x09,
    "Instance_7"  : 0x0a,
    "Instance_8"  : 0x0b,
    "Instance_9"  : 0x0c,
    "Instance_10" : 0x0d,
    "Instance_11" : 0x0e,
    "Instance_12" : 0x0f,
}

message_types = {
    "Probe_modules_request"   : 0x300,
    "Probe_modules_response"  : 0x301,
    "Device_reset"            : 0x200,
    "Device_usb_bootloader"   : 0x201,
    "Device_can_bootloader"   : 0x202,
}

uid_length = 6

def reverse_dict_search(dictionary, value):
    for key in dictionary:
        if dictionary[key] == value:
            return key
    return "Unknown"

class Message:
    def __init__(self, message_type=None, module_type=None, instance=None, can_message=None):
        if can_message is not None:
            self.message_type = (can_message.arbitration_id >> 16) & 0xfff
            self.module_type = (can_message.arbitration_id >> 4) & 0x0f
            self.instance = can_message.arbitration_id & 0x0f
            self.data = can_message.data
        else:
            self.message_type = message_type
            self.module_type = module_type
            self.instance = instance
            self.data = []

    def __str__(self):
        return f"Message type: {reverse_dict_search(message_types,self.message_type)}, Module type: {reverse_dict_search(module_types,self.module_type)}, Instance: {reverse_dict_search(module_instances, self.instance)}"

    def can_message(self):
        return can.Message(arbitration_id=(self.message_type << 16) | self.module_type << 4 | self.instance, data=self.data, is_extended_id=True)

class Module:
    def __init__(self, module_type, instance, uid = []):
        self.module_type = module_type
        self.instance = instance
        self.uid = uid

    def __init__(self, message):
        self.module_type = message.module_type
        self.instance = message.instance
        if len(message.data) == uid_length:
            self.uid = message.data
        else:
            self.uid = []

    def uid_str(self) -> str:
        return ''.join(f'{byte:02x}' for byte in self.uid)

    def module_name(self) -> str:
        return reverse_dict_search(module_types, self.module_type)

    def instance_name(self) -> str:
        return reverse_dict_search(module_instances, self.instance)

    def __str__(self):
        return f"type: {self.module_name()}, instance: {self.instance_name()}, UUID: {self.uid_str()}"
