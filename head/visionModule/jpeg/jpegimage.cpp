#include "jpegimage.h"

class TJpegSegmentsExtractor : public TSegmentsExtractor {
	TJpegExtractorFactory* Parameters;
	TMutableImage<unsigned char>* Image;

public:
	TJpegSegmentsExtractor(TJpegExtractorFactory* parameters,
	                       TMutableImage<unsigned char>* image) {
		Parameters = parameters;
		Image = image;
	}
};

TSegmentsExtractor* TJpegExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	return new TJpegSegmentsExtractor(this, image);
}
