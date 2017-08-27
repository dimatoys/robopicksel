#include "utils/image.h"

class TStatImgExtractorFactory : public TExtractorFactory {
public:
	//TObjectValidator Validator;

	int RegressionLevel;
	int AreaCell;
	double AnomalyThreshold;
	int MinCoreSize;
	TMutableImage<double> RegressionMatrix;

	TStatImgExtractorFactory() {
		RegressionLevel = 6;

		AddParameter("MinCoreSize", &MinCoreSize, 2);
		AddParameter("AreaCell", &AreaCell, 10);
		AddParameter("AnomalyThreshold", &AnomalyThreshold, 0.18);
	}

	TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};
