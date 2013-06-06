//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Header file with Wishbone register map and chip addresses for
//				 FMC ADC 250M 4CH card (ACTIVE and PASSIVE version)
//============================================================================
#ifndef FMC_ADC_250M_4CH_REG_MAP_H_
#define FMC_ADC_250M_4CH_REG_MAP_H_

// REGISTER MAP FOR FMC ADC 250M 4CH
#define FPGA_ISLA_SPI 0x10000
#define FPGA_SI571_I2C 0x20000
#define FPGA_AD9510_SPI 0x30000
#define FPGA_EEPROM_I2C 0x40000
#define FPGA_AMC7823_SPI 0x50000
#define FPGA_CTRL_REGS 0x60000

// System frequency (Wishbone)
#define FPGA_SYS_FREQ 100*1000000

// System interface drivers
#define ISLA_SPI_DRV "ISLA_SPI"
#define SI571_I2C_DRV "SI571_I2C"
#define AD9510_SPI_DRV "AD9510_SPI_SPI"
#define AMC7823_SPI_DRV "AMC7823_SPI"
#define EEPROM_I2C_DRV "EEPROM_I2C"
#define GENERAL_GPIO_DRV "GENERAL_GPIO"

// Chip address
#define SI571_ADDR 0x49
#define AD9510_ADDR 0x01
#define AMC7823_ADDR 0x01
#define ISLA_ADC0_ADDR 0x01
#define ISLA_ADC1_ADDR 0x02
#define ISLA_ADC2_ADDR 0x04
#define ISLA_ADC3_ADDR 0x08

// Wishbone control register addresses
#define WB_FMC_STATUS 0x00
#define WB_TRG_CTRL 0x01
#define WB_ADC_ISLA_CTRL 0x02
#define WB_CLK_CTRL 0x03
#define WB_MONITOR_CTRL 0x04
#define WB_FPGA_CTRL 0x05
#define WB_IDELAY0_CAL 0x06
#define WB_IDELAY1_CAL 0x07
#define WB_IDELAY2_CAL 0x08
#define WB_IDELAY3_CAL 0x09
#define WB_DATA0 0x0A
#define WB_DATA1 0x0B
#define WB_DATA2 0x0C
#define WB_DATA3 0x0D

#endif /* FMC_ADC_250M_4CH_REG_MAP_H_ */
