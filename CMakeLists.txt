cmake_minimum_required(VERSION 3.16)

project(BC_Wallet_Qt VERSION 0.1 LANGUAGES CXX)
set (Project_Executable appBC_Wallet_Qt)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Network Sql Qml Gui Widgets)
find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
message( "OpenSSL found: Version ${OPENSSL_VERSION}")
else()
message("openSSL not found!")
endif()
qt_policy(SET QTP0001 OLD)
#qt_standard_project_setup(REQUIRES 6.5)
set (Project_Headers
    src/Controller.h
    src/DataBaseMain.h
    src/Block.h
    src/Blockchain.h
    src/Transaction.h
    src/Wallet.h

)
set (Project_Sources
    main.cpp
    src/Controller.cpp
    src/DataBaseMain.cpp
    src/Block.cpp
    src/Blockchain.cpp
    src/Transaction.cpp
    src/Wallet.cpp
)
qt_add_executable(${Project_Executable}
    ${Project_Headers}
    ${Project_Sources}
)


target_link_libraries(appBC_Wallet_Qt PRIVATE Qt6::Core Qt6::Sql Qt6::Gui Qt6::Widgets ${OPENSSL_LIBRARIES})
qt_add_qml_module(${Project_Executable}
    URI ${PROJECT_NAME}
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES
        SOURCES
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${Project_Executable} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appBC_Wallet_Qt
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


include(GNUInstallDirs)
install(TARGETS ${Project_Executable}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
