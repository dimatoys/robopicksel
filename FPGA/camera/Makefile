#
TARGET = camera
TARGET2 = test_mem
TARGET3 = dumpjpeg

REMOTE_ACCOUNT = debian@10.0.0.158
REMOTE_RACCOUNT = root@10.0.0.158
REMOTE_PATH = projects/camera
REMOTE_DIR = $(REMOTE_ACCOUNT):$(REMOTE_PATH)

FPGA_ROOT = ..
#SOCEDS_DEST_ROOT = /cygdrive/c/intelFPGA/16.1/embedded
PLATFORM=soc_cv_av

#
CROSS_COMPILE = ${FPGA_ROOT}/debian/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
CFLAGS = -g -Wall  -I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include -I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include/$(PLATFORM) -D$(PLATFORM) -std=c++11
LDFLAGS =  -g -Wall -lstdc++
CC = $(CROSS_COMPILE)gcc
ARCH= arm

LOCAL_CC = gcc


build: $(TARGET3)

$(TARGET): camera.o fpga.o
	$(CC) $(LDFLAGS)   $^ -o $@

$(TARGET2): test_mem.o
	$(CC) $(LDFLAGS)   $^ -o $@  

$(TARGET3): dumpjpeg.o jpeg.o
	g++ $(LDFLAGS) $^ -o $@ -ljpeg

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.C
	$(LOCAL_CC) -g -Wall -std=c++11 -c $< -o $@

deploy: $(TARGET)
	rsync $(TARGET) $(REMOTE_DIR)
	#rsync $(TARGET2) $(REMOTE_DIR)

run: $(TARGET)
	ssh $(REMOTE_RACCOUNT) ~debian/$(REMOTE_PATH)/$(TARGET)

rundata: $(TARGET)
	rm -f data.csv pic.jpg
	ssh $(REMOTE_RACCOUNT) rm -f data.csv
	ssh $(REMOTE_RACCOUNT) ~debian/$(REMOTE_PATH)/$(TARGET)
	rsync -z $(REMOTE_RACCOUNT):data.csv .
	./$(TARGET3)


mem: $(TARGET2)
	ssh $(REMOTE_RACCOUNT) ~debian/$(REMOTE_PATH)/$(TARGET2)


.PHONY: clean
clean:
	rm -f $(TARGET) $(TARET2) *.a *.o *~ 

