import asyncio
import logging

from typing import List, Dict, Tuple

import up_core as up
from up_core import ServoProtocol, ServoError
from up_core import LogLevel
from up_core import EEPROM
from up_core import RAM

from pup_core.model.EEPROMData import EEPROMData
from pup_core.model.RAMData import RAMData
from pup_core.serial.servo_parser import preview_data
from pup_core.serial.serial_manager import SerialManager
from pup_core.signals import ram_signal, eeprom_signal

task_running: Dict[Tuple[str, int], bool] = {}  # 使用 (serial_id, protocol_id) 作为键的任务状态字典
tasks: Dict[Tuple[str, int], List[asyncio.Task]] = {}  # 存储每个 (serial_id, protocol_id) 对应的多个任务


async def write_eeprom(serial_manager: SerialManager, serial_id: str, protocol_id: int, interval: int):
    protocol = ServoProtocol(protocol_id)

    while task_running.get((serial_id, protocol_id), False):
        cmd = protocol.eeprom.buildGetEepromData(
            EEPROM.MODEL_NUMBER_L,
            int(EEPROM.EEPROM_COUNT) - int(EEPROM.MODEL_NUMBER_L) - 1
        )
        try:
            byte_array = await serial_manager.write_wait(serial_id, cmd)
            if byte_array:
                error_code, payload = preview_data(byte_array)
                if error_code == ServoError.NO_ERROR:
                    result = up.parseEEPROMData(payload)
                    eeprom_data = EEPROMData(result)
                    await eeprom_signal.send_async("eeprom_data", serial_id=serial_id, eeprom_data=eeprom_data)
                else:
                    logging.warning(f"请求 EEPROM 数据错误: {error_code}")
            else:
                logging.warning("请求 EEPROM 数据错误")
        except Exception as e:
            logging.error("请求 EEPROM 数据失败 " + str(e))
        await asyncio.sleep(interval)


async def write_ram(serial_manager: SerialManager, serial_id: str, protocol_id: int, interval: int):
    protocol = ServoProtocol(protocol_id)

    while task_running.get((serial_id, protocol_id), False):
        cmd = protocol.ram.buildGetRamData(
            RAM.TORQUE_ENABLE,
            int(RAM.RAM_COUNT) - int(RAM.TORQUE_ENABLE) - 1
        )
        try:
            byte_array = await serial_manager.write_wait(serial_id, cmd)
            if byte_array:
                error_code, payload = preview_data(byte_array)
                if error_code == ServoError.NO_ERROR:
                    result = up.parseRAMData(payload)
                    ram_data = RAMData(result)
                    await ram_signal.send_async("ram_data", serial_id=serial_id, ram_data=ram_data)
                else:
                    logging.warning(f"请求 RAM 数据错误: {error_code}")
            else:
                logging.warning("请求 RAM 数据错误")
        except Exception as e:
            logging.error("请求 RAM 数据失败 " + str(e))
        await asyncio.sleep(interval)


async def start_load_info_task(serial_manager: SerialManager, serial_id: str, protocol_id: int, interval: int):
    """同时启动两个任务：写入EEPROM和写入RAM"""
    global task_running, tasks

    key = (serial_id, protocol_id)

    if not task_running.get(key, False):
        task_running[key] = True

        # 启动EEPROM写入任务
        eeprom_task = asyncio.create_task(write_eeprom(serial_manager, serial_id, protocol_id, interval))
        # 启动RAM写入任务
        ram_task = asyncio.create_task(write_ram(serial_manager, serial_id, protocol_id, interval))

        # 将任务添加到 tasks 字典中
        tasks[key] = [eeprom_task, ram_task]
    else:
        logging.warning(f"任务 {key} 已经在运行中")


async def stop_all_task():
    """停止所有任务，清除任务状态"""
    global task_running, tasks

    # 遍历所有正在运行的任务
    for key in list(task_running.keys()):
        if task_running[key]:
            task_running[key] = False
            # 获取并取消该键对应的所有任务
            for task in tasks.get(key, []):
                task.cancel()

    # 清空任务状态和任务列表
    task_running.clear()
    tasks.clear()


async def stop_task_for_serial(serial_id: str):
    """停止所有与特定 serial_id 相关的任务"""
    global task_running, tasks

    # 找到所有与该 serial_id 相关的任务
    keys_to_stop = [key for key in task_running if key[0] == serial_id]

    for key in keys_to_stop:
        # 停止任务
        if task_running.get(key, False):
            task_running[key] = False
            # 获取并取消所有任务
            for task in tasks.get(key, []):
                task.cancel()
            # 从字典中移除该键的任务
            tasks.pop(key, None)

    # 移除相关的任务状态
    for key in keys_to_stop:
        task_running.pop(key, None)


async def stop_task(serial_id: str, protocol_id: int):
    """停止特定 serial_id 和 protocol_id 对应的任务"""
    global task_running, tasks
    key = (serial_id, protocol_id)

    if task_running.get(key, False):
        task_running[key] = False
        # 获取并取消所有任务
        for task in tasks.get(key, []):
            task.cancel()
        # 移除该键的任务
        tasks.pop(key, None)
