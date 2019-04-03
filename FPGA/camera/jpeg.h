#pragma once

#ifdef NO_JPEG
int save_RGB(const char *filename, void* raw_image, int width, int height){ return 0;}
#else
int int save_RGB(const char *filename, void* raw_image, int width, int height);
#endif
