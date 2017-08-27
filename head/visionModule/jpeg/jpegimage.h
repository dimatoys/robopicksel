#include "utils/image.h"

class TJpegExtractorFactory : public TExtractorFactory {
public:
	TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};
