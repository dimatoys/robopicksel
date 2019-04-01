#pragma once

#include <stdio.h>
#include <jpeglib.h>

int write_jpeg_file(const char *filename, void* raw_image, int width, int height, int bytes_per_pixel, J_COLOR_SPACE color_space );
