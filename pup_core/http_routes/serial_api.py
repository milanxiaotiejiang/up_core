import up_core as serial
from fastapi import APIRouter

router = APIRouter()


def list_serial_ports():
    """列出所有可用的串口设备，并打印信息。"""
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


# 示例路由
@router.get("/list_serial_ports")
def read_root():
    return list_serial_ports()
