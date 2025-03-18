from blinker import signal

error_signal = signal("error_signal")

search_signal = signal("search_signal")

eeprom_signal = signal("eeprom_signal")

ram_signal = signal("ram_signal")

servo_error_signal = signal("servo_error_signal")
