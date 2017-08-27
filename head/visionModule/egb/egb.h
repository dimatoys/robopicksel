#include "utils/image.h"

class TEGBSegmentsExtractor : public TSegmentsExtractor {
	TMutableImage<unsigned char>* Image;
	TMutableImage<char>* Border;
	int Scale;

	TMutableImage<int> Segments;
	TMutableImage<int> SegmentNext;

public:
	TEGBSegmentsExtractor(TMutableImage<unsigned char>* image,
	                      TMutableImage<char>* border,
	                      int scale) :
	    Segments(image->Width, image->Height, 1),
	    SegmentNext(image->Width, image->Height, 1) {
		Image = image;
		Border = border;
		Scale = scale;
	}

	void ExtractSegments(std::list<TArea>& segments);
	void DrawDebugInfo(TMutableRGBImage* image);
};
