from pup_core.servo_parser import ServoError, perform_serial_data


def test_perform_serial_data_valid_packet():
    # 模拟一个有效的数据包
    packet = bytearray([0xFF, 0xFF, 0x01, 0x03, 0x00, 0x0A, 0xF1])  # 最后一个字节是校验和
    error_code, payload = perform_serial_data(packet)

    # 校验返回的错误码是 NO_ERROR，表示没有错误
    assert error_code == ServoError.NO_ERROR
    # 校验返回的 payload 数据是有效的
    assert payload == bytearray([0x0A])  # Ensure payload is a bytearray


def test_perform_serial_data_invalid_checksum():
    # 模拟一个校验和错误的数据包
    packet = bytearray([0xFF, 0xFF, 0x01, 0x03, 0x00, 0x0A, 0xF2])  # 校验和错误
    error_code, payload = perform_serial_data(packet)

    # 校验返回的错误码是 CHECKSUM_ERROR，表示校验和错误
    assert error_code == ServoError.CHECKSUM_ERROR
    assert payload is None


def test_perform_serial_data_invalid_length():
    # 模拟一个长度不足的数据包
    packet = bytearray([0xFF, 0xFF, 0x01])  # 长度小于 6
    error_code, payload = perform_serial_data(packet)

    # 校验返回的错误码是 INVALID_PACKET_LENGTH，表示数据包长度错误
    assert error_code == ServoError.INVALID_PACKET_LENGTH
    assert payload is None
