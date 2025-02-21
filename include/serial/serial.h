/*!
 * Created by noodles on 25-2-19.
 * 串口通信
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <limits>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <serial/v8stdint.h>

#define THROW(exceptionClass, message) throw exceptionClass(__FILE__, \
__LINE__, (message) )

namespace serial {

    /*!
    * 枚举定义了串口可能的字节大小。
    */
    typedef enum {
        fivebits = 5,
        sixbits = 6,
        sevenbits = 7,
        eightbits = 8
    } bytesize_t;

    /*!
    * 枚举定义了串口可能的校验类型。
    */
    typedef enum {
        parity_none = 0,
        parity_odd = 1,
        parity_even = 2,
        parity_mark = 3,
        parity_space = 4
    } parity_t;

    /*!
    * 枚举定义了串口可能的停止位类型。
    */
    typedef enum {
        stopbits_one = 1,
        stopbits_two = 2,
        stopbits_one_point_five
    } stopbits_t;

    /*!
    * 枚举定义了串口可能的流控制类型。
    */
    typedef enum {
        flowcontrol_none = 0,
        flowcontrol_software,
        flowcontrol_hardware
    } flowcontrol_t;

    /*!
    * 设置串口超时的结构体，时间单位为毫秒。
    *
    * 为了禁用字节间超时，将其设置为 Timeout::max()。
    */
    struct Timeout {
#ifdef max
# undef max
#endif

        static uint32_t max() { return std::numeric_limits<uint32_t>::max(); }

        /*!
        * 生成 Timeout 结构体的便捷函数，使用单个绝对超时。
        *
        * \param timeout 一个长整型，定义在调用 read 或 write 后发生超时的毫秒数。
        *
        * \return 表示此简单超时的 Timeout 结构体。
        */
        static Timeout simpleTimeout(uint32_t timeout) {
            return Timeout(max(), timeout, 0, timeout, 0);
        }

        /*! Number of milliseconds between bytes received to timeout on. */
        uint32_t inter_byte_timeout;
        /*! A constant number of milliseconds to wait after calling read. */
        uint32_t read_timeout_constant;
        /*! A multiplier against the number of requested bytes to wait after
         *  calling read.
         */
        uint32_t read_timeout_multiplier;
        /*! A constant number of milliseconds to wait after calling write. */
        uint32_t write_timeout_constant;
        /*! A multiplier against the number of requested bytes to wait after
         *  calling write.
         */
        uint32_t write_timeout_multiplier;

        explicit Timeout(uint32_t inter_byte_timeout_ = 0,
                         uint32_t read_timeout_constant_ = 0,
                         uint32_t read_timeout_multiplier_ = 0,
                         uint32_t write_timeout_constant_ = 0,
                         uint32_t write_timeout_multiplier_ = 0)
                : inter_byte_timeout(inter_byte_timeout_),
                  read_timeout_constant(read_timeout_constant_),
                  read_timeout_multiplier(read_timeout_multiplier_),
                  write_timeout_constant(write_timeout_constant_),
                  write_timeout_multiplier(write_timeout_multiplier_) {}
    };

    /*!
    * 提供便携式串口接口的类。
    */
    class Serial {
    public:
        /*!
        * 创建一个 Serial 对象并在指定端口时打开端口，
        * 否则它将保持关闭状态，直到调用 serial::Serial::open。
        *
        * \param port 一个包含串口地址的 std::string，
        *        在 Windows 上类似于 'COM1'，在 Linux 上类似于 '/dev/ttyS0'。
        *
        * \param baudrate 一个表示波特率的无符号 32 位整数。
        *
        * \param timeout 一个 serial::Timeout 结构体，定义串口的超时条件。 \see serial::Timeout
        *
        * \param bytesize 串行数据传输中每个字节的大小，默认为 eightbits，
        * 可能的值有：fivebits, sixbits, sevenbits, eightbits。
        *
        * \param parity 校验方法，默认为 parity_none，可能的值有：parity_none, parity_odd, parity_even。
        *
        * \param stopbits 使用的停止位数，默认为 stopbits_one，
        * 可能的值有：stopbits_one, stopbits_one_point_five, stopbits_two。
        *
        * \param flowcontrol 使用的流控制类型，默认为 flowcontrol_none，
        * 可能的值有：flowcontrol_none, flowcontrol_software, flowcontrol_hardware。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::IOException
        * \throw std::invalid_argument
        */
        Serial(const std::string &port = "",
               uint32_t baudrate = 9600,
               Timeout timeout = Timeout(),
               bytesize_t bytesize = eightbits,
               parity_t parity = parity_none,
               stopbits_t stopbits = stopbits_one,
               flowcontrol_t flowcontrol = flowcontrol_none);

        /*! Destructor */
        virtual ~Serial();

        /*!
        * 打开串口，只要端口已设置且端口未打开。
        *
        * 如果在构造函数中提供了端口，则不需要显式调用 open。
        *
        * \see Serial::Serial
        *
        * \throw std::invalid_argument
        * \throw serial::SerialException
        * \throw serial::IOException
        */
        void
        open();

        /*!
        * 获取串口的打开状态。
        *
        * \return 如果端口是打开的，返回 true，否则返回 false。
        */
        bool
        isOpen() const;

        /*!
        * 关闭串口。
        */
        void
        close();

        /*!
        * 返回缓冲区中的字符数。
        */
        size_t
        available();

        /*!
        * 阻塞直到有串行数据可读或 read_timeout_constant 毫秒数已过。
        * 当函数退出时，如果端口处于可读状态，则返回 true，否则返回 false
        * （由于超时或选择中断）。
        */
        bool
        waitReadable();

        /*!
        * 阻塞一段时间，时间长度与当前串口设置下传输 count 个字符的时间相对应。
        * 这可以与 waitReadable 一起使用，以从端口读取更大的数据块。
        */
        void
        waitByteTimes(size_t count);

        /*!
        * 从串口读取指定数量的字节到给定的缓冲区中。
        *
        * 读取函数将在以下三种情况下返回：
        *  * 读取到请求的字节数。
        *    * 在这种情况下，请求的字节数将与 read 返回的 size_t 相匹配。
        *  * 发生超时，在这种情况下，读取的字节数将不匹配请求的数量，但不会抛出异常。可能发生的两种超时情况：
        *    * 字节间超时到期，这意味着从串口接收字节之间经过的毫秒数超过了字节间超时。
        *    * 总超时到期，这是通过将读取超时乘数乘以请求的字节数，然后加上读取超时常量来计算的。如果在初始调用读取后经过的总毫秒数超过了这个总数，将发生超时。
        *  * 发生异常，在这种情况下将抛出实际的异常。
        *
        * \param buffer 至少具有请求大小的 uint8_t 数组。
        * \param size 定义要读取的字节数的 size_t。
        *
        * \return 表示读取结果的字节数的 size_t。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        size_t
        read(uint8_t *buffer, size_t size);

        /*!
        * 从串口读取指定数量的字节到给定的缓冲区中。
        *
        * \param buffer 一个对 uint8_t 类型的 std::vector 的引用。
        * \param size 一个定义要读取的字节数的 size_t。
        *
        * \return 一个表示读取结果的字节数的 size_t。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        size_t
        read(std::vector<uint8_t> &buffer, size_t size = 1);

        /*!
        * 从串口读取指定数量的字节到给定的缓冲区中。
        *
        * \param buffer 一个对 std::string 的引用。
        * \param size 一个定义要读取的字节数的 size_t。
        *
        * \return 一个表示读取结果的字节数的 size_t。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        size_t
        read(std::string &buffer, size_t size = 1);

        /*!
        * 从串口读取指定数量的字节并返回一个包含数据的字符串。
        *
        * \param size 一个定义要读取的字节数的 size_t。
        *
        * \return 一个包含从端口读取的数据的 std::string。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        std::string
        read(size_t size = 1);

        /*!
        * 从串口读取一行或直到处理完给定的分隔符。
        *
        * 从串口读取，直到读取到一行。
        *
        * \param buffer 用于存储数据的 std::string 引用。
        * \param size 行的最大长度，默认为 65536 (2^16)。
        * \param eol 用于匹配行尾的字符串。
        *
        * \return 表示读取字节数的 size_t。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        size_t
        readline(std::string &buffer, size_t size = 65536, std::string eol = "\n");

        /*!
        * 从串口读取一行或直到处理完给定的分隔符。
        *
        * 从串口读取，直到读取到一行。
        *
        * \param size 行的最大长度，默认为 65536 (2^16)。
        * \param eol 用于匹配行尾的字符串。
        *
        * \return 一个包含行的 std::string。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        std::string
        readline(size_t size = 65536, std::string eol = "\n");

        /*!
        * 读取多行数据，直到串口超时。
        *
        * 这需要一个大于 0 的超时才能运行。它将读取直到发生超时并返回一个字符串列表。
        *
        * \param size 组合行的最大长度，默认为 65536 (2^16)。
        *
        * \param eol 用于匹配行尾的字符串。
        *
        * \return 包含行的 std::vector<std::string>。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        */
        std::vector<std::string>
        readlines(size_t size = 65536, std::string eol = "\n");

        /*!
        * 将字符串写入串口。
        *
        * \param data 一个包含要写入串口的数据的常量引用。
        *
        * \param size 一个 size_t，指示应从给定数据缓冲区写入的字节数。
        *
        * \return 一个 size_t，表示实际写入串口的字节数。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        * \throw serial::IOException
        */
        size_t
        write(const uint8_t *data, size_t size);

        /*!
        * 将字符串写入串口。
        *
        * \param data 一个包含要写入串口的数据的常量引用。
        *
        * \return 一个 size_t，表示实际写入串口的字节数。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        * \throw serial::IOException
        */
        size_t
        write(const std::vector<uint8_t> &data);

        /*!
        * 将字符串写入串口。
        *
        * \param data 一个包含要写入串口的数据的常量引用。
        *
        * \return 一个 size_t，表示实际写入串口的字节数。
        *
        * \throw serial::PortNotOpenedException
        * \throw serial::SerialException
        * \throw serial::IOException
        */
        size_t
        write(const std::string &data);

        /*!
        * 设置串口标识符。
        *
        * \param port 一个包含串口地址的常量 std::string 引用，
        * 例如在 Windows 上为 'COM1'，在 Linux 上为 '/dev/ttyS0'。
        *
        * \throw std::invalid_argument
        */
        void
        setPort(const std::string &port);

        /*!
        * 获取串口标识符。
        *
        * \see Serial::setPort
        *
        * \throw std::invalid_argument
        */
        std::string
        getPort() const;

        /*!
        * 设置读写操作的超时，使用 Timeout 结构体。
        *
        * 这里描述了两种超时条件：
        *  * 字节间超时：
        *    * inter_byte_timeout 是 serial::Timeout 结构体的一个成员，定义了在串口接收字节之间的最大时间（以毫秒为单位），超过这个时间将发生超时。将其设置为 0 可以防止字节间超时的发生。
        *  * 总时间超时：
        *    * 这个超时条件的常量和乘数部分分别定义在 serial::Timeout 结构体中，适用于读和写操作。如果从调用读或写操作开始经过的总时间（以毫秒为单位）超过了指定的时间，将发生超时。
        *    * 这个限制是通过将乘数部分乘以请求的字节数，然后将该乘积加到常量部分来定义的。例如，如果你希望读操作在一秒钟后超时，无论请求的字节数是多少，那么可以将 read_timeout_constant 设置为 1000，将 read_timeout_multiplier 设置为 0。这个超时条件可以与字节间超时条件一起使用，超时将发生在两个条件之一满足时。这允许用户在响应速度和效率之间进行最大程度的控制。
        *
        * 读写函数将在以下三种情况下返回：读写完成、发生超时或发生异常。
        *
        * 超时设置为 0 将启用非阻塞模式。
        *
        * \param timeout 一个 serial::Timeout 结构体，包含字节间超时，以及读写超时的常量和乘数。
        *
        * \see serial::Timeout
        */
        void
        setTimeout(Timeout &timeout);

        /*!
        * 设置读写操作的超时。
        */
        void
        setTimeout(uint32_t inter_byte_timeout, uint32_t read_timeout_constant,
                   uint32_t read_timeout_multiplier, uint32_t write_timeout_constant,
                   uint32_t write_timeout_multiplier) {
            Timeout timeout(inter_byte_timeout, read_timeout_constant,
                            read_timeout_multiplier, write_timeout_constant,
                            write_timeout_multiplier);
            return setTimeout(timeout);
        }

        /*!
        * 获取读操作的超时时间（以秒为单位）。
        *
        * \return 一个 Timeout 结构体，包含 inter_byte_timeout，以及读写超时的常量和乘数。
        *
        * \see Serial::setTimeout
        */
        Timeout
        getTimeout() const;

        /*!
        * 设置串口的波特率。
        *
        * 可能的波特率取决于系统，但一些安全的波特率包括：
        * 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 28800, 38400, 56000,
        * 57600, 115200
        * 一些其他波特率在某些串口上也受支持：
        * 128000, 153600, 230400, 256000, 460800, 500000, 921600
        *
        * \param baudrate 一个整数，设置串口的波特率。
        *
        * \throw std::invalid_argument
        */
        void
        setBaudrate(uint32_t baudrate);

        /*!
        * 获取串口的波特率。
        *
        * \return 一个整数，表示串口的波特率。
        *
        * \see Serial::buildSetBaudrate
        *
        * \throw std::invalid_argument
        */
        uint32_t
        getBaudrate() const;

        /*!
        * 设置串口每个字节的大小。
        *
        * \param bytesize 串行数据传输中每个字节的大小，
        * 默认为 eightbits，可能的值有：fivebits, sixbits, sevenbits, eightbits。
        *
        * \throw std::invalid_argument
        */
        void
        setBytesize(bytesize_t bytesize);

        /*!
        * 获取串口每个字节的大小。
        *
        * \see Serial::setBytesize
        *
        * \throw std::invalid_argument
        */
        bytesize_t
        getBytesize() const;

        /*!
        * 设置串口的校验方式。
        *
        * \param parity 校验方法，默认为 parity_none，可能的值有：parity_none, parity_odd, parity_even。
        *
        * \throw std::invalid_argument
        */
        void
        setParity(parity_t parity);

        /*!
        * 获取串口的校验方式。
        *
        * \see Serial::setParity
        *
        * \throw std::invalid_argument
        */
        parity_t
        getParity() const;

        /*!
        * 设置串口的停止位。
        *
        * \param stopbits 使用的停止位数，默认为 stopbits_one，
        * 可能的值有：stopbits_one, stopbits_one_point_five, stopbits_two。
        *
        * \throw std::invalid_argument
        */
        void
        setStopbits(stopbits_t stopbits);

        /*!
        * 获取串口的停止位。
        *
        * \see Serial::setStopbits
        *
        * \throw std::invalid_argument
        */
        stopbits_t
        getStopbits() const;

        /*!
        * 设置串口的流控制。
        *
        * \param flowcontrol 使用的流控制类型，默认为 flowcontrol_none，
        * 可能的值有：flowcontrol_none, flowcontrol_software, flowcontrol_hardware。
        *
        * \throw std::invalid_argument
        */
        void
        setFlowcontrol(flowcontrol_t flowcontrol);

        /*!
        * 获取串口的流控制。
        *
        * \see Serial::setFlowcontrol
        *
        * \throw std::invalid_argument
        */
        flowcontrol_t
        getFlowcontrol() const;

        /*!
        * 清空输入和输出缓冲区。
        */
        void
        flush();

        /*!
        * 仅清空输入缓冲区。
        */
        void
        flushInput();

        /*!
        * 仅清空输出缓冲区。
        */
        void
        flushOutput();

        /*!
        * 发送 RS-232 断开信号。参见 tcsendbreak(3)。
        */
        void
        sendBreak(int duration);

        /*!
        * 设置断开条件为给定的级别。默认为 true。
        */
        void
        setBreak(bool level = true);

        /*!
        * 设置 RTS 握手线为给定的级别。默认为 true。
        */
        void
        setRTS(bool level = true);

        /*!
        * 设置 DTR 握手线为给定的级别。默认为 true。
        */
        void
        setDTR(bool level = true);

        /*!
        * 阻塞直到 CTS、DSR、RI、CD 发生变化或有其他中断。
        *
        * 如果在等待过程中发生错误，会抛出异常。
        * 一旦返回，可以检查 CTS、DSR、RI 和 CD 的状态。
        * 如果可用，使用 ioctl 的 TIOCMIWAIT（主要在 Linux 上）进行操作，
        * 分辨率小于 ±1ms，最优可达 ±0.2ms。否则使用轮询方法，分辨率为 ±2ms。
        *
        * \return 如果其中一条线路发生变化，返回 true，否则返回 false。
        *
        * \throw SerialException
        */
        bool
        waitForChange();

        /*!
        * 返回 CTS 线路的当前状态。
        */
        bool
        getCTS();

        /*!
        * 返回 DSR 线路的当前状态。
        */
        bool
        getDSR();

        /*!
        * 返回 RI 线路的当前状态。
        */
        bool
        getRI();

        /*!
        * 返回 CD 线路的当前状态。
        */
        bool
        getCD();

        // Disable copy constructors
        Serial(const Serial &);

    private:

        Serial &operator=(const Serial &);

        // Pimpl idiom, d_pointer
        class SerialImpl;

        SerialImpl *pimpl_;

        // Scoped Lock Classes
        class ScopedReadLock;

        class ScopedWriteLock;

        // Read common function
        size_t
        read_(uint8_t *buffer, size_t size);

        // Write common function
        size_t
        write_(const uint8_t *data, size_t length);

    };

    class SerialException : public std::exception {
        // Disable copy constructors
        SerialException &operator=(const SerialException &);

        std::string e_what_;
    public:
        SerialException(const char *description) {
            std::stringstream ss;
            ss << "SerialException " << description << " failed.";
            e_what_ = ss.str();
        }

        SerialException(const SerialException &other) : e_what_(other.e_what_) {}

        virtual ~SerialException() throw() {}

        virtual const char *what() const throw() {
            return e_what_.c_str();
        }
    };

    class IOException : public std::exception {
        // Disable copy constructors
        IOException &operator=(const IOException &);

        std::string file_;
        int line_;
        std::string e_what_;
        int errno_;
    public:
        explicit IOException(std::string file, int line, int errnum)
                : file_(file), line_(line), errno_(errnum) {
            std::stringstream ss;
#if defined(_WIN32) && !defined(__MINGW32__)
            char error_str [1024];
            strerror_s(error_str, 1024, errnum);
#else
            char *error_str = strerror(errnum);
#endif
            ss << "IO Exception (" << errno_ << "): " << error_str;
            ss << ", file " << file_ << ", line " << line_ << ".";
            e_what_ = ss.str();
        }

        explicit IOException(std::string file, int line, const char *description)
                : file_(file), line_(line), errno_(0) {
            std::stringstream ss;
            ss << "IO Exception: " << description;
            ss << ", file " << file_ << ", line " << line_ << ".";
            e_what_ = ss.str();
        }

        virtual ~IOException() throw() {}

        IOException(const IOException &other) : line_(other.line_), e_what_(other.e_what_), errno_(other.errno_) {}

        int getErrorNumber() const { return errno_; }

        virtual const char *what() const throw() {
            return e_what_.c_str();
        }
    };

    class PortNotOpenedException : public std::exception {
        // Disable copy constructors
        const PortNotOpenedException &operator=(PortNotOpenedException);

        std::string e_what_;
    public:
        PortNotOpenedException(const char *description) {
            std::stringstream ss;
            ss << "PortNotOpenedException " << description << " failed.";
            e_what_ = ss.str();
        }

        PortNotOpenedException(const PortNotOpenedException &other) : e_what_(other.e_what_) {}

        virtual ~PortNotOpenedException() throw() {}

        virtual const char *what() const throw() {
            return e_what_.c_str();
        }
    };

/*!
 * Structure that describes a serial device.
 */
    struct PortInfo {

        /*! Address of the serial port (this can be passed to the constructor of Serial). */
        std::string port;

        /*! Human readable description of serial device if available. */
        std::string description;

        /*! Hardware ID (e.g. VID:PID of USB serial devices) or "n/a" if not available. */
        std::string hardware_id;

    };

    /*!
    * 列出系统上可用的串口
    *
    * 返回一个包含可用串口的向量，每个串口由 serial::PortInfo 数据结构表示：
    *
    * \return serial::PortInfo 的向量。
    */
    std::vector<PortInfo>
    list_ports();

} // namespace serial

#endif
