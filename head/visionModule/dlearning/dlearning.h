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
    
    int StepL1;
    int StepL2;
    
    std::string LearningPictures;
    std::string LearningVector;
    
    TDeepLearningExtractorFactory() :
        PR(S) {
        PR.SetR(R, 3 * Points, 1);
        
        AddParameter("StepL1", &StepL1, 20);
        AddParameter("StepL2", &StepL2, 3);
        AddParameter("LearningPictures", &LearningPictures, "");
        AddParameter("LearningVector", &LearningVector, "");
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
    virtual void DestroyExtractor(TSegmentsExtractor* extractor) {}
    virtual void ParameterUpdated(std::string name);

    void Learn(TImagesLearningDataSource& images);
};


