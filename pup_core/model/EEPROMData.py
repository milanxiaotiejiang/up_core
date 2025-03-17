import json

import up_core as up
from up_core import EEPROM


class EEPROMData:
    def __init__(self, result):
        self.model_number = up.doubleByteToInt(result[EEPROM.MODEL_NUMBER_L], result[EEPROM.MODEL_NUMBER_H])
        self.version = up.singleByteToInt(result[EEPROM.VERSION])
        self.id = up.singleByteToInt(result[EEPROM.ID])
        self.baudrate = up.singleByteToInt(result[EEPROM.BAUDRATE])
        self.return_delay_time = up.singleByteToInt(result[EEPROM.RETURN_DELAY_TIME])
        self.cw_angle_limit = up.combinePosition(result[EEPROM.CW_ANGLE_LIMIT_L], result[EEPROM.CW_ANGLE_LIMIT_H])
        self.ccw_angle_limit = up.combinePosition(result[EEPROM.CCW_ANGLE_LIMIT_L], result[EEPROM.CCW_ANGLE_LIMIT_H])
        self.max_temperature = up.singleByteToInt(result[EEPROM.MAX_TEMPERATURE])
        self.min_voltage = up.singleByteToInt(result[EEPROM.MIN_VOLTAGE])
        self.max_voltage = up.singleByteToInt(result[EEPROM.MAX_VOLTAGE])
        self.max_torque = up.doubleByteToInt(result[EEPROM.MAX_TORQUE_L], result[EEPROM.MAX_TORQUE_H])
        self.status_return_level = up.singleByteToInt(result[EEPROM.STATUS_RETURN_LEVEL])
        self.alarm_led = up.singleByteToInt(result[EEPROM.ALARM_LED])
        self.alarm_shutdown = up.singleByteToInt(result[EEPROM.ALARM_SHUTDOWN])

    def to_dict(self):
        return self.__dict__

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4)
