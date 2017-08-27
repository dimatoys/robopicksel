#include "utils/image.h"

class TKeyPointsExtractorFactory : public TExtractorFactory {
public:
    int GaussianSize;
    int Depth;
    
    
    TKeyPointsExtractorFactory() {
	AddParameter("GaussianSize", &GaussianSize, 5);
        AddParameter("Depth", &Depth, 3);
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};

