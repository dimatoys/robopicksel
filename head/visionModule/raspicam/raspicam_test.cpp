#include "camera.h"
#include <stdio.h>

void callback(void* data, const void* buffer, int buffer_length) {
	printf("Callback: buffer=%X length=%d\n", buffer, buffer_length);
}

int main ( int argc,char **argv ) {

	int width = 1280;
	int height = 720;

	StartCamera(width, height);

	Shoot(callback, NULL);
	Shoot(callback, NULL);
	Shoot(callback, NULL);

	StopCamera();

}
