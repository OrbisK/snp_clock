cmake_minimum_required(VERSION 3.28)
project(avr-cmake-template CXX) # Add CXX for C++ project

# Define the AVR microcontroller and clock frequency
set(MCU atmega48a)
set(F_CPU 1000000)

# Define the target source files (add all source files here)
set(SRC
        src/main.cpp
        src/Time.cpp
        # Add any additional source files here
)

# Set the C++ compiler and its options
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_CXX_FLAGS "-mmcu=${MCU} -DF_CPU=${F_CPU}UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wundef -std=gnu++17")
set(CMAKE_CXX_FLAGS_RELEASE "-Os")
set(CMAKE_CXX_FLAGS_DEBUG "-g")

# Include the src directory
include_directories(SYSTEM "/usr/lib/avr/include" src) # Add src to the include directories

# Define the executable target
add_executable(${PROJECT_NAME} ${SRC})

# Set the objcopy command to create .hex and .eep files from the ELF output
set(OBJCOPY avr-objcopy)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${OBJCOPY} -O ihex -R .eeprom -R .fuse -R .lock -R .signature ${PROJECT_NAME} ${PROJECT_NAME}.hex
        COMMAND ${OBJCOPY} -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.eep
        COMMENT "Creating hex and eep files")

# Add custom target for avrdude to flash the program
set(AVRDUDE avrdude)
set(AVRDUDE_PROGRAMMER usbasp)
set(AVRDUDE_PORT usb) # This should be 'usb' for USBasp programmer
# Baud rate may not be necessary for USBasp, and some versions of avrdude might not support the -b option for USBasp
set(AVRDUDE_FLAGS -p ${MCU} -P ${AVRDUDE_PORT} -c ${AVRDUDE_PROGRAMMER})

add_custom_target(flash
        COMMAND ${AVRDUDE} ${AVRDUDE_FLAGS} -U flash:w:${PROJECT_NAME}.hex:i
        DEPENDS ${PROJECT_NAME}
        COMMENT "Flashing the device")

# Optionally, you can add custom targets for other operations like reading/writing fuses, etc.