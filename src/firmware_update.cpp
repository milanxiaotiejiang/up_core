//
// Created by noodles on 25-3-13.
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <thread>
#include <atomic>
#include <condition_variable>
#include "firmware_update.h"
#include "servo_protocol.h"
#include "logger.h"
#include "servo_protocol_parse.h"


std::vector<std::vector<uint8_t>> FirmwareUpdate::textureBinArray(const std::string &fileName) {
    std::vector<std::vector<uint8_t>> frames;

    std::vector<uint8_t> fileBuffer;
    readFile(fileName, fileBuffer);

    int packetNumber = 1;
    size_t dataSize = fileBuffer.size();
    size_t offset = 0;

    while (offset < dataSize) {
        std::vector<uint8_t> frame;
        buildFrame(fileBuffer, packetNumber, frame);

        frames.push_back(frame);

        offset += 128;
        ++packetNumber;
    }

    return frames;
}

bool FirmwareUpdate::bootloader(uint8_t id) {

    servo::ServoProtocol protocol(id);

    serial::Serial serial("/dev/ttyUSB0", 1000000, serial::Timeout::simpleTimeout(1000));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    auto resetPacket = protocol.buildResetBootLoader();
//    Logger::debug("发送复复位到 bootloader 模式命令：" + bytesToHex(resetPacket));

    serial.flushInput();

    size_t bytes_written = serial.write(resetPacket.data(), resetPacket.size());

    if (bytes_written != resetPacket.size()) {
        Logger::error("sendCommand: Failed to write full frame. Expected: "
                      + std::to_string(resetPacket.size()) + ", Written: " + std::to_string(bytes_written));
        return false;
    }

    bool success = serial.waitReadable();
    if (success) {
        Logger::debug("✅ 发送复位到 bootloader 模式命令成功！");
    } else {
        Logger::error("❌ 发送复位到 bootloader 模式命令失败！");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    serial.close();

    return success;
}

bool FirmwareUpdate::firmware_upgrade() {

//    serial::Serial serial("/dev/ttyUSB0", 9600, serial::Timeout::simpleTimeout(1000));
    upgradeSerial = std::make_shared<serial::Serial>("/dev/ttyUSB0", 9600, serial::Timeout::simpleTimeout(1000));

    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    read_count = 0;
    stop_writing = false;
    stop_reading = false;
    write_finished = false;


    // 读取线程
    std::thread read_thread = std::thread([this] {
        while (!stop_reading) {
            size_t available_bytes = upgradeSerial->available();
            if (available_bytes > 0) {

                Logger::debug("📌 串口已打开，尝试读取 " + std::to_string(available_bytes) + " 字节数据");

                uint8_t read_data[available_bytes];
                size_t bytes_read = upgradeSerial->read(read_data, available_bytes);
                if (bytes_read > 0) {
                    std::ostringstream oss;
                    oss << "Read " << bytes_read << " bytes from the serial port.  ";
                    for (size_t i = 0; i < bytes_read; ++i) {
                        oss << std::hex << static_cast<int>(read_data[i]) << " ";
                    }
                    oss << std::dec;

                    Logger::debug(oss.str());

                    if (read_data[0] == 0x43) {
                        // 读取成功，更新计数
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            ++read_count;
                        }

                        if (read_count >= read_iteration) {
                            Logger::debug("✅ 已成功读取 " + std::to_string(read_iteration) + " 次数据！");
                            stop_writing = true;  // 设置标志以停止写入线程
                            stop_reading = true;  // 设置标志以停止读取线程
                            cv.notify_all();  // 通知主线程
                            break;
                        }

                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });


    // 写入线程
    std::thread write_thread = std::thread([this] {
        for (int i = 0; i < write_iteration; ++i) {
            if (stop_writing) {
                Logger::debug("🛑 写入线程已停止，停止发送数据。");
                break;
            }

            upgradeSerial->flushInput();

            uint8_t data[] = {0x64};  // 创建一个包含 0x64 的数组
            size_t size = sizeof(data);  // 数组的大小

            size_t write_size = upgradeSerial->write(data, size);
            bool success = write_size == size;

            if (success) {
                Logger::debug("✅ 发送握手信号成功！");
            } else {
                Logger::error("❌ 发送握手信号失败！");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 写入完成后设置标志并通知所有等待的线程
        {
            std::lock_guard<std::mutex> lock(mtx);
            write_finished = true;
        }
        cv.notify_all();
    });



    // 主线程等待结果
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return read_count >= 5 || write_finished; });

    stop_writing = true;  // 确保写入线程能够退出
    stop_reading = true;  // 确保读取线程能够退出

    // 确保线程都完成
    if (write_thread.joinable()) {
        write_thread.join();
    }
    if (read_thread.joinable()) {
        read_thread.join();
    }

    // 判断是否读取成功或失败
    if (read_count >= read_iteration) {
        Logger::info("✅ 操作成功，已读取数据！");
    } else if (write_finished) {
        Logger::error("❌ 操作失败，写入完成但未读取到足够的数据！");
        upgradeSerial->close();
    } else {
        Logger::error("❌ 操作失败，未读取到数据！");
        upgradeSerial->close();
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    return read_count >= read_iteration;
}

bool FirmwareUpdate::firmwareUpdate(std::vector<std::vector<uint8_t>> binArray) {

    stop_receive = false;

    std::thread receive_thread = std::thread([this] {
        while (stop_receive) {
            size_t available_bytes = upgradeSerial->available();

            if (available_bytes > 0) {
                Logger::debug("📌 串口已打开，尝试读取 " + std::to_string(available_bytes) + " 字节数据");

                uint8_t read_data[available_bytes];
                size_t bytes_read = upgradeSerial->read(read_data, available_bytes);
                if (bytes_read > 0) {
                    std::ostringstream oss;
                    oss << "Read " << bytes_read << " bytes from the serial port.  ";
                    for (size_t i = 0; i < bytes_read; ++i) {
                        oss << std::hex << static_cast<int>(read_data[i]) << " ";
                    }
                    oss << std::dec;

                    Logger::info(oss.str());

                    uint32_t received_message_id = message_counter;

                    {
                        std::lock_guard<std::mutex> lock(mutex_);

                        std::vector<uint8_t> data_vector(read_data, read_data + available_bytes);

                        // 将接收到的数据存入 map，使用消息 ID 作为键
                        received_data_[received_message_id] = data_vector;

                        // 通知对应的线程，数据已经接收完毕
                        if (message_conditions_.count(received_message_id)) {
                            message_conditions_[received_message_id]->notify_one();
                        }
                    }

                }
            }

        }
    });

    bool success = false;
    int retry = 0;

    for (int i = 0; i < binArray.size(); ++i) {

        bool ref = false;
        retry = 0;

        auto frame = binArray[i];

        ref = sendFrame(frame);

        while (retry < 5) {
            ref = sendFrame(frame);

            if (ref) {
                Logger::info("发送第 " + std::to_string(i) + " 数据包成功！");
                break;  // 发送成功，跳出重试循环
            } else {
                Logger::error("❌ 发送第 " + std::to_string(i) + " 数据包失败！");

                // 增加重试次数
                ++retry;
                if (retry < 5) {
                    Logger::info("重试发送第 " + std::to_string(i) + " 数据包！");
                } else {
                    Logger::error("❌ 重试次数超限，放弃发送第 " + std::to_string(i) + " 数据包！");
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 如果失败超过 5 次，退出循环
        if (!ref) {
            Logger::error("❌ 第 " + std::to_string(i) + " 数据包发送失败，跳过该数据包");
            break;
        }
    }

    Logger::info("固件升级完成！");

    // 结束接收线程
    stop_receive = true;
    if (receive_thread.joinable()) {
        receive_thread.join();
    }

    return success;
}

bool FirmwareUpdate::sendFrame(const std::vector<uint8_t> &frame) {
    // 为每条命令生成一个唯一的消息 ID
    uint32_t message_id = generateMessageId();

    // 创建一个 unique_lock 用于等待接收线程通知
    std::unique_lock<std::mutex> wait_lock(mutex_);

    // 为每个数据包创建一个条件变量
    auto condition_variable = std::make_unique<std::condition_variable>();
    message_conditions_[message_id] = std::move(condition_variable);

    upgradeSerial->flushInput();

    size_t bytes_written = upgradeSerial->write(frame.data(), frame.size());
    if (bytes_written != frame.size()) {
        Logger::error("sendCommand: Failed to write full frame. Expected: "
                      + std::to_string(frame.size()) + ", Written: " + std::to_string(bytes_written));
        return false;
    }


    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);

    // 等待接收线程通知，直到收到相应的数据包
    if (message_conditions_[message_id]->wait_until(wait_lock, timeout, [this, message_id] {
        // 判断条件是等待接收到数据
        return received_data_.count(message_id) > 0;  // 如果已接收到该 message_id 的数据
    })) {
        auto data = received_data_[message_id];
        received_data_.erase(message_id);  // 删除已处理的响应

        message_conditions_.erase(message_id);  // 删除对应的条件变量
        return true;
    } else {
        return false;
    }

}


void FirmwareUpdate::buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame) {
    frame.resize(133);
    frame[0] = 0x01;
    frame[1] = packetNumber;
    frame[2] = 255 - packetNumber;

    size_t dataSize = data.size();
    size_t offset = (packetNumber - 1) * 128;
    size_t copySize = std::min(dataSize - offset, static_cast<size_t>(128));
    std::memcpy(&frame[3], &data[offset], copySize);

    uint16_t crc = calculateCRC(std::vector<uint8_t>(frame.begin() + 3, frame.begin() + 3 + copySize));
    frame[131] = crc >> 8;
    frame[132] = crc & 0xFF;
}

uint16_t FirmwareUpdate::calculateCRC(const std::vector<uint8_t> &data) {
    uint16_t crc = 0;
    for (uint8_t byte: data) {
        crc ^= (byte << 8);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void FirmwareUpdate::readFile(const std::string &fileName, std::vector<uint8_t> &buffer) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

