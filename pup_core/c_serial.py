import up_core as serial


def _open_serial(device, baudrate=9600, parity='none', databits=8, stopbits=1, flowcontrol='none', timeout=1.0):
    """
    打开串口，并根据提供的参数配置。
    :param device: 串口设备路径（如 '/dev/ttyUSB0' 或 'COM1'）
    :param baudrate: 波特率（默认 9600）
    :param parity: 奇偶校验（none, even, odd, mark, space）
    :param databits: 数据位长度（5, 6, 7, 8）
    :param stopbits: 停止位（1, 1.5, 2）
    :param flowcontrol: 流控制模式（none, rtscts, xonxoff）
    :param timeout: 读写操作的超时时间（单位：秒）
    :param raw_mode: 如果启用 raw 模式，处理串口通信时不处理输入输出格式
    :return: 打开的串口对象
    """
    # 设置字节长度映射
    bytesize_map = {
        5: serial.bytesize_t.fivebits,
        6: serial.bytesize_t.sixbits,
        7: serial.bytesize_t.sevenbits,
        8: serial.bytesize_t.eightbits
    }

    # 设置奇偶校验映射
    parity_map = {
        'none': serial.parity_t.parity_none,
        'odd': serial.parity_t.parity_odd,
        'even': serial.parity_t.parity_even,
        'mark': serial.parity_t.parity_mark,
        'space': serial.parity_t.parity_space
    }

    # 设置停止位映射
    stopbits_map = {
        1: serial.stopbits_t.stopbits_one,
        2: serial.stopbits_t.stopbits_two,
        3: serial.stopbits_t.stopbits_one_point_five
    }

    # 设置流控制映射
    flowcontrol_map = {
        'none': serial.flowcontrol_t.flowcontrol_none,
        'software': serial.flowcontrol_t.flowcontrol_software,
        'hardware': serial.flowcontrol_t.flowcontrol_hardware
    }

    # 检查输入的参数是否正确
    if databits not in bytesize_map:
        raise ValueError(f"Unsupported databits: {databits}")
    if parity not in parity_map:
        raise ValueError(f"Unsupported parity: {parity}")
    if stopbits not in stopbits_map:
        raise ValueError(f"Unsupported stopbits: {stopbits}")
    if flowcontrol not in flowcontrol_map:
        raise ValueError(f"Unsupported flowcontrol: {flowcontrol}")

    # 创建串口对象
    try:
        ser = serial.Serial(
            port=device,
            baudrate=baudrate,
            timeout=serial.Timeout.simpleTimeout(int(timeout * 1000)),  # 将超时从秒转换为毫秒
            bytesize=bytesize_map[databits],
            parity=parity_map[parity],
            stopbits=stopbits_map[stopbits],
            flowcontrol=flowcontrol_map[flowcontrol]
        )

        # 打开串口
        # ser.open()
        if not ser.isOpen():
            raise Exception(f"Failed to open serial port: {device}")

        return ser

    except Exception as e:
        raise Exception(f"Error opening serial port: {str(e)}")


def _close_serial(ser):
    """
    关闭串口。
    :param ser: 串口对象
    """
    if ser.isOpen():
        ser.close()


def _list_serial_ports():
    """
    列出所有可用的串口设备，并打印信息。
    """
    ports = serial.list_ports()

    if not ports:
        return {"message": "No available serial ports found."}

    port_list = []
    for port_info in ports:
        port_list.append({
            "port": port_info.port,
            "description": port_info.description,
            "hardware_id": port_info.hardware_id
        })

    return port_list


def _write(ser, data: bytes):
    """
    向串口发送数据。
    :param ser: 串口对象
    :param data: 要发送的数据
    :return:
    """
    if not ser.isOpen():
        raise Exception("Serial port is not open.")

    ser.flushInput()

    bytes_written = ser.write(data)

    if bytes_written != len(data):
        raise Exception("Failed to write all data to serial port.")

    return ser.waitReadable()


def _read(ser, available_bytes):
    """
    从串口读取数据。
    :param ser: 串口对象
    :param available_bytes: 可用字节数
    :return: 读取的数据（字节数组）
    """
    if available_bytes > 0:
        buffer, bytes_read = ser.read(available_bytes)
        return bytearray(buffer)
    else:
        raise Exception("No data available to read from serial port.")
