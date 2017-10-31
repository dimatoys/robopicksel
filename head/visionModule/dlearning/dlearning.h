#include "utils/image.h"

class TDeepLearningExtractorFactory : public TExtractorFactory {
public:

    TPolyRegression PR;

    int StepL1;
    int StepL2;
    
    std::string LearningPictures;
    std::string PRData;

    int D;

    TDeepLearningExtractorFactory() :
        PR(0) {
        AddParameter("StepL1", &StepL1, 20);
        AddParameter("StepL2", &StepL2, 3);
        AddParameter("LearningPictures", &LearningPictures, "");
        AddParameter("PR", &PRData, "0,2,3,51,62,11");
        AddParameter("D", &D, 5653);
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
    virtual void DestroyExtractor(TSegmentsExtractor* extractor) {}
    virtual void ParameterUpdated(std::string name);

    void Learn(TImagesLearningDataSource& images);

    void DumpPRData();
    void ReadPRData();
};


