from pup_core.serial.load_info_task import start_load_info_task, stop_task, stop_all_task, stop_task_for_serial
from pup_core.serial.serial_manager import SerialManager


async def start_info_task(serial_manager: SerialManager, serial_id: str, protocol_id: int, interval: int):
    await start_load_info_task(serial_manager, serial_id, protocol_id, interval)


async def stop_all_info_tasks():
    await stop_all_task()


async def stop_info_task_for_serial_id(serial_id: str):
    await stop_task_for_serial(serial_id)


async def stop_info_task(serial_id: str, protocol_id: int):
    await stop_task(serial_id, protocol_id)
