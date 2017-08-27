#include "kp.h"

#include <math.h>

class TKeyPointsSegmentsExtractor : public TSegmentsExtractor {
	TKeyPointsExtractorFactory* Parameters;
        TMutableImage<unsigned char>* Image;

public:
	TKeyPointsSegmentsExtractor(TKeyPointsExtractorFactory* parameters,
	                            TMutableImage<unsigned char>* image) {
		Parameters = parameters;
		Image = image;
	}

	void ExtractSegments(std::list<TArea>& area);
	void DrawDebugInfo(TMutableRGBImage* image);

	~TKeyPointsSegmentsExtractor(){}
};

TSegmentsExtractor* TKeyPointsExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
	return new TKeyPointsSegmentsExtractor(this, image);
}

void PrintGaussian(int size) {
    
/*    
    double* rm_ptr = regressionMatrix.Cell(0, 0);
    for (int k = 0; k < depth; k++) {
        double sum = 0.0;
	for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                sum += *rm_ptr;
		printf(" %lf", *rm_ptr++);
            }
            printf("\n");
        }
	printf("sum = %lf\n\n", sum);
    }
*/
}


void TKeyPointsSegmentsExtractor::ExtractSegments(std::list<TArea>& area){
    int size = Parameters->GaussianSize;
    double gaussian[size][size];
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            gaussian[x][y] = exp(-(x * x + y * y) * 0.5) * sqrt(x * x + y * y);
            printf(" %lf", gaussian[x][y]);
        }
        printf("\n");
    }
    printf("\n\n");

    TMutableImage<double> xdiv(Image->Width, Image->Height, Image->Depth);
    TMutableImage<double> ydiv(Image->Width, Image->Height, Image->Depth);
    
    int maxx = Image->Width - size;
    int maxy = Image->Height - size;
    for (int x = size; x < maxx; x++) {
        for (int y = size; y < maxy; y++) {
            for(int c = 0; c < Image->Depth; c++) {
                double vx = 0.0;
                double vy = 0.0;
                for (int ca = 1; ca <= size; ca++) {
                    for (int cp = -size; cp <= size; cp++) {
                        double g = gaussian[ca][cp >= 0 ? cp : -cp];
                        vx += (Image->Cell(x + ca, y + cp)[c] - Image->Cell(x - ca, y - cp)[c]) * g;
                        vy += (Image->Cell(x + cp, y + ca)[c] - Image->Cell(x - cp, y - ca)[c]) * g;
                    }
                }
                xdiv.Cell(x, y)[c] = vx;
                ydiv.Cell(x, y)[c] = vy;
            }
        }
    }
 }

void TKeyPointsSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image){
    
}
