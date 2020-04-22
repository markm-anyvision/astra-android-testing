# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
include_directories(/Users/alexvainshtein/Downloads/AstraSDK-v2.1.0-9bced77c41-Linux-arm/AstraSDK-v2.1.0-9bced77c41-20200316T022223Z-Linux-aarch64/include)
#include_directories(~/Library/OpenCV-android-sdk-4.1.2/sdk/native/jni/include)

#add_library(lib_opencv SHARED IMPORTED)
#set_target_properties(lib_opencv PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/armeabi-v7a/libopencv_java4.so)
add_library(libastra_android_bridge SHARED IMPORTED)
set_target_properties(libastra_android_bridge PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libastra_android_bridge.so)
add_library(libastra_core_api SHARED IMPORTED)
set_target_properties(libastra_core_api PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libastra_core_api.so)
add_library(libastra_core SHARED IMPORTED)
set_target_properties(libastra_core PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libastra_core.so)
add_library(libastra_jni SHARED IMPORTED)
set_target_properties(libastra_jni PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libastra_jni.so)
add_library(libastra SHARED IMPORTED)
set_target_properties(libastra PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libastra.so)
add_library(libOniFile SHARED IMPORTED)
set_target_properties(libOniFile PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libOniFile.so)
add_library(libOpenNI2 SHARED IMPORTED)
set_target_properties(libOpenNI2 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libOpenNI2.so)
add_library(libopenni_sensor SHARED IMPORTED)
set_target_properties(libopenni_sensor PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libopenni_sensor.so)
add_library(liborbbec SHARED IMPORTED)
set_target_properties(liborbbec PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/liborbbec.so)
add_library(libusb SHARED IMPORTED)
set_target_properties(libusb PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libusb-1.0.9.so)

#[[add_library(libjpeg_turbo1500 SHARED IMPORTED)
set_target_properties(libjpeg_turbo1500 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libjpeg-turbo1500.so)
add_library(libusb100 SHARED IMPORTED)
set_target_properties(libusb100 PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libusb100.so)
add_library(libuvc SHARED IMPORTED)
set_target_properties(libuvc PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libuvc.so)
add_library(libUVCCamera SHARED IMPORTED)
set_target_properties(libUVCCamera PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../jniLibs/libUVCCamera.so)]]

add_library( # Sets the name of the library.
        native-lib

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        native-lib.cpp)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

#find_library( camera-lib camera2ndk )
#find_library( media-lib mediandk )
find_library( android-lib android )
find_library( gl-lib GLESv2 )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.


target_link_libraries( # Specifies the target library.
        native-lib
        ${log-lib}
        libastra_android_bridge
        libastra_core_api
        libastra_core
        libastra_jni
        libastra
        libOniFile
        libOpenNI2
        libopenni_sensor
        liborbbec
        libusb
        ${android-lib} ${gl-lib})