#include "dlearning.h"

#include <list>
#include <string>

struct Knot {
    
    unsigned char Status;
    int Left;
    int Right;
    int Top;
    int Bottom;
    Knot* Next;
    int X;
    int Y;
    int X1;
    int Y1;
    
    Knot() {
        Status = 0;
        Left = -1;
        Right = -1;
        Top = -1;
        Bottom = -1;
        Next = NULL;
    }
};

class TDeepLearningSegmentsExtractor : public TSegmentsExtractor {
	TDeepLearningExtractorFactory* Parameters;
    TMutableImage<unsigned char>* Image;
    
    int WidthL1;
    int HeightL1;
    int SizeL1;

    Knot* L1;
    Knot* Frontier;
    
public:
	TDeepLearningSegmentsExtractor(TDeepLearningExtractorFactory* parameters) {
		Parameters = parameters;
        L1 = NULL;
        Frontier = NULL;
 	}

    void NewImage(TMutableImage<unsigned char>* image) {
        Image = image;

        int widthL1 = Image->Width / Parameters->StepL1 - 1;
        int heightL1 = Image->Height / Parameters->StepL1 - 1;
        
        if ((L1 == NULL) || (WidthL1 != widthL1) || (HeightL1 != heightL1)) {
            if (L1 != NULL) {
                delete L1;
            }
            WidthL1 = widthL1;
            HeightL1 = heightL1;
            SizeL1 = WidthL1 * HeightL1;
            L1 = new Knot[SizeL1];
        }
    }
    
    bool GetL1(int x, int y);
    void TranslateL1(int x1, int y1, int* x, int* y);
    Knot* GetKnot(int x1, int y1);
    int FindBorder(int x, int y, int sx, int sy, int limit, TArea* area);
    
    Knot* PopKnot();
    void PushKnot(Knot* knot, int x, int y, int x1, int y1);
    
 	void ExtractSegments(std::list<TArea>& area);
	void DrawDebugInfo(TMutableRGBImage* image);

	~TDeepLearningSegmentsExtractor(){
        if (L1 != NULL) {
            delete L1;
        }
    }
};

struct TXIterator : public ILearningDataSource {

	TImagesLearningDataSource& Images;

	unsigned int Size;
	int Index;
	int Data[2];

	TXIterator(TImagesLearningDataSource& images,
	           unsigned int size) :
		Images(images) {
		D = 2;
		Size = size;
	}

	void Reset() {
		Images.Reset();
	}

	double NextElement() {
		return Data[Index++];
	}

	bool NextRecord() {
		TLearningImage::Label clabel;
		double color[3];
		while(Images.Next(clabel, Data[0] ,Data[1], color)) {
			if (clabel == TLearningImage::BACKGROUND) {
				Index = 0;
				return true;
			}
		}
		return false;
	}

	unsigned int GetSize() {
		return Size;
	}
};

struct TYIterator : public ILearningDataSource {

	TImagesLearningDataSource& Images;

	unsigned int Size;
	double Element[3];
	int Index;

	TYIterator(TImagesLearningDataSource& images) :
		Images(images) {
		D = 3;
		Size = CountSize();
	}

	void Reset() {
		Images.Reset();
	}

	double NextElement() {
		return Element[Index++];
	}

	bool NextRecord() {
		TLearningImage::Label clabel;
		int x, y;

		while(Images.Next(clabel, x, y, Element)) {
			if (clabel == TLearningImage::BACKGROUND) {
				Index = 0;
				return true;
			}
		}
		return false;
	}

	unsigned int GetSize() {
		return Size;
	}

	unsigned int CountSize() {
		unsigned int size = 0;
		Images.Reset();
		while(NextRecord()) {
			++size;
		}
		return size;
	}
};

TDeepLearningSegmentsExtractor* instance = NULL;

TSegmentsExtractor* TDeepLearningExtractorFactory::CreateExtractor(TMutableImage<unsigned char>* image) {
    if (instance == NULL) {
        instance = new TDeepLearningSegmentsExtractor(this);
    }

    instance->NewImage(image);

	return instance;
}

void ReadList(std::string str, std::list<std::string>& lst) {
	int start = 0;
	for(std::string::size_type i = 0; i < str.size(); ++i) {
		if (str[i] == ',') {
			std::string value = str.substr(start, i - start);
			//printf("ReadList:%s\n", value.c_str());
			lst.push_back(value);
			start = i + 1;
		}
	}
	if (start < str.size()) {
		std::string value = str.substr(start);
		//printf("ReadList:%s\n", value.c_str());
		lst.push_back(value);
	}
}

void TDeepLearningExtractorFactory::DumpPRData() {
	PRData = "";
	PRData += std::to_string(PR.S);
	PRData += ",";
	PRData += std::to_string(PR.XD);
	PRData += ",";
	PRData += std::to_string(PR.YD);
	unsigned int rsize = PR.XS * PR.YD;
	for (int i = 0; i < rsize; ++i) {
		PRData += ",";
		PRData += std::to_string(PR.R[i]);
	}
}

void TDeepLearningExtractorFactory::ReadPRData() {
	std::list<std::string> lst;
	ReadList(PRData, lst);
	int rsize = lst.size() - 3;
	if (rsize < 0) {
		printf("ReadPRData: data size is too small: %d\n", rsize + 3);
		return;
	}
	auto it = lst.begin();
	auto s = std::stoi(*it++);
	auto xd = std::stoi(*it++);
	auto yd = std::stoi(*it++);
	double r[rsize];
	for (int i = 0; i < rsize; ++i){
		r[i] = std::stod(*it++);
	}
	PR.SetR(r, s, xd, yd);
}

void TDeepLearningExtractorFactory::Learn(TImagesLearningDataSource& images) {

	TYIterator yit(images);
	TXIterator xit(images, yit.GetSize());

	PR.Learn(&xit, &yit);

	D = countOptimalDistance(images, PR);
	DumpPRData();
	//printf("Optimal distance: %u\n", D);
	//printf("Background color: (%f,%f,%f)\n", PR.R[0], PR.R[1], PR.R[2]);
	printf("PR:%s\n", PRData.c_str());
}

void TDeepLearningExtractorFactory::ParameterUpdated(std::string name) {

	if (name == "LearningPictures") {
		if (LearningPictures.length() == 0) {
			return;
		}
		std::list<std::string> lst;
		ReadList(LearningPictures, lst);
		TImagesLearningDataSource images;
		for (std::list<std::string>::iterator it = lst.begin(); it != lst.end();) {
			TLearningImage image;
			image.Path = "../dumps/" + *it++ + ".dump";
			image.X = std::stoi(*it++);
			image.Y = std::stoi(*it++);
			image.RIn = std::stoi(*it++);
			image.ROut = std::stoi(*it++);
			//image.Test("learning_test.jpg");
			TLearningImageIterator iit(image);
			images.AddImage(iit);
		}

		Learn(images);

	} else if (name == "PR"){
		if (PRData.length() == 0) {
			return;
		}
		ReadPRData();
	}
}

bool TDeepLearningSegmentsExtractor::GetL1(int x, int y) {
/*
	double vx[2];
	vx[0] = x;
	vx[1] = y;

	double vy[Parameters->PR.YD];
	Parameters->PR.GetValue(vx, vy);

	return countDistance(vy, Image->Cell(x, y)) > Parameters->D;
*/
	// for S = 0
	return countDistance(Parameters->PR.R, Image->Cell(x, y)) > Parameters->D;
}

void TDeepLearningSegmentsExtractor::TranslateL1(int x1, int y1, int* x, int* y) {
    *x = (x1 + 1) * Parameters->StepL1;
    *y = (y1 + 1) * Parameters->StepL1;
}

Knot* TDeepLearningSegmentsExtractor::GetKnot(int x1, int y1) {
    return L1 + (y1 * WidthL1 + x1);
}

inline int sign(int a) { return a > 0 ? 1 : (a < 0 ? -1 : 0); }

int TDeepLearningSegmentsExtractor::FindBorder(int x, int y, int sx, int sy, int limit, TArea* area) {
    int gap = 0;
    while(true) {
        if (sx < 0) {
            x += sx;
            if (x < limit) {
                break;
            }
        } else {
            if (sx > 0) {
                x += sx;
                if (x > limit) {
                    break;
                }
            } else {
                if (sy < 0) {
                    y += sy;
                    if (y < limit) {
                        break;
                    }
                } else {
                    // sy > 0
                    y += sy;
                    if (y > limit) {
                        break;
                    }
                }
            }
        }
        if (GetL1(x, y)) {
            gap = 0;
        } else {
            if (++gap > 1) {
                if (sx != 0) {
                    x -= gap * sx;
                    int ssx = sign(sx);
                    while (GetL1(x + ssx, y)) {
                        x+= ssx;
                    }
                    if (sx > 0) {
                        if (x > area->MaxX) {
                            area->MaxX = x;
                        }
                    } else {
                        if (x < area->MinX) {
                            area->MinX = x;
                        } 
                    }
                    return x;
                } else {
                    //sy != 0
                    y -= gap * sy;
                    int ssy = sign(sy);
                    while(GetL1(x, y + ssy)) {
                        y += ssy;
                    }
                    if (sy > 0) {
                        if (y > area->MaxY) {
                            area->MaxY = y;
                        }
                    } else {
                        if (y < area->MinY) {
                            area->MinY = y;
                        }
                    }
                    return y;
                }
            }
        }
    }
    return limit;
}

Knot* TDeepLearningSegmentsExtractor::PopKnot() {
    if (Frontier != NULL) {
        Knot* knot = Frontier;
        Frontier = knot->Next;
        return knot;
    }
    return NULL;
}

void TDeepLearningSegmentsExtractor::PushKnot(Knot* knot, int x, int y, int x1, int y1) {
    knot->Status = 1;
    knot->X = x;
    knot->Y = y;
    knot->X1 = x1;
    knot->Y1 = y1;
    knot->Next = Frontier;
    Frontier = knot;
}


void TDeepLearningSegmentsExtractor::ExtractSegments(std::list<TArea>& areas){
    for (int y1 = 0; y1 < HeightL1; ++y1) {
        for (int x1 = 0; x1 < WidthL1; ++x1) {
            Knot* knot = GetKnot(x1, y1);
            if (knot->Status == 0) {
                int x, y;
                TranslateL1(x1, y1, &x, &y);
                if(GetL1(x, y)) {
                    TArea area(x, y);
                    int xleft = x - Parameters->StepL1;
                    int xleftborder = FindBorder(x, y, -Parameters->StepL2, 0, xleft, &area);
                    if (xleftborder < x) {
                        PushKnot(knot, x, y, x1, y1);
                        if ((xleft < xleftborder) || (x1 == 0)) {
                            knot->Left = xleftborder;
                        } else {
                            knot->Left = -2;
                            Knot* leftknot = GetKnot(x1 - 1, y1);
                            leftknot->Right = -2;
                            PushKnot(leftknot, xleft, y, x1 - 1, y1);
                        }
                        
                        while((knot = PopKnot()) != NULL) {
                            if (knot->Left == -1) {
                                if (knot->X1 == 0) {
                                    knot->Left = FindBorder(knot->X, knot->Y, -Parameters->StepL2, 0, 0, &area);
                                } else {
                                    Knot* leftknot = GetKnot(knot->X1 - 1, knot->Y1);
                                    switch (leftknot->Status) {
                                        case 0:
                                            xleft = knot->X - Parameters->StepL1;
                                            xleftborder = FindBorder(knot->X, knot->Y, -Parameters->StepL2, 0, xleft, &area);
                                            if (xleft < xleftborder) {
                                                knot->Left = xleftborder;
                                            } else {
                                                knot->Left = -2;
                                                leftknot->Right = -2;
                                                PushKnot(leftknot, xleft, knot->Y, knot->X1 - 1, knot->Y1);
                                            }
                                            break;
                                        case 1:
                                            knot->Left = -2;
                                            leftknot->Right = -2;
                                            break;
                                        case 2:
                                            knot->Left = FindBorder(knot->X, knot->Y, -Parameters->StepL2, 0, knot->X - Parameters->StepL1, &area);
                                            break;
                                    }
                                }    
                            }
                            if (knot->Right == -1) {
                                if(knot->X1 + 1 >= WidthL1) {
                                    knot->Right = FindBorder(knot->X, knot->Y, Parameters->StepL2, 0, Image->Width - 1, &area);
                                } else {
                                    Knot* rightknot = GetKnot(knot->X1 + 1, knot->Y1);
                                    switch(rightknot->Status) {
                                        case 0:
                                        {
                                            int xright = knot->X + Parameters->StepL1;
                                            int xrightborder = FindBorder(knot->X, knot->Y, Parameters->StepL2, 0, xright, &area);
                                            if (xright > xrightborder) {
                                                knot->Right = xrightborder;
                                            } else {
                                                knot->Right = -2;
                                                rightknot->Left = -2;
                                                PushKnot(rightknot, xright, knot->Y, knot->X1 + 1, knot->Y1);
                                            }
                                            break;
                                        }
                                        case 1:
                                            knot->Right = -2;
                                            rightknot->Left = -2;
                                            break;
                                        case 2:
                                            knot->Right = FindBorder(knot->X, knot->Y, Parameters->StepL2, 0, knot->X + Parameters->StepL1, &area);
                                            break;
                                    }
                                }
                            }
                            if (knot->Top == -1) {
                                if (knot->Y1 == 0) {
                                    knot->Top = FindBorder(knot->X, knot->Y, 0, -Parameters->StepL2, 0, &area);
                                } else {
                                    Knot* topknot = GetKnot(knot->X1, knot->Y1 - 1);
                                    switch (topknot->Status) {
                                        case 0: {
                                            int top = knot->Y - Parameters->StepL1;
                                            int topborder = FindBorder(knot->X, knot->Y, 0, -Parameters->StepL2, top, &area);
                                            if (top < topborder) {
                                                knot->Top = topborder;
                                            } else {
                                                knot->Top = -2;
                                                topknot->Bottom = -2;
                                                PushKnot(topknot, knot->X, top, knot->X1, knot->Y1 - 1);
                                            }    
                                            break;
                                        }
                                        case 1:
                                            knot->Top = -2;
                                            topknot->Bottom = -2;
                                            break;
                                        case 2:
                                            knot->Top = FindBorder(knot->X, knot->Y, 0, -Parameters->StepL2, knot->Y - Parameters->StepL1, &area);
                                            break;
                                    }
                                }    
                            }
                            if (knot->Bottom == -1) {
                                if(knot->Y1 + 1 >= HeightL1) {
                                    knot->Bottom = FindBorder(knot->X, knot->Y, 0, Parameters->StepL2, Image->Height - 1, &area);
                                } else {
                                    Knot* bottomknot = GetKnot(knot->X1, knot->Y1 + 1);
                                    switch(bottomknot->Status) {
                                        case 0:
                                        {
                                            int bottom = knot->Y + Parameters->StepL1;
                                            int bottomborder = FindBorder(knot->X, knot->Y, 0, Parameters->StepL2, bottom, &area);
                                            if (bottom > bottomborder) {
                                                knot->Bottom = bottomborder;
                                            } else {
                                                knot->Bottom = -2;
                                                bottomknot->Top = -2;
                                                PushKnot(bottomknot, knot->X, bottom, knot->X1, knot->Y1 + 1);
                                            }
                                            break;
                                        }
                                        case 1:
                                            knot->Bottom = -2;
                                            bottomknot->Top = -2;
                                            break;
                                        case 2:
                                            knot->Bottom = FindBorder(knot->X, knot->Y, 0, Parameters->StepL2, knot->Y + Parameters->StepL1, &area);
                                            break;
                                    }
                                }
                            }
                        }
                        areas.push_back(area);
                    } else {
                        knot->Status = 2;
                    }
                } else {
                    knot->Status = 2;
                }
            }
        }
    }
}

void TDeepLearningSegmentsExtractor::DrawDebugInfo(TMutableRGBImage* image){
    
    // Mode = 1 - normal
    // Mode = 2 draw L1
    
    if (Mode == 2) {
		unsigned char fill[] = {255, 255, 255};
		for (int y = 0; y < image->Height; y += 2) {
			for (int x = 0; x < image->Width; x += 2) {
				if (GetL1(x, y)) {
					memcpy(image->Cell(x, y), fill, image->Depth);
				}
			}
		}
	} else {
		unsigned char obj[] = {255, 0, 0};
		unsigned char h[] = {0, 0, 255};
		unsigned char v[] = {0, 255, 0};
		unsigned char fill[] = {255, 255, 255};
	   
		for (int y1 = 0; y1 < HeightL1; ++y1) {
			for (int x1 = 0; x1 < WidthL1; ++x1) {
				Knot* knot = GetKnot(x1, y1);
				if (knot->Status == 1) {
					int x, y;
					TranslateL1(x1, y1, &x, &y);
					image->DrawPointer(x, y, 2, obj);
					printf("(%d,%d)->(%d,%d) left=%d right=%d top=%d bottom=%d\n", x1,y1,x,y,knot->Left, knot->Right, knot->Top, knot->Bottom);
					if (knot->Top >= 0) {
						image->DrawPointer(x, knot->Top, 2, v);
					}
					if (knot->Bottom >= 0) {
						image->DrawPointer(x, knot->Bottom, 2, v);
					}
					if (knot->Left >= 0) {
						image->DrawPointer(knot->Left, y, 2, h);
					}
					if (knot->Right >= 0) {
						image->DrawPointer(knot->Right, y, 2, h);
					}
				}
			}
		}
	}
}
/*
 1505247485
 * 1505247578
 
 *  1505247611
 * 1505247639
 * 
 * 
 */
