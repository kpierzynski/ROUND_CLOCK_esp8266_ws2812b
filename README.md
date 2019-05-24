# ROUND_CLOCK_esp8266_ws2812b

export PATH=~/Desktop/esp-open-sdk/xtensa-lx106-elf/bin/:$PATH

make flash -j4 -C ROUND_CLOCK_esp8266_ws2812b/ ESPPORT=/dev/ttyUSB0 && minicom
