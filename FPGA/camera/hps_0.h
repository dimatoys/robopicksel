#ifndef _ALTERA_HPS_0_H_
#define _ALTERA_HPS_0_H_

/*
 * This file was automatically generated by the swinfo2header utility.
 * 
 * Created from SOPC Builder system 'soc_system' in
 * file '/cygdrive/c/usr/dima/FPGA/test_GHRD/soc_system.sopcinfo'.
 */

/*
 * This file contains macros for module 'hps_0' and devices
 * connected to the following masters:
 *   h2f_axi_master
 *   h2f_lw_axi_master
 * 
 * Do not include this header file and another header file created for a
 * different module or master group at the same time.
 * Doing so may result in duplicate macro names.
 * Instead, use the system header file which has macros with unique names.
 */

/*
 * Macros for device 'sysid_qsys', class 'altera_avalon_sysid_qsys'
 * The macros are prefixed with 'SYSID_QSYS_'.
 * The prefix is the slave descriptor.
 */
#define SYSID_QSYS_COMPONENT_TYPE altera_avalon_sysid_qsys
#define SYSID_QSYS_COMPONENT_NAME sysid_qsys
#define SYSID_QSYS_BASE 0x10000
#define SYSID_QSYS_SPAN 8
#define SYSID_QSYS_END 0x10007
#define SYSID_QSYS_ID 2899645186
#define SYSID_QSYS_TIMESTAMP 1551425552

/*
 * Macros for device 'led_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'LED_PIO_'.
 * The prefix is the slave descriptor.
 */
#define LED_PIO_COMPONENT_TYPE altera_avalon_pio
#define LED_PIO_COMPONENT_NAME led_pio
#define LED_PIO_BASE 0x10040
#define LED_PIO_SPAN 16
#define LED_PIO_END 0x1004f
#define LED_PIO_BIT_CLEARING_EDGE_REGISTER 0
#define LED_PIO_BIT_MODIFYING_OUTPUT_REGISTER 0
#define LED_PIO_CAPTURE 0
#define LED_PIO_DATA_WIDTH 8
#define LED_PIO_DO_TEST_BENCH_WIRING 0
#define LED_PIO_DRIVEN_SIM_VALUE 0
#define LED_PIO_EDGE_TYPE NONE
#define LED_PIO_FREQ 50000000
#define LED_PIO_HAS_IN 0
#define LED_PIO_HAS_OUT 1
#define LED_PIO_HAS_TRI 0
#define LED_PIO_IRQ_TYPE NONE
#define LED_PIO_RESET_VALUE 0

/*
 * Macros for device 'dipsw_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'DIPSW_PIO_'.
 * The prefix is the slave descriptor.
 */
#define DIPSW_PIO_COMPONENT_TYPE altera_avalon_pio
#define DIPSW_PIO_COMPONENT_NAME dipsw_pio
#define DIPSW_PIO_BASE 0x10080
#define DIPSW_PIO_SPAN 16
#define DIPSW_PIO_END 0x1008f
#define DIPSW_PIO_IRQ 0
#define DIPSW_PIO_BIT_CLEARING_EDGE_REGISTER 1
#define DIPSW_PIO_BIT_MODIFYING_OUTPUT_REGISTER 0
#define DIPSW_PIO_CAPTURE 1
#define DIPSW_PIO_DATA_WIDTH 4
#define DIPSW_PIO_DO_TEST_BENCH_WIRING 0
#define DIPSW_PIO_DRIVEN_SIM_VALUE 0
#define DIPSW_PIO_EDGE_TYPE ANY
#define DIPSW_PIO_FREQ 50000000
#define DIPSW_PIO_HAS_IN 1
#define DIPSW_PIO_HAS_OUT 0
#define DIPSW_PIO_HAS_TRI 0
#define DIPSW_PIO_IRQ_TYPE EDGE
#define DIPSW_PIO_RESET_VALUE 0

/*
 * Macros for device 'button_pio', class 'altera_avalon_pio'
 * The macros are prefixed with 'BUTTON_PIO_'.
 * The prefix is the slave descriptor.
 */
#define BUTTON_PIO_COMPONENT_TYPE altera_avalon_pio
#define BUTTON_PIO_COMPONENT_NAME button_pio
#define BUTTON_PIO_BASE 0x100c0
#define BUTTON_PIO_SPAN 16
#define BUTTON_PIO_END 0x100cf
#define BUTTON_PIO_IRQ 1
#define BUTTON_PIO_BIT_CLEARING_EDGE_REGISTER 1
#define BUTTON_PIO_BIT_MODIFYING_OUTPUT_REGISTER 0
#define BUTTON_PIO_CAPTURE 1
#define BUTTON_PIO_DATA_WIDTH 4
#define BUTTON_PIO_DO_TEST_BENCH_WIRING 0
#define BUTTON_PIO_DRIVEN_SIM_VALUE 0
#define BUTTON_PIO_EDGE_TYPE FALLING
#define BUTTON_PIO_FREQ 50000000
#define BUTTON_PIO_HAS_IN 1
#define BUTTON_PIO_HAS_OUT 0
#define BUTTON_PIO_HAS_TRI 0
#define BUTTON_PIO_IRQ_TYPE EDGE
#define BUTTON_PIO_RESET_VALUE 0

/*
 * Macros for device 'pio_reg1', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG1_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG1_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG1_COMPONENT_NAME pio_reg1
#define PIO_REG1_BASE 0x100d0
#define PIO_REG1_SPAN 16
#define PIO_REG1_END 0x100df
#define PIO_REG1_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG1_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG1_CAPTURE 0
#define PIO_REG1_DATA_WIDTH 32
#define PIO_REG1_DO_TEST_BENCH_WIRING 0
#define PIO_REG1_DRIVEN_SIM_VALUE 0
#define PIO_REG1_EDGE_TYPE NONE
#define PIO_REG1_FREQ 50000000
#define PIO_REG1_HAS_IN 0
#define PIO_REG1_HAS_OUT 1
#define PIO_REG1_HAS_TRI 0
#define PIO_REG1_IRQ_TYPE NONE
#define PIO_REG1_RESET_VALUE 0

/*
 * Macros for device 'pio_reg2', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG2_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG2_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG2_COMPONENT_NAME pio_reg2
#define PIO_REG2_BASE 0x100e0
#define PIO_REG2_SPAN 16
#define PIO_REG2_END 0x100ef
#define PIO_REG2_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG2_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG2_CAPTURE 0
#define PIO_REG2_DATA_WIDTH 32
#define PIO_REG2_DO_TEST_BENCH_WIRING 0
#define PIO_REG2_DRIVEN_SIM_VALUE 0
#define PIO_REG2_EDGE_TYPE NONE
#define PIO_REG2_FREQ 50000000
#define PIO_REG2_HAS_IN 0
#define PIO_REG2_HAS_OUT 1
#define PIO_REG2_HAS_TRI 0
#define PIO_REG2_IRQ_TYPE NONE
#define PIO_REG2_RESET_VALUE 0

/*
 * Macros for device 'pio_reg3', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG3_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG3_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG3_COMPONENT_NAME pio_reg3
#define PIO_REG3_BASE 0x100f0
#define PIO_REG3_SPAN 16
#define PIO_REG3_END 0x100ff
#define PIO_REG3_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG3_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG3_CAPTURE 0
#define PIO_REG3_DATA_WIDTH 32
#define PIO_REG3_DO_TEST_BENCH_WIRING 0
#define PIO_REG3_DRIVEN_SIM_VALUE 0
#define PIO_REG3_EDGE_TYPE NONE
#define PIO_REG3_FREQ 50000000
#define PIO_REG3_HAS_IN 1
#define PIO_REG3_HAS_OUT 0
#define PIO_REG3_HAS_TRI 0
#define PIO_REG3_IRQ_TYPE NONE
#define PIO_REG3_RESET_VALUE 0

/*
 * Macros for device 'pio_reg4', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG4_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG4_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG4_COMPONENT_NAME pio_reg4
#define PIO_REG4_BASE 0x10100
#define PIO_REG4_SPAN 16
#define PIO_REG4_END 0x1010f
#define PIO_REG4_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG4_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG4_CAPTURE 0
#define PIO_REG4_DATA_WIDTH 32
#define PIO_REG4_DO_TEST_BENCH_WIRING 0
#define PIO_REG4_DRIVEN_SIM_VALUE 0
#define PIO_REG4_EDGE_TYPE NONE
#define PIO_REG4_FREQ 50000000
#define PIO_REG4_HAS_IN 1
#define PIO_REG4_HAS_OUT 0
#define PIO_REG4_HAS_TRI 0
#define PIO_REG4_IRQ_TYPE NONE
#define PIO_REG4_RESET_VALUE 0

/*
 * Macros for device 'pio_reg5', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG5_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG5_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG5_COMPONENT_NAME pio_reg5
#define PIO_REG5_BASE 0x10110
#define PIO_REG5_SPAN 16
#define PIO_REG5_END 0x1011f
#define PIO_REG5_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG5_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG5_CAPTURE 0
#define PIO_REG5_DATA_WIDTH 32
#define PIO_REG5_DO_TEST_BENCH_WIRING 0
#define PIO_REG5_DRIVEN_SIM_VALUE 0
#define PIO_REG5_EDGE_TYPE NONE
#define PIO_REG5_FREQ 50000000
#define PIO_REG5_HAS_IN 1
#define PIO_REG5_HAS_OUT 0
#define PIO_REG5_HAS_TRI 0
#define PIO_REG5_IRQ_TYPE NONE
#define PIO_REG5_RESET_VALUE 0

/*
 * Macros for device 'pio_reg6', class 'altera_avalon_pio'
 * The macros are prefixed with 'PIO_REG6_'.
 * The prefix is the slave descriptor.
 */
#define PIO_REG6_COMPONENT_TYPE altera_avalon_pio
#define PIO_REG6_COMPONENT_NAME pio_reg6
#define PIO_REG6_BASE 0x10120
#define PIO_REG6_SPAN 16
#define PIO_REG6_END 0x1012f
#define PIO_REG6_BIT_CLEARING_EDGE_REGISTER 0
#define PIO_REG6_BIT_MODIFYING_OUTPUT_REGISTER 0
#define PIO_REG6_CAPTURE 0
#define PIO_REG6_DATA_WIDTH 16
#define PIO_REG6_DO_TEST_BENCH_WIRING 0
#define PIO_REG6_DRIVEN_SIM_VALUE 0
#define PIO_REG6_EDGE_TYPE NONE
#define PIO_REG6_FREQ 50000000
#define PIO_REG6_HAS_IN 1
#define PIO_REG6_HAS_OUT 0
#define PIO_REG6_HAS_TRI 0
#define PIO_REG6_IRQ_TYPE NONE
#define PIO_REG6_RESET_VALUE 0

/*
 * Macros for device 'jtag_uart', class 'altera_avalon_jtag_uart'
 * The macros are prefixed with 'JTAG_UART_'.
 * The prefix is the slave descriptor.
 */
#define JTAG_UART_COMPONENT_TYPE altera_avalon_jtag_uart
#define JTAG_UART_COMPONENT_NAME jtag_uart
#define JTAG_UART_BASE 0x20000
#define JTAG_UART_SPAN 8
#define JTAG_UART_END 0x20007
#define JTAG_UART_IRQ 2
#define JTAG_UART_READ_DEPTH 64
#define JTAG_UART_READ_THRESHOLD 8
#define JTAG_UART_WRITE_DEPTH 64
#define JTAG_UART_WRITE_THRESHOLD 8


#endif /* _ALTERA_HPS_0_H_ */