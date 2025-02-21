//
// Created by noodles on 25-2-19.
//
#include "add.h"
#include "servo.h"
#include <gtest/gtest.h>

TEST(SystemTest, Architecture) {
    servo::ServoProtocol servo(0x00);
    servo::ServoProtocol servo1(0x01);
    servo::ServoProtocol servo2(0x02);
    servo::ServoProtocol servox(0XFE);

    std::cout << "测试 WRITE DATA 指令..." << std::endl;

    // 例 1: 修改 ID 号为 0
    std::vector<uint8_t> cmd1 = servo1.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                 static_cast<uint8_t>(servo::EEPROM::ID), {0x00});
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x01, 0x04, 0x03, 0x03, 0x00, 0xF4};
    EXPECT_EQ(cmd1, expected_cmd1);

    // 例 2: 角度限制 0~150°
    std::vector<uint8_t> cmd2 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::EEPROM::CCW_ANGLE_LIMIT_L),
                                                                {0xFF, 0x01});
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x08, 0xFF, 0x01, 0xEF};
    EXPECT_EQ(cmd2, expected_cmd2);

    // 例 3: 最高温度限制 80℃
    std::vector<uint8_t> cmd3 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::EEPROM::MAX_TEMPERATURE),
                                                                {0x50});
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x0B, 0x50, 0x9D};
    EXPECT_EQ(cmd3, expected_cmd3);

    // 例 4: 工作电压范围 6V~9V
    std::vector<uint8_t> cmd4 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::EEPROM::MIN_VOLTAGE),
                                                                {0x3C, 0x5A});
    std::vector<uint8_t> expected_cmd4 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x0C, 0x3C, 0x5A, 0x55};
    EXPECT_EQ(cmd4, expected_cmd4);

    // 例 5: 输出力矩限制一半
    std::vector<uint8_t> cmd5 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::EEPROM::MAX_TORQUE_L),
                                                                {0xFF, 0x01});
    std::vector<uint8_t> expected_cmd5 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x0E, 0xFF, 0x01, 0xE9};
    EXPECT_EQ(cmd5, expected_cmd5);

    // 例 6: 设置 0 号舵机不返回数据
    std::vector<uint8_t> cmd6 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::EEPROM::STATUS_RETURN_LEVEL),
                                                                {0x00});
    std::vector<uint8_t> expected_cmd6 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x10, 0x00, 0xE8};
    EXPECT_EQ(cmd6, expected_cmd6);

    // 例 7: 让 0 号舵机卸载
    std::vector<uint8_t> cmd7 = servo.eeprom.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                                static_cast<uint8_t>(servo::RAM::TORQUE_ENABLE),
                                                                {0x00});
    std::vector<uint8_t> expected_cmd7 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x18, 0x00, 0xE0};
    EXPECT_EQ(cmd7, expected_cmd7);

    // 例 8: 0 号舵机中速运动至 150°
    std::vector<uint8_t> cmd8 = servo.ram.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                             static_cast<uint8_t>(servo::RAM::GOAL_POSITION_L),
                                                             {0x00, 0x02, 0x00, 0x02});
    std::vector<uint8_t> expected_cmd8 = {0xFF, 0xFF, 0x00, 0x07, 0x03, 0x1E, 0x00, 0x02, 0x00, 0x02, 0xD3};
    EXPECT_EQ(cmd8, expected_cmd8);

    // 例 9: REG_WRITE + ACTION 让两个舵机同步运动
    std::vector<uint8_t> cmd9a = servo2.ram.buildCommandPacket(servo::ORDER::REG_WRITE,
                                                               static_cast<uint8_t>(servo::RAM::GOAL_POSITION_L),
                                                               {0x00, 0x00});
    std::vector<uint8_t> expected_cmd9a = {0xFF, 0xFF, 0x02, 0x05, 0x04, 0x1E, 0x00, 0x00, 0xD6};
    EXPECT_EQ(cmd9a, expected_cmd9a);

    std::vector<uint8_t> cmd9b = servo1.ram.buildCommandPacket(servo::ORDER::REG_WRITE,
                                                               static_cast<uint8_t>(servo::RAM::GOAL_POSITION_L),
                                                               {0xFF, 0x03});
    std::vector<uint8_t> expected_cmd9b = {0xFF, 0xFF, 0x01, 0x05, 0x04, 0x1E, 0xFF, 0x03, 0xD5};
    EXPECT_EQ(cmd9b, expected_cmd9b);

    std::vector<uint8_t> cmd9c = servox.ram.buildCommandPacket(servo::ORDER::ACTION, 0, {});
    std::vector<uint8_t> expected_cmd9c = {0xFF, 0xFF, 0xFE, 0x02, 0x05, 0xFA};
    EXPECT_EQ(cmd9c, expected_cmd9c);

    // 例 10: 只允许修改 0x18~0x23
    std::vector<uint8_t> cmd10 = servo.ram.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                              static_cast<uint8_t>(servo::RAM::LOCK), {0x01});
    std::vector<uint8_t> expected_cmd10 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x2F, 0x01, 0xC8};
    EXPECT_EQ(cmd10, expected_cmd10);

    // 例 11: 设置加速度=4, 减速度=6
    std::vector<uint8_t> cmd11 = servo.ram.buildCommandPacket(servo::ORDER::WRITE_DATA,
                                                              static_cast<uint8_t>(servo::RAM::MIN_PWM_L),
                                                              {0x04, 0x06});
    std::vector<uint8_t> expected_cmd11 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x30, 0x04, 0x06, 0xBD};
    EXPECT_EQ(cmd11, expected_cmd11);

    std::cout << "✅ 所有 WRITE DATA 指令测试通过！" << std::endl;
}

TEST(ServoEEPROMTest, BuildCommands) {
    servo::ServoEEPROM servo(0x00);
    servo::ServoEEPROM servo1(0x01);

    // 1️⃣ 测试设置舵机 ID
    std::vector<uint8_t> cmd1 = servo1.buildSetID(0x00);
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x01, 0x04, 0x03, 0x03, 0x00, 0xF4};
    EXPECT_EQ(cmd1, expected_cmd1);

    // 2️⃣ 测试设置波特率
    std::vector<uint8_t> cmd2 = servo1.buildSetBaudrate(500000);
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x01, 0x04, 0x03, 0x04, 0x03, 0xF0};
    EXPECT_EQ(cmd2, expected_cmd2);

    // 3️⃣ 测试设定角度限制 (0° ~ 150°)
    std::vector<uint8_t> cmd3 = servo.buildSetAngleLimit(0, 150);
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x08, 0xFF, 0x01, 0xEF};
    EXPECT_EQ(cmd3, expected_cmd3);

    // 4️⃣ 测试设定最大温度 (80°C)
    std::vector<uint8_t> cmd4 = servo.buildSetMaxTemperature(0x50);
    std::vector<uint8_t> expected_cmd4 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x0B, 0x50, 0x9D};
    EXPECT_EQ(cmd4, expected_cmd4);

    // 5️⃣ 测试设定电压范围 (6V ~ 9V)
    std::vector<uint8_t> cmd5 = servo.buildSetVoltageRange(6, 9);
    std::vector<uint8_t> expected_cmd5 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x0C, 0x3C, 0x5A, 0x55};
    EXPECT_EQ(cmd5, expected_cmd5);

    // 6️⃣ 测试设定最大扭矩 (50%)
    std::vector<uint8_t> cmd6 = servo.buildSetMaxTorque(511);
    std::vector<uint8_t> expected_cmd6 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x0E, 0xFF, 0x01, 0xE9};
    EXPECT_EQ(cmd6, expected_cmd6);

    // 7️⃣ 测试设定应答返回级别
    std::vector<uint8_t> cmd7 = servo.buildSetStatusReturnLevel(servo::StatusReturnLevel::NO_RESPONSE);
    std::vector<uint8_t> expected_cmd7 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x10, 0x00, 0xE8};
    EXPECT_EQ(cmd7, expected_cmd7);

    // 🔹 8️⃣ 测试设定 LED 报警
    // 设置 LED 在 过热（BIT2）& 校验和错误（BIT4） 时闪烁
    servo::AlarmLEDConfig alarmLedConfig = servo::AlarmLEDConfig::OVERHEAT | servo::AlarmLEDConfig::CHECKSUM_ERROR;
    std::vector<uint8_t> cmd8 = servo.buildSetAlarmLED(alarmLedConfig);
    std::vector<uint8_t> expected_cmd8 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x11, 0x14, 0xD3}; // ✅ 修正
    EXPECT_EQ(cmd8, expected_cmd8);

    // 🔹 9️⃣ 测试设定报警卸载条件
    // 设定卸载条件：不启用任何卸载保护（NONE）
    servo::AlarmShutdownConfig alarmShutdownConfig = servo::AlarmShutdownConfig::NONE;
    std::vector<uint8_t> cmd9 = servo.buildSetAlarmShutdown(alarmShutdownConfig);
    std::vector<uint8_t> expected_cmd9 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x12, 0x00, 0xE6}; // ✅ 修正
    EXPECT_EQ(cmd9, expected_cmd9);

    std::cout << "✅ 所有 ServoEEPROM 测试通过！" << std::endl;
}

TEST(ServoRAMTest, BuildCommands) {
    servo::ServoRAM servo(0x00);  // 0号舵机

    // 使能扭矩
    std::vector<uint8_t> cmd1 = servo.buildSetTorqueEnabled(false);
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x18, 0x00, 0xE0};
    EXPECT_EQ(cmd1, expected_cmd1);

    // 关闭 LED
    std::vector<uint8_t> cmd2 = servo.buildSetLEDEnabled(false);
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x00, 0x04, 0x03, 0x19, 0x00, 0xDF};
    EXPECT_EQ(cmd2, expected_cmd2);

    // 让 0 号舵机以 **中速 (50%)** 运行到 **150°**
    std::vector<uint8_t> cmd3 = servo.buildMoveToWithSpeed(150.0f, 0.5f);
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0x00, 0x07, 0x03, 0x1E, 0x00, 0x02, 0x00, 0x02, 0xD3};
    EXPECT_EQ(cmd3, expected_cmd3);

    // 设置舵机运行的加速度和减速度
    std::vector<uint8_t> cmd4 = servo.buildSetAccelerationDeceleration(4, 6);
    std::vector<uint8_t> expected_cmd4 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x22, 0x04, 0x06, 0xCB};
    EXPECT_EQ(cmd4, expected_cmd4);

    std::vector<uint8_t> cmd5 = servo.buildMoveToPosition(150.0f);
    std::vector<uint8_t> expected_cmd5 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x1E, 0x00, 0x02, 0xD7};
    EXPECT_EQ(cmd5, expected_cmd5);

    // **测试最小PWM**
    std::vector<uint8_t> cmd6 = servo.buildSetMinPWM(90);
    std::vector<uint8_t> expected_cmd6 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x30, 0x5A, 0x00, 0xBD};
    EXPECT_EQ(cmd3, expected_cmd3);

}

TEST(ServoRAMTest, MoveToPosition) {
    servo::ServoRAM servo1(0x01);
    servo::ServoRAM servo2(0x02);

    // **同步移动到 150°**
    std::vector<uint8_t> cmd1 = servo2.buildAsyncMoveToPosition(0.0f);
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x02, 0x05, 0x04, 0x1E, 0x00, 0x00, 0xD6};
    EXPECT_EQ(cmd1, expected_cmd1);

    // **异步写 300°**
    std::vector<uint8_t> cmd2 = servo1.buildAsyncMoveToPosition(300.0f);
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x01, 0x05, 0x04, 0x1E, 0xFF, 0x03, 0xD5};
    EXPECT_EQ(cmd2, expected_cmd2);

    // **触发 ACTION 让舵机同时运动**
    servo::ServoRAM broadcast(0xFE);
    std::vector<uint8_t> cmd3 = broadcast.buildActionCommand();
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0xFE, 0x02, 0x05, 0xFA};
    EXPECT_EQ(cmd3, expected_cmd3);
}

TEST(ServoRAMTest, GetServoStatus) {
    servo::ServoRAM servo(0x00);

    // **读取当前位置**
    std::vector<uint8_t> cmd1 = servo.buildGetPosition();
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x24, 0x02, 0xD3};
    EXPECT_EQ(cmd1, expected_cmd1);

    // **读取当前速度**
    std::vector<uint8_t> cmd2 = servo.buildGetSpeed();
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x26, 0x02, 0xD1};
    EXPECT_EQ(cmd2, expected_cmd2);

    // **读取当前负载**
    std::vector<uint8_t> cmd3 = servo.buildGetLoad();
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x28, 0x02, 0xCF};
    EXPECT_EQ(cmd3, expected_cmd3);

    // **读取当前电压**
    std::vector<uint8_t> cmd4 = servo.buildGetVoltage();
    std::vector<uint8_t> expected_cmd4 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x2A, 0x01, 0xCE};
    EXPECT_EQ(cmd4, expected_cmd4);

    // **读取当前温度**
    std::vector<uint8_t> cmd5 = servo.buildGetTemperature();
    std::vector<uint8_t> expected_cmd5 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x2B, 0x01, 0xCD};
    EXPECT_EQ(cmd5, expected_cmd5);

    // **检查 REG WRITE 标志**
    std::vector<uint8_t> cmd6 = servo.buildCheckRegWriteFlag();
    std::vector<uint8_t> expected_cmd6 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x2C, 0x01, 0xCC};
    EXPECT_EQ(cmd6, expected_cmd6);

    // **检查 MOVING 运行标志**
    std::vector<uint8_t> cmd7 = servo.buildCheckMovingFlag();
    std::vector<uint8_t> expected_cmd7 = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x2E, 0x01, 0xCA};
    EXPECT_EQ(cmd7, expected_cmd7);
}

TEST(ServoRAMTest, WheelMode) {
    servo::Motor motor(0x00);

    // **进入电机调速模式**
    std::vector<uint8_t> cmd = motor.buildEnterWheelMode();
    std::vector<uint8_t> expected_cmd = {0xFF, 0xFF, 0x00, 0x07, 0x03, 0x06, 0x00, 0x00, 0x00, 0x00, 0xEF};
    EXPECT_EQ(cmd, expected_cmd);

    // 测试顺时针 50% 速度
    std::vector<uint8_t> cmd1 = motor.buildSetWheelSpeed(0.5f);
    std::vector<uint8_t> expected_cmd1 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x20, 0xFF, 0x01, 0xD3};
    EXPECT_EQ(cmd1, expected_cmd1);

    // 测试逆时针 50% 速度
    std::vector<uint8_t> cmd2 = motor.buildSetWheelSpeed(-0.5f);
    std::vector<uint8_t> expected_cmd2 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x20, 0xFF, 0x01, 0xD7};
    EXPECT_EQ(cmd2, expected_cmd2);

    // 测试最大顺时针速度
    std::vector<uint8_t> cmd3 = motor.buildSetWheelSpeed(1.0f);
    std::vector<uint8_t> expected_cmd3 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x20, 0xFF, 0x03, 0xD1};
    EXPECT_EQ(cmd3, expected_cmd3);

    // 测试最大逆时针速度
    std::vector<uint8_t> cmd4 = motor.buildSetWheelSpeed(-1.0f);
    std::vector<uint8_t> expected_cmd4 = {0xFF, 0xFF, 0x00, 0x05, 0x03, 0x20, 0xFF, 0x03, 0xD5};
    EXPECT_EQ(cmd4, expected_cmd4);

    // 还原顺时针 & 逆时针角度限制
    std::vector<uint8_t> cmd5 = motor.buildRestoreAngleLimits();
    std::vector<uint8_t> expected_cmd5 = {0xFF, 0xFF, 0x00, 0x07, 0x03, 0x06, 0x00, 0x00, 0xFF, 0x03, 0xED};

    EXPECT_EQ(cmd5, expected_cmd5);
}
