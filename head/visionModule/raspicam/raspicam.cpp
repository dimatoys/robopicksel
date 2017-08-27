#include "raspicam.h"
#include "camera.h"
#include <stdio.h>
#include <string.h>

static raspicam::RaspiCam* Camera = NULL;
static int Length = 0;
static unsigned char* Buffer = NULL;

void StartCamera(int width, int height) {
	if (Camera == NULL) {
		Camera = new raspicam::RaspiCam();
	}
    Camera->setWidth(width);
    Camera->setHeight(height);

    if ( !Camera->open() ) {
    	printf("Error opening camera");
    }

    Length = Camera->getImageBufferSize();
    printf("Camera buff size=%d\n", Length);
    Buffer = new unsigned char[Length];
}

void SetCameraParameter(const char* parameter, int value) {
	if (parameter != NULL) {
		if (!strcmp(parameter, CAMERA_PARAM_BRIGHTNESS)) {
			printf("SetParameter: Brightness :%d\n", value);
			Camera->setBrightness(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_SHARPNESS)) {
			printf("SetParameter: Sharpness :%d\n", value);
			Camera->setSharpness(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_CONTRAST)) {
			printf("SetParameter: Contrast :%d\n", value);
			Camera->setContrast(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_ISO)) {
			printf("SetParameter: ISO :%d\n", value);
			Camera->setISO(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_SATURATION)) {
			printf("SetParameter: Saturation :%d\n", value);
			Camera->setSaturation(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_STABILIZATION)) {
			printf("SetParameter: VideoStabilization :%s\n", value > 0 ? "true" : "false");
			Camera->setVideoStabilization(value > 0);
		}else if(!strcmp(parameter, CAMERA_PARAM_COMPENSATION)) {
			printf("SetParameter: ExposureCompensation :%d\n", value);
			Camera->setExposureCompensation(value);
		}else if(!strcmp(parameter, CAMERA_PARAM_EXPOSURE)) {
			printf("SetParameter: Exposure :%d\n", value);
			Camera->setExposure((raspicam::RASPICAM_EXPOSURE)value);
		}else if(!strcmp(parameter, CAMERA_PARAM_METERING)) {
			printf("SetParameter: Metering :%d\n", value);
			Camera->setMetering((raspicam::RASPICAM_METERING)value);
		}else if(!strcmp(parameter, CAMERA_PARAM_SHUTTER_SPEED)) {
			printf("SetParameter: ShutterSpeed :%d\n", value);
			Camera->setShutterSpeed(value);
		} else {
			printf("Unknown parameter: %s\n", parameter);
		}
	}
}

bool Shoot(CameraCBFunction callback, void* data) {
    if (Camera->grab()){
    	Camera->retrieve(Buffer);
    	(*callback)(data, Buffer, Length);
    	return true;
    } else {
    	printf("Error grabbing\n");
    	return false;
    }

}

void StopCamera() {
    Camera->release();
    if (Buffer != NULL) {
    	delete Buffer;
    	Buffer = NULL;
    }
    if (Camera != NULL) {
    	delete Camera;
    	Camera = NULL;
    }
}
