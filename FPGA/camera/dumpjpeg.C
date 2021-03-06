#include "jpeg.h"

#include <iostream>
#include <fstream>
#include <string.h>

struct TMap {

	uint32_t* Map;

	uint32_t Width;
	uint32_t Height;
	uint32_t Depth;
	uint32_t WinSize;
	uint8_t* Buffer;

	uint32_t MinV;
	uint32_t MaxV;

	TMap(uint32_t width, uint32_t height, uint32_t depth, uint32_t winSize, uint8_t* buffer) {
		Width = width;
		Height = height;
		Depth = depth;
		WinSize = winSize;
		Buffer = buffer;
		Map = new uint32_t[Height * (Width - WinSize * 2)];
	}

	~TMap(){
		delete Map;
	}

	uint8_t* GetRGB(uint32_t x, uint32_t y) {
		return Buffer + (y * Width + x) * Depth;
	}

	void Load() {
		MinV = 0xFFFFFFFF;
		MaxV = 0;

		auto ptr = Map;
		for (uint32_t y = 0; y < Height; ++y) {
			uint32_t win1[Depth];
			uint32_t win2[Depth];
			memset(win1, 0, Depth * sizeof(uint32_t));
			memset(win2, 0, Depth * sizeof(uint32_t));

			for (uint32_t i = 0; i < WinSize; ++i) {
				for (uint32_t j = 0; j < Depth; ++j) {
					win1[j] += GetRGB(i, y)[j];
					win2[j] += GetRGB(i + WinSize, y)[j];
				}
			}

			//printf("[%u,%u,%u] [%u,%u,%u]\n", win1[0], win1[1], win1[2], win2[0], win2[1], win2[2] );

			for (uint32_t x = WinSize; x < Width - WinSize; ++x) {
				uint32_t v = 0;
				for (uint32_t j = 0; j < Depth; ++j) {
					v += (win1[j] - win2[j]) * (win1[j] - win2[j]);
					win1[j] += (uint32_t)GetRGB(x, y)[j] - (uint32_t)GetRGB(x - WinSize, y)[j];
					win2[j] += (uint32_t)GetRGB(x + WinSize, y)[j] - (uint32_t)GetRGB(x, y)[j];
				}
				*ptr++ = v;
				if (v > MaxV) {
					MaxV = v;
				}
				if (v < MinV) {
					MinV = v;
				}
			}
		}
	}
};

struct TWin {
	uint32_t WinSize;
	uint32_t HistorySize;
	uint32_t Count;
	uint32_t* YSum;
	uint32_t* XYSum;
	uint32_t HHead;
	uint32_t XSum;
	uint32_t Head;

	uint32_t* Data;
	

	TWin() {
		Data = NULL;
	}

	~TWin() {
		if (Data != NULL) {
			delete Data;
			delete YSum;
			delete XYSum;
		}
	}

	void init(uint32_t winSize) {
		init(winSize, 1);
	}

	void init(uint32_t winSize, uint32_t historySize) {
		if (Data != NULL) {
			delete Data;
			delete YSum;
			delete XYSum;
		}
		WinSize = winSize;
		HistorySize = historySize;
		Data = new uint32_t[winSize];
		YSum = new uint32_t[historySize];
		XYSum = new uint32_t[historySize];
		Head = 0;
		Count = 0;
		HHead = 0;
		YSum[HHead] = 0;
		XYSum[HHead] = 0;
		XSum = (winSize - 1) * winSize / 2;
	}

	void Add(uint32_t value) {
		auto newHHead = (HHead + 1) % HistorySize;
		if (Count < WinSize) {
			YSum[newHHead] = YSum[HHead];
			XYSum[newHHead] = XYSum[HHead] + Count++ * value;
		} else  {
			YSum[newHHead] = YSum[HHead] - Data[Head];
			XYSum[newHHead] = XYSum[HHead] + (WinSize - 1) * value - YSum[newHHead];
		}
		YSum[newHHead] += value;
		Data[Head++] = value;
		if (Head == WinSize) {
			Head = 0;
		}
		HHead = newHHead;
	}

	uint32_t GetAvg() {
		return GetAvg(0);
	}

	uint32_t GetAvg(uint32_t hist) {
		return YSum[(HHead + HistorySize - hist) % HistorySize] / Count;
	}

	int32_t GetTrend() {
		return GetTrend(0);
	}

	int32_t GetTrend(uint32_t hist) {
		auto idx = (HHead + HistorySize - hist) % HistorySize;
		return WinSize * (int64_t)XYSum[idx] - XSum * (int64_t)YSum[idx];
	}

	uint32_t GetValue(uint32_t hist) {
		return Data[(Head + HistorySize - hist -1) % HistorySize];
	}
};

struct TBorder {
	uint32_t X;
	uint32_t AreaSum[3];
	uint32_t AreaCount;
};

struct TImage {

	uint8_t* Buffer;
	uint32_t  Size;

	uint32_t Width;
	uint32_t Height;
	uint32_t Depth;

	uint8_t* GetRGB(uint32_t x, uint32_t y) {
		return Buffer + (y * Width + x) * Depth;
	}

	void loadDump8(const char* fileName) {
		std::ifstream f;
		f.open(fileName);
		f >> Size;
		//printf("size=%u\n", Size);
		Buffer = new uint8_t[Size];
		for (uint32_t i = 0 ; i < Size; ++i) {
			uint32_t v;
			f >> v;
			Buffer[i] = (uint8_t)v;
		}
		f.close();
	}

	void process10() {

		Width = 752;

		int depth = 3;
		int width = Width / 2;
		int height = Size / Width / 2 + 1;
		uint8_t* rgb = new uint8_t[width * height * depth];
		for (uint32_t i = 0; i < Size; ++i) {
			uint32_t x = i % Width;
			uint32_t y = i / Width;
			uint8_t v = Buffer[i] /*>> 2*/;
			uint8_t* pixel = rgb + ((y / 2) * width + (x / 2)) * 3;
			if (y % 2 == 0) {
				if (x % 2 == 0) {
					pixel[2] = v;
				} else {
					pixel[1] = v / 2;
				}
			} else {
				if (x % 2 == 0) {
					pixel[1] += v / 2;
				} else {
					pixel[0] = v;
				}
			}
			//printf("%u, %u, %u\n", (uint32_t)pixel[0], (uint32_t)pixel[1], (uint32_t)pixel[2]);
		}

		save_RGB("pic.jpg", rgb, width, height);
	}

	void processRGB() {

		Width = 376;
		Height = 240;
		Depth = 3;

		save_RGB("pic.jpg", Buffer, Width, Height);
	}

	void process2(const char* in_file, uint32_t win_size, const char* file_name) {

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;
		
		TMap map(Width, Height, Depth, win_size, Buffer);

		map.Load();
		printf("[%u,%u]\n", map.MinV, map.MaxV);

		auto ptr = map.Map;
		for (uint32_t y = 0; y < Height; ++y) {
			for (uint32_t x = win_size; x < Width - win_size; ++x) {
				uint8_t c = (uint8_t)((*ptr++ - map.MinV) * (uint64_t)256 / (map.MaxV - map.MinV));
				auto pixel = GetRGB(x,y);
				pixel[0] = c;
				pixel[1] = c;
				pixel[2] = c;
			}
		}

		save_RGB(file_name, Buffer, Width, Height);
	}

	void process3(const char* in_file, uint32_t win_size, float thresholdPct, const char* file_name) {

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;
		
		TMap map(Width, Height, Depth, win_size, Buffer);

		map.Load();

		uint32_t threshold = (map.MaxV - map.MinV) * thresholdPct + map.MinV;
		printf("[%u,%u] threshold=%u\n", map.MinV, map.MaxV, threshold);

		auto ptr = map.Map;
		for (uint32_t y = 0; y < Height; ++y) {
			for (uint32_t x = win_size; x < Width - win_size; ++x) {
				uint8_t c = *ptr++ > threshold ? 255 : 0;
				auto pixel = GetRGB(x,y);
					for (uint32_t i = 0; i < Depth; ++i) {
						pixel[i] = c;
					}
			}
		}

		save_RGB(file_name, Buffer, Width, Height);
	}

	void process4(const char* in_file, uint32_t win_size, float thresholdPct, const char* file_name) {

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;
		
		TMap map(Width, Height, Depth, win_size, Buffer);

		map.Load();

		uint32_t threshold = (map.MaxV - map.MinV) * thresholdPct + map.MinV;
		printf("[%u,%u] threshold=%u\n", map.MinV, map.MaxV, threshold);

		auto ptr = map.Map;
		for (uint32_t y = 0; y < Height; ++y) {
			auto     x_start = win_size;
			uint16_t sum[Depth];
			memset(sum, 0, Depth * sizeof(uint16_t));
			for (uint32_t x = win_size; x < Width - win_size; ++x) {
				if (*ptr++ > threshold) {
					auto pixel = GetRGB(x,y);
					for (uint32_t i = 0; i < Depth; ++i) {
						if (x_start < x) {
							uint8_t c = (uint8_t)(sum[i] / (x - x_start));
							for (auto j = x_start; j< x; ++j) {
								GetRGB(j, y)[i] = c;
							}
						}
						sum[i] = 0;
						pixel[i] = 255;
					}
					x_start = x + 1;
				} else {
					auto pixel = GetRGB(x,y);
					for (uint32_t i = 0; i < Depth; ++i) {
						sum[i] += pixel[i];
						pixel[i] = 0;
					}
				}
			}
			auto x = Width - win_size;
			if (x_start < x) {
				for (uint32_t i = 0; i < Depth; ++i) {
					uint8_t c = (uint8_t)(sum[i] / (x - x_start));
					for (auto j = x_start; j< x; ++j) {
						GetRGB(j, y)[i] = c;
					}
				}
			}
		}

		save_RGB(file_name, Buffer, Width, Height);
	}

	void processLine(const char* in_file, uint32_t line) {

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;

		uint32_t maxWinSize = 20;

		TWin win1[maxWinSize][Depth];
		TWin win2[maxWinSize][Depth];

		for(uint32_t winSize = 1; winSize <= maxWinSize; ++winSize) {
			for (uint32_t i = 0; i < Depth; ++i) {
				win1[winSize - 1][i].init(winSize );
				win2[winSize - 1][i].init(winSize);
			}
		}

		for (uint32_t x = 0; x < Width - maxWinSize; x++) {
			uint64_t sum[maxWinSize];
			memset(sum, 0, maxWinSize * sizeof(uint64_t));
			uint8_t* pixel = GetRGB(x, line);
			for (uint32_t winSize = 1; winSize <= maxWinSize;++winSize) {
				uint8_t* pixel2 = GetRGB(x + winSize, line);
				for (uint32_t i = 0; i < Depth; ++i) {
					win1[winSize - 1][i].Add(pixel[i]);
					win2[winSize - 1][i].Add(pixel2[i]);

					auto avg = win1[winSize - 1][i].GetAvg() - win2[winSize - 1][i].GetAvg();
					sum[winSize - 1] += avg * avg;
					//sum[winSize - 1] += (win1[winSize - 1][i].Sum - win2[winSize - 1][i].Sum) * (win1[winSize - 1][i].Sum - win2[winSize - 1][i].Sum) / winSize/ winSize;
				}
			}
			if (x >= maxWinSize) {
				std::cout << x;
				for (uint32_t i = 0; i < Depth; ++i) {
					std::cout << ',' << (uint32_t)pixel[i];
				}
				for (uint32_t i = 0; i < maxWinSize; ++i) {
					std::cout << ',' << sum[i];
				}
				std::cout << std::endl;
			}
		}
	}

	void processLine2(const char* in_file, uint32_t line) {

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;

		uint32_t winSize = 4;

		TWin trend;
		trend.init(winSize, winSize);

		TWin win1[Depth];

		for (uint32_t i = 0; i < Depth; ++i) {
			win1[i].init(winSize, winSize * 2);
		}

		for (uint32_t x = 0; x < Width; x++) {
			uint8_t* pixel = GetRGB(x, line);
			for (uint32_t i = 0; i < Depth; ++i) {
				win1[i].Add(pixel[i]);
			}

			if (x >= winSize * 2 + winSize / 2) {
				uint64_t sum = 0;;
				for (uint32_t i = 0; i < Depth; ++i) {
					auto avg = win1[i].GetAvg() - win1[i].GetAvg(winSize);
					sum += avg * avg;
				}
				trend.Add(sum);

				auto xv = x - winSize - winSize / 2;
				std::cout << xv;
				pixel = GetRGB(xv, line);
				for (uint32_t i = 0; i < Depth; ++i) {
					std::cout << ',' << (uint32_t)pixel[i];
				}
				std::cout << ',' << trend.GetValue(winSize / 2);
				std::cout << ',' << trend.GetTrend();
				std::cout << std::endl;
			}
		}
	}

	void process5(uint32_t pic,
	              uint32_t winSize,
	              int32_t trendThreshold,
	              uint32_t minHAreaCount,
	              uint32_t areasDiffThreshold) {

		char in_file[100];
		char out_file[100];

		sprintf(in_file, "data/data%u.csv", pic);
		sprintf(out_file, "pic%u_%u_%d_%u_%u.jpg", pic, winSize, trendThreshold, minHAreaCount, areasDiffThreshold);

		loadDump8(in_file);

		Width = 376;
		Height = 240;
		Depth = 3;

		uint32_t vWinSize = 4;

		TWin trend;

		TWin win1[Depth];

		TBorder borders[vWinSize][Width / minHAreaCount];
		uint32_t bcount[vWinSize];
		uint32_t vHead = 0;

		for (uint32_t y = 0; y < Height;++y) {
			bcount[vHead] = 0;
			trend.init(winSize, winSize);
			for (uint32_t i = 0; i < Depth; ++i) {
				win1[i].init(winSize, winSize * 2);
				borders[vHead][0].AreaSum[i] = 0;
			}
			borders[vHead][0].AreaCount = 0;

			int32_t lastTrend = 0;
			for (uint32_t x = 0; x < Width; x++) {

				uint8_t* pixel = GetRGB(x, y);
				for (uint32_t i = 0; i < Depth; ++i) {
					win1[i].Add(pixel[i]);
				}

				int32_t vx = (int32_t)x - winSize - winSize / 2;
				if (vx >= 0) {
					uint64_t sumDiff = 0;
					for (uint32_t i = 0; i < Depth; ++i) {
						auto avg = win1[i].GetAvg() - win1[i].GetAvg(winSize);
						sumDiff += avg * avg;
					}
					trend.Add(sumDiff);
					auto newTrend = trend.GetTrend();

					if (lastTrend > 0 &&
						newTrend < 0 &&
						lastTrend - newTrend > trendThreshold &&
						borders[vHead][bcount[vHead]].AreaCount > minHAreaCount){

						// Apply areasDiffThreshold
						/*
						if (bcount[vHead] > 0) {
							uint64_t areaSumDiff = 0;
							for (uint32_t i = 0; i < Depth; ++i) {
								auto avg = borders[vHead][bcount[vHead]].AreaSum[i] / borders[vHead][bcount[vHead]].AreaCount - borders[vHead][bcount[vHead] - 1].AreaSum[i] / borders[vHead][bcount[vHead] - 1].AreaCount;
								areaSumDiff += avg * avg;
							}
							if( areaSumDiff < areasDiffThreshold) {
								bcount[vHead]--;
								for (uint32_t i = 0; i < Depth; ++i) {
									borders[vHead][bcount[vHead]].AreaSum[i] += borders[vHead][bcount[vHead] + 1].AreaSum[i];
								}
								borders[vHead][bcount[vHead]].AreaCount += borders[vHead][bcount[vHead] + 1].AreaCount;
							}
						}
						*/

						// Border
						borders[vHead][bcount[vHead]++].X = vx;
						borders[vHead][bcount[vHead]].AreaCount = 0;
						for(uint32_t i = 0; i < Depth; ++i) {
							borders[vHead][bcount[vHead]].AreaSum[i] = 0;
						}

					} else {
						// Same area
						for (uint32_t i = 0; i < Depth; ++i) {
							borders[vHead][bcount[vHead]].AreaSum[i] += win1[i].GetValue(x - vx);
						}
						borders[vHead][bcount[vHead]].AreaCount++;
					}

					lastTrend = newTrend;
				}
			}
			printf("%u:%u\n", y, bcount[vHead]);

			// Draw borders and areas
			for (uint32_t i = 0; i < bcount[vHead]; ++i) {
				uint8_t avg[Depth];
				for (uint32_t j = 0; j < Depth; ++j) {
					avg[j] = (uint8_t)(borders[vHead][i].AreaSum[j] / borders[vHead][i].AreaCount);
				}
				for (uint32_t j = 0; j < borders[vHead][i].AreaCount; ++j) {
					uint8_t* pixel = GetRGB(borders[vHead][i].X - j, y);
					for (uint32_t k = 0; k < Depth; ++k) {
						pixel[k] = avg[k];
					}
				}
				uint8_t* pixel = GetRGB(borders[vHead][i].X, y);
				pixel[0] = 255;
				pixel[1] = 255;
				pixel[2] = 255;
			}

			vHead = (vHead + 1) % vWinSize;
		}

		save_RGB(out_file, Buffer, Width, Height);
	}

};

void test_win1(){
	TWin win;
	win.init(4, 4);
	uint32_t v = 1;
	for (uint32_t i = 0; i < 4;++i) {
		win.Add(v);
		printf("%u: %u %u %d %d\n", v, win.YSum[win.HHead], win.XYSum[win.HHead], win.GetTrend(), win.GetTrend(2));
	}

	v = 2; 
	for (uint32_t i = 0; i < 11;++i) {
		win.Add(v);
		printf("%u: %u %u %d %d\n", v, win.YSum[win.HHead], win.XYSum[win.HHead], win.GetTrend(), win.GetTrend(2));
	}
	v = 1; 
	for (uint32_t i = 0; i < 11;++i) {
		win.Add(v);
		printf("%u: %u %u %d %d\n", v, win.YSum[win.HHead], win.XYSum[win.HHead], win.GetTrend(), win.GetTrend(2));
	}
}

int main(int argc, char **argv)
{
	TImage img;

	//img.loadDump8("data.csv");
	//img.process10();
	//img.processRGB();
	/*
	img.process2("datal6.csv",2,"pic6_2.jpg");
	img.process2("datal6.csv",3,"pic6_3.jpg");
	img.process2("datal6.csv",5,"pic6_5.jpg");
	img.process2("datal6.csv",6,"pic6_6.jpg");
	img.process2("datal6.csv",7,"pic6_7.jpg");
	img.process2("datal6.csv",8,"pic6_8.jpg");
	img.process2("datal6.csv",9,"pic6_9.jpg");
	img.process2("datal6.csv",10,"pic6_10.jpg");
	img.process2("datal6.csv",11,"pic6_11.jpg");
	img.process2("datal6.csv",12,"pic6_12.jpg");
	img.process2("datal6.csv",13,"pic6_13.jpg");
	img.process2("datal6.csv",14,"pic6_14.jpg");
	img.process2("datal6.csv",15,"pic6_15.jpg");
	img.process2("datal6.csv",16,"pic6_16.jpg");
	img.process2("datal6.csv",17,"pic6_17.jpg");
	img.process2("datal6.csv",18,"pic6_18.jpg");
	img.process2("datal6.csv",19,"pic6_19.jpg");
	img.process2("datal6.csv",20,"pic6_20.jpg");
	img.process2("datal6.csv",25,"pic6_25.jpg");
	img.process2("datal6.csv",30,"pic6_30.jpg");
	img.process2("datal6.csv",50,"pic6_50.jpg");
	*/

/*
	img.process3("datal6.csv",5, 0.01, "pic6_5_01.jpg");
	img.process3("datal6.csv",5, 0.02, "pic6_5_02.jpg");
	img.process3("datal6.csv",5, 0.03, "pic6_5_03.jpg");
	img.process3("datal6.csv",5, 0.04, "pic6_5_04.jpg");
	img.process3("datal6.csv",5, 0.05, "pic6_5_05.jpg");
	img.process3("datal6.csv",5, 0.06, "pic6_5_06.jpg");
	img.process3("datal6.csv",5, 0.07, "pic6_5_07.jpg");
	img.process3("datal6.csv",5, 0.08, "pic6_5_08.jpg");
	img.process3("datal6.csv",5, 0.09, "pic6_5_09.jpg");
	img.process3("datal6.csv",5, 0.11, "pic6_5_11.jpg");
	img.process3("datal6.csv",5, 0.12, "pic6_5_12.jpg");
	img.process3("datal6.csv",5, 0.13, "pic6_5_13.jpg");
	img.process3("datal6.csv",5, 0.14, "pic6_5_14.jpg");
	img.process3("datal6.csv",5, 0.15, "pic6_5_15.jpg");
	img.process3("datal6.csv",5, 0.16, "pic6_5_16.jpg");
	img.process3("datal6.csv",5, 0.17, "pic6_5_17.jpg");
	img.process3("datal6.csv",5, 0.18, "pic6_5_18.jpg");
	img.process3("datal6.csv",5, 0.19, "pic6_5_19.jpg");
*/
	//img.process3("datal6.csv",5, 0.1, "pic6_5_10.jpg");
	//img.process3("datal6.csv",5, 0.2, "pic6_5_20.jpg");
	//img.process3("datal6.csv",5, 0.3, "pic6_5_30.jpg");
	//img.process3("datal6.csv",5, 0.4, "pic6_5_40.jpg");

	//img.process4("datal6.csv",5, 0.1, "pic6_5_10c.jpg");
	//img.process4("datal6.csv",5, 0.01, "pic6_5_01c.jpg");
	//img.process4("datal6.csv",5, 0.02, "pic6_5_02c.jpg");

	//img.processLine("datal6.csv", 100);
	//img.processLine("data/data4.csv", 100);
	//img.processLine2("data/data4.csv", 100);
	//img.process5("data/data4.csv", "pic4_4_200.jpg", 4, 200);
	img.process5(4, 4, 400, 4, 400);
	//img.process5("data/data4.csv", "pic4_4_500.jpg", 4, 500);
	//test_win1();


	return 0;
}

