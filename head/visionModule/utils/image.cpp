#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits>
#include <algorithm>

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

    Reset();
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

    printf("samples=%u size=%u D=%u S=%u\n", samples, size, data->D, (unsigned int)S);
    
    if (samples < size) {
        return false;
    }

    XD = data->D;
    XS = size;

    double t[XS * samples];
    double* pt = t;
    double record[XD];
    data->Reset();
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

void TPolyRegression::Predict(double* px, double* y) {
    for (unsigned int i = 0; i < YD; ++i) {
        double v = 0;
        for (unsigned int j = 0; j < XS; ++j) {
            v+= px[j] * R[j* YD + i];
        }
        *y++ = v;
    }
}

void TPolyRegression::GetValue(const double* x, double* y) {
    //PrepareX(x);
    double px[XS];
    fv(S, XD, x, px);
    Predict(px, y);
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

int countDistance(const TRGB<unsigned char>& color1, const unsigned char* color2) {
	int result = 0.0;
	for (int i = 0; i < 3; ++i) {
		int d = color1.RGB[i] - (int)color2[i];
		result += d * d;
	}
	return result;
}

int countDistance(const TRGB<unsigned char>& color1, const TRGB<unsigned char>& color2) {
	int result = 0.0;
	for (int i = 0; i < 3; ++i) {
		int d = color1.RGB[i] - (int)color2.RGB[i];
		result += d * d;
	}
	return result;
}

int countDistance(int r, int g, int b, const unsigned char* color2) {
	int dr = (r - color2[0]);
	int dg = (g - color2[1]);
	int db = (b - color2[2]);
	return dr * dr + dg * dg + db * db;
}

int countDistance(const double* color1, const unsigned char* color2) {
	int d = 0;
	for (int i = 0; i < 3; ++i) {
		int sd = (int)color1[i] - (int)color2[i];
		d += sd * sd;
	}
	return d;
}

double ILearningIterator::CountDistance(TLearningImage::Label& label,
										 const TRGB<unsigned char>& color) {
	Reset();
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	double result = 0;
	int x,y;
	while((ecolor = Next(elabel, x, y))) {
		if (elabel = label) {
			result += countDistance(color, ecolor);
		}
	}
	return result;
}

bool ILearningIterator::GetAverage(TLearningImage::Label label,
								   TRGB<unsigned char>& avgcolor,
								   TRGB<unsigned char>& mincolor,
								   TRGB<unsigned char>& maxcolor) {
	double sumcolor[3];
	for (int i = 0; i < 3; ++i) {
		sumcolor[i] = 0;
		mincolor.RGB[i] = 255;
		maxcolor.RGB[i] = 0;
	}
	Reset();
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	unsigned int n = 0;
	int x, y;
	//printf("GetAverage: begin iterate\n");
	while((ecolor = Next(elabel, x, y)) != NULL) {
		//printf("GetAverage: it n=%u label=%u elabel=%u\n", n, (int)label, (int)elabel);
		if (elabel == label) {
			for (int i = 0; i < 3; ++i) {
				sumcolor[i] += ecolor[i];
				if (ecolor[i] < mincolor.RGB[i]) {
					mincolor.RGB[i] = ecolor[i];
				}
				if (ecolor[i] > maxcolor.RGB[i]) {
					maxcolor.RGB[i] = ecolor[i];
				}
			}
			++n;
		}
	}
	//printf("GetAverage: n=%u\n", n);
	if (n > 0) {
		for (int i = 0; i < 3; ++i) {
			avgcolor.RGB[i] = sumcolor[i] / n;
		}
		return true;
	} else {
		return false;
	}
}

void TLearningImageIterator::Reset() {
	X = -1;
	Y = 0;
	if (!Dump.IsAllocated()) {
		Dump.LoadDump(Data.Path);
	}
}

const unsigned char* TLearningImageIterator::Next(TLearningImage::Label& label, int& x, int& y) {
	if (++X >= Dump.Width) {
		if (++Y >= Dump.Height) {
			return NULL;
		}
		X = 0;
	}
	label = Data.GetLabel(X, Y);
	x = X;
	y = Y;
	return Dump.Cell(X, Y);
}

void TImagesLearningDataSource::AddImage(TLearningImage& image){
	TLearningImageIterator it(image);
	Images.push_back(it);
}

void TImagesLearningDataSource::Reset() {
	ImgsIt = Images.begin();
	if (ImgsIt != Images.end()) {
		ImgsIt->Reset();
	}
}

const unsigned char* TImagesLearningDataSource::Next(TLearningImage::Label& label, int& x, int& y) {
	while(true) {
		const unsigned char* result = ImgsIt->Next(label, x, y);
		if (result != NULL) {
			return result;
		}
		if (++ImgsIt == Images.end()) {
			return NULL;
		}
		ImgsIt->Reset();
	}
	return NULL;
}

void TGradientBoost::MakeDistancesArray(int maxDistance) {
	TRGB<char> dir;
	for (dir.RGB[0] = -maxDistance; dir.RGB[0] <= maxDistance; ++dir.RGB[0]) {
		for (dir.RGB[1] = -maxDistance; dir.RGB[1] <= maxDistance; ++dir.RGB[1]) {
			for (dir.RGB[2] = -maxDistance; dir.RGB[2] <= maxDistance; ++dir.RGB[2]) {
				Distances.push_back(dir);
			}
		}
	}
	std::sort(Distances.begin(), Distances.end(), [](const TRGB<char>& a, const TRGB<char>& b) {
        return b.S2() < a.S2();   
    });

	int maxDistance2 = maxDistance * maxDistance;
	std::vector< TRGB<char> >::iterator it = Distances.begin();
	while (it != Distances.end()) {
		if (it->S2() > maxDistance2) {
			break;
		}
		++it;
	}
	Distances.erase(it, Distances.end());
}

void TGradientBoost::InitCache(const TRGB<unsigned char>& mincolor, const TRGB<unsigned char>& maxcolor) {
	Min0 = mincolor.RGB[0];
	Min1 = mincolor.RGB[1];
	Min2 = mincolor.RGB[2];
	Max0 = maxcolor.RGB[0];
	Max1 = maxcolor.RGB[1];
	Max2 = maxcolor.RGB[2];
	S1 = maxcolor.RGB[1] - mincolor.RGB[1] + 1;
	S2 = maxcolor.RGB[2] - mincolor.RGB[2] + 1;
	int size = (maxcolor.RGB[0] - mincolor.RGB[0] + 1) * S1 * S2;
	if (Cache != NULL) {
		delete Cache;
	}
	Cache = new unsigned char[size];
	memset(Cache, 0, size);
}

unsigned char& TGradientBoost::CacheValue(const TRGB<unsigned char>& rgb) {
	return Cache[((rgb.RGB[0] - Min0) * S1 + rgb.RGB[1] - Min1) * S2 + rgb.RGB[2] - Min2];
}

TGradientBoost::TGradientBoost(int maxDistance) {
	MakeDistancesArray(maxDistance);
	Cache = NULL;
}

bool TGradientBoost::Boost(TImagesLearningDataSource& images, TLearningImage::Label label) {
	TRGB<unsigned char> mincolor;
	TRGB<unsigned char> maxcolor;
	if (images.GetAverage(label, Color, mincolor, maxcolor)) {
		InitCache(mincolor, maxcolor);
		D = images.CountDistance(label, Color);
		std::vector< TRGB<char> >::const_iterator it = Distances.begin();
		TRGB<unsigned char> ccolor;
		while(it != Distances.end()) {
			int c0 = Color.RGB[0] + (int)it->RGB[0];
			if (c0 < Min0) {
				continue;
			}
			if (c0 > Max0) {
				continue;
			}
			int c1 = Color.RGB[1] + (int)it->RGB[1];
			if (c1 < Min1) {
				continue;
			}
			if (c1 > Max1) {
				continue;
			}
			int c2 = Color.RGB[2] + (int)it->RGB[2];
			if (c2 < Min2) {
				continue;
			}
			if (c2 > Max2) {
				continue;
			}
			ccolor.RGB[0] = (unsigned char)c0;
			ccolor.RGB[1] = (unsigned char)c1;
			ccolor.RGB[2] = (unsigned char)c2;
			if (CacheValue(ccolor) == 0) {
				double d = images.CountDistance(label, ccolor);
				CacheValue(ccolor) = 1;
				if (d <= D) {
					Color = ccolor;
					D = d;
					it = Distances.begin();
					continue;
				}
			}
			++it;
		}
		return true;
	}
	return false;
}

TGradientBoost::~TGradientBoost() {
	if (Cache != NULL) {
		delete Cache;
	}
}

unsigned int countErrors(TImagesLearningDataSource& images,
						 const TRGB<unsigned char>& color,
						 int d,
						 unsigned int& neg,
						 unsigned int& fp) {
	images.Reset();
	unsigned int pos = 0;
	neg = 0;
	fp = 0;
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	int x, y;
	unsigned int errors = 0;
	while((ecolor = images.Next(elabel, x, y))) {
		if ((elabel == TLearningImage::BACKGROUND) || (elabel == TLearningImage::OBJECT)) {
			if (countDistance(color, ecolor) < d) {
				if (elabel == TLearningImage::BACKGROUND) {
					++pos;
				} else {
					++fp;
				}
			} else {
				if (elabel == TLearningImage::OBJECT) {
					++pos;
				} else {
					++neg;
				}
			}
		}
	}
	return pos;
}

unsigned int countErrors(TImagesLearningDataSource& images,
						  TPolyRegression& pr,
						  int d,
						  unsigned int& neg,
						  unsigned int& fp) {
	images.Reset();
	unsigned int pos = 0;
	neg = 0;
	fp = 0;
	const unsigned char* ecolor;
	TLearningImage::Label elabel;
	int x, y;
	unsigned int errors = 0;
	double vx[2];
	double rgb[pr.YD];
	while((ecolor = images.Next(elabel, x, y))) {
		if ((elabel == TLearningImage::BACKGROUND) || (elabel == TLearningImage::OBJECT)) {
			vx[0] = x;
			vx[1] = y;
			pr.GetValue(vx, rgb);
			if (countDistance(rgb, ecolor) < d) {
				if (elabel == TLearningImage::BACKGROUND) {
					++pos;
				} else {
					++fp;
				}
			} else {
				if (elabel == TLearningImage::OBJECT) {
					++pos;
				} else {
					++neg;
				}
			}
		}
	}
	printf("countErrors: d=%d pos=%u\n", d, pos);
	return pos;
}

unsigned int getOptimalDistanceFast(TImagesLearningDataSource& images,
									const TRGB<unsigned char>& color,
									unsigned int splitParts) {
	TRGB<unsigned char> avgcolor;
	TRGB<unsigned char> mincolor;
	TRGB<unsigned char> maxcolor;
	unsigned int fp, neg;
	images.GetAverage(TLearningImage::OBJECT, avgcolor, mincolor, maxcolor);
	unsigned int bd = (unsigned int)countDistance(color, avgcolor) / 2;
	unsigned int area = bd;
	unsigned int pos = 0;
	while (area > 0) {
		unsigned int step = 2 * area / splitParts;
		if (step == 0) {
			step = 1;
		}
		//printf("area=%u step=%u\n", area, step);
		for (unsigned int td = bd - area; td <= bd + area; td += step) {
			unsigned int tpos = countErrors(images, color, td, neg, fp);
			if (tpos > pos) {
				bd = td;
				pos = tpos;
				//printf("new: d=%u pos=%u\n", bd,pos);
			}
		}
		area = 2 * area / splitParts;
	}
	return bd;
}

unsigned int getOptimalDistanceSlow(TImagesLearningDataSource& images,
								const TRGB<unsigned char>& color,
								int maxDistance) {
	TRGB<unsigned char> avgcolor;
	TRGB<unsigned char> mincolor;
	TRGB<unsigned char> maxcolor;
	unsigned int fp, neg;
	images.GetAverage(TLearningImage::OBJECT, avgcolor, mincolor, maxcolor);
	int d = countDistance(color, avgcolor) / 2;
	printf("getOptimalDistance: d=%d b=(%u,%u,%u) avg=(%u,%u,%u) md=%d\n", d, (unsigned int)color.RGB[0], (unsigned int)color.RGB[1], (unsigned int)color.RGB[2], 
		(unsigned int)avgcolor.RGB[0], (unsigned int)avgcolor.RGB[1], (unsigned int)avgcolor.RGB[2], maxDistance);
	int imin = d - maxDistance;
	int imax = d + maxDistance;
	unsigned int bd = -1;
	unsigned int pos = 0;
	for (int td = imin; td <= imax; ++td) {
		unsigned int tpos = countErrors(images, color, td, neg, fp);
		if (tpos > pos) {
			bd = td;
			pos = tpos;
		}
	}
	printf("getOptimalDistance: bd=%u pos=%u\n", bd, pos);
	int cd;
	int dir;
	if (bd > d) {
		cd = imax;
		dir = 1;
	} else {
		cd = imin;
		dir = -1;
	}
	while (cd - bd < maxDistance) {
		cd += dir;
		unsigned int tpos = countErrors(images, color, cd, neg, fp);
		if (tpos >= pos) {
			bd = cd;
			pos = tpos;
		}
	}
	return bd;
}

unsigned int countOptimalDistance(TImagesLearningDataSource& images, TPolyRegression& pr) {
	int d0 = 0;
	int step = 500;
	unsigned int fp, neg;

	unsigned int pos0 = countErrors(images, pr, d0, neg, fp);
	unsigned int pos1;
	while ((pos1 = countErrors(images, pr, d0 + step, neg, fp)) <= pos0 ) {
		step /= 2;
	}

	if (step == 0) {
		return d0;
	}

	while (true) {
		unsigned int pos2 = countErrors(images, pr, d0 + step * 2, neg, fp);
		if (pos2 < pos1) {
			while (true) {
				step /= 10;
				if (step > 0) {
					pos1 = countErrors(images, pr, d0 + step, neg, fp);
					if (pos1 > pos0) {
						break;
					}
				} else {
					return d0;
				}
			}
		} else {
			pos0 = pos1;
			d0 += step;
		}
	}
/*
	for (int d = 0; d < 10000; ++d) {
		pos = countErrors(images, pr, d, neg, fp);
		printf("%d,%u,%u,%u\n", d, pos, neg, fp);
	}
*/
}
