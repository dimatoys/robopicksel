#include <stdint.h>
#include "hps_0.h"

class FPGA {
	void init_mem();
	void close_mem();

	bool WaitIIC(uint32_t& vr3);
	bool WaitIICDebug(uint32_t& vr3, bool csv=false, int limit=10000);

public:

	int Fd;
	void* VirtualBase;
	void* AxiVirtualBase;

	FPGA() {
		Fd = -1;
		VirtualBase = (void*)0;
		AxiVirtualBase = (void*)0;
		init_mem();
	}

	~FPGA() {
		close_mem();
	}

	//lightweight HPS-to-FPGA bridge
	void* GetLightweight();

	void WriteReg(uint32_t reg, uint32_t value);
	uint32_t ReadReg(uint32_t reg);

	bool WriteIIC(uint8_t address, uint8_t reg, uint8_t value, bool debug = false);
	bool ReadIIC(uint8_t address, uint8_t reg, uint8_t& value, bool debug = false);

	void DumpMem(uint32_t size);
	void WriteMem(uint32_t address, uint8_t value);
};

