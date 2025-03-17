import json

import up_core as up
from up_core import RAM


class RAMData:
    def __init__(self, result):
        self.torque_enable = up.singleByteToInt(result[RAM.TORQUE_ENABLE])
        self.led = up.singleByteToInt(result[RAM.LED])
        self.cw_compliance_margin = up.singleByteToInt(result[RAM.CW_COMPLIANCE_MARGIN])
        self.ccw_compliance_margin = up.singleByteToInt(result[RAM.CCW_COMPLIANCE_MARGIN])
        self.cw_compliance_slope = up.singleByteToInt(result[RAM.CW_COMPLIANCE_SLOPE])
        self.ccw_compliance_slope = up.singleByteToInt(result[RAM.CCW_COMPLIANCE_SLOPE])
        self.goal_position = up.combinePosition(result[RAM.GOAL_POSITION_L], result[RAM.GOAL_POSITION_H])
        self.speed = up.combineSpeed(result[RAM.MOVING_SPEED_L], result[RAM.MOVING_SPEED_H])
        self.acceleration = up.singleByteToInt(result[RAM.ACCELERATION])
        self.deceleration = up.singleByteToInt(result[RAM.DECELERATION])
        self.present_position = up.combinePosition(result[RAM.PRESENT_POSITION_L], result[RAM.PRESENT_POSITION_H])
        self.present_speed = up.combineSpeed(result[RAM.PRESENT_SPEED_L], result[RAM.PRESENT_SPEED_H])
        self.present_load = up.doubleByteToInt(result[RAM.PRESENT_LOAD_L], result[RAM.PRESENT_LOAD_H])
        self.present_voltage = up.singleByteToInt(result[RAM.PRESENT_VOLTAGE])
        self.temperature = up.singleByteToInt(result[RAM.TEMPERATURE])
        self.reg_write = up.singleByteToInt(result[RAM.REG_WRITE])
        self.moving = up.singleByteToInt(result[RAM.MOVING])
        self.lock = up.singleByteToInt(result[RAM.LOCK])
        self.min_pwm = up.doubleByteToInt(result[RAM.MIN_PWM_L], result[RAM.MIN_PWM_H])

    def to_dict(self):
        return self.__dict__

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4)
