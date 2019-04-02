#pragma once

#ifdef NO_JPEG
enum  	J_COLOR_SPACE {
  JCS_UNKNOWN,
  JCS_GRAYSCALE,
  JCS_RGB,
  JCS_YCbCr,
  JCS_CMYK,
  JCS_YCCK,
  JCS_RGB565
};
int write_jpeg_file(const char *filename, void* raw_image, int width, int height, int bytes_per_pixel, J_COLOR_SPACE color_space ) {
	return 0;
}
#else

#include <stdio.h>
#include <jpeglib.h>
int write_jpeg_file(const char *filename, void* raw_image, int width, int height, int bytes_per_pixel, J_COLOR_SPACE color_space );

#endif
