# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define a placa como pico_w para ativar o suporte a Wi-Fi
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Importa o SDK do Pico
include(pico_sdk_import.cmake)

# Nome do projeto integrado
project(main C CXX ASM)

# Inicializa o SDK
pico_sdk_init()

# Adiciona os arquivos do seu projeto
add_executable(main
    main.c
    src/hcsr04.c
    inc/ssd1306_i2c.c
)

# Configurações do programa
pico_set_program_version(main "1.0")

# Configura I/O: ativa UART e USB
pico_enable_stdio_uart(main 0) # UART0 (GPIO0/1) is reserved for Bluetooth.
pico_enable_stdio_usb(main 1)

# Bibliotecas necessárias para o projeto integrado
target_link_libraries(main
    pico_stdlib
    hardware_adc
    hardware_i2c
    hardware_uart
    hardware_pwm
    hardware_pio
    pico_multicore
)

# Diretórios onde estão seus headers personalizados
target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/inc
)

# Opções de compilação
target_compile_options(main PRIVATE -Wall)

# Geração dos arquivos binários UF2, bin, etc.
pico_add_extra_outputs(main)
