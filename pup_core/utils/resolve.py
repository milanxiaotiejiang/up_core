def identify_mode(byte_buffer: bytes):
    """
    根据返回的字节流来区分是舵机模式还是电机模式。
    """
    if len(byte_buffer) != 4:
        raise ValueError("byte_buffer 数据长度不足，无法识别模式。")

    # 检查电机模式特征
    if byte_buffer == bytes([0x00, 0x00, 0x00, 0x00]):
        return False

    # 检查舵机模式特征
    elif byte_buffer == bytes([0x00, 0x00, 0xFF, 0x03]):
        return True

    raise ValueError("无法识别的模式")
