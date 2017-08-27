#pragma once

#define CAMERA_PARAM_BRIGHTNESS "BRIGHTNESS" // 0,100 def 50
#define CAMERA_PARAM_SHARPNESS "SHARPNESS"   // -100..100 def 0
#define CAMERA_PARAM_CONTRAST "CONTRAST" //-100 to 100 def 0
#define CAMERA_PARAM_ISO "ISO" // 100 to 800 def 400
#define CAMERA_PARAM_SATURATION "SATURATION" //-100 to 100 def 0
#define CAMERA_PARAM_STABILIZATION "STABILIZATION" // 0 1 def 0
#define CAMERA_PARAM_COMPENSATION "COMPENSATION" // -10,10 def 0

/*
 *      RASPICAM_EXPOSURE_OFF = 0,
        RASPICAM_EXPOSURE_AUTO,
        RASPICAM_EXPOSURE_NIGHT,
        RASPICAM_EXPOSURE_NIGHTPREVIEW,
        RASPICAM_EXPOSURE_BACKLIGHT,
        RASPICAM_EXPOSURE_SPOTLIGHT,
        RASPICAM_EXPOSURE_SPORTS,
        RASPICAM_EXPOSURE_SNOW,
        RASPICAM_EXPOSURE_BEACH,
        RASPICAM_EXPOSURE_VERYLONG,
        RASPICAM_EXPOSURE_FIXEDFPS,
        RASPICAM_EXPOSURE_ANTISHAKE,
        RASPICAM_EXPOSURE_FIREWORKS
 */

#define CAMERA_PARAM_EXPOSURE "EXPOSURE" // 0 - def AUT0

/*
 *      RASPICAM_METERING_AVERAGE,
        RASPICAM_METERING_SPOT,
        RASPICAM_METERING_BACKLIT,
        RASPICAM_METERING_MATRIX
 *
 */

#define CAMERA_PARAM_METERING "METERING" // def AVERAGE
#define CAMERA_PARAM_SHUTTER_SPEED "SHUTTER_SPEED" // ? (max 330000)  0 - auto   if not auto EXPOSURE=FIXEDFPS

typedef void (*CameraCBFunction)(void* data, const void* buffer, int buffer_length);

void StartCamera(int width, int height);
void SetCameraParameter(const char* parameter, int value);
bool Shoot(CameraCBFunction callback, void* data);
void StopCamera();
