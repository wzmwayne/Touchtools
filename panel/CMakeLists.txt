set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

add_executable(TouchtoolsPanel
    PanelMain.cpp
    ConfigPanel.cpp
    WppConfigPage.cpp
    UsbConfigPage.cpp
    TtIpc.cpp
)

target_link_libraries(TouchtoolsPanel
    Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network
)
set_target_properties(TouchtoolsPanel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    WIN32_EXECUTABLE TRUE
)
