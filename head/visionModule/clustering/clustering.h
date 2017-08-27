#include "utils/image.h"

class TClusteringExtractorFactory : public TExtractorFactory {
public:
	TMutableImage<int> ImageBorder;
	TObjectValidator Validator;

	int Scale;
	int SemgentMaxSimilarityStartValue;
	int SemgentMaxSimilarityLimit;

	TClusteringExtractorFactory() :
		ImageBorder() {
		AddParameter("Scale", &Scale, 1);
		AddParameter("SemgentMaxSimilarityStartValue", &SemgentMaxSimilarityStartValue, 150);
		AddParameter("SemgentMaxSimilarityLimit", &SemgentMaxSimilarityLimit, 300);
		AddParameter("MinBackgroundAreaPct", &Validator.MinBackgroundAreaPct);
		AddParameter("MinObjectAreaPct", &Validator.MinObjectAreaPct);
		AddParameter("MaxObjectAreaPct", &Validator.MaxObjectAreaPct);
	}

	TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};
