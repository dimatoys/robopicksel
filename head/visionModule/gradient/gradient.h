#include "utils/image.h"

class TGradientExtractorFactory : public TExtractorFactory {
	int Scale;
	int Threshold;
	int MinSize;

	TMutableImage<char> ImageBorder;

public:
	TGradientExtractorFactory(const char* borderFileName) :
		ImageBorder(borderFileName) {
		AddParameter("Scale", &Scale, 10);
		AddParameter("Threshold", &Threshold, 1000);
		AddParameter("MinSize", &MinSize, 400);
	}

	TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};
