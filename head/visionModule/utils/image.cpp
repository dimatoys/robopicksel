#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits>

#include "image.h"
/*
void TImage::SaveDump(const char* fileName) {
    FILE *outfile = fopen( fileName, "wb");
    if (outfile) {
    	int length = Width * Height * Depth;
        fwrite(&Width, 1, sizeof(int), outfile);
        fwrite(&Height, 1, sizeof(int), outfile);
        fwrite(&Depth, 1, sizeof(int), outfile);
        fwrite(Image, 1, length, outfile);
        fclose( outfile );
    }
}

long TImage::GetDeviation() {
	int size = Width * Height;
	unsigned long mean[Depth];
	memset(mean, 0, sizeof(mean));
	unsigned long sqxsum = 0;
	unsigned char* ptr = Image;
	for (int i = 0; i < size; i++) {
		for (int j = 0; j < Depth; j++) {
			long value = *ptr++;
			mean[j] += value;
			sqxsum += value * value;
		}
	}

	long meansq = 0;
	for (int j = 0; j < Depth; j++) {
		meansq += mean[j] * mean[j];
	}

	return (sqxsum - meansq / size) / size;
}
*/
void TMutableRGBImage::ConvertToYUV() {
	int ids = Width * Height;
	for (int i = 0; i < ids; i++) {
		unsigned char* pixel = Cell(i);
		double R = pixel[0];
		double G = pixel[1];
		double B = pixel[2];
		pixel[0] = (unsigned char)(0.299 * R + 0.587 * G + 0.114 * B);
		pixel[1] = (unsigned char)(-0.14713 * R - 0.28886 * G + 0.436 * B + 128);
		pixel[2] = (unsigned char)(0.615 * R - 0.51499 * G - 0.10001 * B + 128);
	}
	//ColorSpace = JCS_YCbCr;
}

void ReverseMatrix(int n, double* matrix, double* inv){

	for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j)
                inv[i * n + j] = 1.0;
            else
                inv[i * n + j] = 0.0;
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j) {
                double ratio = matrix[j * n + i] / matrix[i * n + i];
                for (int k = 0; k < n; k++) {
                    matrix[j * n + k] -= ratio * matrix[i * n + k];
                }
                for (int k = 0; k < n; k++) {
                    inv[j * n + k] -= ratio * inv[i * n + k];
                }
            }
        }
    }
    for (int i = 0; i < n; i++) {
        double a = matrix[i * n + i];
        for (int j = 0; j < n; j++) {
            matrix[i * n + j] /= a;
        }
        for (int j = 0; j < n; j++) {
            inv[i * n + j] /= a;
        }
    }
}

double f(int x, int y, int i) {
	//printf("%d,%d %d\n", x, y, i);
	switch(i) {
	case 0:
		return 1;
	case 1:
		return x;
	case 2:
		return y;
	case 3:
		return x * x;
	case 4:
		return y * y;
	case 5:
		return x * y;
	case 6:
		return x * x * x;
	case 7:
		return y * y * y;
	case 8:
		return x * x * y;
	case 9:
		return x * y * y;
	case 10:
		return x * x * x * x;
	case 11:
		return y * y * y * y;
	case 12:
		return x * x * y * y;
	case 13:
		return x * x * x * y;
	case 14:
		return x * y * y * y;
	}
	return 0;
}

/*
 counts polynom components values:
 s - max power
 d - dimension
 x - vector of X
 r - result
 */

double* fv(unsigned char s, unsigned char d, const double* x, double* r) {
    *r++ = 1.0;
    
    if(s > 0) {
        const double* cx = x;
        for (unsigned char i = 0; i < d; ++i) {
            *r++ = *cx++;
        }
        
        if (s > 1) {
            for (unsigned char i = 0; i < d; ++i) {
                for (unsigned char j = i; j< d; ++j) {
                    *r++ = x[i] * x[j];
                }
            }
            
            if (s > 2) {
                for (unsigned char i = 0; i < d; ++i) {
                    for (unsigned char j = i; j< d; ++j) {
                        for (unsigned char k = j; k < d; ++k) {
                            *r++ = x[i] * x[j] * x[k];
                        }
                    }
                }
            }
        }
    }
    
    return r;
}


double b(int k, int i, int width, int height) {
	double b = 0;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			b += f(x, y, i) * f(x, y, k);
			//printf("[%d,%d]=%lf\n", x, y, b);
		}
	}
	return b;
}

void MakeRegressionMatrix(TMutableImage<double>* regressionMatrix) {

	double bm[regressionMatrix->Depth * regressionMatrix->Depth];
	for (int k = 0; k < regressionMatrix->Depth; k++) {
		for (int i = 0; i < regressionMatrix->Depth; i++) {
			bm[k * regressionMatrix->Depth + i] = b(k, i, regressionMatrix->Width, regressionMatrix->Height);
		}
	}

	double inv[regressionMatrix->Depth * regressionMatrix->Depth];
	ReverseMatrix(regressionMatrix->Depth, bm, inv);

	double* rm = regressionMatrix->Cell(0,0);
	for (int k = 0; k < regressionMatrix->Depth; k++) {
		for (int y = 0; y < regressionMatrix->Height; y++) {
			for (int x = 0; x < regressionMatrix->Width; x++) {
				double v = 0;
				for (int i = 0; i < regressionMatrix->Depth; i++) {
					v += f(x, y, i) * inv[k * regressionMatrix->Depth + i];
				}
				*rm++ = v;
			}
		}
	}
}

bool ILearningDataSource::NextRecord(double* values) {
    if (NextRecord()) {
        for(unsigned char i = 0; i < D; ++i) {
            *values++ = NextElement();
        }
        return true;
    }
    return false;
}

void ILearningDataSource::ReadAll(double* values) {
    while(NextRecord()) {
        for(unsigned char i = 0; i < D; ++i) {
            *values++ = NextElement();
        }
    }
}

unsigned int POLY_SIZE[][10] = {{1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
                                {1, 3, 6, 10, 15, 21, 28, 36, 45, 55},
                                {1, 4, 10, 20, 35, 56, 84, 120, 165, 220},
                                {1, 5, 15, 35, 70, 126, 210, 330, 495, 715},
                                {1, 6, 21, 56, 126, 252, 462, 792, 1287, 2002},
                                {1, 7, 28, 84, 210, 462, 924, 1716, 3003, 5005},
                                {1, 8, 36, 120, 330, 792, 1716, 3432, 6435, 11440},
                                {1, 9, 45, 165, 495, 1287, 3003, 6435, 12870, 24310},
                                {1, 10, 55, 220, 715, 2002, 5005, 11440, 24310, 48620},
                                {1, 11, 66, 286, 1001, 3003, 8008, 19448, 43758, 92378},
                                {1, 12, 78, 364, 1365, 4368, 12376, 31824, 75582, 167960},
                                {1, 13, 91, 455, 1820, 6188, 18564, 50388, 125970, 293930},
                                {1, 14, 105, 560, 2380, 8568, 27132, 77520, 203490, 497420},
                                {1, 15, 120, 680, 3060, 11628, 38760, 116280, 319770, 817190},
                                {1, 16, 136, 816, 3876, 15504, 54264, 170544, 490314, 1307504},
                                {1, 17, 153, 969, 4845, 20349, 74613, 245157, 735471, 2042975},
                                {1, 18, 171, 1140, 5985, 26334, 100947, 346104, 1081575, 3124550},
                                {1, 19, 190, 1330, 7315, 33649, 134596, 480700, 1562275, 4686825},
                                {1, 20, 210, 1540, 8855, 42504, 177100, 657800, 2220075, 6906900}};

bool TPolyRegression::GenerateMX(ILearningDataSource* data) {
    
    if (MX != NULL) {
        delete MX;
        MX = NULL;
    }
    
    unsigned int size = POLY_SIZE[data->D - 1][S];
    unsigned int samples = data->GetSize();

    //printf("samples=%u size=%u\n", samples, size);
    
    if (samples < size) {
        return false;
    }
    
    XD = data->D;
    XS = size;
        
    double t[XS * samples];
    double* pt = t;
    double record[XD];
    while(data->NextRecord(record)) {
        pt = fv(S, XD, record, pt);
    }
    
    //printf("%f %f\n%f %f\n", t[0], t[1], t[2], t[3]);
    
    double t2[XS * XS];
    double* pt2 = t2;
    
    for (unsigned int y = 0; y < XS; ++y) {
        for (unsigned int x = 0; x < XS; ++x){
            double v = 0;
            for (unsigned int i = 0; i < samples; ++i) {
                v += t[y + i * XS] * t[x + i * XS];
            }
            *pt2++ = v;
            //printf("[%u,%u] %f\n", x,y,v);
        }
    }
    
    double t1[XS * XS];
    
    ReverseMatrix(XS, t2, t1);

    //printf("Rev: %f %f\n%f %f\n", t1[0], t1[1], t1[2], t1[3]);

    
    MX = new double[size * samples];
    double* pmx = MX;
    
    for (unsigned int y = 0; y < size; ++y) {
        for (unsigned int x = 0; x < samples; ++x){
            double v = 0;
            for (unsigned int i = 0; i < size; ++i) {
                v += t1[i + size * y] * t[i + size * x];
            }
            *pmx++ = v;
            //printf("[%d,%d] %f\n", x,y,v);
        }
    }

    return true;
}

void TPolyRegression::NewY(ILearningDataSource* datay) {


    if (R != NULL) {
        delete R;
    }
    unsigned int samples = datay->GetSize();
    YD = datay->D;

    R = new double[XS * YD];
    double* pr = R;
    
    double y[YD * samples];
    datay->ReadAll(y);
    //printf("y: %f %f\n", y[0], y[1]);
    
    for (unsigned int x = 0; x < XS; ++x) {
        for (unsigned int iy = 0; iy < YD; ++iy) {
            double v = 0;
            for (unsigned int i = 0; i < samples; ++i) {
                v += MX[i + x * samples] * y[i * YD + iy];
            }
            *pr++ = v;
            //printf("v=%f\n", v);
        }
    }
}

bool TPolyRegression::Learn(ILearningDataSource* x, ILearningDataSource* y) {
    if (GenerateMX(x)) {
        NewY(y);
        return true;
    }
    return false;
}

void TPolyRegression::PrepareX(const double* x) {
    if (PX != NULL) {
        delete PX;
    }
    PX = new double[XS];
    
    fv(S, XD, x, PX);
}

void TPolyRegression::Predict(double* y) {
    for (unsigned int i = 0; i < YD; ++i) {
        double v = 0;
        for (unsigned int j = 0; j < XS; ++j) {
            v+= PX[j] * R[j* YD + i];
        }
        *y++ = v;
    }
}

void TPolyRegression::GetValue(const double* x, double* y) {
    PrepareX(x);
    Predict(y);
}

void TPolyRegression::SetR(const double* r, unsigned int xd, unsigned int yd) {
    XD = xd;
    XS = POLY_SIZE[XD - 1][S];
    YD = yd;
    
    if (R != NULL) {
        delete R;
    }
    
    R = new double[XS * YD];
    
    memcpy(R, r, XS * YD * sizeof(double));
}

double DoublesLearningDatasource::NextElement() {
	return Data[Position++];
}

bool DoublesLearningDatasource::NextRecord() {
	return Data.size() < Position * D;
}

void DoublesLearningDatasource::Add(double element) {
	Data.push_back(element);
}

TLearningImage::Label TLearningImage::GetLabel(int x, int y) {
	int r2 = sqrt((x - X) * (x - X) + (y - Y) * (y - Y));
	if (r2 <= RIn) {
		return OBJECT;
	} else {
		if (r2 >= ROut) {
			return BACKGROUND;
		} else {
			return UNKNOWN;
		}
	}

}

void TLearningImage::Test(const char* file) {
	TMutableRGBImage image(Path);
	int rin2 = RIn * RIn;
	int rout2 = ROut * ROut;
	
	unsigned char in[] = {255, 255, 255};
	unsigned char out[] = {0, 0, 0};
	
	for (int y = 0; y < image.Height; y += 5) {
		for (int x = 0; x < image.Width; x += 5) {
			switch(GetLabel(x, y)) {
			case OBJECT:
				image.DrawPointer(x, y, 2, in);
				break;
			case BACKGROUND:
				image.DrawPointer(x, y, 2, out);
				break;
			}
		}
	}
	image.SaveJpg(file);
}

double countDistance(const unsigned char* color1, const unsigned char* color2) {
	double result = 0.0;
	for (int i = 0; i < 3; ++i) {
		double d = color1[i] - (double)color2[i];
		result += d * d;
	}
	return result;
}

double ILearningIterator::CountDistance(const unsigned char* color,
										TLearningImage::Label& label) {
	Reset();
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	double result = 0;
	while((ecolor = Next(elabel))) {
		if (elabel = label) {
			result += countDistance(color, ecolor);
		}
	}
	return result;
}

bool ILearningIterator::GetAverage(TLearningImage::Label& label,
								   unsigned char* avgcolor,
								   unsigned char* mincolor,
								   unsigned char* maxcolor) {
	double sumcolor[3];
	for (int i = 0; i < 3; ++i) {
		sumcolor[i] = 0;
		mincolor[i] = 255;
		maxcolor[i] = 0;
	}
	Reset();
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	unsigned int n = 0;
	while((ecolor = Next(elabel))) {
		if (elabel = label) {
			for (int i = 0; i < 3; ++i) {
				sumcolor[i] += ecolor[i];
				if (ecolor[i] < mincolor[i]) {
					mincolor[i] = ecolor[i];
				}
				if (ecolor[i] > maxcolor[i]) {
					maxcolor[i] = ecolor[i];
				}
			}
			++n;
		}
	}
	if (n > 0) {
		for (int i = 0; i < 3; ++i) {
			avgcolor[i] = sumcolor[i] / n;
		}
		return true;
	} else {
		return false;
	}
}

void TLearningImageIterator::Reset() {
	X = -1;
	Y = 0;
}

void TLearningImageIterator::Init(TLearningImage* image) {
	Dump.LoadDump(image->Path);
	Data = image;
	Reset();
}

const unsigned char* TLearningImageIterator::Next(TLearningImage::Label& label) {
	if (++X >= Dump.Width) {
		if (++Y >= Dump.Height) {
			return NULL;
		}
		X = 0;
	}
	label = Data->GetLabel(X, Y);
	return Dump.Cell(X, Y);
}

void TImagesLearningDataSource::AddImage(TLearningImage& image){
	Images.push_back(image);
}

void TImagesLearningDataSource::Reset() {
	ImgsIt = Images.begin();
	if (ImgsIt != Images.end()) {
		LIIt.Init(&*ImgsIt);
	}
}

const unsigned char* TImagesLearningDataSource::Next(TLearningImage::Label& label) {
	const unsigned char* result = LIIt.Next(label);
	if (result == NULL) {
		if (ImgsIt != Images.end()) {
			LIIt.Init(&*ImgsIt);
			result = LIIt.Next(label);
		}
	}
	return result;
}

bool fullIteration(TImagesLearningDataSource& images,
				   TLearningImage::Label& label,
				   unsigned char* color) {
	unsigned char avgcolor[3];
	unsigned char mincolor[3];
	unsigned char maxcolor[3];
	if (images.GetAverage(label, avgcolor, mincolor, maxcolor)) {
		printf("fullIteration:(%u,%u,%u)-(%u,%u,%u)\n", (unsigned int)mincolor[0], (unsigned int)mincolor[1], (unsigned int)mincolor[2],
			(unsigned int)maxcolor[0], (unsigned int)maxcolor[1], (unsigned int)maxcolor[2]);
		unsigned char rgb[3];
		double min = std::numeric_limits<double>::max();
		for (rgb[0] = mincolor[0]; rgb[0] <= maxcolor[0]; rgb[0]++) {
			for (rgb[1] = mincolor[1]; rgb[1] <= maxcolor[1]; rgb[1]++) {
				for (rgb[2] = mincolor[2]; rgb[2] <= maxcolor[2]; rgb[2]++) {
					double d = imges.CountDistance(rgb, label);
					if (d < min) {
						min = d;
						memcpy(color, rgb, 3);
					}
				}
			}
		}
		return true;
	} else {
		return false;
	}
}

bool gradientBoost(TImagesLearningDataSource& images,
				   TLearningImage::Label& label,
				   unsigned char* color) {

	unsigned char mincolor[3];
	unsigned char maxcolor[3];
	if (images.GetAverage(label, color, mincolor, maxcolor)) {
		double d = images.CountDistance(label, color);
		return true;
	} else {
		return false;
	}
}

unsigned int countErrors(TImagesLearningDataSource& images, const unsigned char* color, double d) {
	Reset();
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	unsigned int errors = 0;
	while((ecolor = Next(elabel))) {
		if ((elabel == BACKGROUND) || (elabel == OBJECT)) {
			countDistance(color, ecolor);
			// TODO
		}
	}
}

unsigned int getOptimalDistance(TImagesLearningDataSource& images, const unsigned char* color) {
	unsigned char avgcolor[3];
	unsigned char mincolor[3];
	unsigned char maxcolor[3];
	images.GetAverage(TLearningImage::OBJECT, avgcolor, mincolor, maxcolor);
	double d = countDistance(color, avgcolor);
}
