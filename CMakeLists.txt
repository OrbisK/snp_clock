cmake_minimum_required(VERSION  3.28)
project(avr-cmake-template)
# Define the project name and the AVR microcontroller
set(MCU atmega48a)
set(F_CPU 1000000)

# Define the target source file
set(TARGET main)
set(SRC ${TARGET}.c)

# Set the compiler and its options
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_C_FLAGS "-mmcu=${MCU} -DF_CPU=${F_CPU}UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wstrict-prototypes -Wundef -std=gnu99")
set(CMAKE_C_FLAGS_RELEASE "-Os")
set(CMAKE_C_FLAGS_DEBUG "-g")

include_directories(SYSTEM "/usr/lib/avr/include")

# Define the executable target
add_executable(${TARGET} ${SRC})

# Set the objcopy command to create .hex and .eep files from the ELF output
set(OBJCOPY avr-objcopy)
add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature ${TARGET} ${TARGET}.hex
        COMMAND ${OBJCOPY} -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex ${TARGET} ${TARGET}.eep
        COMMENT "Creating hex and eep files")

# Add custom target for avrdude to flash the program
set(AVRDUDE avrdude)
set(AVRDUDE_PROGRAMMER USBasp)
set(AVRDUDE_PORT m328p)
set(AVRDUDE_BAUD 19200)
set(AVRDUDE_FLAGS -p ${MCU} -P ${AVRDUDE_PORT} -b ${AVRDUDE_BAUD} -c ${AVRDUDE_PROGRAMMER})

add_custom_target(flash
        COMMAND ${AVRDUDE} ${AVRDUDE_FLAGS} -U flash:w:${TARGET}.hex
        DEPENDS ${TARGET}
        COMMENT "Flashing the device")

# Optionally, you can add custom targets for other operations like reading/writing fuses, etc.