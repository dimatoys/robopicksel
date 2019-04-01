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

int reset_Bridges(void* virtual_base, bool lw, bool hpsToFpga) {
	// ./mem 0xFFD0501C 7 - 2 - 1   - page 3-28 of cv_5_HPS_tech_ref.pdf
	// ./mem 0xFF800000 0x10 + 0x8 - page 7-26 of cv_5_HPS_tech_ref.pdf

	void* map_byte_addr = MAP_ADDRESS(virtual_base, 0xFFD0501C);

	uint32_t data = alt_read_word(map_byte_addr);

	printf ("0xFFD0501C=%u\n", data);

	bool need_reset_lw = lw && ((data & 2) > 0);
	bool need_reset_hpsToFpga = hpsToFpga && ((data & 1) > 0);

	if (need_reset_lw || need_reset_hpsToFpga) {
		// not initialized yet
		data -= (need_reset_lw ? 2 : 0) + (need_reset_hpsToFpga ? 1 : 0);
		alt_write_word(map_byte_addr, data);

		alt_write_word(MAP_ADDRESS(virtual_base, 0xFF800000), (need_reset_lw ? 0x10 : 0) + (need_reset_hpsToFpga ? 0x8: 0));
	}

	return 0;
}

int main() {
	int fd = open( "/dev/mem", O_RDWR | O_SYNC);
	if (fd == -1 ) {
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}

	//lightweight HPS-to-FPGA bridge
	printf("virtual_base\n");
	void* virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
	if( virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap() failed...\n" );
		close( fd );
		return(1);
	}

	//HPS-to-FPGA bridge
	printf("axi_virtual_base\n");
	void* axi_virtual_base = mmap( NULL, HW_FPGA_AXI_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, ALT_AXI_FPGASLVS_OFST );

	if( axi_virtual_base == MAP_FAILED ) {
		printf( "ERROR: axi mmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	printf("Reset bridges\n");
	if (reset_Bridges(virtual_base, true, true) < 0) {
		printf("ERROR: error initializing LW-HPS-to-FPGA\n");
		close(fd);
		return(1);
	}

	for (uint32_t i = 0; i < 0x100; ++i) {
		uint32_t v = alt_read_word(MAP_AXI_ADDRESS(axi_virtual_base, i));
		printf("%04X: %02X\n", i, v);
	}

	printf("Unmap\n");
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	printf("Unmap axi\n");
	if( munmap( axi_virtual_base, HW_FPGA_AXI_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	printf("close\n");
	close( fd );

	return(0);
}
