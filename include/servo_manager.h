//
// Created by noodles on 25-3-11.
//

#ifndef UP_CORE_SERVO_MANAGER_H
#define UP_CORE_SERVO_MANAGER_H

#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <deque>
#include "logger.h"
#include "serial/serial.h"
#include "servo.h"

class ServoManager {
private:
    ServoManager() = default;

    ServoManager(ServoManager &) = delete;

    ServoManager &operator=(const ServoManager &) = delete;

public:
    ~ServoManager() = default;

    static auto &instance() {
        static ServoManager obj;
        return obj;
    }

private:
    std::thread searchThread;

    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop_predicate{true};

    std::atomic<bool> isSearching{false};

    std::string searchPort;
    std::deque<int> dequeBauds;
    std::unordered_map<int, std::shared_ptr<Servo>> mapServos;

    std::atomic<int> searchID{0};

    std::function<void(int, int, int)> callback;

    void startSearchThread();

public:

    bool searching() {
        return isSearching.load();
    }

    void setCallback(std::function<void(int, int, int)> callback) {
        this->callback = std::move(callback);
    }

    // 启动搜索线程
    void startSearchServoID(const std::string &port, const std::vector<int> &baudrates);

    // 停止搜索线程
    void stopSearchServoID();

};


#endif //UP_CORE_SERVO_MANAGER_H
