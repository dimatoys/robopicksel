#include "dlearning.h"

class TDeepLearningSegmentsExtractor : public TSegmentsExtractor {
	TDeepLearningExtractorFactory* Parameters;
    TMutableImage<unsigned char>* Image;

public:
	TDeepLearningSegmentsExtractor(TDeepLearningExtractorFactory* parameters,
                                   TMutableImage<unsigned char>* image) {
		Parameters = parameters;
		Image = image;
	}

	void ExtractSegments(std::list<TArea>& area);
	void DrawDebugInfo(TMutableRGBImage* image);

	~TDeepLearningSegmentsExtractor(){}
};

TSegmentsExtractor* TDeepLearningExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	return new TDeepLearningSegmentsExtractor(this, image);
}

void TDeepLearningSegmentsExtractor::ExtractSegments(std::list<TArea>& area){
}

void TDeepLearningSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image){
    double v[3];
    double r;
    
    unsigned char obj[] = {255, 0, 0};
    unsigned char fill[] = {255, 255, 255};
    
    for(int y = 10; y < Image->Height - 10; y+= 10) {
        for (int x = 10; x < Image->Width - 10; x+= 10) {
            unsigned char* pixel = Image->Cell(x, y);
            v[0] = pixel[0];
            v[1] = pixel[1];
            v[2] = pixel[2];
            Parameters->PR.GetValue(v, &r);
            image->DrawPointer(x, y, 1, r > 0.3 ? obj: fill);
        }
    } 
}
