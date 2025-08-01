cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME MY_FREE_RTOS)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE

    Middlewares/HardwareDrive/src/drv_gpio_output.c
    Middlewares/HardwareDrive/src/drv_timer_countdown.c
    Middlewares/HardwareDrive/src/drv_gpio_input.c
    Middlewares/HardwareDrive/src/drv_uart_dma.c
    Middlewares/HardwareDrive/src/drv_timer_pwm.c
    Middlewares/HardwareDrive/src/drv_timer_encoder.c
    Middlewares/HardwareDrive/src/drv_can_int.c
    
    Middlewares/PrivateLibrary/src/lib_gpio_output.c
    Middlewares/PrivateLibrary/src/lib_soft_timer.c
    Middlewares/PrivateLibrary/src/lib_gpio_input.c
    Middlewares/PrivateLibrary/src/lib_uart_protocol.c
    Middlewares/PrivateLibrary/src/lib_timer_pwm.c
    Middlewares/PrivateLibrary/src/lib_uart_log.c
    Middlewares/PrivateLibrary/src/lib_uart_debug.c

    Middlewares/Process/src/protector.c
    Middlewares/Process/src/manual.c
    Middlewares/Process/src/com.c
    Middlewares/Process/src/motor.c
    Middlewares/Process/src/maintain.c

)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Middlewares/HardwareDrive/inc
    Middlewares/PrivateLibrary/inc
    Middlewares/Process/inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add compile options to suppress unused parameter warnings
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    -Wno-unused-parameter
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    # Add user defined libraries
)

# Generate .bin file
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/MY_FREE_RTOS.bin
    COMMENT "Generating MY_FREE_RTOS.bin"
)