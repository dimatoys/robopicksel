#include "clustering.h"

#include <map>

class TClusteringSegmentsExtractor : public TSegmentsExtractor {
	TClusteringExtractorFactory* Parameters;
	TMutableImage<unsigned char>* Image;

	unsigned SemgentMaxSimilarity;
	unsigned CurrentSegment;

	TMutableImage<int> Segments;
	int* Background;
	unsigned BSize;

	void TestDiff(TMutableImage<unsigned char>* sampler,
	              TSamplerCell c,
	              int x, int y,
	              int* border);

	void ExtractSegmentOfSimilarity(TMutableImage<unsigned char>* smoothed, int seed, int* border);

	bool DetectBackground(TMutableImage<unsigned char>* sampler);

	void TestGap(int x, int y, int currentGap, TArea* area);
	void ExtractGapsByFilling(std::list<TArea>& segments, int* small, int* big);

public:
	TClusteringSegmentsExtractor(TClusteringExtractorFactory* parameters,
	                             TMutableImage<unsigned char>* image) :
	    Segments() {
		Parameters = parameters;
		Image = image;
		Background = NULL;
	}

	~TClusteringSegmentsExtractor() {
		if (Background != NULL) {
			delete Background;
		}
	}

	void ExtractSegments(std::list<TArea>& segments);
	void DrawDebugInfo(TMutableRGBImage* image);
};

TSegmentsExtractor* TClusteringExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	Validator.SetImageSize(image->Width, image->Height);
	return new TClusteringSegmentsExtractor(this, image);
}

void TClusteringSegmentsExtractor::TestDiff(TMutableImage<unsigned char>* sampler,
                                            TSamplerCell c,
                                            int x,
                                            int y,
                                            int* border) {
	int idx = Segments.Idx(x, y);
	int* cell = Segments.Cell(idx);
	int cellValue = *cell;
	if (cellValue == NO_BORDER) {
		if (sampler->Diff(c, sampler->Cell(x, y)) < SemgentMaxSimilarity) {
			*cell = CurrentSegment;
			Background[BSize++] = idx;
		}
	} else {
		if (cellValue < FIRST_SEGMENT) {
			*border |= cellValue;
		}
	}
}

void TClusteringSegmentsExtractor::ExtractSegmentOfSimilarity(TMutableImage<unsigned char>* smoothed, int seed, int* border) {
	unsigned idx = 0;
	BSize = 0;
	Background[BSize++] = seed;
	*Segments.Cell(seed) = CurrentSegment;
	while(BSize > idx) {
		int x, y;
		Segments.IdxToXY(Background[idx++], x, y);
		TSamplerCell c = smoothed->Cell(x, y);
		// Set Segments elements to Current segment if the point is similar
		TestDiff(smoothed, c, x - 1, y, border);
		TestDiff(smoothed, c, x - 1, y - 1, border);
		TestDiff(smoothed, c, x - 1, y + 1, border);
		TestDiff(smoothed, c, x, y - 1, border);
		TestDiff(smoothed, c, x, y + 1, border);
		TestDiff(smoothed, c, x + 1, y, border);
		TestDiff(smoothed, c, x + 1, y - 1, border);
		TestDiff(smoothed, c, x + 1, y + 1, border);
	}
}

bool TClusteringSegmentsExtractor::DetectBackground(TMutableImage<unsigned char>* sampler) {

	if ((Parameters->ImageBorder.IsAllocated() && ((Parameters->ImageBorder.Width != sampler->Width) || (Parameters->ImageBorder.Height != sampler->Height))) ||
		!Parameters->ImageBorder.IsAllocated()) {

		printf("Allocate: %d %d\n", sampler->Width, sampler->Height);
		Parameters->ImageBorder.Allocate(sampler->Width, sampler->Height, 1, NO_BORDER);

		int border = BORDER_TOP;
		Parameters->ImageBorder.DrawHorizontal(0, 0, sampler->Width, &border);

		border = BORDER_BOTTOM;
		Parameters->ImageBorder.DrawHorizontal(0, sampler->Height - 1, sampler->Width, &border);

		border = BORDER_LEFT;
		Parameters->ImageBorder.DrawVertical(0, 0, sampler->Height, &border);

		border = BORDER_RIGHT;
		Parameters->ImageBorder.DrawVertical(sampler->Width - 1, 0, sampler->Height, &border);
	}

	do {
		TValidationResult validation;
		Segments.CopyFrom(&Parameters->ImageBorder);
		CurrentSegment = FIRST_SEGMENT;

		/*
		int border = 0;
		ExtractSegmentOfSimilarity(sampler, 130, &border);
		return true;
		*/

		int size = sampler->Size();
		for(int seed = 0; seed < size; seed++) {
			if(*Segments.Cell(seed) == NO_BORDER) {
				// Set Segments elements to Current segment if the point is similar
				int border = 0;
				ExtractSegmentOfSimilarity(sampler, seed, &border);
				Parameters->Validator.ValidateBackgroundBySize(BSize * Parameters->Scale * Parameters->Scale, border, validation);
				if (validation.LikelyBackground > 0.7) {
					return true;
				}
				CurrentSegment++;
			}
		}

		SemgentMaxSimilarity += 20;

	} while (SemgentMaxSimilarity <= Parameters->SemgentMaxSimilarityLimit);
	return false;
}

void TClusteringSegmentsExtractor::TestGap(int x, int y, int currentGap, TArea* area) {
	int idx = Segments.Idx(x, y);
	int* cell = Segments.Cell(idx);
	int cellValue = *cell;
	if (cellValue == NO_BORDER || (cellValue >= FIRST_SEGMENT && cellValue < CurrentSegment)) {
		*cell = currentGap;
		//printf("add: %d,%d %d -> %d\n", x, y, cellValue, currentGap);
		Background[BSize++] = idx;
	} else {
		if (cellValue < FIRST_SEGMENT) {
			area->AtBorder |= (char)cellValue;
		}
	}
}

void TClusteringSegmentsExtractor::ExtractGapsByFilling(std::list<TArea>& segments, int* small, int* big ) {
	int size = Segments.Width * Segments.Height;
	int idx = BSize;
	int currentGap = CurrentSegment + 1;
	for (int seed = 0; seed < size; seed++) {
		int* cell = Segments.Cell(seed);
		if (*cell == NO_BORDER || (*cell >= FIRST_SEGMENT && *cell < CurrentSegment)) {
			int x,y;
			Segments.IdxToXY(seed, x, y);
			//printf("seed: %d,%d : %d -> %d\n", x, y, *cell, currentGap);
			TArea area(x, y);
			Background[BSize++] = seed;
			*cell = seed;
			do {
				Segments.IdxToXY(Background[idx++], x, y);
				//printf("gap %d (%d,%d)\n", currentGap, x, y);
				area.Add(x, y);
				TestGap(x - 1, y - 1, currentGap, &area);
				TestGap(x, y - 1, currentGap, &area);
				TestGap(x + 1, y - 1, currentGap, &area);
				TestGap(x - 1, y, currentGap, &area);
				TestGap(x + 1, y, currentGap, &area);
				TestGap(x - 1, y + 1, currentGap, &area);
				TestGap(x, y + 1, currentGap, &area);
				TestGap(x + 1, y + 1, currentGap, &area);
			} while(idx < BSize);

			area.Size *= Parameters->Scale * Parameters->Scale;
			area.MinX *= Parameters->Scale;
			area.MinY *= Parameters->Scale;
			area.MaxX *= Parameters->Scale;
			area.MaxY *= Parameters->Scale;
			double validation = Parameters->Validator.ValidateArea(&area);
			if (validation == 0) {
				segments.push_back(area);
			} else {
				if (validation < 0) {
					(*small)++;
				} else {
					(*big)++;
				}
			}

			currentGap++;
		}
	}
}

void TClusteringSegmentsExtractor::ExtractSegments(std::list<TArea>& segments) {
	TMutableImage<unsigned char>* smoothed;
	if (Parameters->Scale != 1) {
		smoothed = new TMutableImage<unsigned char>(Image, Parameters->Scale);
	} else {
		smoothed = Image;
	}
	int size = smoothed->Size();
	Background = new int[size];
	TValidationResult validation;
	SemgentMaxSimilarity = Parameters->SemgentMaxSimilarityStartValue;

	if (DetectBackground(smoothed)) {
		int backgroundSize = BSize;
		int small = 0;
		int big = 0;
		//std::list<TArea> lsegments;
		ExtractGapsByFilling(segments, &small, &big);
		printf("%d: %lu objects, bsize=%d,  small=%d big=%d\n", SemgentMaxSimilarity, segments.size(), backgroundSize, small, big);
	}

	if (smoothed != Image) {
		delete smoothed;
	}
}

void TClusteringSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {
	unsigned char colors[][3] = {{ 0xFF, 0xFF, 0xFF},
	                             { 0xFF, 0xFF, 0x80},
	                             { 0xFF, 0x80, 0xFF},
	                             { 0xFF, 0x80, 0x80},
	                             { 0x80, 0xFF, 0xFF},
	                             { 0x80, 0xFF, 0x80},
	                             { 0x80, 0x80, 0xFF},
	                             { 0x80, 0x80, 0x80},
	                             { 0xFF, 0xFF, 0x00},
	                             { 0xFF, 0x00, 0xFF},
	                             { 0xFF, 0x00, 0xFF},
	                             { 0xFF, 0x00, 0x00},
	                             { 0x00, 0xFF, 0xFF},
	                             { 0x00, 0xFF, 0x00},
	                             { 0x00, 0x00, 0xFF},
	                             { 0x00, 0x00, 0x00}};

	for (int x = 0; x < Segments.Width; x++) {
		for(int y = 0; y < Segments.Height; y++) {
			int segment = *Segments.Cell(x, y);
			if (segment > CurrentSegment) {
				image->DrawPointer(x * Parameters->Scale + Parameters->Scale / 2,
				                   y * Parameters->Scale + Parameters->Scale / 2,
				                   2, colors[segment % 16]);
			} else {
				if (segment < FIRST_SEGMENT) {
					image->DrawPointer(x * Parameters->Scale + Parameters->Scale / 2,
					                   y * Parameters->Scale + Parameters->Scale / 2,
					                   2, colors[15]);
				}
			}
			if (x % 10 == 0 && y %10 == 0) {
				image->DrawPointer(x * Parameters->Scale + Parameters->Scale,
				                   y * Parameters->Scale + Parameters->Scale,
				                   1, colors[0]);
			}
		}
	}
}
