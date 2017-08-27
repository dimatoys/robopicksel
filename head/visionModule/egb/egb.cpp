#include "egb.h"

#include <algorithm>
#include <vector>
#include <map>

unsigned int* g_Distances;

bool ElementsDistance(int i, int j) {
	return g_Distances[i] < g_Distances[j];
}

void TEGBSegmentsExtractor::ExtractSegments(std::list<TArea>& segments) {
	printf("ExtractSegments: start\n");

	int numClusters = Image->Width * Image->Height;
	int segmentSize[numClusters];
	for (int i = 0; i < numClusters; i++) {
		*Segments.Cell(i) = i;
		*SegmentNext.Cell(i) = i;
		segmentSize[i] = 1;
	}

	int sortIndexSize = Image->Height * (Image->Width - 1) +
	                    Image->Width * (Image->Height - 1) +
	                    2 * (Image->Width - 1) * (Image->Height - 1);

	unsigned short aidx1[sortIndexSize];
	unsigned short aidx2[sortIndexSize];
	unsigned int adistance[sortIndexSize];

	int i = 0;
	for(int y = 0; y < Image->Height; y++) {
		int leftIdx = Segments.Idx(0, y);
		TSamplerCell leftPixel = Image->Cell(0, y);
		for (int x = 1; x < Image->Width; x++) {
			TSamplerCell rightPixel = Image->Cell(x, y);
			unsigned diff = Image->Diff(leftPixel, rightPixel);
			aidx1[i] = leftIdx;
			leftIdx = Segments.Idx(x, y);
			aidx2[i] = leftIdx;
			adistance[i++] = (unsigned short)(diff < 0x10000 ? diff : 0xFFFF);
			leftPixel = rightPixel;
		}
	}

	for(int x = 0; x < Image->Width; x++) {
		int topIdx = Segments.Idx(x,0);
		TSamplerCell topPixel = Image->Cell(x, 0);
		for (int y = 1; y < Image->Height; y++) {
			TSamplerCell bottomPixel = Image->Cell(x, y);
			int diff = Image->Diff(topPixel, bottomPixel);
			aidx1[i] = topIdx;
			topIdx = Segments.Idx(x, y);
			aidx2[i] = topIdx;
			adistance[i++] = (unsigned short)(diff < 0x10000 ? diff : 0xFFFF);
			topPixel = bottomPixel;
		}
	}

	for(int x = 1; x < Image->Width; x++) {
		for (int y = 1; y < Image->Height; y++) {
			int diff = Image->Diff(Image->Cell(x, y), Image->Cell(x - 1, y - 1));
			aidx1[i] = Segments.Idx(x, y);
			aidx2[i] = Segments.Idx(x - 1, y - 1);
			adistance[i++] = (unsigned short)(diff < 0x10000 ? diff : 0xFFFF);

			diff = Image->Diff(Image->Cell(x - 1, y), Image->Cell(x, y - 1));
			aidx1[i] = Segments.Idx(x - 1, y);
			aidx2[i] = Segments.Idx(x, y - 1);
			adistance[i++] = (unsigned short)(diff < 0x10000 ? diff : 0xFFFF);
		}
	}

	std::vector<int> sortIndex(sortIndexSize);
	for (unsigned i = 0; i < sortIndexSize; i++) {
		sortIndex[i] = i;
	}

	printf("ExtractSegments: sort\n");

	g_Distances = adistance;
	std::sort(sortIndex.begin(), sortIndex.end(), ElementsDistance);

	printf("ExtractSegments: make segments\n");

	for (std::vector<int>::const_iterator i = sortIndex.begin(); i != sortIndex.end(); ++i) {

		if (adistance[*i] > 80) {
			break;
		}
		unsigned idx1 = aidx1[*i];
		unsigned idx2 = aidx2[*i];

		int majorSegment = *Segments.Cell(idx1);
		int secondSegment = *Segments.Cell(idx2);
		if ((majorSegment != secondSegment) /*&& ((segmentSize[majorSegment] < 200) || (segmentSize[secondSegment] < 200))*/) {
			// join segments
			segmentSize[majorSegment] += segmentSize[secondSegment];
			int prt;
			int next = idx2;
			do {
				prt = next;
				*Segments.Cell(prt) = majorSegment;
				next = *SegmentNext.Cell(prt);
			} while(next != idx2);
			int pidx1 = *SegmentNext.Cell(idx1);
			*SegmentNext.Cell(prt) = pidx1;
			*SegmentNext.Cell(idx1) = idx2;
			//if (--numClusters == 2000) {
			//	break;
			//}
		}
	}
}

void TEGBSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {

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

	std::map<int,int> segmentColor;
	int colorIdx = 0;
	for (int x = 0; x < Segments.Width; x++) {
		for(int y = 0; y < Segments.Height; y++) {
			int segment = *Segments.Cell(x, y);
			std::map<int,int>::iterator it = segmentColor.find(segment);
			int color;
			if (it == segmentColor.end()) {
				color = colorIdx++;
				segmentColor[segment] = color;
			} else {
				color = it->second;
			}
			image->DrawPointer(x * Scale + Scale * 3 / 2, y * Scale + Scale * 3 / 2, 5, Image->Cell(x,y));
			image->DrawPointer(x * Scale + Scale * 3 / 2, y * Scale + Scale * 3 / 2, 2, colors[color % 16]);
			if (x % 10 == 0 && y %10 == 0) {
				image->DrawPointer(x * Scale + Scale, y * Scale + Scale, 1, colors[0]);
			}
		}
	}
}

/*
void TEGBSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {

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

	for(int y = 0; y < Image->Height; y++) {
		for (int x = 0; x < Image->Width - 1; x++) {
			unsigned distance = *DistancesRight.Cell(x, y);
			if (distance <= 40) {
				image->FillRect(x * Scale + Scale / 2, y * Scale + Scale / 2 - 2, Scale, 4, colors[distance % 15]);
			}
		}
	}

	for(int x = 0; x < Image->Width; x++) {
		for (int y = 0; y < Image->Height - 1; y++) {
			unsigned distance = *DistancesBottom.Cell(x, y);
			if (distance <= 40) {
				image->FillRect(x * Scale + Scale / 2 - 2, y * Scale + Scale / 2, 4, Scale, colors[distance % 15]);
			}
		}
	}
}
*/
/*
void TEGBSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {

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

	int sortIndexSize = Image->Height * (Image->Width - 1) + Image->Width * (Image->Height - 1);
	std::vector<int> sortIndex(sortIndexSize);
	for (unsigned i = 0; i < sortIndexSize; i++) {
		sortIndex[i] = i;
	}

	printf("ExtractSegments: sort\n");

	g_DistancesRight = &DistancesRight;
	g_DistancesBottom = &DistancesBottom;
	g_DistancesRightSize = DistancesRight.Size();
	std::sort(sortIndex.begin(), sortIndex.end(), ElementsDistance);

	printf("ExtractSegments: make segments\n");

	for (std::vector<int>::const_iterator i = sortIndex.begin(); i != sortIndex.end(); ++i) {
		unsigned idx1, idx2, distance;
		if (*i < g_DistancesRightSize) {
			idx1 = *i;
			int x, y;
			DistancesRight.IdxToXY(idx1, x, y);
			distance = *DistancesRight.Cell(idx1);
			image->FillRect(x * Scale + Scale / 2, y * Scale + Scale / 2 - 2, Scale, 4, colors[distance % 15]);
		} else {
			idx1 = *i - g_DistancesRightSize;
			int x, y;
			DistancesBottom.IdxToXY(idx1, x, y);
			distance = *DistancesBottom.Cell(idx1);
			image->FillRect(x * Scale + Scale / 2 - 2, y * Scale + Scale / 2, 4, Scale, colors[distance % 15]);
		}
		if (distance >= 40) {
			break;
		}
	}
*/
/*
void TEGBSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image, int scale) {
	int maxValue = 65536;
	int percentile[maxValue];
	memset(percentile, 0, maxValue * sizeof(int));
	for (int i = 0; i < DistancesRight.Size(); i++) {
		percentile[*DistancesRight.Cell(i)]++;
	}
	for (int i = 0; i < DistancesBottom.Size(); i++) {
		percentile[*DistancesBottom.Cell(i)]++;
	}

	int levels = 8;
	int values[levels];
	int v = 0;
	int sum = 0;
	double ignore = 0.9;
	int totalNum = DistancesRight.Size() + DistancesBottom.Size();
	int ignorenum = totalNum * ignore;
	for (int i = 0; i < maxValue; i++) {
		sum += percentile[i];
		if (sum >= ignorenum) {
			while(sum - ignorenum > (v + 1) * (totalNum - ignorenum) / levels) {
				values[v++] = i;
				printf("level [%d] = %d\n", v, i);
			}
		}
	}

	//printf("DrawDebugInfo: min = %u max= %u\n", (unsigned)min, (unsigned)max);

	unsigned char cellColor[] = {0xFF, 0x80, 0x80};

	for (int x = 0; x < DistancesRight.Width; x++) {
		for(int y = 0; y < DistancesRight.Height; y++) {
			unsigned int distance = *DistancesRight.Cell(x,y);
			//printf(" %4X", distance);
			unsigned char color[3];
			int i;
			for (i = 0; i < levels; i++) {
				if (distance < values[i]) {
					break;
				}
			}
			color[0] = i * 255 / levels;
			//color[0] = (distance - min) * 255 / (max - min);
			color[1] = color[0];
			color[2] = color[0];
			image->DrawPointer((x + 1) * scale, y * scale + scale * 0.5, 1, color);

			image->DrawPointer(x * scale + scale * 0.5, y * scale + scale * 0.5, 1, Image->Cell(x, y));
			image->DrawPointer(x * scale, y * scale, 1, cellColor);

			//unsigned char* current = Image->Cell(x, y);
			//unsigned char* right = Image->Cell(x + 1, y);
			//printf("(%4d, %3d) [%03u,%03u,%03u]-[%03u,%03u,%03u] right %u\n", x * scale, y * scale,
			//	(unsigned)current[0], (unsigned)current[1], (unsigned)current[2],
			//	(unsigned)right[0], (unsigned)right[1], (unsigned)right[2], distance);

		}
		//printf("\n");
	}

	for (int x = 0; x < DistancesBottom.Width; x++) {
		for(int y = 0; y < DistancesBottom.Height; y++) {
			unsigned int distance = *DistancesBottom.Cell(x,y);
			//printf(" %4X", distance);
			unsigned char color[3];
			int i;
			for (i = 0; i < levels; i++) {
				if (distance < values[i]) {
					break;
				}
			}
			color[0] = i * 255 / levels;
			//color[0] = (distance - min) * 255 / (max - min);
			color[1] = color[0];
			color[2] = color[0];
			image->DrawPointer(x * scale + scale * 0.5, (y + 1) * scale, 1, color);

			//unsigned char* current = Image->Cell(x, y);
			//unsigned char* bottom = Image->Cell(x + 1, y);
			//printf("(%4d, %3d) [%03u,%03u,%03u]-[%03u,%03u,%03u] bottom %u\n", x * scale, y * scale,
			//	(unsigned)current[0], (unsigned)current[1], (unsigned)current[2],
			//	(unsigned)bottom[0], (unsigned)bottom[1], (unsigned)bottom[2], distance);

		}
		//printf("\n");
	}
}
*/
/*
void TEGBSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {
	unsigned char cellColor[] = {0xFF, 0x80, 0x80};

	for (int x = 0; x < DistancesRight.Width; x++) {
		for(int y = 0; y < DistancesRight.Height; y++) {
			unsigned int distance = *DistancesRight.Cell(x,y);
			unsigned char color[3];
			color[0] = distance > 5 ? 255 : 50;
			color[1] = color[0];
			color[2] = color[0];
			image->DrawPointer((x + 1) * Scale, y * Scale + Scale * 0.5, 1, color);

			image->DrawPointer(x * Scale + Scale * 0.5, y * Scale + Scale * 0.5, 1, (unsigned char*)Image->Cell(x, y));
			image->DrawPointer(x * Scale, y * Scale, 1, cellColor);

			//unsigned char* current = Image->Cell(x, y);
			//unsigned char* right = Image->Cell(x + 1, y);
			//printf("(%4d, %3d) [%03u,%03u,%03u]-[%03u,%03u,%03u] right %u\n", x * scale, y * scale,
			//	(unsigned)current[0], (unsigned)current[1], (unsigned)current[2],
			//	(unsigned)right[0], (unsigned)right[1], (unsigned)right[2], distance);

		}
		//printf("\n");
	}

	for (int x = 0; x < DistancesBottom.Width; x++) {
		for(int y = 0; y < DistancesBottom.Height; y++) {
			unsigned int distance = *DistancesBottom.Cell(x,y);
			unsigned char color[3];
			color[0] = distance > 5 ? 255 : 50;
			color[1] = color[0];
			color[2] = color[0];
			image->DrawPointer(x * Scale + Scale * 0.5, (y + 1) * Scale, 1, color);

			//unsigned char* current = Image->Cell(x, y);
			//unsigned char* bottom = Image->Cell(x + 1, y);
			//printf("(%4d, %3d) [%03u,%03u,%03u]-[%03u,%03u,%03u] bottom %u\n", x * scale, y * scale,
			//	(unsigned)current[0], (unsigned)current[1], (unsigned)current[2],
			//	(unsigned)bottom[0], (unsigned)bottom[1], (unsigned)bottom[2], distance);

		}
		//printf("\n");
	}
}
*/
