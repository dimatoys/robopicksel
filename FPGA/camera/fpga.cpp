#include "fpga.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "hps_0.h"

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

//setting for the HPS2FPGA AXI Bridge
#define ALT_AXI_FPGASLVS_OFST (0xC0000000) // axi_master
#define HW_FPGA_AXI_SPAN (0x40000000) // Bridge span 1GB
#define HW_FPGA_AXI_MASK ( HW_FPGA_AXI_SPAN - 1 )

#define MAP_ADDRESS(virtual_base, byte_address) ALT_CAST(void*, ALT_CAST(char*, virtual_base) + ((byte_address) & HW_REGS_MASK))
#define MAP_AXI_ADDRESS(axi_virtual_base, byte_address) ALT_CAST(void*, ALT_CAST(char*, axi_virtual_base) + ((byte_address) & HW_FPGA_AXI_MASK))

const uint32_t IIC_MODE_WRITE    = 0x00;
const uint32_t IIC_MODE_READ     = 0x80;

const uint32_t IIC_ENABLE        = 0x1000000;

#define IIC_REG(_reg)  ((((uint32_t)_reg) & 0xFF ) << 8)
#define IIC_WR_VALUE(_val)  ((((uint32_t)_val) & 0xFF) << 16)

#define IIC_BUSY(_r3)  (((_r3) & 0x100) > 0)
#define IIC_VALUE(_r3) ((_r3) & 0xFF)
#define IIC_ASK(_r3)   (((_r3) >> 9) & 7)
#define IIC_CLOCK(_r3)  ((_r3) >> 12)

static const char* CSTATE[] = {
	"CSTATE_START     ",
	"CSTATE_CLOCK_DOWN",
	"CSTATE_SET_BIT   ",
	"CSTATE_CLOCK_UP  ",
	"CSTATE_STOP      ",
	"CSTATE_END       "
};

static const char* TSTATE[] = {	
	"TSTATE_ADDR       ",
	"TSTATE_RW         ",
	"TSTATE_ASK_ADDR   ",
	"TSTATE_REGISTER   ",
	"TSTATE_ASK_REG    ",
	"TSTATE_WR_DATA    ",
	"TSTATE_ASK_WR     ",
	"TSTATE_R_START    ",
	"TSTATE_ADDR_RD    ",
	"TSTATE_ADDR_RD_1  ",
	"TSTATE_ASK_ADDR_RD",
	"TSTATE_RD_DATA    ",
	"TSTATE_RD_NASK    ",
	"TSTATE_END        ",
	"TSTATE_STOP       ",
	"TSTATE_ASK_VAL_RD ",
	"TSTATE_ASK_WR1    "
};


void FPGA::init_mem() {
	Fd = open( "/dev/mem", O_RDWR | O_SYNC);
	if (Fd == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		exit(1);
	}
}

//lightweight HPS-to-FPGA bridge
void* FPGA::GetLightweight() {
	if (VirtualBase == (void*)0) {
		VirtualBase = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, Fd, HW_REGS_BASE );
		if( VirtualBase != MAP_FAILED ) {

			AxiVirtualBase = mmap( NULL, HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, Fd, ALT_AXI_FPGASLVS_OFST );

			// ./mem 0xFFD0501C 7 - 2 - 1   - page 3-28 of cv_5_HPS_tech_ref.pdf
			// ./mem 0xFF800000 0x10 + 0x8 - page 7-26 of cv_5_HPS_tech_ref.pdf

			void* map_byte_addr = MAP_ADDRESS(VirtualBase, 0xFFD0501C);
			uint32_t data = alt_read_word(map_byte_addr);
			printf ("0xFFD0501C=%u\n", data);

			bool need_reset_lw = (data & 2) > 0;
			bool need_reset_hpsToFpga = (data & 1) > 0;

			if (need_reset_lw || need_reset_hpsToFpga) {
				// not initialized yet
				data -= (need_reset_lw ? 2 : 0) + (need_reset_hpsToFpga ? 1 : 0);
				alt_write_word(map_byte_addr, data);

				alt_write_word(MAP_ADDRESS(VirtualBase, 0xFF800000), (need_reset_lw ? 0x10 : 0) + (need_reset_hpsToFpga ? 0x8: 0));
			}

		} else {
			VirtualBase = (void*)0;
			printf( "ERROR: mmap() failed...\n" );
			close_mem();
		}
	}
	return VirtualBase;
}

void FPGA::close_mem() {
	if (AxiVirtualBase != (void*)0) {
		munmap( VirtualBase, HW_FPGA_AXI_SPAN );
		AxiVirtualBase = (void*)0;
	}

	if (VirtualBase != (void*)0) {
		munmap( VirtualBase, HW_REGS_SPAN );
		VirtualBase = (void*)0;
	}

	if (Fd != -1) {
		close(Fd);
		Fd = -1;
	}
}

void FPGA::WriteReg(uint32_t reg, uint32_t value) {
	alt_write_word(MAP_ADDRESS(GetLightweight(), ALT_LWFPGASLVS_OFST + reg), value);
}

uint32_t FPGA::ReadReg(uint32_t reg) {
	return alt_read_word(MAP_ADDRESS(GetLightweight(), ALT_LWFPGASLVS_OFST + reg));
}

bool FPGA::WriteIIC(uint8_t address, uint8_t reg, uint8_t value, bool debug) {
	void* virtual_base = GetLightweight();
	alt_write_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG1_BASE), 0x0); // reset ena
	alt_write_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG1_BASE),
	               address |
	               IIC_MODE_WRITE |
	               IIC_REG(reg) |
	               IIC_WR_VALUE(value) |
	               IIC_ENABLE);

	uint32_t vr3;
	return debug ? WaitIICDebug(vr3) : WaitIIC(vr3);
}

bool FPGA::ReadIIC(uint8_t address, uint8_t reg, uint8_t& value, bool debug) {
	void* virtual_base = GetLightweight();
	alt_write_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG1_BASE), 0x0); // reset ena
	alt_write_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG1_BASE),
	               address |
	               IIC_MODE_READ |
	               IIC_REG(reg) |
	               IIC_ENABLE);

	uint32_t vr3;
	if (debug ? WaitIICDebug(vr3) : WaitIIC(vr3)) {
		value = (uint8_t)IIC_VALUE(vr3);
		return true;
	}
	return false;
}


bool FPGA::WaitIIC(uint32_t& vr3) {
	void* virtual_base = GetLightweight();
	bool started = false;
	while(true) {
		vr3 = alt_read_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG3_BASE));
		if (IIC_BUSY(vr3)) {
			started = true;
		} else {
			if (started) {
				if(IIC_ASK(vr3) == 0) {
					return true;
				} else {
					printf("ASK=%X\n", IIC_ASK(vr3));
					return false;
				}
			}
		}
	}
}

bool FPGA::WaitIICDebug(uint32_t& vr3, bool csv, int limit) {
	uint32_t r3[limit];
	uint32_t r4[limit];
	void* virtual_base = GetLightweight();
	const char* format = csv ? "%u,%X,%s,%X,%s,%s,%s,%s,%d\n" : "%4u v=%2X %s ask=%X %s %s SCL=%s SDA=%s delay=%d %s\n";
	bool completed = false;
	bool started = false;
	while (!completed) {
		int i = 0;
		while (i < limit) {
			vr3 = alt_read_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG3_BASE));
			r3[i] = vr3;
			r4[i++] = alt_read_word(MAP_ADDRESS(virtual_base, ALT_LWFPGASLVS_OFST + PIO_REG4_BASE));
			if (!IIC_BUSY(vr3)) {
				if (started) {
					completed = true;
					break;
				}
			} else {
				started = true;
			}
		}

		for (int j = 0; j < i; ++j) {
			bool busy = IIC_BUSY(r3[j]);
			uint32_t cstate = (r4[j] >> 8) & 0xF;
			uint32_t tstate = r4[j] & 0x1F;
			printf(format,
			   IIC_CLOCK(r3[j]),
			   IIC_VALUE(r3[j]),
			   busy ? "BUSY" : "READY",
			   IIC_ASK(r3[j]),
			   cstate < 6 ? CSTATE[cstate] : "CSTATE_UNDEFINED",
			   tstate < 15 ? TSTATE[tstate] : "TSTATE_UNDEFINED",
			   (r4[j] & 0x2000) > 0 ? "1" : "0",
			   (r4[j] & 0x1000) > 0 ? "1" : "0",
			   r4[j] >> 16,
			   (r4[j] & 0x20) > 0 ? "R" : "W");
		}
	}
	return true;
}

void FPGA::DumpMem(uint32_t size) {
	GetLightweight();
	for (uint32_t i = 0; i < size; ++i ) {
		uint8_t v = alt_read_byte(MAP_AXI_ADDRESS(AxiVirtualBase, i));
		printf("%04X: %02X\n", i, (uint32_t)v);
	}
}

void FPGA::WriteMem(uint32_t address, uint8_t value) {
	alt_write_byte(MAP_AXI_ADDRESS(AxiVirtualBase, address), value);
}

