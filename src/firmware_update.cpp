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
#include <utility>
#include "firmware_update.h"
#include "servo_protocol.h"
#include "logger.h"
#include "servo_protocol_parse.h"

bool FirmwareUpdate::upgrade(std::string port_input, int baud_rate, const std::string &bin_path,
                             int total_retry, int handshake_threshold, int frame_retry_count, int sign_retry_count) {

    Logger::debug(
            "参数 port_input: " + port_input + " baud_rate: " + std::to_string(baud_rate) + " bin_path: " + bin_path +
            " total_retry: " + std::to_string(total_retry) + " handshake_threshold: " +
            std::to_string(handshake_threshold) +
            " frame_retry_count: " + std::to_string(frame_retry_count) + " sign_retry_count: " +
            std::to_string(sign_retry_count));

    // 保存串口设备路径到成员变量
    // 使用 std::move 优化字符串传递，避免不必要的复制
    this->port = std::move(port_input);

    // 保存当前波特率到成员变量
    // 波特率决定了与设备通信的速度，单位为比特/秒
    this->current_baud_rate = baud_rate;

    // 保存握手成功计数阈值
    // 当接收到的握手确认次数达到此值时，认为握手成功
    this->handshake_count = handshake_threshold;

    // 保存固件数据帧发送的最大重试次数
    // 每个数据帧发送失败后最多重试这么多次
    this->fire_ware_frame_retry = frame_retry_count;

    // 保存结束标志发送的最大重试次数
    // 结束标志用于通知设备固件传输已完成
    this->wave_sign_retry = sign_retry_count;

    // 创建二维字节数组，用于存储处理后的固件数据
    // 外层 vector 中的每个元素是一个完整的数据帧
    std::vector<std::vector<uint8_t>> binArray;

    // 尝试读取并解析固件文件
    try {
        // 调用 textureBinArray 方法将固件文件转换为数据帧数组
        // 该方法会将固件文件分割成多个 128 字节的数据包，并添加帧头和 CRC 校验
        binArray = textureBinArray(bin_path);
    } catch (const std::exception &e) {
        // 捕获并处理标准异常，记录具体错误信息
        // e.what() 返回异常的描述信息
        Logger::error("1 ❌ 读取固件文件失败：" + std::string(e.what()));
        return false;
    } catch (...) {
        // 捕获所有其他类型的异常，记录一般性错误信息
        // 这是一个安全保障措施，确保即使出现未预期的异常也不会导致程序崩溃
        Logger::error("1 ❌ 读取固件文件失败：未知错误");
        return false;
    }

    // 声明操作结果变量
    // 用于跟踪升级过程中各个步骤的成功/失败状态
    bool ref;

    // 开始固件升级主循环，最多尝试 total_retry 次
    // 每次循环都会执行完整的升级流程，如果任何步骤失败则重试整个流程
    for (int i = 0; i < total_retry; ++i) {

        // 第一步：启动舵机的 Bootloader 模式
        // 0x01 是舵机的 ID 号，标识要升级的具体舵机设备
        ref = bootloader(0x01);
        if (!ref) {
            // Bootloader 启动失败，记录错误并继续下一次重试
            Logger::error("2 ❌ Bootloader 启动失败，重试中...");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            continue;  // 跳过当前循环的剩余部分，直接开始下一次重试
        }

        // 第二步：与设备进行握手，建立固件升级通信
        // 此步骤将设备切换到固件升级所需的通信协议和波特率
        ref = firmware_upgrade();
        if (!ref) {
            // 握手失败，记录错误并继续下一次重试
            Logger::error("3 ❌ 固件升级失败，重试中...");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            continue;
        }

        // 第三步：发送实际的固件数据帧
        // binArray 包含所有分割好的固件数据包
        ref = firmwareUpdate(binArray);
        if (!ref) {
            // 固件数据传输失败，记录错误并继续下一次重试
            Logger::error("4 ❌ 固件更新失败，重试中...");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            continue;
        }

        // 第四步：发送结束标志，通知设备固件传输已完成
        // 设备收到结束标志后会验证接收到的固件并重启
        ref = wave();
        if (!ref) {
            // 发送结束标志失败，记录错误并继续下一次重试
            Logger::error("5 ❌ 发送结束标志失败，重试中...");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            continue;
        }

        // 所有步骤都成功完成，记录成功信息
        Logger::info("✅ 固件更新流程完成！");

        // 成功完成整个流程，跳出重试循环
        break;
    }

    // 返回最终的操作结果
    // true: 升级成功完成
    // false: 所有重试都失败，升级失败
    return ref;
}

std::vector<std::vector<uint8_t>> FirmwareUpdate::textureBinArray(const std::string &fileName) {
    Logger::info("1 开始读取固件文件：" + fileName);

    // 创建一个二维数组用于存储分解后的数据帧
    // 每个元素是一个 vector<uint8_t>，代表一个完整的固件数据帧
    std::vector<std::vector<uint8_t>> frames;

    // 创建缓冲区用于存储从文件中读取的所有二进制数据
    // 这个 vector 将包含整个固件文件的原始字节内容
    std::vector<uint8_t> fileBuffer;

    // 调用 readFile 函数读取指定文件的内容到 fileBuffer 中
    // 如果文件不存在或无法访问，readFile 会抛出异常
    readFile(fileName, fileBuffer);

    // 初始化包序号为 1，固件升级协议要求从 1 开始计数
    // 这个序号会包含在每个数据帧的帧头中，用于设备识别数据顺序
    int packetNumber = 1;

    // 获取文件总大小，用于控制分包循环的结束
    size_t dataSize = fileBuffer.size();

    // 初始化偏移量为 0，表示从文件开始处理数据
    // 该偏移量用于追踪当前处理到的文件位置
    size_t offset = 0;

    Logger::info("1 固件文件大小：" + std::to_string(dataSize) + " 字节");

    // 循环处理文件内容，每次处理 128 字节，直到处理完整个文件
    // 固件升级协议规定每个数据帧包含 128 字节的有效数据
    while (offset < dataSize) {
        // 创建一个新的数据帧用于存储当前处理的 128 字节数据及其帧头和 CRC
        std::vector<uint8_t> frame;

        // 调用 buildFrame 函数构建数据帧
        // fileBuffer: 包含整个文件内容的缓冲区
        // packetNumber: 当前包的序号
        // frame: 输出参数，存储构建好的数据帧
        buildFrame(fileBuffer, packetNumber, frame);

        // 将构建好的数据帧添加到帧列表中
        // 这些帧将按顺序发送给设备进行固件升级
        frames.push_back(frame);

        // 更新文件偏移量，前进 128 字节准备处理下一块数据
        // 128 是协议规定的每个数据帧的有效载荷大小
        offset += 128;

        // 增加包序号，为下一个数据帧做准备
        // 包序号用于设备识别数据顺序，防止数据包乱序或丢失
        ++packetNumber;
    }

    Logger::info("1 固件文件分包完成，共 " + std::to_string(frames.size()) + " 个数据帧");

    // 返回包含所有数据帧的列表
    // 调用方将使用这些帧按顺序发送给设备完成固件升级
    return frames;
}

bool FirmwareUpdate::bootloader(uint8_t id) {
    // 创建舵机协议对象，用于构建通信数据包
    // 参数 id 是舵机的 ID 号，用于标识要升级的具体舵机设备
    servo::ServoProtocol protocol(id);

    // 创建串口连接对象
    // port: 串口设备名（如 "/dev/ttyUSB0"）
    // current_baud_rate: 当前波特率，用于与舵机的正常通信
    // simpleTimeout(1000): 设置操作超时时间为 1 秒
    serial::Serial serial(port, current_baud_rate, serial::Timeout::simpleTimeout(1000));

    // 短暂延时 10 毫秒，确保串口连接稳定
    // 这个延时可以防止在串口刚打开时立即发送数据导致的通信问题
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 构建复位到 bootloader 模式的命令数据包
    // buildResetBootLoader() 是 ServoProtocol 类中的方法，用于生成特定的复位命令
    auto resetPacket = protocol.buildResetBootLoader();
    // 注释掉的调试日志，显示发送命令的十六进制表示
    Logger::debug("2 发送复位到 bootloader 模式命令：" + bytesToHex(resetPacket));

    // 清空串口输入缓冲区，确保后续读取的是最新的响应数据
    // 这样可以避免之前可能残留在缓冲区中的数据干扰当前操作
    serial.flushInput();

    // 通过串口发送复位命令到舵机
    // resetPacket.data() 返回指向数据包内容的指针
    // resetPacket.size() 返回数据包的大小（字节数）
    // write() 方法返回实际写入的字节数
    size_t bytes_written = serial.write(resetPacket.data(), resetPacket.size());

    // 验证数据是否完全发送成功
    // 如果写入的字节数不等于数据包大小，表示发送过程中出现错误
    if (bytes_written != resetPacket.size()) {
        // 记录错误信息，包含预期写入和实际写入的字节数
        Logger::error("2 ❌ 发送数据失败，预期写入 " + std::to_string(resetPacket.size()) + " 字节，实际写入 " +
                      std::to_string(bytes_written) + " 字节");
        return false;  // 返回失败结果
    }

    // 等待舵机返回响应数据
    // waitReadable() 方法会阻塞当前线程，直到串口有数据可读或超时
    // 成功接收到响应数据时返回 true，超时则返回 false
    bool success = serial.waitReadable();

    // 根据等待结果输出相应的日志信息
    if (success) {
        // 成功收到响应，操作完成
        Logger::debug("2 ✅ 发送复位到 bootloader 模式命令成功！");
    } else {
        // 等待超时，未收到响应
        Logger::error("2 ❌ 发送复位到 bootloader 模式命令失败！");
    }

    // 再次短暂延时 10 毫秒，确保所有通信操作完成
    // 这个延时可以让舵机有足够时间完成复位过程
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 关闭串口连接，释放资源
    // 后续的固件升级操作会使用不同的通信参数重新打开串口
    serial.close();

    // 返回操作结果
    // true: 复位命令成功发送并收到响应
    // false: 复位命令发送失败或未收到响应
    return success;
}

bool FirmwareUpdate::firmware_upgrade() {
    // 创建串口连接对象，使用共享指针管理生命周期
    // port: 串口设备名（如 "/dev/ttyUSB0"）
    // 9600: 波特率，固件升级模式下的通信速率
    // simpleTimeout(1000): 设置超时时间为 1 秒
    upgradeSerial = std::make_shared<serial::Serial>(port, 9600, serial::Timeout::simpleTimeout(1000));

    // 短暂延时 5 毫秒，等待串口完成初始化
    // 这有助于确保串口已准备好进行数据传输
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    // 初始化状态变量
    read_count = 0;       // 成功读取握手回应的计数器
    stop_writing = false; // 写入线程停止标志
    stop_reading = false; // 读取线程停止标志
    write_finished = false; // 写入完成标志

    // 创建并启动读取线程
    // 使用 lambda 表达式定义线程执行函数，捕获 this 指针以访问类成员
    std::thread read_thread = std::thread([this] {
        // 循环监听串口数据，直到收到停止信号
        while (!stop_reading) {
            // 检查串口缓冲区是否有可读数据
            size_t available_bytes = upgradeSerial->available();
            if (available_bytes > 0) {
                // 输出调试日志，显示待读取的字节数
                Logger::debug("3 📌 串口已打开，尝试读取 " + std::to_string(available_bytes) + " 字节数据");

                // 创建动态大小的缓冲区来存储读取的数据
                // 注意：C++ 标准不保证变长数组的可用性，部分编译器支持这种扩展
                uint8_t read_data[available_bytes];

                // 从串口读取数据到缓冲区，并获取实际读取的字节数
                size_t bytes_read = upgradeSerial->read(read_data, available_bytes);

                // 如果成功读取了数据（字节数大于 0）
                if (bytes_read > 0) {
                    // 构建日志信息，包含读取的字节数和数据的十六进制表示
                    std::ostringstream oss;
                    oss << "3 读取 " << bytes_read << " 字节数据：";
                    for (size_t i = 0; i < bytes_read; ++i) {
                        oss << std::hex << static_cast<int>(read_data[i]) << " ";
                    }
                    // 恢复为十进制格式
                    oss << std::dec;

                    // 输出格式化后的调试信息
                    Logger::debug(oss.str());

                    // 检查首字节是否为预期的握手应答信号（0x43）
                    if (read_data[0] == handshake_sign) {
                        // 收到握手应答，增加计数
                        // 使用互斥锁保护共享变量的修改
                        {
                            std::lock_guard<std::mutex> lock(mtx);
                            ++read_count;
                        }

                        // 检查是否已达到所需的握手应答次数（默认为 3 次）
                        if (read_count >= read_iteration) {
                            // 达到目标次数，输出成功信息
                            Logger::debug("3 ✅ 已成功读取 " + std::to_string(read_iteration) + " 次数据！");

                            // 设置停止标志，通知其他线程可以结束
                            stop_writing = true;  // 停止写入线程
                            stop_reading = true;  // 停止当前读取线程

                            // 通知等待的线程（主线程）条件已满足
                            cv.notify_all();

                            // 跳出循环，结束读取线程
                            break;
                        }
                    }
                }
            }

            // 短暂休眠 5 毫秒，降低 CPU 占用率
            // 这个值需要根据实际应用场景调整，太长会增加响应延迟，太短会增加 CPU 负载
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // 创建并启动写入线程
    // 该线程负责向设备发送握手请求信号
    std::thread write_thread = std::thread([this] {
        // 尝试发送多次握手请求（默认 10 次）
        for (int i = 0; i < write_iteration; ++i) {
            // 检查是否收到停止信号
            if (stop_writing) {
                // 收到停止信号，输出日志并退出循环
                Logger::debug("3 🛑 写入线程已停止，停止发送数据。");
                break;
            }

            // 清空串口输入缓冲区，确保接收到的是最新响应
            upgradeSerial->flushInput();

            // 准备发送握手请求数据（0x64）
            uint8_t data[] = {request_sing};
            size_t size = sizeof(data);  // 计算数据大小（这里为 1 字节）

            // 发送数据到串口并获取实际写入的字节数
            size_t write_size = upgradeSerial->write(data, size);

            // 检查是否成功写入全部数据
            bool success = write_size == size;

            // 根据写入结果输出相应的日志
            if (success) {
                Logger::debug("3 ✅ 发送握手信号成功！");
            } else {
                Logger::error("3 ❌ 发送握手信号失败！");
            }

            // 每次发送后等待 100 毫秒，给设备留出响应时间
            // 这个延时也避免了过于频繁地发送请求导致设备无法及时处理
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 所有写入尝试完成后，设置写入完成标志
        {
            // 使用互斥锁保护共享变量的修改
            std::lock_guard<std::mutex> lock(mtx);
            write_finished = true;
        }

        // 通知所有等待的线程（主线程）写入已完成
        cv.notify_all();
    });

    // 主线程等待读取足够的握手应答或写入操作完成
    // 使用条件变量和互斥锁实现线程同步
    std::unique_lock<std::mutex> lock(mtx);

    // 等待条件满足：已读取到足够次数的握手应答或所有写入尝试已完成
    cv.wait(lock, [this] { return read_count >= handshake_count || write_finished; });

    // 设置停止标志，确保两个子线程能够正确退出
    stop_writing = true;
    stop_reading = true;

    // 等待两个子线程结束
    // joinable() 检查线程是否可以被 join（即线程是否仍在运行）
    if (write_thread.joinable()) {
        write_thread.join();  // 等待写入线程结束
    }
    if (read_thread.joinable()) {
        read_thread.join();   // 等待读取线程结束
    }

    // 根据读取计数判断握手过程是否成功
    // 成功条件：读取到的握手应答次数达到或超过预设阈值（read_iteration）
    if (read_count >= read_iteration) {
        // 握手成功，输出成功信息
        Logger::info("3 ✅ 操作成功，已读取数据！");
    } else if (write_finished) {
        // 所有写入尝试已完成，但未收到足够的握手应答
        Logger::error("3 ❌ 操作失败，写入完成但未读取到足够的数据！");
        upgradeSerial->close();  // 关闭串口连接
    } else {
        // 其他情况（理论上不应该发生）
        Logger::error("3 ❌ 操作失败，未读取到数据！");
        upgradeSerial->close();  // 关闭串口连接
    }

    // 操作完成后短暂延时 100 毫秒
    // 给系统一些时间来处理和稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 成功之后无需关闭，下面的挥手需要继续使用串口
    // 失败时关闭串口连接，释放资源
    if (!(read_count >= read_iteration))
        if (upgradeSerial->isOpen())
            upgradeSerial->close();  // 关闭串口连接

    // 返回升级握手结果
    // true: 握手成功（收到足够次数的握手应答）
    // false: 握手失败
    return read_count >= read_iteration;
}

bool FirmwareUpdate::firmwareUpdate(std::vector<std::vector<uint8_t>> binArray) {
    // 设置停止接收标志为 false，表示接收线程应该继续运行
    // 这个原子变量用于线程间的安全通信
    stop_receive = false;

    // 创建并启动接收线程，用于接收设备返回的响应数据
    // 使用 lambda 表达式作为线程函数，捕获 this 指针以访问类成员
    std::thread receive_thread = std::thread([this] {
        // 循环执行，直到 stop_receive 标志被设置为 true
        while (!stop_receive) {
            // 检查串口缓冲区中是否有可读数据
            // available() 方法返回当前可读取的字节数
            size_t available_bytes = upgradeSerial->available();

            // 如果有数据可读，则进行读取处理
            if (available_bytes > 0) {
                // 输出调试信息，显示尝试读取的字节数
                Logger::debug("4 📌 串口已打开，尝试读取 " + std::to_string(available_bytes) + " 字节数据");

                // 创建一个临时缓冲区来存储读取的数据
                // C++ 变长数组（不是所有编译器都支持，但在这里使用）
                uint8_t read_data[available_bytes];

                // 从串口读取数据到缓冲区，并获取实际读取的字节数
                size_t bytes_read = upgradeSerial->read(read_data, available_bytes);

                // 如果成功读取了数据（字节数大于0）
                if (bytes_read > 0) {
                    // 构建日志信息，包含读取的字节数和十六进制表示的数据内容
                    std::ostringstream oss;
                    oss << "4 从串口读取 " << bytes_read << " 字节数据。  ";
                    // 将每个字节转换为十六进制显示
                    for (size_t i = 0; i < bytes_read; ++i) {
                        oss << std::hex << static_cast<int>(read_data[i]) << " ";
                    }
                    // 恢复十进制格式
                    oss << std::dec;

                    // 输出读取的数据详情
                    Logger::debug(oss.str());

                    // 获取当前消息的 ID，用于关联请求和响应
                    // 注意：这里直接使用了当前的 message_counter 值，而不是先递增
                    uint32_t received_message_id = message_counter;

                    // 进入临界区，保护共享数据的访问
                    {
                        // 使用互斥锁保护对共享数据结构的修改
                        std::lock_guard<std::mutex> lock(mutex_);

                        // 将读取的原始字节数组转换为 vector 容器
                        // 这样可以更方便地存储和处理
                        std::vector<uint8_t> data_vector(read_data, read_data + available_bytes);

                        // 将接收到的数据以消息 ID 为键存入 map 中
                        // 这样发送线程可以根据消息 ID 找到对应的响应
                        received_data_[received_message_id] = data_vector;

                        // 检查是否有等待该消息 ID 的条件变量
                        // 如果有，通知等待的线程（通常是发送该命令的线程）数据已就绪
                        if (message_conditions_.count(received_message_id)) {
                            message_conditions_[received_message_id]->notify_one();
                        }
                    }
                }
            }

            // 短暂休眠，避免 CPU 占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // 初始化成功标志和重试计数器
    bool success = false;  // 标记整个固件更新过程是否成功
    int retry;  // 当前数据包的重试次数计数器

    // 遍历所有固件数据包并逐个发送
    // binArray 中的每个元素是一个包含完整帧数据的 vector<uint8_t>
    for (int i = 0; i < binArray.size(); ++i) {
        // 为当前数据包初始化发送结果标志和重试计数器
        bool ref;  // 当前帧发送结果
        retry = 0;  // 重置重试计数器

        // 获取当前要发送的数据帧
        auto frame = binArray[i];

        Logger::debug("4 文件第 " + std::to_string(i) + " 数据包：" + bytesToHex(frame));

        // 尝试发送当前帧，最多重试 fire_ware_frame_retry 次
        while (retry < fire_ware_frame_retry) {
            // 调用 sendFrame 方法发送数据帧并获取发送结果
            ref = sendFrame(frame);

            // 如果发送成功
            if (ref) {
                // 输出成功日志
                Logger::debug("4 发送第 " + std::to_string(i) + " 数据包成功！");

                // 如果是最后一个数据包且成功发送，则标记整个过程成功完成
                if (i == binArray.size() - 1) {
                    success = true;
                }
                break;  // 发送成功，跳出重试循环
            } else {
                // 发送失败，记录错误日志
                Logger::error("4 ❌ 发送第 " + std::to_string(i) + " 数据包失败！");

                // 增加重试次数
                ++retry;
            }

            // 在重试前等待 100 毫秒
            // 这个延迟可以让设备有时间处理和恢复
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // 如果当前帧在多次重试后依然发送失败，则退出整个升级过程
        if (!ref) {
            // 记录失败信息
            Logger::error("4 ❌ 第 " + std::to_string(i) + " 数据包发送失败，跳过该数据包");
            break;  // 退出循环，整个升级过程失败
        }
    }

    // 无论成功与否，输出固件升级完成的信息
    if (success)
        Logger::info("4 固件升级完成！");
    else
        Logger::error("4 固件升级失败！");

    // 通知接收线程停止运行
    stop_receive = true;

    // 等待接收线程完成（如果还在运行）
    if (receive_thread.joinable()) {
        receive_thread.join();  // 等待接收线程结束，避免资源泄露
    }

    // 成功时无需关闭，下面的挥手需要继续使用串口
    // 失败时关闭串口连接，释放资源
    if (!success)
        if (upgradeSerial->isOpen())
            upgradeSerial->close();  // 关闭串口连接

    // 返回升级结果
    // true: 所有数据包都成功发送
    // false: 至少有一个数据包发送失败
    return success;
}

bool FirmwareUpdate::sendFrame(const std::vector<uint8_t> &frame) {

    // 为每条命令生成一个唯一的消息 ID
    // 这个 ID 用于跟踪和匹配发送的命令与接收到的响应
    uint32_t message_id = generateMessageId();

    // 创建一个 unique_lock 用于等待接收线程通知
    // mutex_ 是保护共享数据的互斥锁
    // unique_lock 比 lock_guard 更灵活，支持条件变量的等待操作
    std::unique_lock<std::mutex> wait_lock(mutex_);

    // 为每个数据包创建一个条件变量
    // 条件变量用于线程间的同步，当接收线程收到对应响应时会通知发送线程
    auto condition_variable = std::make_unique<std::condition_variable>();
    // 将条件变量与消息 ID 关联并存储在 map 中
    message_conditions_[message_id] = std::move(condition_variable);

    // 清空串口输入缓冲区，确保接收到的是最新数据
    upgradeSerial->flushInput();

    // 发送帧数据到串口
    // frame.data() 返回指向底层数组的指针
    // frame.size() 返回数组的大小（字节数）
    size_t bytes_written = upgradeSerial->write(frame.data(), frame.size());

    // 检查是否所有数据都已成功写入
    // 如果写入的字节数与帧大小不一致，表示发送失败
    if (bytes_written != frame.size()) {
//        Logger::error("sendCommand: Failed to write full frame. Expected: "
//                      + std::to_string(frame.size()) + ", Written: " + std::to_string(bytes_written));
        Logger::error("4 ❌ 发送数据失败，预期写入 " + std::to_string(frame.size()) + " 字节，实际写入 " +
                      std::to_string(bytes_written) + " 字节");
        return false;
    }

    // 设置等待超时时间为当前时间加上 1000 毫秒（1 秒）
    // 如果在规定时间内没有收到响应，则认为操作失败
    auto timeout = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);

    // 等待接收线程通知，直到收到相应的数据包或超时
    // wait_until 函数会阻塞当前线程直到条件满足、超时或虚假唤醒
    if (message_conditions_[message_id]->wait_until(wait_lock, timeout, [this, message_id] {
        // 判断条件是否满足：检查是否已接收到对应 message_id 的数据
        // 当接收线程收到数据后，会在 received_data_ 中添加对应 message_id 的记录
        return received_data_.count(message_id) > 0;
    })) {
        // 条件满足，表示已接收到对应的响应数据
        auto data = received_data_[message_id];  // 获取接收到的数据

        // 清理资源：从 map 中移除已处理的响应数据
        received_data_.erase(message_id);

        // 清理资源：移除对应的条件变量
        message_conditions_.erase(message_id);

        return true;  // 发送成功并收到响应
    } else {
        // 等待超时，未收到响应
        return false;  // 发送失败或未收到响应
    }
}

bool FirmwareUpdate::wave() {
    // 初始化成功标志和重试计数器
    // success: 用于记录发送操作是否成功
    // retry: 用于跟踪当前重试次数
    bool success;
    int retry = 0;

    // 在达到最大重试次数之前，持续尝试发送结束标志
    // wave_sign_retry: 最大重试次数，通过构造函数或 upgrade 方法设置
    while (retry < wave_sign_retry) {
        // 清空串口输入缓冲区，确保接收到的是最新数据
        // 避免之前可能残留的数据干扰当前操作
        upgradeSerial->flushInput();

        // 准备要发送的数据：wave_sign (0x04)
        // wave_sign 是一个预定义的常量，表示固件升级完成的结束标志
        // 该标志用于通知设备固件传输已完成
        uint8_t data[] = {wave_sign};
        size_t size = sizeof(data);  // 计算数据大小，这里为 1 字节

        // 通过串口写入数据并获取实际写入的字节数
        // upgradeSerial->write() 返回实际成功写入的字节数
        size_t write_size = upgradeSerial->write(data, size);

        // 判断是否成功写入所有数据
        // 如果实际写入的字节数等于预期字节数，则表示写入成功
        success = write_size == size;

        if (success) {
            // 写入成功，记录日志并退出重试循环
            Logger::debug("5 ✅ 发送挥手信号成功！");
            break;
        } else {
            // 写入失败，记录错误日志
            Logger::error("5 ❌ 发送挥手信号失败！");

            // 增加重试计数器
            retry++;
            // 记录当前重试次数
            Logger::debug("5 重试第 " + std::to_string(retry) + " 次...");

            // 等待一小段时间再进行下一次重试
            // 这个延时可以让设备有时间处理前一次请求
            // 也可以避免过快地重试导致通信拥堵
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }

    // 如果达到最大重试次数后仍未成功，记录最终错误日志
    // 这是对整个重试过程的总结，帮助诊断问题
    if (!success) {
        Logger::error("5 ❌ 发送挥手信号失败，已重试 " + std::to_string(wave_sign_retry) + " 次！");
    }

    if (upgradeSerial->isOpen())
        upgradeSerial->close();  // 关闭串口连接

    // 返回最终的操作结果
    // true: 成功发送结束标志
    // false: 所有重试都失败
    return success;
}


void FirmwareUpdate::buildFrame(const std::vector<uint8_t> &data, int packetNumber, std::vector<uint8_t> &frame) {
    // 设置帧大小为 133 字节
    // 包含帧头 (3 字节)、数据 (128 字节)、CRC校验和 (2 字节)
    frame.resize(133);

    // 设置帧头第一个字节为 0x01，表示数据帧的类型标识
    frame[0] = 0x01;

    // 设置帧头第二个字节为包序号，用于标识当前是第几个数据包
    frame[1] = packetNumber;

    // 设置帧头第三个字节为包序号的反码 (255 - packetNumber)
    // 这是一种简单的错误检测机制，用于验证包序号的正确性
    frame[2] = 255 - packetNumber;

    // 计算原始数据的总大小
    size_t dataSize = data.size();

    // 计算当前数据包在原始数据中的起始偏移位置
    // 每个数据包携带 128 字节数据，所以偏移量为 (包序号-1) * 128
    size_t offset = (packetNumber - 1) * 128;

    // 计算当前包需要复制的数据大小
    // 如果是最后一个包，可能不足 128 字节，因此取 min 值
    size_t copySize = std::min(dataSize - offset, static_cast<size_t>(128));

    // 使用 memcpy 将数据从原始缓冲区复制到帧的数据区域（从索引 3 开始）
    // &frame[3] 指向帧中数据部分的起始位置
    // &data[offset] 指向原始数据中当前包对应的起始位置
    std::memcpy(&frame[3], &data[offset], copySize);

    // 计算 CRC 校验和，仅对数据部分进行 CRC 计算
    // 从 frame[3] 开始，长度为 copySize 的数据
    // 创建一个临时的 vector 用于 CRC 计算
    uint16_t crc = calculateCRC(std::vector<uint8_t>(frame.begin() + 3, frame.begin() + 3 + copySize));

    // 将 16 位 CRC 值分为高 8 位和低 8 位，存储在帧的最后两个字节
    // crc >> 8 获取高 8 位
    frame[131] = crc >> 8;
    // crc & 0xFF 获取低 8 位
    frame[132] = crc & 0xFF;
}

uint16_t FirmwareUpdate::calculateCRC(const std::vector<uint8_t> &data) {
    // 初始化 CRC 值为 0
    // CRC（循环冗余校验）用于检测数据传输或存储过程中的错误
    uint16_t crc = 0;

    // 遍历输入数据的每个字节
    for (uint8_t byte: data) {
        // 将当前字节左移 8 位后与 CRC 进行异或运算
        // 这将当前字节放在 CRC 的高 8 位位置上
        crc ^= (byte << 8);

        // 对每个字节的每一位进行处理（共 8 位）
        // 这是 CRC-16-CCITT 多项式除法的位级模拟
        for (int i = 0; i < 8; ++i) {
            // 检查 CRC 的最高位（第 15 位）是否为 1
            if (crc & 0x8000) {
                // 如果最高位为 1，将 CRC 左移 1 位后与多项式 0x1021 异或
                // 0x1021 是 CRC-16-CCITT 标准的多项式值 (x^16 + x^12 + x^5 + 1)
                crc = (crc << 1) ^ 0x1021;
            } else {
                // 如果最高位为 0，只需将 CRC 左移 1 位
                crc <<= 1;
            }
        }
    }

    // 返回计算得到的 16 位 CRC 校验值
    return crc;
}

void FirmwareUpdate::readFile(const std::string &fileName, std::vector<uint8_t> &buffer) {
    // 以二进制模式打开文件
    // fileName: 文件路径和名称
    // std::ios::binary: 以二进制模式打开，避免文本模式下可能的换行符转换
    std::ifstream file(fileName, std::ios::binary);

    // 检查文件是否成功打开
    // 如果文件不存在或无法访问，抛出运行时异常
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    // 将文件内容读入到缓冲区中
    // std::istreambuf_iterator<char>(file): 创建一个文件流的迭代器，指向文件的开始
    // std::istreambuf_iterator<char>(): 创建一个结束迭代器
    // assign() 函数将文件的所有内容复制到 buffer 中
    // 这种方法可以一次性高效地读取整个文件内容
    buffer.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // 函数结束时，file 对象会自动析构并关闭文件
}