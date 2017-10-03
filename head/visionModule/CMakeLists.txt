cmake_minimum_required(VERSION 2.8)
project( picam )
SET(COMPILE_DEFINITIONS -Werror -DLOCAL=0)
SET(CMAKE_CXX_FLAGS -DRASPBERRYPI=1)
SET(CMAKE_CXX_STANDARD 11)

#set(raspicam_DIR /home/pi/projects/raspicam-master/build)
#find_package(raspicam REQUIRED)

#set(OpenCV_DIR /home/pi/projects/opencv-2.4.9/release)
#FIND_PACKAGE( OpenCV REQUIRED )

#include_directories(/usr/include/python2.7)
include_directories(.)

#link_directories(/usr/lib/python2.7/config-arm-linux-gnueabihf)

set(SRC_VISION_MODULE
    module/visionModule.cpp
    #raspicam/raspicam.cpp
    utils/jpeg.cpp
    utils/image.cpp
    #gradient/gradient.cpp
    #clustering/clustering.cpp
    jpeg/jpegimage.cpp
    #cvpyr/cvpyr.cpp
    #objects/validator.cpp
    statimg/statimg.cpp
    kp/kp.cpp
    dlearning/dlearning.cpp)

add_library(visionModule SHARED ${SRC_VISION_MODULE})
target_link_libraries(visionModule jpeg)
