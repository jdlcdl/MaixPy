# Generated by Kconfiglib (https://github.com/ulfalizer/Kconfiglib)

#
# Toolchain configuration
#
CONFIG_TOOLCHAIN_PATH="/opt/kendryte-toolchain/bin"
CONFIG_TOOLCHAIN_PREFIX="riscv64-unknown-elf-"
# end of Toolchain configuration

#
# Components configuration
#

#
# Board config
#
CONFIG_BOARD_MAIX=y
# CONFIG_BOARD_M5STICK is not set
# CONFIG_BOARD_TWATCH is not set
CONFIG_LCD_DEFAULT_WIDTH=320
CONFIG_LCD_DEFAULT_HEIGHT=240
CONFIG_LCD_DEFAULT_FREQ=15000000
CONFIG_SENSOR_FREQ=24000000
CONFIG_CPU_DEFAULT_FREQ=390000000
# end of Board config

CONFIG_COMPONENT_DRIVERS_ENABLE=y

#
# Drivers configuration
#
CONFIG_WS2812_ENABLE=n
CONFIG_HTPA_ENABLE=n
CONFIG_AMG88XX_ENABLE=n

#
# SD card
#
CONFIG_SPI_SD_CARD_FORCE_HIGH_SPEED=y
# end of SD card

CONFIG_COMPONENT_KENDRYTE_SDK_ENABLE=y

#
# Kendryte SDK configurations
#
CONFIG_SDK_LOG_LEVEL=5
# CONFIG_FREERTOS_ENABLE is not set
# end of Kendryte SDK configurations

CONFIG_COMPONENT_MICROPYTHON_ENABLE=y

#
# Micropython configurations
#
CONFIG_MAIXPY_GC_HEAP_SIZE=0xF0000

#
# Modules configurations
#
CONFIG_MAIXPY_IDE_SUPPORT=n
CONFIG_MAIXPY_NES_ENABLE=n
CONFIG_MAIXPY_AUDIO_ENABLE=n
CONFIG_MAIXPY_VIDEO_ENABLE=n
CONFIG_MAIXPY_SPEECH_ENABLE=n
CONFIG_MAIXPY_THREAD_ENABLE=y
CONFIG_MAIXPY_TOUCH_SCREEN_ENABLE=n
CONFIG_MAIXPY_OMV_MINIMUM=n
CONFIG_MAIXPY_OMV_CONV_YUV_FAST=n
CONFIG_MAIXPY_OMV_DOUBLE_BUFF=y
CONFIG_MAIXPY_OMV_BINARY_OPS=y
CONFIG_MAIXPY_OMV_MATH_OPS=y
CONFIG_MAIXPY_OMV_MEDIAN=y
CONFIG_MAIXPY_OMV_QRCODES=y
CONFIG_MAIXPY_WS2812_ENABLE=n
CONFIG_MAIXPY_HTPA_ENABLE=n
CONFIG_MAIXPY_AMG88XX_ENABLE=n
CONFIG_MAIXPY_ULTRASONIC_ENABLE=n
CONFIG_MAIXPY_ONEWIRE_ENABLE=n
CONFIG_MAIXPY_NETWORK_ENABLE=n
CONFIG_MICROPYTHON_PY_URE_ENABLE=n
CONFIG_MICROPYTHON_ULAB_ENALBE=n
CONFIG_MICROPYTHON_KMODEL_V4_SUPPORT=n
CONFIG_MAIXPY_SPEECH_RECOGNIZER_ENABLE=n
CONFIG_MICROPY_SSL_ENABLE=n
CONFIG_MAIXPY_WIZNET5K_ENABLE=n
CONFIG_MAIXPY_MIC_ARRAY_ENABLE=n
CONFIG_MAIXPY_LVGL_ENABLE=n
CONFIG_MAIXPY_LODEPNG_ENABLE=n
CONFIG_MAIXPY_FLASH_ENABLE=y
CONFIG_MAIXPY_HASHLIB_ENABLE=n
CONFIG_MAIXPY_BTC_HASHLIB_ENABLE=y
CONFIG_MAIXPY_QRCODE_ENABLE=y
CONFIG_MAIXPY_SECP256K1_ENABLE=y
CONFIG_MAIXPY_USE_MEMZIP=y
# end of Modules configurations

#
# Debug configurations
#
# CONFIG_MAIXPY_K210_UARTHS_DEBUG is not set
# end of Debug configurations

CONFIG_BUILTIN_PY_DIR="${PROJECT_PATH}/builtin_py"
CONFIG_MAIXPY_BUILTIN_PY_PYE=n
CONFIG_MAIXPY_BUILTIN_PY_BOARD=n
CONFIG_MAIXPY_BUILTIN_PY_FPIOA_MANAGER=n
# end of Micropython configurations

#
# Others
#
# end of Others

CONFIG_COMPONENT_SPIFFS_ENABLE=y

#
# SPIFFS Configuration
#

#
# SPIFFS Cache Configuration
#
CONFIG_SPIFFS_CACHE=y
CONFIG_SPIFFS_CACHE_WR=y
# CONFIG_SPIFFS_CACHE_STATS is not set
# end of SPIFFS Cache Configuration

CONFIG_SPIFFS_SIZE=0x300000
CONFIG_SPIFFS_START_ADDR=0xD00000
CONFIG_SPIFFS_EREASE_SIZE=0x1000
CONFIG_SPIFFS_LOGICAL_BLOCK_SIZE=0x20000
CONFIG_SPIFFS_LOGICAL_PAGE_SIZE=0x1000
CONFIG_SPIFFS_OBJ_NAME_LEN=128
CONFIG_SPIFFS_USE_MAGIC=y
CONFIG_SPIFFS_USE_MAGIC_LENGTH=y
CONFIG_SPIFFS_META_LENGTH=0

#
# Debug Configuration
#
# CONFIG_SPIFFS_DBG is not set
# CONFIG_SPIFFS_API_DBG is not set
# CONFIG_SPIFFS_GC_DBG is not set
# CONFIG_SPIFFS_CACHE_DBG is not set
# CONFIG_SPIFFS_CHECK_DBG is not set
# end of Debug Configuration
# end of SPIFFS Configuration

CONFIG_COMPONENT_UTILS_ENABLE=y
CONFIG_COMPONENT_LIB_SIPEED_KPU_SRC_ENABLE=n
# end of Components configuration

#
# Others
#
# CONFIG_LIB_NNCASE_SOURCE_CODE_ENABLE is not set
# end of Others
