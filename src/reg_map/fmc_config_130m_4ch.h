//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Header file with Wishbone register map and chip addresses for
//         FMC ADC 130M 4CH card (ACTIVE and PASSIVE version)
//============================================================================
#ifndef FMC_ADC_130M_4CH_REG_MAP_H_
#define FMC_ADC_130M_4CH_REG_MAP_H_

#include <inttypes.h>

// REGISTER MAP FOR FMC ADC 130M 4CH
// FPGA register map

/************************** BPM-SW firmware **************************/
// Should be autodiscovered by SDB
//#define FMC130M_BASE_ADDR 0x30010000
#define FMC130M_BASE_ADDR 0x00310000

#define FPGA_CTRL_REGS                  (FMC130M_BASE_ADDR + 0x0000)
#define FPGA_SI571_I2C                  (FMC130M_BASE_ADDR + 0x0100)
#define FPGA_AD9510_SPI                 (FMC130M_BASE_ADDR + 0x0200)
#define FPGA_EEPROM_I2C                 (FMC130M_BASE_ADDR + 0x0300)
#define FPGA_LM75A_I2C                  (FMC130M_BASE_ADDR + 0x0400)

#define DSP_BASE_ADDR 0x00308000

#define DSP_CTRL_REGS                   (DSP_BASE_ADDR + 0x00000000)
#define DSP_BPM_SWAP                    (DSP_BASE_ADDR + 0x00000100)

#define WB_ACQ_BASE_ADDR 0x00330000

/*************** Memory Regions *************/
#define MEM_TOTAL_SIZE                  (1 << 29) // 512 MB
//#define MEM_TOTAL_SIZE                  (1 << 28) // 256 MB
#define MEM_REGION_SIZE                 (MEM_TOTAL_SIZE / 8)
//#define MEM_REGION_SIZE                 (MEM_TOTAL_SIZE / 2)
//#define MEM_REGION_SIZE                 (MEM_TOTAL_SIZE / 64)

// ADC
#define ADC_CHAN_ID                     0
#define DDR3_ADC_SAMPLE_SIZE            8 // 8 Bytes -> ADC0 = 16-bit / ADC1 = 16-bit ...
#define DDR3_ADC_START_ADDR             0x0
#define DDR3_ADC_END_ADDR               (DDR3_ADC_START_ADDR + 2*MEM_REGION_SIZE - DDR3_ADC_SAMPLE_SIZE)
//#define DDR3_ADC_END_ADDR               (DDR3_ADC_START_ADDR + MEM_REGION_SIZE - DDR3_ADC_SAMPLE_SIZE)
//#define DDR3_ADC_MAX_SAMPLES            ((DDR3_ADC_END_ADDR-DDR3_ADC_START_ADDR) / DDR3_ADC_SAMPLE_SIZE)
//FIXME TESTING
#define DDR3_ADC_MAX_SAMPLES            (MEM_TOTAL_SIZE/DDR3_ADC_SAMPLE_SIZE)

// TBT AMP
#define TBTAMP_CHAN_ID                  1
#define DDR3_TBTAMP_SAMPLE_SIZE         16 // 16 Bytes -> TBTAMP0 = 32-bit / TBTAMP1 = 32-bit ...
/* FIXME: Testing! */
//#define DDR3_TBTAMP_START_ADDR          (0x800000)
#define DDR3_TBTAMP_START_ADDR          (DDR3_ADC_END_ADDR + DDR3_ADC_SAMPLE_SIZE)
#define DDR3_TBTAMP_END_ADDR            (DDR3_TBTAMP_START_ADDR + MEM_REGION_SIZE - DDR3_TBTAMP_SAMPLE_SIZE)
//FIXME TESTING
//#define DDR3_TBTAMP_MAX_SAMPLES         ((DDR3_TBTAMP_END_ADDR-DDR3_TBTAMP_START_ADDR) / DDR3_TBTAMP_SAMPLE_SIZE)
#define DDR3_TBTAMP_MAX_SAMPLES         (MEM_TOTAL_SIZE/DDR3_TBTAMP_SAMPLE_SIZE)

// TBT POS
#define TBTPOS_CHAN_ID                  2
#define DDR3_TBTPOS_SAMPLE_SIZE         16 // 16 Bytes -> X = 32-bit / Y = 32-bit ...
// TESTING !!!
//#define DDR3_TBTPOS_START_ADDR          (0x0)
#define DDR3_TBTPOS_START_ADDR          (DDR3_TBTAMP_END_ADDR + DDR3_TBTAMP_SAMPLE_SIZE)
#define DDR3_TBTPOS_END_ADDR            (DDR3_TBTPOS_START_ADDR + 2*MEM_REGION_SIZE - DDR3_TBTPOS_SAMPLE_SIZE)
//#define DDR3_TBTPOS_MAX_SAMPLES         ((DDR3_TBTPOS_END_ADDR-DDR3_TBTPOS_START_ADDR) / DDR3_TBTPOS_SAMPLE_SIZE)
//FIXME TESTING
#define DDR3_TBTPOS_MAX_SAMPLES         (MEM_TOTAL_SIZE/DDR3_TBTPOS_SAMPLE_SIZE)

// FOFB AMP
#define FOFBAMP_CHAN_ID                 3
#define DDR3_FOFBAMP_SAMPLE_SIZE        16 // 16 Bytes -> FOFBAMP0 = 32-bit / FOFBAMP1 = 32-bit ...
// Testing
//#define DDR3_FOFBAMP_START_ADDR         (0x0)
#define DDR3_FOFBAMP_START_ADDR         (DDR3_TBTPOS_END_ADDR + DDR3_TBTPOS_SAMPLE_SIZE)
#define DDR3_FOFBAMP_END_ADDR           (DDR3_FOFBAMP_START_ADDR + MEM_REGION_SIZE - DDR3_FOFBAMP_SAMPLE_SIZE)
//#define DDR3_FOFBAMP_MAX_SAMPLES        ((DDR3_FOFBAMP_END_ADDR-DDR3_FOFBAMP_START_ADDR) / DDR3_FOFBAMP_SAMPLE_SIZE)
//FIXME TESTING
#define DDR3_FOFBAMP_MAX_SAMPLES        (MEM_TOTAL_SIZE/DDR3_FOFBAMP_SAMPLE_SIZE)

// FOFB POS
#define FOFBPOS_CHAN_ID                 4
#define DDR3_FOFBPOS_SAMPLE_SIZE        16 // 16 Bytes -> X = 32-bit / Y = 32-bit ...
//#define DDR3_FOFBPOS_START_ADDR         (DDR3_FOFBAMP_START_ADDR)
//#define DDR3_FOFBPOS_START_ADDR         (0x0)
#define DDR3_FOFBPOS_START_ADDR         (DDR3_FOFBAMP_END_ADDR + DDR3_FOFBAMP_SAMPLE_SIZE)
#define DDR3_FOFBPOS_END_ADDR           (DDR3_FOFBPOS_START_ADDR + 2*MEM_REGION_SIZE - DDR3_FOFBPOS_SAMPLE_SIZE)
//#define DDR3_FOFBPOS_MAX_SAMPLES        ((DDR3_FOFBPOS_END_ADDR-DDR3_FOFBPOS_START_ADDR) / DDR3_FOFBPOS_SAMPLE_SIZE)
//FIXME TESTING
#define DDR3_FOFBPOS_MAX_SAMPLES        (MEM_TOTAL_SIZE/DDR3_FOFBPOS_SAMPLE_SIZE)

#define END_CHAN_ID                     5

struct ddr3_acq_chan_s {
  uint32_t id;
  uint32_t start_addr;
  uint32_t end_addr;
  uint32_t max_samples;
  uint32_t samples_size;
};

extern const struct ddr3_acq_chan_s ddr3_acq_chan[END_CHAN_ID];

/**********************************************************************/

/********************** FMC-ADC-HDL firmware **************************/
// Should be autodiscovered by SDB
//#define FMC130M_BASE_ADDR 0x00000000
//
//#define FPGA_SI571_I2C                (FMC130M_BASE_ADDR + 0x010000)
//#define FPGA_AD9510_SPI               (FMC130M_BASE_ADDR + 0x020000)
//#define FPGA_EEPROM_I2C               (FMC130M_BASE_ADDR + 0x030000)
//#define FPGA_LM75A_I2C                (FMC130M_BASE_ADDR + 0x040000)
//#define FPGA_CTRL_REGS                (FMC130M_BASE_ADDR + 0x050000)
/**********************************************************************/

// System frequency
//#define FPGA_SYS_FREQ 200*1000000
#define DEFAULT_ADC_CLK                 113515008     // in Hertz
#define FPGA_SYS_FREQ                   (100*1000000) // Wishbone freq is 100MHz!!!

// System interface drivers
#define SI571_I2C_DRV                   "SI571_I2C"
#define AD9510_SPI_DRV                  "AD9510_SPI"
#define EEPROM_I2C_DRV                  "EEPROM_I2C"
#define LM75A_I2C_DRV                   "LM75A_I2C"
#define GENERAL_GPIO_DRV                "GENERAL_GPIO"

// Chip address
#define SI571_ADDR                      (0x49)
#define AD9510_ADDR                     (0x01)
#define EEPROM_ADDR                     (0x50)
#define LM75A_ADDR_1                    (0x49)
#define LM75A_ADDR_2                    (0x48)

// Wishbone control register addresses
#define WB_FMC_STATUS                   (0x00 << WB_GR_SHIFT)
#define WB_TRG_CTRL                     (0x01 << WB_GR_SHIFT)
#define WB_ADC_LTC_CTRL                 (0x02 << WB_GR_SHIFT)
#define WB_CLK_CTRL                     (0x03 << WB_GR_SHIFT)
#define WB_MONITOR_CTRL                 (0x04 << WB_GR_SHIFT)
#define WB_FPGA_CTRL                    (0x05 << WB_GR_SHIFT)
#define WB_IDELAY0_CAL                  (0x06 << WB_GR_SHIFT)
#define WB_IDELAY1_CAL                  (0x07 << WB_GR_SHIFT)
#define WB_IDELAY2_CAL                  (0x08 << WB_GR_SHIFT)
#define WB_IDELAY3_CAL                  (0x09 << WB_GR_SHIFT)
#define WB_DATA0                        (0x0A << WB_GR_SHIFT)
#define WB_DATA1                        (0x0B << WB_GR_SHIFT)
#define WB_DATA2                        (0x0C << WB_GR_SHIFT)
#define WB_DATA3                        (0x0D << WB_GR_SHIFT)
#define WB_FPGA_DCM_CTRL                (0x0E << WB_GR_SHIFT)

// DSP control register addresses
#include "pos_calc_regs.h"
// DSP control register addresses
#include "wb_bpm_swap.h"
// ACQ core control register addresses
#include "wb_acq_core_regs.h"

#endif /* FMC_ADC_130M_4CH_REG_MAP_H_ */
