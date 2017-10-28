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
	factory.Set("LearningPictures", "1508455794,136,103,25,45,1508455752,137,77,25,45,1508455696,160,120,0,0,1508455775,210,85,23,45,");
}

int main(int argc, const char **argv) {
    //test1();
    //test2();
    //test3();
	test4();
}
