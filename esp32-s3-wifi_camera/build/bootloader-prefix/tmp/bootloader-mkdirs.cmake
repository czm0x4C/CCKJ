# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "F:/ESPIDF/esp-idf/components/bootloader/subproject"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/tmp"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/src/bootloader-stamp"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/src"
  "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/CCKJ/esp32-s3-wifi_camera/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
