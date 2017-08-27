#include "gradient.h"

#define STATUS_UNTESTED 0
#define STATUS_IN_FRONTIER 1
#define STATUS_FLAT 2
#define STATUS_SKIP 3

class TGradientImage : public TMutableImage<int>, public TSegmentsExtractor {

	int Threshold;
	int MinAreaSize;
	int Scale;
	int* FrontierArray;

public:

	TMutableImage<char> StatusMatrix;

	TGradientImage(TMutableImage<unsigned char>* image,
	               TMutableImage<char>* imageBorder,
	               int threshold,
	               int minAreaSize,
	               int scale) :
		TMutableImage<int>(image->Width / scale - 2, image->Height / scale - 2, 1),
		StatusMatrix(imageBorder)
	{
		Threshold = threshold;
		MinAreaSize = minAreaSize / scale;
		Scale = scale;
		FrontierArray = new int[Width * Height];

		TMutableImage<unsigned char> smooth(image, scale);
		//smooth.ConvertToYUV();
		for (int x = 1; x < smooth.Width - 1; x++) {
			for (int y = 1; y < smooth.Height - 1; y++) {
				unsigned char* cell = smooth.Cell(x, y);
				*Cell(x - 1, y - 1) = smooth.Diff(cell, smooth.Cell(x - 1, y)) +
				                      smooth.Diff(cell, smooth.Cell(x + 1, y)) +
				                      smooth.Diff(cell, smooth.Cell(x, y - 1)) +
				                      smooth.Diff(cell, smooth.Cell(x, y + 1));
			}
		}
	}

	~TGradientImage() {
		if (FrontierArray != NULL) {
			delete FrontierArray;
		}
	}

	void ExtractSegments(std::list<TArea>& area);

	bool IsHighGradient(int idx, bool& skipped);

	bool TestPoint(int x,
	               int y,
	               int& frontierIdx,
	               char& atBorder,
	               char borderType);

	void DrawDebugInfo(TMutableRGBImage* image);
};

bool TGradientImage::IsHighGradient(int idx, bool& skipped) {
	char* status = StatusMatrix.Cell(idx);
	if (*status == STATUS_UNTESTED) {
		if (*Cell(idx) > Threshold) {
			*status = STATUS_IN_FRONTIER;
			return true;
		} else {
			*status = STATUS_FLAT;
		}
	} else {
		skipped = *status == STATUS_SKIP;
	}
	return false;
}

bool TGradientImage::TestPoint(int x,
                               int y,
                               int& frontierIdx,
                               char& atBorder,
                               char borderType) {
	//printf("TestPoint: %d %d\n", x, y);
	if ((x < 0) || (x >= Width) || (y < 0) || (y >= Height)) {
		atBorder |= borderType;
		return false;
	}

	int idx = Idx(x, y);
	bool skipped;
	if (IsHighGradient(idx, skipped)) {
		FrontierArray[frontierIdx++] = idx;
		return true;
	}

	if (skipped) {
		atBorder |= BORDER_SKIP;
	}

	//printf("TestPoint: false\n");
	return false;
}

void TGradientImage::ExtractSegments(std::list<TArea>& segments) {
	int size = Width * Height;
	int frontierIdx = 0;

	//printf("ExtractAreas: size=%d\n", size);

	for (int i = 0; i < size; i++) {
		bool skipped;
		if (IsHighGradient(i, skipped)) {
			TArea area;
			int frontierHead = frontierIdx;
			area.Start = frontierHead;
			IdxToXY(i, area.MinX, area.MinY);
			area.MaxX = area.MinX;
			area.MaxY = area.MinY;
			//printf("add f\n");
			FrontierArray[frontierIdx++] = i;
			area.AtBorder = NO_BORDER;
			while (frontierHead < frontierIdx) {
				int x, y;
				IdxToXY(FrontierArray[frontierHead++], x, y);
				//printf("EA: %d,%d\n", x, y);
				if(TestPoint(x - 1, y, frontierIdx, area.AtBorder, BORDER_LEFT)) {
					if (x - 1 < area.MinX) {
						area.MinX = x - 1;
					}
				}
				if (TestPoint(x + 1, y, frontierIdx, area.AtBorder, BORDER_RIGHT) ) {
					if (x + 1 > area.MaxX) {
						area.MaxX = x + 1;
					}
				}
				if (TestPoint(x, y - 1, frontierIdx, area.AtBorder, BORDER_TOP)) {
					if (y - 1 < area.MinY) {
						area.MinY = y - 1;
					}
				}
				if (TestPoint(x, y + 1, frontierIdx, area.AtBorder, BORDER_BOTTOM)) {
					if (y + 1 > area.MaxY) {
						area.MaxY = y + 1;
					}
				}
			}
			area.Size = frontierHead - area.Start;
			printf("Area: %d [%d,%d - %d,%d] BORDER: %s-%s-%s-%s\n", area.Size, area.MinX, area.MinY,
				area.MaxX, area.MaxY,
				(area.AtBorder & BORDER_LEFT) > 0 ? "LEFT": "",
				(area.AtBorder & BORDER_RIGHT) > 0 ? "RIGHT": "",
				(area.AtBorder & BORDER_TOP) > 0 ? "TOP": "",
				(area.AtBorder & BORDER_BOTTOM) > 0 ? "BOTTOM": "");
			if (area.Size >= MinAreaSize) {
				area.Size *= Scale * Scale;
				area.MinX *= Scale;
				area.MinY *= Scale;
				area.MaxX *= Scale;
				area.MaxY *= Scale;
				segments.push_back(area);
			}
		}
	}
	//printf("ExtractAreas: Ok\n");
}

void TGradientImage::DrawDebugInfo(TMutableRGBImage* image) {

	unsigned char colorFrontier[] = {0x80, 0xFF, 0x80};
	unsigned char colorSkip[] = {0xFF, 0x80, 0x80};

	int size = Width * Height;
	for (int i = 0; i < size; i++) {
		char status = *StatusMatrix.Cell(i);
		if (status == STATUS_IN_FRONTIER) {
			int x, y;
			IdxToXY(i, x, y);
			image->DrawPointer(x * Scale + Scale * 3 / 2, y * Scale + Scale * 3 / 2, 2, colorFrontier);
		} else {
			if (status == STATUS_SKIP) {
				int x, y;
				IdxToXY(i, x, y);
				image->DrawPointer(x * Scale + Scale * 3 / 2, y * Scale + Scale * 3 / 2, 2, colorSkip);
			}
		}
	}
}

TSegmentsExtractor* TGradientExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	return new TGradientImage(image,
	                          &ImageBorder,
	                          Threshold,
	                          MinSize,
	                          Scale);
}
