//
// Created by noodles on 25-3-11.
//

#include <memory>
#include "servo_manager.h"
#include "serial/serial.h"
#include "servo.h"
#include "thread"

void ServoManager::startSearchServoID(const std::string &port, const std::vector<int> &baudrates) {
    if (isSearching.load()) {
        Logger::info("正在搜索中...");
        return;
    }

    isSearching.store(true);
    stop_predicate.store(false);   // 重置 stop_thread 为 false，允许重新启动搜索

    this->searchPort = port;

    this->dequeBauds.clear();
    this->dequeBauds = std::deque<int>(baudrates.begin(), baudrates.end());

    this->mapServos.clear();

    startSearchThread();
}

void ServoManager::startSearchThread() {
    Logger::info("开始搜索...");

    searchThread = std::thread([this]() {
        while (true) {
            std::unique_lock<std::mutex> lock(mtx);

            cv.wait_for(lock, std::chrono::milliseconds(searchTimeout), [this] {
                return stop_predicate.load();
            });

            if (stop_predicate) {
                isSearching.store(false);
                Logger::info("搜索线程已停止");
                break;  // 安全停止线程
            }

            if (dequeBauds.empty()) {
                isSearching.store(false);
                Logger::info("搜索完成");
                break;
            }

            int baud = dequeBauds.front();  // 获取队列头部元素
            std::shared_ptr<Servo> servo = mapServos[baud]; // 获取对应的舵机对象
            if (!servo) {

                Logger::info("  搜索波特率：" + std::to_string(baud));

                std::shared_ptr<serial::Serial> serialPtr =
                        std::make_shared<serial::Serial>(searchPort, baud, serial::Timeout::simpleTimeout(1000));
                servo = std::make_shared<Servo>(serialPtr);
                servo->init();

                // 设置舵机数据回调
                servo->setDataCallback([this, baud, &servo](const std::vector<uint8_t> &data) {
                    if (isVerify) {
                        Logger::info("      校验 ID: " + std::to_string(searchID.load()));
                        auto performID = servo->performID(data);
                        if (performID.first) {
                            int id = this->searchID.load();
                            int error = performID.second.second;


                            if (callback)
                                callback(baud, id, error);
                        } else {
                            Logger::error("      ID: " + std::to_string(performID.second.first) + ", 错误码: " +
                                          std::to_string(performID.second.second));
                        }
                    }
                });

                searchID.store(0);  // 重置 ID

                mapServos[baud] = servo;    // 保存舵机对象到 map 中
            }

            int currentId = searchID.load();

            Logger::info("      开启搜索 ID: " + std::to_string(currentId));

            servo::ServoProtocol servoProtocol(currentId);
            auto data = servoProtocol.buildPingPacket();

            bool success = servo->sendCommand(data);
            if (success) {
                Logger::info("      呼叫 ID: " + std::to_string(currentId) + " 成功");
                if (!isVerify) {
                    Logger::info("      无校验 ID: " + std::to_string(currentId));
                    if (callback)
                        callback(baud, currentId, 0);
                }
            } else {
                Logger::info("      呼叫 ID: " + std::to_string(currentId) + " 失败");
            }

            searchID.fetch_add(1);

            if (searchID.load() > 254) {
                searchID.store(0);
                dequeBauds.pop_front(); // 弹出已使用的波特率
            }
        }
    });

    searchThread.detach();
}

void ServoManager::stopSearchServoID() {
    Logger::info("手动停止搜索...");

    stop_predicate.store(true);
    cv.notify_all();

    if (searchThread.joinable()) {
        searchThread.join();
    }

    // 结束后重置 searchThread，避免后续误用
    searchThread = std::thread();
}