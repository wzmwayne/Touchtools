set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

add_executable(WPPTouchHelper
    main.cpp
    Config.cpp
    FloatingControl.cpp
    FloatingToolbar.cpp
    KeySimulator.cpp
    USBAssistant.cpp
)

target_link_libraries(WPPTouchHelper
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network
)

set_target_properties(WPPTouchHelper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)
