#include "utils/image.h"
#include "dlearning/dlearning.h"


void test2() {
    
    double x[] = {1,2,3};
    double r[10];
    
    fv(2, 3, x, r);
    
    for (int i = 0; i < 10; ++i) {
        printf("%f\n", r[i]);
    }
}

void test3() {
    TMutableRGBImage image("../dumps/1502667194.dump");
    
    TDeepLearningExtractorFactory factory;
    
    TSegmentsExtractor* se = factory.CreateExtractor(&image);
    
    se->DrawDebugInfo(&image);
    
    image.SaveJpg("test3.jpg");
}

void test4() {
	TDeepLearningExtractorFactory factory;
	factory.Set("LearningPictures", "1509593010,200,175,20,40,1509592959,42,155,24,38,1509592845,160,120,30,60,1509592995,271,58,29,45,1509592982,80,163,20,40,1509592942,138,93,25,45,1509592926,149,100,25,45,1509592898,148,71,25,42,");
}

void test5() {
	TDeepLearningExtractorFactory factory;
    TMutableRGBImage image("../dumps/1510199011.dump");
    TSegmentsExtractor* segmenter = factory.CreateExtractor(&image);

	std::list<TArea> areas;
	segmenter->ExtractSegments(areas);

	for(std::list<TArea>::iterator area = areas.begin();  area != areas.end(); area++) {
		printf("show: size=%d x=%d..%d y=%d..%d bb=%d type=%d\n", area->Size, area->MinX, area->MaxX, area->MinY, area->MaxY, area->AtBorder, area->ObjectType);
	}
}

int main(int argc, const char **argv) {
    //test1();
    //test2();
    //test3();
	//test4();
	test5();
}
