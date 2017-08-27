#include "utils/image.h"

class TCVPyrExtractorFactory : public TExtractorFactory {
public:
	TMutableImage<int> ImageBorder;
	TObjectValidator Validator;

	int Depth;
	double Threshold1;
	double Threshold2;
	int Debug;

	TCVPyrExtractorFactory(const char* borderFileName) :
		ImageBorder(borderFileName) {
		AddParameter("Depth", &Depth, 2);
		AddParameter("Threshold1", &Threshold1, 80.0);
		AddParameter("Threshold2", &Threshold2, 90.0);
		AddParameter("DebugType", &Debug, 0);
	}

	TSegmentsExtractor* CreateExtractor(TMutableImage<unsigned char>* image);
};
