#include "utils/image.h"
#include "dlearning/dlearning.h"

class TData : public ILearningDataSource {
    int R;
    unsigned int I;
    double* Data;
    
    public:
        TData(double* data, unsigned int d, unsigned int n) {
            R = -1;
            N = n;
            D = d;
            Data = data;
        }

    double NextElement(){
        return Data[R * D + I++];
    }
    
    bool NextRecord() {
        I = 0;
        return ++R < N;
    }    
};

void test1(){
    double x[] = {0,1};
    double y[] = {0,1};
    
    TData xs(x, 1, 2);
    TData ys(y, 1, 2);
    
    TPolyRegression pr(1);
    if (pr.Learn(&xs, &ys)) {
    
        double rx = 2;
        double ry = 0;
    
        pr.GetValue(&rx, &ry);
    
        printf("y=%f\n", ry);
    } else {
        printf("Learn error\n");
    }
}

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

int main(int argc, const char **argv) {
    //test1();
    //test2();
    test3();
}
