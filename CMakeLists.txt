cmake_minimum_required(VERSION 3.5)

project(Totale VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

# libarchive
find_library(LIB_ARCHIVE archive PATHS "/third-party/build/libarchive/lib")
find_path(LIB_ARCHIVE_INCLUDE archive.h PATHS "/third-party/build/libarchive/include")
include_directories(${LIB_ARCHIVE_INCLUDE})
add_library(libarchive_obj OBJECT IMPORTED)
set_target_properties(libarchive_obj PROPERTIES IMPORTED_LOCATION ${LIB_ARCHIVE})

# protobuf
#find_package(Protobuf CONFIG REQUIRED)
find_path(PROTOBUF_INCLUDE_DIRS google/protobuf/arena.h PATHS "/third-party/build/protobuf/include")
include_directories(${PROTOBUF_INCLUDE_DIRS})
find_library(PROTOBUF_LIB protobuf PATHS "/third-party/build/protobuf/lib")
add_library(protobuf_obj OBJECT IMPORTED)

#XZ liblzma
find_library(LIB_XZ lzma PATHS "/third-party/build/xz/bin")
find_path(LIB_XZ_INCLUDE lzma.h PATHS "/third-party/build/xz/include")
include_directories(${LIB_XZ_INCLUDE})
find_package(liblzma PATHS "/third-party/build/xz/" REQUIRED CONFIG)
add_library(liblzma_obj OBJECT IMPORTED)
set_target_properties(liblzma_obj PROPERTIES IMPORTED_LOCATION ${LIB_XZ})


# translations
#set(TS_FILES
#    "${CMAKE_SOURCE_DIR}/translations/Totale_en_US.ts"
#    "${CMAKE_SOURCE_DIR}/translations/Totale_it_IT.ts"
#    "${CMAKE_SOURCE_DIR}/translations/Totale_ja_JP.ts"
#    "${CMAKE_SOURCE_DIR}/translations/Totale_ru_RU.ts")
qt_add_resources(QRC_FILES translations.qrc)


# code
set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        #${TS_FILES}
)

set(ADDED_CODE
        src/phone.cpp
        src/phone.h
        src/translations.cpp
        src/fileManager.cpp
        src/fileManager.h
        src/extractor.cpp
        src/extractor.h
        src/bootloader.cpp
        src/bootloader.h
        src/updMetadata/update_metadata.pb.cc
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    #qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    if(WIN32)
        set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/imgs/ICON/res.rc")
        qt_add_executable(Total
            main.cpp
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            ${app_icon_resource_windows}
            ${ADDED_CODE}
            $<TARGET_OBJECTS:libarchive_obj>
            $<TARGET_OBJECTS:liblzma_obj>
            $<TARGET_OBJECTS:protobuf_obj>
            ${QRC_FILES}
        )

target_link_libraries(Total PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
            ${LIB_ARCHIVE}
            ${LIB_XZ}
            protobuf::libprotobuf protobuf::libprotoc
        )
    elseif(APPLE)
        set(MACOSX_BUNDLE_ICON_FILE icon.icns)
        set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/imgs/ICON/icon.icns")
        set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        qt_add_executable(Total
            MACOSX_BUNDLE
            main.cpp
            ${PROJECT_SOURCES}
            ${app_icon_macos}
            ${ADDED_CODE}
            $<TARGET_OBJECTS:libarchive_obj>
            $<TARGET_OBJECTS:liblzma_obj>
            $<TARGET_OBJECTS:protobuf_obj>
            ${QRC_FILES}
        )
        target_link_libraries(Total PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
            ${LIB_ARCHIVE}
            ${LIB_XZ}
            protobuf::libprotobuf protobuf::libprotoc
        )
    endif()
#    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Totale APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    #qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    if(ANDROID)
        add_library(Totale SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Totale
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Total PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)


#target_include_directories(Total PRIVATE
#    ${Protobuf_INCLUDE_DIRS}
#)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Totale)
endif()
set_target_properties(Total PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

include(GNUInstallDirs)
install(TARGETS Total
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Total)
endif()
