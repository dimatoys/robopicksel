#include <stdio.h>
#include <string.h>

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

