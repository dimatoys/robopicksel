#include "utils/image.h"

const double V[] = {51, 62, 11};

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
        PR.SetR(V, 0, 2, 3);
        
        AddParameter("StepL1", &StepL1, 20);
        AddParameter("StepL2", &StepL2, 3);
        AddParameter("LearningPictures", &LearningPictures, "");
        AddParameter("PR", &PRData);
        AddParameter("D", &D, 5653);

        DumpPRData();
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
    virtual void DestroyExtractor(TSegmentsExtractor* extractor) {}
    virtual void ParameterUpdated(std::string name);

    void Learn(TImagesLearningDataSource& images);

    void DumpPRData();
};


