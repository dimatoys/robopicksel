#include <stdio.h>
#include <unistd.h>
#include "camera.h"

void Callback(void* data, const void* buffer, int buffer_length) {
	printf("Callback: length=%d\n", buffer_length);
}

int main(int argn, char** args) {

	StartCamera(1280, 720);

	sleep(1);

	printf("Shoot1\n");
	Shoot(Callback, NULL);

	sleep(1);
	//Disable();

	printf("Shoot2\n");
	Shoot(Callback, NULL);

	sleep(1);
	//Disable();

	printf("Shoot3\n");
	Shoot(Callback, NULL);

	sleep(1);
	//Disable();

	printf("stop camera\n");
	StopCamera();
}
