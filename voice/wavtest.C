#include <stdio.h>
#include <cstdint>
#include <string.h>
#include <stdlib.h> 

class TMinMaxCounter {

	int16_t* Buffer;
	int BufferSize;
	int BBegin;
	bool BFull;

	int8_t Stat[65536];

public:

	int16_t Min;
	int16_t Max;

	TMinMaxCounter(int bufferSize) {

		Min = 16383;
		Max = -16383;

		BufferSize = bufferSize;
		Buffer = new int16_t[BufferSize];
		BBegin = 0;
		BFull = false;
		memset(Stat, 0, 65536);
	}

	~TMinMaxCounter() {
		if (Buffer != NULL) {
			delete Buffer;
		}
	}

	void Add(int16_t value) {
		++Stat[value + 16384];
		if (value > Max) {
			Max = value;
		}
		if (value < Min) {
			Min = value;
		}

		if (BFull) {
			int16_t old = Buffer[BBegin];
			if (--Stat[old + 16384] == 0) {
				if (old == Max) {
					while (Stat[--old + 16384] == 0);
					Max = old;
				} else {
					if (old == Min) {
						while (Stat[++old + 16384] == 0);
						Min = old;
					}
				}
			}
		}

		Buffer[BBegin++] = value;
		BBegin %= BufferSize;
		if (!BFull && (BBegin == 0)) {
			BFull = true;
		}
	}
};

template <class T>
class TPercentile {
	T Values[0x10000];

public:

	T Total;

	TPercentile() {
		memset(Values, 0, 0x10000 * sizeof(T));
		Total = 0;
	}

	void Add(int16_t value) {
		Values[value + 0x00008000L]++;
		Total++;
	}

	int16_t GetPercentile(double percentile) {
		int64_t count = Total * percentile;
		uint16_t value = 0;
		while (count > 0) {
			count -= Values[value++];
			if (value == 0) {
				return 0x7FFF;
			}
		}
		return (int16_t)(value + 0x00008000L);
	}
};

class TEnergyCounter {

	int16_t*	ABuffer;
	int			ABegin;

	uint32_t*	EBuffer;
	int			EBegin;

public:

	int			ABufferSize;
	int32_t		ASum;

	int			EBufferSize;
	uint32_t	ESum;

	TEnergyCounter(int avgBufferSize, int evergyBufferSize) {
		ABufferSize = avgBufferSize;
		ABuffer = new int16_t[ABufferSize];
		ABegin = 0;
		ASum = 0;
		memset(ABuffer, 0, ABufferSize * sizeof(int16_t));

		EBufferSize = evergyBufferSize;
		EBuffer = new uint32_t[EBufferSize];
		EBegin = 0;
		ESum = 0;
		memset(EBuffer, 0, EBufferSize * sizeof(int32_t));
	}

	~TEnergyCounter() {
		delete ABuffer;
		delete EBuffer;
	}

	void Add(int16_t value) {
		ASum += value - ABuffer[ABegin];
		ABuffer[ABegin++] = value;
		ABegin %= ABufferSize;

		auto diff = value - ASum / ABufferSize;
		auto sigma = diff * diff;
		ESum += sigma - EBuffer[EBegin];
		EBuffer[EBegin++] = sigma;
		EBegin %= EBufferSize;
	}
};

static int
check_wav_header(char *header, int expected_sr)
{
    int sr;

    if (header[34] != 0x10) {
        printf("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
    	printf("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
    	printf("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
    	printf("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}

void test1(const char* fname) {
    int16_t adbuf[2048];
    int32_t k;
    FILE *rawfd;

    //TMinMaxCounter mm(60);
    TEnergyCounter ec1(16, 16);
    TEnergyCounter ec2(50, 50);

    if ((rawfd = fopen(fname, "rb")) == NULL) {
        printf("Failed to open file '%s' for reading", fname);
    }

    char waveheader[44];
    fread(waveheader, 1, 44, rawfd);

	if (!check_wav_header(waveheader, 16000)) {
    	printf("Failed to process file '%s' due to format mismatch.\n", fname);
    }

	uint32_t threshold = ec1.EBufferSize * ec1.EBufferSize * 25000;
	long SpeachEnd = -1;
	long interval = (long)(16000 * 0.3);

	printf("time,value,10_10,50_50\n");
	long time = 0;
    while ((k = fread(adbuf, sizeof(int16_t), 2048, rawfd)) > 0) {
    	for (int i = 0; i < k; ++i, ++time) {
    		//mm.Add(adbuf[i]);
    		ec1.Add(adbuf[i]);
    		ec2.Add(adbuf[i]);
    		//ec3.Add(adbuf[i]);
    		//ec4.Add(adbuf[i]);

    		int32_t avg1 = ec1.ESum / (ec1.EBufferSize * ec1.EBufferSize);
    		int32_t avg2 = ec2.ESum / (ec2.EBufferSize * ec2.EBufferSize);
    		//int32_t avg3 = ec3.ESum / (ec3.EBufferSize * ec3.EBufferSize);
    		//int32_t avg4 = ec4.ESum / (ec4.EBufferSize * ec4.EBufferSize);
        	//printf("%d,%d,%d,%d\n", time / 16L,
        	//							(int)adbuf[i],
			//							avg1, avg2);

        	if (time == SpeachEnd) {
        		printf("End: %ld\n", time / 16L);
        	}

        	if (time > SpeachEnd && ec1.ESum > threshold) {
        		printf("Start: %ld\n", (time - interval) / 16L);
        		SpeachEnd = time + interval;
        	}
    	}
    }
    fclose(rawfd);
}

int32_t countEnergy(const int16_t* buffer, int size) {
	int32_t sum = 0;
	for (int i = 0; i < size; ++i) {
		sum += buffer[i];
	}

	int32_t avg = sum / size;

	int32_t sum2 = 0;
	for (int i = 0; i < size; ++i) {
		int32_t diff = buffer[i] - avg;
		sum2 += diff * diff;
	}
	return sum2;
}

void countFreq(const int16_t* buffer, int size) {
}

// WAVE File Header
struct Wav {
	// RIFF chunk descriptor
	char	ChunkID[4];			// "RIFF"              
	int		ChunkSize;			// 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
	char	Format[4];			// "WAVE"
	// fmt sub-chunk
	char	Subchunk1ID[4];		// "fmt "
	int		Subchunk1Size;		// bytes remaining in subchunk, 16 if uncompressed
	short	AudioFormat;		// 1 = uncompressed
	short	NumChannels;		// mono or stereo
	int		SampleRate;
	int		ByteRate;			// == SampleRate * NumChannels * BitsPerSample/8
	short	BlockAlign;			// == NumChannels * BitsPerSample/8
	short	BitsPerSample;
	// data sub-chunk
	char	Subchunk2ID[4];		// "data"
	int		Subchunk2Size;		// == NumSamples * NumChannels * BitsPerSample/8
};

void analyzeEnergy(const char* inFileName, int winSize) {

	double freq = 16000.0;

	FILE* inFile = fopen(inFileName, "r");

	if (inFile == NULL) {
		printf("Failed to open file '%s' for reading\n", inFileName);
		exit(-1);
	}

	struct Wav HeaderInfo;
	fread(&HeaderInfo, sizeof(Wav), 1, inFile);

	double time = 0;
	int16_t buf[winSize];
	while (fread(buf, sizeof(int16_t), winSize, inFile) == winSize) {
		int32_t energy = countEnergy(buf, winSize);
		printf("%u,%d\n", (unsigned)(time * 1000), energy);
		time += winSize / freq;
	}

	fclose(inFile);
}

class TVoiceDetector {
public:
	int Freq;
	int16_t LastValue;
	int LastZ;
	unsigned Energy;
	int StartVoice;
	int VoiceTo;
	int VoiceFrom;
	int I;

	TVoiceDetector(int freq) {
		Freq = freq;
		LastValue = 0;
		LastZ = 0;
		Energy = 0;
		StartVoice = -1;
		VoiceTo = -1;
		VoiceFrom = -1;
		I = 0;
	}

	bool Process(int16_t currValue) {
		bool endOfPhrase = false;
		Energy += abs(currValue);
		if (LastValue < 0 && currValue > 0) {
			// analyze a single sinusoida

			// count the sinusoida 
			auto currPeriod = I - LastZ;
			if (Energy / 100 >= currPeriod && Freq / 85 > currPeriod) {
				if (StartVoice == -1) {
					StartVoice = LastZ;
				}
				if (I >= StartVoice + 50 * Freq / 1000) {
					if (VoiceTo < 0) {
						VoiceFrom = StartVoice;
					}
					VoiceTo = I + 300 * Freq / 1000;
				}
			} else {
				StartVoice = -1;
			}
			//printf("%f,%u,%u,%d,%d\n", i / (double)freq, currPeriod, energy / currPeriod,
			//		startVoice >= 0 ? (i - startVoice) * 1000 / freq : 0,
			//		voiceTo >= i ? voiceTo - i : 0);
			if (VoiceTo >= 0 && VoiceTo < I) {
				VoiceTo = -1;
				endOfPhrase = true;
			}
			LastZ = I;
			Energy = 0;
		}
		LastValue = currValue;
		I++;
		return endOfPhrase;
	}
};

void analyzeFreq(const char* inFileName) {

	FILE* inFile = fopen(inFileName, "rb");

	if (inFile == NULL) {
		printf("Failed to open file '%s' for reading\n", inFileName);
		exit(-1);
	}

	struct Wav HeaderInfo;
	fread(&HeaderInfo, sizeof(Wav), 1, inFile);
	int freq = HeaderInfo.SampleRate;

	int winSize = 1 * freq;
	int buffers = 10;
	int16_t buf[winSize * buffers];

	TVoiceDetector vd(freq);

	while (true) {
		long size = fread(buf, sizeof(int16_t), winSize, inFile);
		if (size > 0) {
			for (int i = 0; i < size; ++i) {
				if (vd.Process(buf[i]) ) {
					printf("C:%f - %f\n", vd.VoiceFrom / (double)vd.Freq, vd.I / (double)vd.Freq);
				}
			}
		} else {
			break;
		}
	}

	fclose(inFile);
}

int main(int argc, char *argv[]) {
	//test1("test16m_lmax_close1.wav");
	//test1("test16m_lmax_far1.wav");
	//test1("eye_far2.wav");
	//analyzeEnergy("long.wav", 40);
	analyzeFreq("long.wav");

	return 0;
}
