/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2010 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * continuous.c - Simple pocketsphinx command-line application to test
 *                both continuous listening/silence filtering from microphone
 *                and continuous file transcription.
 */

/*
 * This is a simple example of pocketsphinx application that uses continuous listening
 * with silence filtering to automatically segment a continuous stream of audio input
 * into utterances that are then decoded.
 * 
 * Remarks:
 *   - Each utterance is ended when a silence segment of at least 1 sec is recognized.
 *   - Single-threaded implementation for portability.
 *   - Uses audio library; can be replaced with an equivalent custom library.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <string>

#include <sys/time.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#else
#include <sys/select.h>
#endif

#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>

#include "pocketsphinx.h"


#include <sys/ioctl.h>
#include <termios.h>

bool kbhit()
{
    termios term;
    tcgetattr(0, &term);

    termios term2 = term;
    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);

    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);

    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}



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

static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    {"-inmic",
     ARG_BOOLEAN,
     "no",
     "Transcribe audio from microphone."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION
};

static ps_decoder_t *ps;
static cmd_ln_t *config;
static FILE *rawfd;

static void
print_word_times()
{
    int frame_rate = cmd_ln_int32_r(config, "-frate");
    ps_seg_t *iter = ps_seg_iter(ps);
    while (iter != NULL) {
        int32 sf, ef, pprob;
        float conf;

        ps_seg_frames(iter, &sf, &ef);
        pprob = ps_seg_prob(iter, NULL, NULL, NULL);
        conf = logmath_exp(ps_get_logmath(ps), pprob);
        printf("%s %.3f %.3f %f\n", ps_seg_word(iter), ((float)sf / frame_rate),
               ((float) ef / frame_rate), conf);
        iter = ps_seg_next(iter);
    }
}

static int
check_wav_header(char *header, int expected_sr)
{
    int sr;

    if (header[34] != 0x10) {
        E_ERROR("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
        E_ERROR("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
        E_ERROR("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
        E_ERROR("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}

/*
 * Continuous recognition from a file
 */
static void
recognize_from_file()
{
    int16 adbuf[2048];
    const char *fname;
    const char *hyp;
    int32 k;
    uint8 utt_started, in_speech;
    int32 print_times = cmd_ln_boolean_r(config, "-time");

    fname = cmd_ln_str_r(config, "-infile");
    if ((rawfd = fopen(fname, "rb")) == NULL) {
        E_FATAL_SYSTEM("Failed to open file '%s' for reading",
                       fname);
    }
    
    if (strlen(fname) > 4 && strcmp(fname + strlen(fname) - 4, ".wav") == 0) {
        char waveheader[44];
	fread(waveheader, 1, 44, rawfd);
	if (!check_wav_header(waveheader, (int)cmd_ln_float32_r(config, "-samprate")))
    	    E_FATAL("Failed to process file '%s' due to format mismatch.\n", fname);
    }

    if (strlen(fname) > 4 && strcmp(fname + strlen(fname) - 4, ".mp3") == 0) {
	E_FATAL("Can not decode mp3 files, convert input file to WAV 16kHz 16-bit mono before decoding.\n");
    }
    
    ps_start_utt(ps);
    utt_started = FALSE;

    while ((k = fread(adbuf, sizeof(int16), 2048, rawfd)) > 0) {
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
        if (in_speech && !utt_started) {
            utt_started = TRUE;
        } 
        if (!in_speech && utt_started) {
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL);
            if (hyp != NULL)
        	printf("%s\n", hyp);
            if (print_times)
        	print_word_times();
            fflush(stdout);

            ps_start_utt(ps);
            utt_started = FALSE;
        }
    }
    ps_end_utt(ps);
    if (utt_started) {
        hyp = ps_get_hyp(ps, NULL);
        if (hyp != NULL) {
    	    printf("%s\n", hyp);
    	    if (print_times) {
    		print_word_times();
	    }
	}
    }
    
    fclose(rawfd);
}

/* Sleep for specified msec */
static void
sleep_msec(int32 ms)
{
#if (defined(_WIN32) && !defined(GNUWINCE)) || defined(_WIN32_WCE)
    Sleep(ms);
#else
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
#endif
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int sendMessage(std::string host, std::string call, std::string body) {

	std::string message = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\
<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\
 <s:Body>\
  <u:" + call + " xmlns:u=\"urn:Belkin:service:basicevent:1\">" + body + "</u:" + call + ">\
 </s:Body>\
</s:Envelope>";

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 49153;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(host.c_str());
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    std::string buffer;
    buffer += "POST /upnp/control/basicevent1 HTTP/1.1\r\n";
    buffer += "SOAPACTION: \"urn:Belkin:service:basicevent:1#" + call + "\"\r\n";
	buffer += "Content-Length: " + std::to_string(message.length()) + "\r\n";
	buffer += "Content-Type: text/xml; charset=\"utf-8\"\r\n";
	buffer += "HOST: " + host + ":" + std::to_string(portno) + "\r\n";
	buffer += "User-Agent: CyberGarage-HTTP/1.0\r\n";
	buffer += "\r\n";
	buffer += message;
    n = write(sockfd,buffer.c_str(),buffer.length());
    if (n < 0) 
         error("ERROR writing to socket");
    char inbuffer[1000];
    bzero(inbuffer,1000);
    n = read(sockfd,inbuffer,1000);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("n=%d\n%s\n", n, inbuffer);
    bzero(inbuffer,1000);
    n = read(sockfd,inbuffer,1000);
    if (n < 0) 
         error("ERROR reading from socket");
    printf("n=%d\n%s\n", n, inbuffer);
    close(sockfd);
    return 0;
}
/*
int main(int argc, char *argv[]) {

	std::string host = "10.0.0.230";

	std::string call_get = "GetBinaryState";
	std::string body_get = "";
	
	std::string call_set = "SetBinaryState";
	std::string body_set_on = "<BinaryState>1</BinaryState>";
	std::string body_set_off = "<BinaryState>0</BinaryState>";

	//sendMessage(host, call_get, body_get);
	sendMessage(host, call_set, body_set_off);
	//sendMessage(host, call_set, body_set_on);
}
*/

	std::string host = "10.0.0.230";

	std::string call_get = "GetBinaryState";
	std::string body_get = "";
	
	std::string call_set = "SetBinaryState";
	std::string body_set_on = "<BinaryState>1</BinaryState>";
	std::string body_set_off = "<BinaryState>0</BinaryState>";

/*
 * Main utterance processing loop:
 *     for (;;) {
 *        start utterance and wait for speech to process
 *        decoding till end-of-utterance silence will be detected
 *        print utterance result;
 *     }
 */
static void
recognize_from_microphone()
{
    ad_rec_t *ad;
    int16 adbuf[2048];
    uint8 utt_started, in_speech;
    int32 k;
    char const *hyp;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    if (ps_start_utt(ps) < 0)
        E_FATAL("Failed to start utterance\n");
    utt_started = FALSE;
    E_INFO("Ready....\n");

    for (;;) {
        if ((k = ad_read(ad, adbuf, 2048)) < 0)
            E_FATAL("Failed to read audio\n");
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
        if (in_speech && !utt_started) {
            utt_started = TRUE;
            E_INFO("Listening...\n");
        }
        if (!in_speech && utt_started) {
            // speech -> silence transition, time to start new utterance
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL );
            if (hyp != NULL) {
                printf("==========================================================\n");
                printf("=======================%s===========================\n", hyp);
                printf("==========================================================\n");
                if (strcmp(hyp, "LUMUS MAXIMA") == 0) {
					sendMessage(host, call_set, body_set_on);
				} else {
					if (strcmp(hyp, "LUMUS MINIMA") == 0) {
						sendMessage(host, call_set, body_set_off);
					}
				}
                
                fflush(stdout);
            }

            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            utt_started = FALSE;
            E_INFO("Ready....\n");
        }
        sleep_msec(100);
    }
    ad_close(ad);
}

static void
recognize_from_microphone2(const char* fileName)
{
    ad_rec_t *ad;
    const int bufSize = 16000 * 60;
    int16 adbuf[bufSize];
    int16* bufPtr;
    uint8 utt_started, in_speech;
    int32 k;
    char const *hyp;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");
    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");
/*
    if (ps_start_utt(ps) < 0)
        E_FATAL("Failed to start utterance\n");
    utt_started = FALSE;
    E_INFO("Ready....\n");
*/

	FILE* fout = fopen(fileName, "w");

	TEnergyCounter ec(50, 50);
	uint32_t threshold = ec.EBufferSize * ec.EBufferSize * 25000;
	long SpeachEnd = -1;
	long interval = (long)(16000 * 0.6);
	long time = 0;
	bufPtr = adbuf;
	int16* speechStart = NULL;

    while (!kbhit()) {
        if ((k = ad_read(ad, bufPtr, 2048)) < 0)
            E_FATAL("Failed to read audio\n");

		if (k > 0) {
			fwrite(bufPtr, 2, k, fout);

			for (int i = 0; i < k; ++i, ++time) {
				ec.Add(bufPtr[i]);
				if (time < SpeachEnd) {
					continue;
				}

				if (ec.ESum > threshold) {
					if (time > SpeachEnd) {
						printf("Start\n");
					}
					SpeachEnd = time + interval;  
				} else {
					if (time == SpeachEnd) {
						printf("End\n");
					}
				}
			}
		} else {
			//printf("wait\n");
			sleep_msec(100);
		}
/*
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
        if (in_speech && !utt_started) {
            utt_started = TRUE;
            E_INFO("Listening...\n");
        }
        if (!in_speech && utt_started) {
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL );
            if (hyp != NULL) {
                printf("==========================================================\n");
                printf("=======================%s===========================\n", hyp);
                printf("==========================================================\n");
                if (strcmp(hyp, "LUMUS MAXIMA") == 0) {
					sendMessage(host, call_set, body_set_on);
				} else {
					if (strcmp(hyp, "LUMUS MINIMA") == 0) {
						sendMessage(host, call_set, body_set_off);
					}
				}
                
                fflush(stdout);
            }

            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            utt_started = FALSE;
            E_INFO("Ready....\n");
        }
*/
    }
    ad_close(ad);
    fclose(fout);
}

static void
recognize_from_microphone3(const char* fileName) {

	const int BUFFER_MS = 60000;
	const int MIN_PHRASE_MS = 500;
	const int MAX_PHRASE_MS = 10000;

	const int ENERGY_SAMPLE_MS = 10;
	
	const int INIT_BUFFER_SEC = 2;

	int sampleRate = (int) cmd_ln_float32_r(config, "-samprate");
	printf("Sample rate: %d\n", sampleRate);

	const int bufSize = sampleRate * INIT_BUFFER_SEC / 1000;
	const int minPhraseSamples = sampleRate * MIN_PHRASE_MS / 1000;
	const int maxPhraseSamples = sampleRate * MAX_PHRASE_MS / 1000;
	const int energySamples = sampleRate * ENERGY_SAMPLE_MS / 1000;

    ad_rec_t *ad;
    int16_t* adbuf = new int16_t[bufSize];
    int bufHead;
    uint8 utt_started, in_speech;
    char const *hyp;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"), sampleRate)) == NULL)
        E_FATAL("Failed to open audio device\n");

    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

	FILE* fout = fopen(fileName, "w");

	int time = 0;
	bufHead = 0;

	while (!kbhit()) {

		while (TRUE) {
			int32 chunkSize = ad_read(ad, adbuf + bufHead, bufSize - bufHead);
			if (chunkSize < 0)
				E_FATAL("Failed to read audio\n");

			bufHead += chunkSize;
			if (bufHead < bufSize) {
				break;
			}
			// bufHead == bufSize here

			memcpy(adbuf, adbuf + bufSize - maxPhraseSamples, maxPhraseSamples * sizeof(int16_t));
			bufHead = maxPhraseSamples;
		}

		if (bufHead >= minPhraseSamples) {
			int samplesHead = bufHead;
			while ((samplesHead -= energySamples) > 0) {
				int32_t sampleEnergy = countEnergy(adbuf + samplesHead, energySamples);
			}
		} else {
			sleep_msec(100);
		}
/*
			fwrite(bufPtr, 2, k, fout);

			for (int i = 0; i < k; ++i, ++time) {
				ec.Add(bufPtr[i]);
				if (time < SpeachEnd) {
					continue;
				}

				if (ec.ESum > threshold) {
					if (time > SpeachEnd) {
						printf("Start\n");
					}
					SpeachEnd = time + interval;
				} else {
					if (time == SpeachEnd) {
						printf("End\n");
					}
				}
			}
*/
	}

	ad_close(ad);
	fclose(fout);
	delete adbuf;
}

void detectVoice(const int16* bufferStart,
                 int startIdx,
                 int endIdx,
                 int freq,
                 int& voiceStart,
                 int& voiceEnd) {
	voiceStart = endIdx;
	voiceEnd = -1;

	int16 lastValue = 0;
	unsigned energy = 0;
	auto lastZ = endIdx;
	int firstVoicePeriod = -1;
	while (--endIdx >= startIdx && endIdx >= voiceStart - 300 * freq / 1000) {
		currValue = bufferStart[endIdx];
		energy += abs(currValue);
		if (lastValue < 0 && currValue > 0) {
			auto currPeriod = lastZ - endIdx;
			if (energy / 100 >= currPeriod && freq / 85 > currPeriod) {
				if (firstVoicePeriod >= 0) {
					if (endIdx <= firstVoicePeriod - 50 * freq / 1000) {
						if (voiceEnd < 0) {
							voiceEnd = firstVoicePeriod;
						}
						voiceStart = endIdx;
					}
				} else {
					firstVoicePeriod = lastZ;
				}
			} else {
				firstVoicePeriod = -1;
			}
			lastZ = endIdx;
		}
		lastValue = currValue;
	}
}

static void
recognize_from_microphone4()
{
	struct timeval t;

	int freq = 16000;
	int MAX_DELAY_BUFFER = 60 * freq;
	int MAX_PHRASE_BUFFER = 3 * freq;
	int MIN_BUFFER = freq / 10; // at least 100ms

	gettimeofday(&t, 0);
	printf("open:%ld.%06ld\n", t.tv_sec, t.tv_usec);
	ad_rec_t* ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"), freq);
    if (ad == NULL)
        E_FATAL("Failed to open audio device\n");

	gettimeofday(&t, 0);
	printf("start:%ld.%06ld\n", t.tv_sec, t.tv_usec);

	if (ad_start_rec(ad) < 0)
		E_FATAL("Failed to start recording\n");

	int16* adbuf = new int16[MAX_DELAY_BUFFER + MAX_PHRASE_BUFFER];
	int buffIdx = 0;
	int detectStart = 0;

	int startBoundary = -1;
	int endBoundary = -1;

	while (!kbhit()) {

		gettimeofday(&t, 0);
		printf("read:%ld.%06ld\n", t.tv_sec, t.tv_usec);


		int32_t k = ad_read(ad, adbuf + bufIdx, MAX_DELAY_SEC + MAX_PHRASE_SEC - buffIdx);
		if (k < 0)
			E_FATAL("Failed to read audio\n");

		buffIdx += k;

		if (buffIdx - detectStart > MIN_BUFFER) {
			gettimeofday(&t, 0);
			printf("OK:%ld.%06ld %d bytes\n", t.tv_sec, t.tv_usec, k);
			
			int voiceStart;
			int voiceEnd;
			detectVoice(adbuf, detectStart, buffIdx, freq, voiceStart, voiceEnd);
			if (voiceEnd >= 0) {
				auto startBoundary2 = voiceStart - 300 * freq / 1000;
				if (startBoundary < 0 || startBoundary2 > detectStart) {
					startBoundary = startBoundary2 > 0 ? startBoundary2 : 0;
				}
				endBoundary = voiceEnd + 300 * freq / 1000;
				if (endBoundary > buffIdx) {
					// in speach
					detectStart = buffIdx;
					//TODO; check the buffer
					continue;
				}
				// speach is ended, recognize
			} else {
				// no more voice detected
				if (startBoundary < 0) {
					// no voice at all
					buffIdx = 0;
					continue;
				}

				if (endBoundary > buffIdx) {
					endBoundary = buffIdx;
				}
				// recognize 
			}

			// run recognition
			startBoundary = -1;
			endBoundary = -1;
			buffIdx = 0;

		} else {
			gettimeofday(&t, 0);
			printf("sleep:%ld.%06ld\n", t.tv_sec, t.tv_usec);
			sleep_msec(100);
		}
    }
    ad_close(ad);
    delete adbuf;
}

/*
export LD_LIBRARY_PATH=/usr/local/lib
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
pocketsphinx_continuous -hmm /usr/local/share/pocketsphinx/model/en-us/en-us -lm 6049.lm -dict 6049.dic -samprate 16000/8000/48000 -inmic yes -adcdev plughw:1


gcc -o wvc continuous.c -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`


g++ -o wvc wvc.C -std=c++11 -DMODELDIR=\"`pkg-config --variable=modeldir pocketsphinx`\" `pkg-config --cflags --libs pocketsphinx sphinxbase`


./wvc -hmm /usr/local/share/pocketsphinx/model/en-us/en-us -lm 8531.lm -dict 8531.dic -samprate 16000/8000/48000 -inmic yes -adcdev plughw:1
./wvc -hmm /usr/local/share/pocketsphinx/model/en-us/en-us -lm 8531.lm -dict 8531.dic -samprate 16000/8000/48000 -inmic yes -adcdev plughw:2


*/

	struct Wav                          // WAVE File Header
	{
        // RIFF chunk descriptor
        char ChunkID[4];                // "RIFF"              
        int ChunkSize;                  // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
        char Format[4];                 // "WAVE"
        // fmt sub-chunk
        char Subchunk1ID[4];            // "fmt "
        int Subchunk1Size;              // bytes remaining in subchunk, 16 if uncompressed
        short AudioFormat;              // 1 = uncompressed
        short NumChannels;              // mono or stereo
        int SampleRate;                
        int ByteRate;                   // == SampleRate * NumChannels * BitsPerSample/8
        short BlockAlign;               // == NumChannels * BitsPerSample/8
        short BitsPerSample;
        // data sub-chunk
        char Subchunk2ID[4];            // "data"
        int Subchunk2Size;              // == NumSamples * NumChannels * BitsPerSample/8
    };

void analyzeEnergy(const char* inFileName, int winSize) {

	double freq = 16000.0;

	FILE* inFile = fopen(inFileName, "r");

	if (inFile == NULL) {
		E_FATAL_SYSTEM("Failed to open file '%s' for reading", inFileName);
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

using namespace std;
#include <iostream>

void wavTest(const char* fileName) {
	
	struct Wav HeaderInfo;
	
	FILE* fptr;                         // File pointer
	fptr = fopen(fileName,"r");    // Open wav file for reading           
	fread(&HeaderInfo, sizeof(Wav), 1, fptr);
	fclose(fptr);

    //***** OUTPUT WAV HEADER *****//
    cout << "Chunk ID: " << HeaderInfo.ChunkID[0] << HeaderInfo.ChunkID[1] << HeaderInfo.ChunkID[2] << HeaderInfo.ChunkID[3] << endl;
    cout << "Chunk Size: " << HeaderInfo.ChunkSize << endl;
    cout << "Format: " << HeaderInfo.Format[0] << HeaderInfo.Format[1] << HeaderInfo.Format[2] << HeaderInfo.Format[3] <<endl;
    cout << "Sub-chunk1 ID: " << HeaderInfo.Subchunk1ID[0] << HeaderInfo.Subchunk1ID[1] << HeaderInfo.Subchunk1ID[2] << HeaderInfo.Subchunk1ID[3] <<endl;
    cout << "Sub-chunk1 Size: " << HeaderInfo.Subchunk1Size << endl;
    cout << "Audio Format: " << HeaderInfo.AudioFormat << endl;
    cout << "Number of Channels: " << HeaderInfo.NumChannels << endl;
    cout << "Sample Rate: " << HeaderInfo.SampleRate << endl;
    cout << "Byte Rate: " << HeaderInfo.ByteRate << endl;
    cout << "Block Align: " << HeaderInfo.BlockAlign << endl;
    cout << "Bits Per Sample: " << HeaderInfo.BitsPerSample << endl;
    cout << "Sub-chunk2 ID: " << HeaderInfo.Subchunk2ID[0] << HeaderInfo.Subchunk2ID[1] << HeaderInfo.Subchunk2ID[2] << HeaderInfo.Subchunk2ID[3] << endl;
    cout << "Sub-chunk2 Size: " << HeaderInfo.Subchunk2Size << endl << endl;

    float NumSamples = HeaderInfo.Subchunk2Size/(HeaderInfo.BitsPerSample/8);
    cout << "Sampling Frequency: " <<  HeaderInfo.SampleRate << endl;
    cout << "Number of Samples: " << fixed << NumSamples << endl;
    cout << "Length of File: " << NumSamples/HeaderInfo.SampleRate << " secs" <<  endl << endl;
}

void writeWav(const char* fileName, void* buffer, int NumSamples) {
	struct Wav HeaderInfo;
	memcpy(HeaderInfo.ChunkID, "RIFF", 4);
	memcpy(HeaderInfo.Format, "WAVE", 4);
	memcpy(HeaderInfo.Subchunk1ID, "fmt ", 4);
	HeaderInfo.Subchunk1Size = 16;
	HeaderInfo.AudioFormat = 1;
	HeaderInfo.NumChannels = 1;
	HeaderInfo.SampleRate = 16000;                
	HeaderInfo.BitsPerSample = 16;
	HeaderInfo.BlockAlign = HeaderInfo.NumChannels * HeaderInfo.BitsPerSample/8;
	HeaderInfo.ByteRate = HeaderInfo.SampleRate * HeaderInfo.BlockAlign;
	memcpy(HeaderInfo.Subchunk2ID, "data", 4);
	HeaderInfo.Subchunk2Size = NumSamples * HeaderInfo.BlockAlign;
	HeaderInfo.ChunkSize = 4 + (8 + HeaderInfo.Subchunk1Size) + (8 + HeaderInfo.Subchunk2Size);

	FILE* outFile = fopen(fileName, "w");
	fwrite(&HeaderInfo, sizeof(HeaderInfo), 1, outFile);
	fwrite(buffer, sizeof(int16_t), NumSamples, outFile);
	fclose(outFile);
}

void wavTest2() {
	char* buffer = new char[10000000];
	FILE* inFile = fopen("out1.bin", "r");
	size_t size = fread(buffer, 1, 10000000, inFile);
	fclose(inFile);
	writeWav("long.wav", buffer, size / 2);
	delete buffer;
}
/*
int
main(int argc, char *argv[])
{
	//wavTest("test33.wav");
	//wavTest2();
	analyzeEnergy("long.wav", 320);
	return 0;
}
*/


int
main(int argc, char *argv[])
{

    char const *cfg;

    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }

    if (config == NULL || (cmd_ln_str_r(config, "-infile") == NULL && cmd_ln_boolean_r(config, "-inmic") == FALSE)) {
	E_INFO("Specify '-infile <file.wav>' to recognize from file or '-inmic yes' to recognize from microphone.\n");
        cmd_ln_free_r(config);
	return 1;
    }

    ps_default_search_args(config);
    ps = ps_init(config);
    if (ps == NULL) {
        cmd_ln_free_r(config);
        return 1;
    }

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (cmd_ln_str_r(config, "-infile") != NULL) {
        recognize_from_file();
    } else if (cmd_ln_boolean_r(config, "-inmic")) {
        recognize_from_microphone4();
    }

    ps_free(ps);
    cmd_ln_free_r(config);

    return 0;
}

