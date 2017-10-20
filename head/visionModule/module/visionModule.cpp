#include "visionModule.h"
//#include "clustering/clustering.h"
#include "jpeg/jpegimage.h"
#include "statimg/statimg.h"
//#include "kp/kp.h"
#include "dlearning/dlearning.h"

#include <ctime>

void extractorInit(TObjectsExtractor* data, const char* algorithm) {
	TExtractorFactory* factory;
	//if (strcmp(algorithm, "clustering") == 0) {
	//	factory = new TClusteringExtractorFactory();
	//} else {
		if (strcmp(algorithm, "statimg") == 0) {
			factory = new TStatImgExtractorFactory();
		} else {
                    if (strcmp(algorithm, "dlearning") == 0) {
                        factory = new TDeepLearningExtractorFactory();
                    } else {
			factory = new TJpegExtractorFactory();
                    }
		}
	//}
	data->Factory = factory;
	data->NumParameters = 0;
	for(std::map<std::string,int*>::const_iterator it = factory->IntParameters.begin(); it != factory->IntParameters.end(); ++it) {
		data->Parameters[data->NumParameters].Name = it->first.c_str();
		data->Parameters[data->NumParameters++].Type = TYPE_INT;
	}

	for(std::map<std::string,double*>::const_iterator it = factory->DoubleParameters.begin(); it != factory->DoubleParameters.end(); ++it) {
		data->Parameters[data->NumParameters].Name = it->first.c_str();
		data->Parameters[data->NumParameters++].Type = TYPE_FLOAT;
	}

	for(std::map<std::string,std::string*>::const_iterator it = factory->StringParameters.begin(); it != factory->StringParameters.end(); ++it) {
		data->Parameters[data->NumParameters].Name = it->first.c_str();
		data->Parameters[data->NumParameters++].Type = TYPE_STRING;
	}
}

void extractorSetInt(TObjectsExtractor* data, const char* parameter, int value) {
	((TExtractorFactory*)data->Factory)->Set(parameter, value);
}

int extractorGetInt(TObjectsExtractor* data, const char* parameter) {
	int value = 0;
	((TExtractorFactory*)data->Factory)->Get(parameter, &value);
	return value;
}

void extractorSetFloat(TObjectsExtractor* data, const char* parameter, float value) {
	((TExtractorFactory*)data->Factory)->Set(parameter, (double)value);
}

float extractorGetFloat(TObjectsExtractor* data, const char* parameter) {
	double value = 0;
	((TExtractorFactory*)data->Factory)->Get(parameter, &value);
	return value;
}

void extractorSetDouble(TObjectsExtractor* data, const char* parameter, double value) {
	((TExtractorFactory*)data->Factory)->Set(parameter, value);
}

double extractorGetDouble(TObjectsExtractor* data, const char* parameter) {
	double value = 0;
	((TExtractorFactory*)data->Factory)->Get(parameter, &value);
	return value;
}

void extractorSetString(TObjectsExtractor* data, const char* parameter, const char* value) {
	((TExtractorFactory*)data->Factory)->Set(parameter, value);
}

const char* extractorGetString(TObjectsExtractor* data, const char* parameter) {
	std::string value = "";
	((TExtractorFactory*)data->Factory)->Get(parameter, &value);
	return value.c_str();
}

void ExtractFeatures(TMutableImage<unsigned char>* image,
                     TExtractorFactory* extractorFactury,
                     char* debugImageFileName,
                     int mode,
                     int& numObjects,
                     int maxObjects,
                     DetectedObject* objects) {

	TSegmentsExtractor* segmenter = extractorFactury->CreateExtractor(image);

	std::list<TArea> areas;

	printf("ExtractFeatures: Extract areas\n");

	segmenter->ExtractSegments(areas);

	printf("ExtractFeature: extracted: areas=%lu\n", areas.size());

	//std::sort(areas.begin(), areas.end(), greater);

	numObjects = 0;

	for(std::list<TArea>::iterator area = areas.begin();  area != areas.end(); area++) {
		DetectedObject& obj = objects[numObjects];
		obj.Size = area->Size;
		obj.MinX = area->MinX;
		obj.MinY = area->MinY;
		obj.MaxX = area->MaxX;
		obj.MaxY = area->MaxY;
		obj.BorderBits = area->AtBorder;
		obj.ObjectType = area->ObjectType;
		printf("show: size=%d x=%d..%d y=%d..%d bb=%d type=%d\n", obj.Size, obj.MinX, obj.MaxX, obj.MinY, obj.MaxY, obj.BorderBits, obj.ObjectType);

		if (++numObjects >= maxObjects) {
			break;
		}
	}

	if (debugImageFileName != NULL) {
		TMutableRGBImage debugImage(image);

		unsigned char colorInsightObject[] = {0xFF, 0x40, 0x40};
		unsigned char colorBorderObject[] = {0x40, 0x40, 0xFF};
		//unsigned char colorFrontier[] = {0x80, 0xFF, 0x80};
		//unsigned char colorSkip[] = {0xFF, 0x80, 0x80};

		printf("ExtractFeatures:draw objects\n");

		for (int i = 0; i < numObjects; i++) {
			for (int j = 0; j < 5; j++) {
				debugImage.DrawRect(objects[i].MinX - j,
				                    objects[i].MinY - j,
				                    objects[i].MaxX - objects[i].MinX + 2 * j,
				                    objects[i].MaxY - objects[i].MinY + 2 * j,
				                    objects[i].BorderBits == 0 ? colorInsightObject : colorBorderObject);
			}
		}

		printf("ExtractFeatures:draw horizontal: w=%d h=%d d=%d\n", debugImage.Width, debugImage.Height, debugImage.Depth);

		memset(debugImage.Cell(0, debugImage.Height / 2), 0xFF, debugImage.Width * debugImage.Depth);

		printf("ExtractFeatures:draw vertical\n");

		int x = debugImage.Width / 2;
		for (int y = 0; y < debugImage.Height; y++) {
			memset(debugImage.Cell(x, y), 0xFF,debugImage.Depth);
		}

		segmenter->DrawDebugInfo(&debugImage);

		printf("ExtractFeatures: Ok DebugImage\n");

		debugImage.SaveJpg(debugImageFileName);
	}
	extractorFactury->DestroyExtractor(segmenter);

	printf("ExtractFeatures: Ok\n");
}

static VisionData* g_ModuleVisionData = NULL;

void streamStart(VisionData* data) {
	g_ModuleVisionData = data;
}

void writeImage(char* buffer) {

    TMutableImage<unsigned char> image((unsigned char*)buffer, g_ModuleVisionData->Width, g_ModuleVisionData->Height, g_ModuleVisionData->Depth);

    if (g_ModuleVisionData->DumpTemplate != NULL) {
	char fileName[100];
        g_ModuleVisionData->DumpId = (long)time(NULL);
    	sprintf(fileName, g_ModuleVisionData->DumpTemplate, g_ModuleVisionData->DumpId);
    	image.SaveDump(fileName);
    }

    ExtractFeatures(&image,
	            (TExtractorFactory*)g_ModuleVisionData->ObjectsExtractor->Factory,
	            g_ModuleVisionData->FileName,
	            g_ModuleVisionData->Mode,
	            g_ModuleVisionData->NumObjects,
	            MAX_OBJECTS,
	            g_ModuleVisionData->Objects);

}

void flush() {}
