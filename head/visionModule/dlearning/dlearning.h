#include "utils/image.h"

const unsigned char S = 2;
const unsigned char Points = 1;
    
const double R[] = {1.32433886e-01,
                    1.85654775e-03,
                    -7.42018708e-03,
                    6.26838325e-03,
                    3.90237888e-05,
                    -9.45425805e-05,
                    5.17856248e-05,
                    1.24865567e-04,
                    -2.32869503e-04,
                    1.34534411e-04};

class TDeepLearningExtractorFactory : public TExtractorFactory {
public:
    
    TPolyRegression PR;
    
    TDeepLearningExtractorFactory() :
        PR(S) {
        PR.SetR(R, 3 * Points, 1);
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};


