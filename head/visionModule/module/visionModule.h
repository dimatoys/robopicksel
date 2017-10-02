extern "C" {

	const int TYPE_INT = 0;
	const int TYPE_FLOAT = 1;
	const int TYPE_STRING = 2;
	const int MAX_NAME_SIZE = 30;

	struct TExtractorParameter {
		const char*	Name;
		int 	Type;
	};

	const int MAX_PARAMETERS = 20;

	struct TObjectsExtractor {
		void* Factory;
		int  NumParameters;
		TExtractorParameter Parameters[MAX_PARAMETERS];
	};

	struct DetectedObject {
        int MinX;
        int MinY;
        int MaxX;
        int MaxY;
        int Size;
        int BorderBits;
        int ObjectType;
    };

    const int MAX_OBJECTS = 10;

    struct VisionData {
        TObjectsExtractor* ObjectsExtractor;
        char* FileName;
        int Width;
        int Height;
        int Depth;
        int Status;
        char* DumpTemplate;
        long DumpId;
        int NumObjects;
        DetectedObject Objects[MAX_OBJECTS];
    };

    void extractorInit(TObjectsExtractor* data, const char* algorithm);
    void extractorSetInt(TObjectsExtractor* data, const char* parameter, int value);
    int extractorGetInt(TObjectsExtractor* data, const char* parameter);
    void extractorSetDouble(TObjectsExtractor* data, const char* parameter, double value);
    double extractorGetDouble(TObjectsExtractor* data, const char* parameter);
    void extractorSetString(TObjectsExtractor* data, const char* parameter, const char* value);
    const char* extractorGetString(TObjectsExtractor* data, const char* parameter);

    void streamStart(VisionData* data);
    void writeImage(char* data);
    void flush();
}
