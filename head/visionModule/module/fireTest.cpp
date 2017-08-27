#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include <cvaux.h>

using namespace cv;
using namespace std;

#include "visionModule.h"

void TestVisionModule() {
	VisionData data;
	data.Width = 1280;
	data.Height = 720;
	data.Depth = 3;
	data.GradientScale = 10;
	data.GradientThreshold = 4500;
	data.GradientMinSize = 1000;
	data.FileName = "test1.jpg";
	data.DumpFileName = "../dumps/1453890388.dump";
	//data.DumpFileName = "../dumps/1438146364.dump";
	//data.DumpFileName = "../dumps/.dump";
	//data.DumpFileName = "../dumps/.dump";
	//data.DumpFileName = "../dumps/.dump";
	//data.DumpFileName = "../dumps/.dump";
	//data.DumpFileName = "../dumps/.dump";
#ifdef VISION_CLUSTER
	data.Border = "../head/cborder.dump";
#else
	data.Border = "../head/border.dump";
#endif
	printf("Init PiCamera\n");

	cameraStart(&data);

	cameraFire(&data);

	printf("status=%d\n", data.Status);

	cameraRelease(&data);
}

void TestGripperDetection() {

	unsigned int gripperThreshold = 130;
	const char* zeroImageFileName = "../dumps/1438146330.dump";
	int radius = 2;

	VisionData data;
	data.Width = 1280;
	data.Height = 720;
	data.Depth = 3;
	data.GradientScale = 10;
	data.Border = "../head/border.dump";

	TMutableImage<unsigned char> zeroImage(zeroImageFileName);

	if (zeroImage.Size() > 0) {
		TImage image(zeroImage.Cell(0), zeroImage.Width, zeroImage.Height, zeroImage.Depth);
		TMutableRGBImage smooth(&image/*, data.GradientScale, image.Width, image.Height*/);

		TMutableImage<char> borderImage = new TMutableImage<char>(smooth.Width - 2, smooth.Height - 2, 1);
		memset(borderImage.Cell(0), NO_BORDER, borderImage.Size());

		for (int x = 0; x < borderImage.Width; x++) {
			for (int y = 0; y < borderImage.Height; y++) {
				unsigned char* rgb = smooth.Cell(x + 1, y + 1);
				if ((unsigned int)rgb[0] + (unsigned int)rgb[1] + (unsigned int)rgb[2] < gripperThreshold) {
					for (int i = x - radius; i <= x + radius; i++) {
						for (int j = y - radius; j <= y + radius; j++) {
							if ((i >= 0) && (i < borderImage.Width) && (j >= 0) && (j < borderImage.Height)) {
								*borderImage.Cell(i,j) = 3;
							}
						}
					}
				}
			}
		}

		borderImage.SaveDump(data.Border);
	}
}

void TestMetric1(const char* dumpFile, const char* jpgFile) {
	TMutableRGBImage image(dumpFile);
	image.ConvertToYUV();

	int ids = image.Width * image.Height;
	for (int i = 0; i < ids; ++i) {
		image.Cell(i)[1] = 128;
		image.Cell(i)[2] = 128;
	}

	image.SaveJpg(jpgFile);
}

void CvsDump(const char* dumpFile, const char* testJpg) {
	TMutableImage<unsigned char> image(dumpFile);

	printf("R,G,B\n");
	for (int y = 0; y < image.Height; y++) {
		for(int x = 0; x < image.Width; x++) {
			printf("%u,%u,%u\n" , (unsigned)image.Cell(x,y)[0] , (unsigned)image.Cell(x,y)[1] , (unsigned)image.Cell(x,y)[2]);
		}
	}
}

void CvsDumpYUV(const char* dumpFile, const char* testJpg) {
	TMutableRGBImage image(dumpFile);
	image.ConvertToYUV();

	printf("Y,U,V\n");
	for (int y = 0; y < image.Height; y++) {
		for(int x = 0; x < image.Width; x++) {
			printf("%u,%u,%u\n" , (unsigned)image.Cell(x,y)[0] , (unsigned)image.Cell(x,y)[1] , (unsigned)image.Cell(x,y)[2]);
		}
	}
}

void Convert(const char* src, const char* dst) {
	TMutableRGBImage image(src);
	printf("image: %d %d\n", image.Width, image.Height);
	image.SaveJpg(dst);
}

void TestDetection(IplImage* src,
                   int depth,
                   int threshold1,
                   int threshold2) {
    IplImage* dst = cvCreateImage(cvGetSize(src), 8, 3);
    CvSeq* comp = NULL;
    CvMemStorage* storage = cvCreateMemStorage(0);

    cvPyrSegmentation(src, dst, storage, &comp, depth, threshold1, threshold2);

    int numObjects = 0;
    int numCorrect = 0;
    for (int i = 0; i < comp->total; i++) {
        CvConnectedComp* cc = (CvConnectedComp*) cvGetSeqElem(comp, i);

        int area = cc->rect.width * cc->rect.height;
        if ((area > 62000) && (area < 122500)) {
            numObjects++;
            if ((cc->rect.x >= 445) && (cc->rect.x <= 465) &&
                (cc->rect.y >= 163) && (cc->rect.y <= 214) &&
                (cc->rect.x + cc->rect.width >= 754) && (cc->rect.x + cc->rect.width <= 784) &&
                (cc->rect.y + cc->rect.height >= 478) && (cc->rect.y + cc->rect.height <= 514)) {
                    numCorrect++;
            }
        }
    }

    cvReleaseMemStorage( &storage );

    printf("%d %03d %03d: %d %d %s\n", depth, threshold1, threshold2, numObjects, numCorrect,
           (numCorrect > 0) && (numObjects == numCorrect) ? "***" : "");
}

void TestOpenCV1(const char* srcFileName) {
	IplImage* src=cvLoadImage(srcFileName);
	TestDetection(src, 2, 80, 90);
}

void TestOpenCV2(const char* srcFileName) {
	TMutableRGBImage image(srcFileName);
	IplImage* src= cvCreateImageHeader(cvSize(image.Width, image.Height), IPL_DEPTH_8U, 3);
	cvSetData( src, image.Cell(0,0), image.Width * 3 );
	//cvSaveImage("test.jpg", src);
	TestDetection(src, 2, 80, 90);
}

void TestPictureLighting(const char* fileName) {
	TMutableRGBImage image;
	image.LoadJpg(fileName);
	printf("TestPictureLighting: %d %d %d\n", image.Width, image.Height, image.Depth);

	int size = image.Width * image.Height;
	long mean[image.Depth];
	memset(mean, 0, sizeof(mean));
	long sqxsum = 0;
	for (int i = 0; i < size; i++) {
		unsigned char* cell = image.Cell(i);
		for (int j = 0; j < image.Depth; j++) {
			mean[j] += cell[j];
			sqxsum += cell[j] * (long)cell[j];
		}
	}

	long meansq = 0;
	for (int j = 0; j < image.Depth; j++) {
		meansq += mean[j] * mean[j];
		mean[j] = mean[j] / size;
	}

/*
	long sqsum = 0;
	for(int i = 0; i < size; i++) {
		unsigned char* cell = image.Cell(i);
		for (int j = 0; j < image.Depth; j++) {
			sqsum += (cell[j] - mean[j]) * (cell[j] - mean[j]);
		}
	}
*/
	TImage img(image.Cell(0), image.Width, image.Height, image.Depth);


	printf("%s mean=(%ld,%ld,%ld) dev=%ld dev2=%ld\n", fileName, mean[0], mean[1], mean[2], (sqxsum - meansq / size) / size, img.GetDeviation());
}


int main(int argc, const char **argv) {
	//TestGripperDetection();
	//TestVisionModule();
	//TestMetric1("../dumps/1438146364.dump", "metrics1.jpg");
	//CvsDump("../dumps/1438146364.dump", "test.jpg");
	//CvsDumpYUV("../dumps/1438146364.dump", "testYUV.jpg");
	//printf("convert: %s to %s\n", argv[1], argv[2]);
	//Convert(argv[1], argv[2]);
	//TestOpenCV2(argv[1]);
	TestPictureLighting(argv[1]);
}
