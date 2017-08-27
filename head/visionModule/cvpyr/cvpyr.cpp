#include "cvpyr.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include <cvaux.h>

class TCVPyrSegmentsExtractor : public TSegmentsExtractor {

	TCVPyrExtractorFactory* Parameters;
	TMutableImage<unsigned char>* Image;
	IplImage* DstImage;
	std::list<TArea> CandidateAreas;

public:
	TCVPyrSegmentsExtractor(TCVPyrExtractorFactory* parameters,
	                        TMutableImage<unsigned char>* image)  {
		Parameters = parameters;
		Image = image;
		DstImage = cvCreateImage(cvSize(Image->Width, Image->Height), 8, 3);
	}

	~TCVPyrSegmentsExtractor() {
		cvReleaseImage(&DstImage);
	}

	void ExtractSegments(std::list<TArea>& areas);

	void DrawDebugInfo(TMutableRGBImage* image);
};

TSegmentsExtractor* TCVPyrExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	Validator.SetImageSize(image->Width, image->Height);
	return new TCVPyrSegmentsExtractor(this, image);
}

void TCVPyrSegmentsExtractor::ExtractSegments(std::list<TArea>& areas) {
	IplImage* src= cvCreateImageHeader(cvSize(Image->Width, Image->Height), IPL_DEPTH_8U, 3);
	cvSetData(src, Image->Cell(0,0), Image->Width * 3 );
	CvSeq* comp = NULL;
	CvMemStorage* storage = cvCreateMemStorage(0);

	cvPyrSegmentation(src, DstImage, storage, &comp, Parameters->Depth, Parameters->Threshold1, Parameters->Threshold2);
	printf("cvPyrSegmentation: %d %f %f -> %d\n", Parameters->Depth, Parameters->Threshold1, Parameters->Threshold2, comp->total);

	for (int i = 0; i < comp->total; i++) {
		CvConnectedComp* cc = (CvConnectedComp*) cvGetSeqElem(comp, i);

		TArea area;
		area.MinX = cc->rect.x;
		area.MinY = cc->rect.y;
		area.MaxX = cc->rect.x + cc->rect.width;
		area.MaxY = cc->rect.y + cc->rect.height;
		area.AtBorder = Parameters->Debug;

		CandidateAreas.push_back(area);

		if (Parameters->Validator.ValidateArea(&area) == 0) {
			areas.push_back(area);
		}
	}

	printf("ExtractSegments:relese\n");

	cvReleaseMemStorage( &storage );
	//cvReleaseImage(&src);

	printf("ExtractSegments:Ok\n");
}

void TCVPyrSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {

	printf("DrawDebugInfo: begin\n");
	if (Parameters->Debug == 1) {
		memcpy(image->Cell(0,0), DstImage->imageData, image->Width * image->Height * image->Depth);
	}
	unsigned char color[] = {0x40, 0xFF, 0x40};
	for(std::list<TArea>::iterator area = CandidateAreas.begin();  area != CandidateAreas.end(); area++) {
		for (int j = 0; j < 3; j++) {
			int a = area->MinX + area->MinY + area->MaxX + area->MaxY;
			image->DrawRect(area->MinX - j,
					area->MinY - j,
					area->MaxX - area->MinX + 2 * j,
					area->MaxY - area->MinY + 2 * j,
					color);
		}
	}
	printf("DrawDebugInfo: Ok\n");
}
