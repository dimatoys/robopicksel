#include "statimg.h"

class TStatImgSegmentsExtractor : public TSegmentsExtractor {
	TStatImgExtractorFactory* Parameters;
	TMutableImage<unsigned char>* Image;

	TMutableImage<unsigned short> AnomalyMatrix;
    double* A;
    int ASize;
    
    double GetPixelAnomaly(int x, int y);
    void MakeSmoothing();
	void MakeAnomalyMatrix(int& biggestCoreIdx);
	double GetAreaAnomaly(int ax, int ay);
	void ExtractObjectFromCore(TArea* area, int coreIdx);
    void DetectObjectType(TArea* area, int coreIdx);
    void DrawAnomalies(TMutableRGBImage* image);
    void DrawAnomalies2(TMutableRGBImage* image);

public:
	TStatImgSegmentsExtractor(TStatImgExtractorFactory* parameters,
	                          TMutableImage<unsigned char>* image) :
	    AnomalyMatrix(image->Width / parameters->AreaCell, image->Height / parameters-> AreaCell, 1) {
		Parameters = parameters;
		Image = image;
        A = NULL;
	}

	void ExtractSegments(std::list<TArea>& area);
	void DrawDebugInfo(TMutableRGBImage* image);

	~TStatImgSegmentsExtractor(){
		if (A != NULL) {
			delete A;
		}
	}

};

TSegmentsExtractor* TStatImgExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	return new TStatImgSegmentsExtractor(this, image);
}

inline double f6(double* a, int x, int y) {
	return a[0] + x * (a[1] + x * a[3] + y * a[5]) + y * (a[2] + y * a[4]);
}

double TStatImgSegmentsExtractor::GetPixelAnomaly(int x, int y) {
    double* a = A;
    int n = Parameters->RegressionMatrix.Depth;
    double anomalies = 0;
	for (int c = 0; c < Image->Depth; c++) {
        double v = Image->Cell(x,y)[c] - f6(a, x, y);
        a += n;
		anomalies += v * v;
	}
    return anomalies;
}

double TStatImgSegmentsExtractor::GetAreaAnomaly(int ax, int ay) {
	double anomalies = 0;
	int areaCell = Parameters-> AreaCell;
	for (int yc = 0; yc < areaCell; yc++) {
		for (int xc = 0; xc < areaCell; xc++) {
			int x = ax * areaCell + xc;
			int y = ay * areaCell + yc;
            anomalies += GetPixelAnomaly(x, y);
		}
	}
	return anomalies / areaCell / areaCell;
}

void TStatImgSegmentsExtractor::MakeSmoothing() {

	int size = Image->Depth * Parameters->RegressionMatrix.Depth;
	if (A == NULL) {
		ASize = size;
		A = new double[ASize];
	} else {
		if (ASize != size) {
			delete A;
			ASize = size;
			A = new double[ASize];
		}
	}

	int n = Parameters->RegressionMatrix.Depth;
	double* rm = Parameters->RegressionMatrix.Cell(0, 0);
    double* a = A;
	for (int c = 0; c < Image->Depth; c++) {
		double* rm_ptr = rm;
		for (int k = 0; k < n; k++) {
			double v = 0;
			for (int y = 0; y < Image->Height; y++) {
				for (int x = 0; x < Image->Width; x++) {
					v += Image->Cell(x, y)[c] * *rm_ptr++;
				}
			}
            *a++ = v;
		}
	}
}

void TStatImgSegmentsExtractor::MakeAnomalyMatrix(int& biggestCoreIdx) {

	MakeSmoothing();

	//double threshold = Parameters->AnomalyThreshold * Parameters->AnomalyThreshold * 65536 * Image->Depth  * Parameters-> AreaCell * Parameters-> AreaCell;
	double threshold = Parameters->AnomalyThreshold * Parameters->AnomalyThreshold * 65536 * Image->Depth;
	unsigned short yline[AnomalyMatrix.Width];
	memset(yline, 0, AnomalyMatrix.Width * sizeof(unsigned short));
	biggestCoreIdx = -1;
	unsigned short biggestCoreSize = Parameters->MinCoreSize - 1;
	for(int y0 = 0; y0 < AnomalyMatrix.Height; y0++) {
		unsigned short xline = 0;
		for(int x0 = 0; x0 < AnomalyMatrix.Width; x0++) {
			/*
			double anomalies = 0;
			for (int yc = 0; yc < Parameters-> AreaCell; yc++) {
				for (int xc = 0; xc < Parameters-> AreaCell; xc++) {
					int x = x0 * Parameters-> AreaCell + xc;
					int y = y0 * Parameters-> AreaCell + yc;
                    anomalies += GetPixelAnomaly(x, y);
				}
			}
			*/
			double anomalies = GetAreaAnomaly(x0, y0);
			if (anomalies > threshold) {
				++xline;
				unsigned short ylinev = ++yline[x0];
				unsigned short core = 1;
				if ((x0 > 0) && (y0 > 0)) {
					core += *AnomalyMatrix.Cell(x0 - 1, y0 - 1);
					if (core > xline) {
						core = xline;
					}
					if (core > ylinev) {
						core  = ylinev;
					}
					if (core > biggestCoreSize) {
						biggestCoreSize = core;
						biggestCoreIdx = AnomalyMatrix.Idx(x0, y0);
					}
				}
				*AnomalyMatrix.Cell(x0, y0) = core;
                //printf("MakeAnomalyMatrix: (%d,%d) = %d\n", x0, y0, core);
			} else {
				xline = 0;
				yline[x0] = 0;
				*AnomalyMatrix.Cell(x0, y0) = 0;
			}
		}
	}
}

void TStatImgSegmentsExtractor::ExtractObjectFromCore(TArea* area, int coreIdx) {
	int x0, y0;
	AnomalyMatrix.IdxToXY(coreIdx, x0, y0);
	unsigned short core = *AnomalyMatrix.Cell(coreIdx);
	int xmax = x0;
	int ymax = y0;
	int xmin = xmax - core + 1;
	int ymin = ymax - core + 1;
	int size = core * core;

	for (int i = 0; i < core; i++) {
		int x, y;
		for (x = x0 + 1; x < AnomalyMatrix.Width; ++x) {
			if (*AnomalyMatrix.Cell(x, y0 - i) == 0) {
				break;
			}
			++size;
		}
		if (x - 1 > xmax) {
			xmax = x - 1;
		}

		for (x = x0 - core ; x >= 0; --x) {
			if (*AnomalyMatrix.Cell(x, y0 - i) == 0) {
				break;
			}
			++size;
		}
		if (x + 1 < xmin) {
			xmin = x + 1;
		}

		for (y = y0 + 1; y < AnomalyMatrix.Height; ++y) {
			if (*AnomalyMatrix.Cell(x0 - i, y) == 0) {
				break;
			}
			++size;
		}
		if (y - 1 > ymax) {
			ymax = y - 1;
		}

		for (y = y0 - core; y >= 0; --y) {
			if (*AnomalyMatrix.Cell(x0 - i, y) == 0) {
				break;
			}
			++size;
		}
		if (y + 1 < ymin) {
			ymin = y + 1;
		}
	}

	area->MinX = xmin * Parameters->AreaCell;
	area->MinY = ymin * Parameters->AreaCell;
	area->MaxX = (xmax + 1) * Parameters->AreaCell;
	area->MaxY = (ymax + 1) * Parameters->AreaCell;
	area->Size = size * Parameters->AreaCell * Parameters->AreaCell;
	area->AtBorder = (xmin == 0 ? BORDER_LEFT : 0) +
	                 (xmax == AnomalyMatrix.Width -1 ? BORDER_RIGHT : 0) +
	                 (ymin == 0 ? BORDER_TOP : 0) +
	                 (ymin == AnomalyMatrix.Height - 1 ? BORDER_BOTTOM : 0);
}

void TStatImgSegmentsExtractor::DetectObjectType(TArea* area, int coreIdx) {
    int x0, y0;
    AnomalyMatrix.IdxToXY(coreIdx, x0, y0);
    //printf("DetectObjectType: (x,y)=(%d,%d) core=%d area=%d\n", x0, y0, (int)*AnomalyMatrix.Cell(coreIdx), Parameters->AreaCell);
    x0 *= Parameters->AreaCell;
    y0 *= Parameters->AreaCell;
    unsigned short coreSize = (*AnomalyMatrix.Cell(coreIdx) - 1) * Parameters->AreaCell;
    //printf("DetectObjectType: (X,Y)=(%d,%d) size=%d\n", x0, y0, (int)coreSize);
    int redblue = 0;
    for (int i = 0; i < coreSize; i++) {
        //printf("DetectObjectType: i=%d (x,y) = (%d,%d)\n", i, x0 - i, y0 - i);
        unsigned char* cell = Image->Cell(x0 - i, y0 - i);
        for (int c = 0; c < Image->Depth; c++) {
             redblue += (cell[0] - (int)cell[2]);
        }
    }
    //printf("DetectObjectType: redblue=%d\n", redblue);
    area->ObjectType = redblue > 0 ? 1 : 2;
}

void TStatImgSegmentsExtractor::ExtractSegments(std::list<TArea>& areas) {

	
    if (Parameters->RegressionMatrix.ReAllocate(Image->Width, Image->Height, Parameters->RegressionLevel)) {
    	MakeRegressionMatrix(&Parameters->RegressionMatrix);
    }

    int biggestCoreIdx = -1;
    MakeAnomalyMatrix(biggestCoreIdx);
    if (biggestCoreIdx >= 0) {
	printf("ExtractSegments: Core size: %d\n", (int)*AnomalyMatrix.Cell(biggestCoreIdx));
	TArea area;
	ExtractObjectFromCore(&area, biggestCoreIdx);
        //printf("ExtractSegments: extracted\n");
        DetectObjectType(&area, biggestCoreIdx);
        //printf("ExtractSegments: detected\n");
	areas.push_back(area);
    }
}

void TStatImgSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image) {
    DrawAnomalies2(image);
}

void TStatImgSegmentsExtractor::DrawAnomalies2(TMutableRGBImage* image) {
/*
	double min = 1e10;
    double max = 0;
    
    for (int y = 0; y < image->Height; y++) {
		for (int x =0; x < image->Width; x++) {
            double anomaly = GetPixelAnomaly(x, y);
            if (anomaly < min) {
                min = anomaly;
            }
            if (anomaly > max) {
                max = anomaly;
            }
        }
    }
    printf("anomaly: min=%f max=%f\n", min, max);
*/
    const int max = 20000;
    const int bars = 50;
    const int interval = max / bars;
    int histogram[bars];
    memset(histogram, 0, bars * sizeof(int));
    for (int y = 0; y < image->Height; y++) {
		for (int x =0; x < image->Width; x++) {
            double anomaly = GetPixelAnomaly(x, y);
            int box = (int)(anomaly / interval);
            if (box >= bars) {
                box = bars - 1;
            }
            histogram[box]++;
        }
    }

    for (int  i = 0 ; i < bars - 1; i++) {
        printf("%u-%u: %u\n", i * interval, (i + 1) * interval, histogram[i]);
    }
    printf("%u-*****: %u\n", (bars - 1) * interval, histogram[bars - 1]);
    
    for (int y = 0; y < image->Height; y++) {
		for (int x =0; x < image->Width; x++) {
            //unsigned char ar = (unsigned char)(255 * (GetPixelAnomaly(x, y) - min) / (max - min));
            //unsigned int ar = (unsigned int)GetPixelAnomaly(x, y) * 255 / 21000;
            //if (ar > 255) {
            //    ar = 255;
            //}

            unsigned char ar = GetPixelAnomaly(x, y) > 3500 ? 255 : 0;

            image->Cell(x, y)[0] = ar;
            image->Cell(x, y)[1] = ar;
            image->Cell(x, y)[2] = ar;
        }
    }
}

void TStatImgSegmentsExtractor::DrawAnomalies(TMutableRGBImage* image) {

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
	unsigned char* colorObj = colors[0];

	//printf("DebugInfo: w=%d h=%d\n", image->Width, image->Height);

	for (int y = 0; y < AnomalyMatrix.Height; y++) {
		for (int x =0; x < AnomalyMatrix.Width; x++) {
			//printf("(%d,%d) %d\n", x, y, a);
			if (*AnomalyMatrix.Cell(x, y) > 0) {
				image->DrawPointer(x * Parameters->AreaCell + Parameters->AreaCell / 2, y * Parameters->AreaCell + Parameters->AreaCell / 2, 4, colors[*AnomalyMatrix.Cell(x, y) % 16]);
			}
		}
	}

}
