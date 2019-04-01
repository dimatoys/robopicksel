#include "fpga.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define CAMERA_ADDRESS_OV2640 0x30
#define CAMERA_ADDRESS_OV7670 0x21
#define CAMERA_ADDRESS_MT9 0x48


void test_ov2640(FPGA& fpga) {
	bool status = fpga.WriteIIC(CAMERA_ADDRESS_OV2640, 0xFF, 0x01, false);

	printf("Wrote: %s\n", status ? "SUCCESS" : "FAIL");

	uint8_t value;
	status = fpga.ReadIIC(CAMERA_ADDRESS_OV2640, 0x0A, value, false);

	printf("Read1: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

	status = fpga.ReadIIC(CAMERA_ADDRESS_OV2640, 0x0B, value, false);

	printf("Read2: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

}

void test_ov7670(FPGA& fpga) {

	uint8_t value;
	bool status = fpga.ReadIIC(CAMERA_ADDRESS_OV7670, 0x0A, value, false);

	printf("Read1: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

	status = fpga.ReadIIC(CAMERA_ADDRESS_OV7670, 0x0B, value, false);

	printf("Read2: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

}

void test_mt9(FPGA& fpga) {

	uint8_t value;
	bool status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x0A, value, false);

	printf("Read1: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x0B, value, false);

	printf("Read2: %s value %X\n", status ? "SUCCESS" : "FAIL", (uint32_t)value);

}

void test_mt9_2(FPGA& fpga) {

	uint8_t valueH, valueL;
	bool status;
	uint32_t value;

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x00, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );


	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x01, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x02, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x03, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x04, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x05, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x06, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x08, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x09, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x0A, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );

	fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0x0B, valueH, false);
	status = fpga.ReadIIC(CAMERA_ADDRESS_MT9, 0xF0, valueL, false);
	value = valueH * 256 + valueL;
	printf("Read: %s value %04X %d\n", status ? "SUCCESS" : "FAIL", value, value );
}

void test_mt9_3(FPGA& fpga) {

	while(true) {
		uint32_t r5 = fpga.ReadReg(PIO_REG5_BASE);
		uint32_t r6 = fpga.ReadReg(PIO_REG6_BASE);
		printf("%u %u %u\n", r5, r6 / 0x10000, r6 & 0xFFFF);
	}
}

void test_mt9_4(FPGA& fpga) {

	while(true) {
		uint32_t r5 = fpga.ReadReg(PIO_REG5_BASE);
		printf("%u %u\n", r5 / 0x10000, r5 & 0xFFFF);
	}
}

void test_mt9_mem1(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0);
	fpga.WriteReg(PIO_REG2_BASE, 1);
	usleep(1000*1000);
	fpga.DumpMem(100);
}

void test_mem1(FPGA& fpga) {
	fpga.WriteMem(0x10,0x56);
	fpga.WriteMem(0x06,0x21);
	fpga.DumpMem(20);
}

void test_mem2(FPGA& fpga) {

	fpga.WriteReg(PIO_REG2_BASE, 0x56);
	fpga.WriteReg(PIO_REG1_BASE, 0x0006 | 0x6000);
	fpga.WriteReg(PIO_REG1_BASE, 0);

	fpga.WriteReg(PIO_REG2_BASE, 0x42);
	fpga.WriteReg(PIO_REG1_BASE, 0x0010 | 0x6000);
	fpga.WriteReg(PIO_REG1_BASE, 0);

	fpga.WriteReg(PIO_REG1_BASE, 0x0006 | 0x4000);
	uint32_t r3 = fpga.ReadReg(PIO_REG3_BASE);
	printf("r3 = %u\n", r3);

}

void test_mem3(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0xFFFFFFFF);
	fpga.WriteReg(PIO_REG1_BASE, 0x10000);
	fpga.WriteReg(PIO_REG1_BASE, 0x0);

	fpga.WriteReg(PIO_REG2_BASE, 0xFFFFFFFA);
	fpga.WriteReg(PIO_REG1_BASE, 0x1FFFF);
	fpga.WriteReg(PIO_REG1_BASE, 0x0);

	fpga.WriteReg(PIO_REG2_BASE, 0xFFFFFFFB);
	fpga.WriteReg(PIO_REG1_BASE, 0x1FFFE);


	fpga.WriteReg(PIO_REG1_BASE, 0x0000);
	uint32_t v1 = fpga.ReadReg(PIO_REG3_BASE);

	fpga.WriteReg(PIO_REG1_BASE, 0xFFFF);
	uint32_t v2 = fpga.ReadReg(PIO_REG3_BASE);

	fpga.WriteReg(PIO_REG1_BASE, 0xFFFE);
	uint32_t v3 = fpga.ReadReg(PIO_REG3_BASE);

	printf("v1=%x v2=%x v3=%x\n", v1, v2, v3);
}

void test_mem4(FPGA& fpga) {
	fpga.DumpMem(100);
}

void DumpMemLight(FPGA& fpga, uint32_t num) {
	for (uint32_t address = 0; address < num; ++address) {
		fpga.WriteReg(PIO_REG1_BASE, address);
		uint32_t v = fpga.ReadReg(PIO_REG3_BASE);
		printf("%5X:(%3u,%3u) %3X %4u\n", address, address % 752, address / 752, v,v);
	}
}

#include <iostream>
#include <fstream>

void SaveMemLight(FPGA& fpga, uint32_t num, const char* file_name, uint32_t mask) {
	std::ofstream f;
	f.open(file_name);
	f << num << std::endl;
	for (uint32_t address = 0; address < num; ++address) {
		fpga.WriteReg(PIO_REG1_BASE, address);
		uint32_t v = fpga.ReadReg(PIO_REG3_BASE) & mask;
		f << v << std::endl;
	}
	f.close();
}

void test_mt9_5(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0);
	fpga.WriteReg(PIO_REG2_BASE, 1);

	usleep(100*1000);

	DumpMemLight(fpga, 100000);

}

void test_mt9_6(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0);
	fpga.WriteReg(PIO_REG2_BASE, 1);

	usleep(100*1000);
	//fpga.WriteReg(PIO_REG2_BASE, 0);

	SaveMemLight(fpga, 100000, "data.csv", 0x03f);

}
void test_mt9_7(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0);
	fpga.WriteReg(PIO_REG2_BASE, 1);

	usleep(100*1000);
	//fpga.WriteReg(PIO_REG2_BASE, 0);

	SaveMemLight(fpga, 270720, "data.csv", 0x00FF);
}

void Save(uint8_t* buffer, uint32_t size, const char* file_name) {
	std::ofstream f;
	f.open(file_name);
	f << size << std::endl;
	for (uint32_t address = 0; address < size; ++address) {
		f << (uint32_t)buffer[address] << std::endl;
	}
	f.close();
}


void test_mt9_8(FPGA& fpga) {
	fpga.WriteReg(PIO_REG2_BASE, 0);
	fpga.WriteReg(PIO_REG2_BASE, 1);
	auto t = clock();
	uint32_t status = 0;
	for (int i = 0; i < 200000;++i) {
		status = fpga.ReadReg(PIO_REG4_BASE) & 0x3;
		if ((status & 0x01) == 0) {
			break;
		}
	}
	auto t1 = clock();

	uint32_t buffer[67680];
	uint32_t* ptr = buffer;
	for (uint32_t address = 0; address < 67680; ++address) {
		fpga.WriteReg(PIO_REG1_BASE, address);
		*ptr++ = fpga.ReadReg(PIO_REG3_BASE);
	}
	
	auto t2 = clock();

	printf("t1=%012.9f t2=%012.9f status= %03X\n", ((float)(t1 - t)) / CLOCKS_PER_SEC,
		((float)(t2 - t1)) / CLOCKS_PER_SEC, status);

	Save((uint8_t*)buffer, 270720, "data.csv");
}

int main(int argc, char **argv)
{
	FPGA fpga;
	//test_ov2640(fpga);
	//test_ov7670(fpga);
	//test_mt9_2(fpga);
	//test_mt9_3(fpga);
	//test_mt9_mem1(fpga);
	//test_mem1(fpga);
	//test_mem2(fpga);
	//test_mem3(fpga);
	//test_mem4(fpga);
	//test_mt9_5(fpga);
	//test_mt9_6(fpga);
	//test_mt9_7(fpga);
	test_mt9_8(fpga);
	return 0;
}

