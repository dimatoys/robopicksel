#include "utils/image.h"

class TDeepLearningExtractorFactory : public TExtractorFactory {
public:

    TPolyRegression PR;

    int StepL1;
    int StepL2;
    int MinKnots;
    
    std::string LearningPictures;
    std::string PRData;

    int D;

    TDeepLearningExtractorFactory() :
        PR(0) {
        AddParameter("StepL1", &StepL1, 20);
        AddParameter("StepL2", &StepL2, 3);
        AddParameter("MinKnots", &MinKnots, 2);
        AddParameter("LearningPictures", &LearningPictures, "");
        AddParameter("PR", &PRData, "0,2,3,114.533685,119.348970,63.209661");
		AddParameter("D", &D, 6000);
    }

    TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
    virtual void DestroyExtractor(TSegmentsExtractor* extractor) {}
    virtual void ParameterUpdated(std::string name);

    void Learn(TImagesLearningDataSource& images);

    void DumpPRData();
    void ReadPRData();
};


