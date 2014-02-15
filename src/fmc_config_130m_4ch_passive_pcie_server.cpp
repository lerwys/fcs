//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for configuration FMC ADC 130M 4CH card (PASSIVE version)
//               Also checks and tests avaiable chips on the board
//                               Includes:
//               - firmware identification
//                               - LED configuration
//               - trigger configuration
//               - LM75A configuration and data acquisition (temperature sensor)
//               - EEPROM 24A64 check
//               - LTC2208 configuration (4 ADC chips)
//               - Clock and data lines calibation (IDELAY)
//               - most of the functions provides assertions to check if data is written to the chip (checks readback value)
//============================================================================

#include <iostream>
#include <string>
#include <unistd.h> /* getopt */
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <stdint.h>

#include "fmc_config_130m_4ch_board.h"
#include "reg_map/fmc_config_130m_4ch.h"

#include "config.h"
#include "plat_opts.h"
#include "data.h"
#include "wishbone/rs232_syscon.h"
#include "wishbone/pcie_link.h"
#include "platform/fmc130m_plat.h"
#include "common.h"
#include "tcp_server.h"

using namespace std;

#define S "bsmp_server: "

fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
tcp_server *tcp_server_p;

/***************************************************************/ 
/**********************   BSMP methods  **********************/
/***************************************************************/

//struct bsmp_func_info
//{
//    uint8_t id;                     // ID of the function, used in the protocol
//    uint8_t input_size;             // How many bytes of input
//    uint8_t output_size;            // How many bytes of output
//};
//
//typedef uint8_t (*bsmp_func_t) (uint8_t *input, uint8_t *output);
//struct bsmp_func
//{
//    struct bsmp_func_info info;     // Information about the function
//    bsmp_func_t           func_p;   // Pointer to the function to be executed
//};

/*********************************/
/******** Blink Function *********/
/*********************************/

#define FMC130M_BLINK_LEDS_ID 0
#define FMC130M_BLINK_LEDS_IN 0
#define FMC130M_BLINK_LEDS_OUT 4

uint8_t fmc130m_blink_leds(uint8_t *input, uint8_t *output)
{
    printf(S"blinking leds...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->blink_leds();

    return 0; // Success!!
}

static struct bsmp_func fmc130m_blink_leds_func = {
  {FMC130M_BLINK_LEDS_ID,
   FMC130M_BLINK_LEDS_IN,
   FMC130M_BLINK_LEDS_OUT},
  fmc130m_blink_leds
};

/*******************************************/
/******** Config Defaults Function *********/
/*******************************************/

#define FMC130M_CONFIG_DEFAULTS_ID 1
#define FMC130M_CONFIG_DEFAULTS_IN 0
#define FMC130M_CONFIG_DEFAULTS_OUT 4

uint8_t fmc130m_config_defaults(uint8_t *input, uint8_t *output)
{
    printf(S"Configuring Defaults...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->config_defaults();

    return 0; // Success!!
}

static struct bsmp_func fmc130m_config_defaults_func = {
  {FMC130M_CONFIG_DEFAULTS_ID,
   FMC130M_CONFIG_DEFAULTS_IN,
   FMC130M_CONFIG_DEFAULTS_OUT},
  fmc130m_config_defaults
};

/********************************************/
/********** Set/Get KX  Function **********/
/*******************************************/

#define FMC130M_SET_KX_ID 2
#define FMC130M_SET_KX_IN 4
#define FMC130M_SET_KX_OUT 4

uint8_t fmc130m_set_kx(uint8_t *input, uint8_t *output)
{
    uint32_t kx = *((uint32_t *) input);
    
    printf(S"Setting Kx value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_kx(kx, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_kx_func = {
  {FMC130M_SET_KX_ID,
   FMC130M_SET_KX_IN,
   FMC130M_SET_KX_OUT},
  fmc130m_set_kx
};

#define FMC130M_GET_KX_ID 3
#define FMC130M_GET_KX_IN 0
#define FMC130M_GET_KX_OUT 4

uint8_t fmc130m_get_kx(uint8_t *input, uint8_t *output)
{
    uint32_t kx_out;
    
    printf(S"Getting Kx value...\n");
     fmc_config_130m_4ch_board_p->set_kx(NULL, &kx_out);
    *((uint32_t *)output) = kx_out;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_kx_func = {
  {FMC130M_GET_KX_ID,
   FMC130M_GET_KX_IN,
   FMC130M_GET_KX_OUT},
  fmc130m_get_kx
};

/********************************************/
/********** Set/Get KY  Function **********/
/*******************************************/

#define FMC130M_SET_KY_ID 4
#define FMC130M_SET_KY_IN 4
#define FMC130M_SET_KY_OUT 4

uint8_t fmc130m_set_ky(uint8_t *input, uint8_t *output)
{
    uint32_t ky = *((uint32_t *) input);
    printf(S"Setting Ky value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_ky(ky, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_ky_func = {
  {FMC130M_SET_KY_ID,
   FMC130M_SET_KY_IN,
   FMC130M_SET_KY_OUT},
  fmc130m_set_ky
};

#define FMC130M_GET_KY_ID 5
#define FMC130M_GET_KY_IN 0
#define FMC130M_GET_KY_OUT 4

uint8_t fmc130m_get_ky(uint8_t *input, uint8_t *output)
{
    uint32_t ky_out;
    
    printf(S"Getting Ky value...\n");
     fmc_config_130m_4ch_board_p->set_ky(NULL, &ky_out);
    *((uint32_t *)output) = ky_out;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_ky_func = {
  {FMC130M_GET_KY_ID,
   FMC130M_GET_KY_IN,
   FMC130M_GET_KY_OUT},
  fmc130m_get_ky
};

/********************************************/
/********** Set/Get KSUM  Function **********/
/*******************************************/

#define FMC130M_SET_KSUM_ID 6
#define FMC130M_SET_KSUM_IN 4
#define FMC130M_SET_KSUM_OUT 4

uint8_t fmc130m_set_ksum(uint8_t *input, uint8_t *output)
{
    uint32_t ksum = *((uint32_t *) input);
    printf(S"Setting Ksum value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_ksum(ksum, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_ksum_func = {
  {FMC130M_SET_KSUM_ID,
   FMC130M_SET_KSUM_IN,
   FMC130M_SET_KSUM_OUT},
  fmc130m_set_ksum
};

#define FMC130M_GET_KSUM_ID 7
#define FMC130M_GET_KSUM_IN 0
#define FMC130M_GET_KSUM_OUT 4

uint8_t fmc130m_get_ksum(uint8_t *input, uint8_t *output)
{
    uint32_t ksum_out;
    
    printf(S"Getting Ksum value...\n");
     fmc_config_130m_4ch_board_p->set_ksum(NULL, &ksum_out);
    *((uint32_t *)output) = ksum_out;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_ksum_func = {
  {FMC130M_GET_KSUM_ID,
   FMC130M_GET_KSUM_IN,
   FMC130M_GET_KSUM_OUT},
  fmc130m_get_ksum
};

/*************************************************/
/********** Set/Get Switching Functions **********/
/*************************************************/

#define FMC130M_SET_SW_ON_ID 8
#define FMC130M_SET_SW_ON_IN 0
#define FMC130M_SET_SW_ON_OUT 4

uint8_t fmc130m_set_sw_on(uint8_t *input, uint8_t *output)
{
    printf(S"Setting Switching ON!\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_on(NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_on_func = {
  {FMC130M_SET_SW_ON_ID,
   FMC130M_SET_SW_ON_IN,
   FMC130M_SET_SW_ON_OUT},
  fmc130m_set_sw_on
};

#define FMC130M_SET_SW_OFF_ID 9
#define FMC130M_SET_SW_OFF_IN 0
#define FMC130M_SET_SW_OFF_OUT 4

uint8_t fmc130m_set_sw_off(uint8_t *input, uint8_t *output)
{
    printf(S"Setting Switching OFF!\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_off(NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_off_func = {
  {FMC130M_SET_SW_OFF_ID,
   FMC130M_SET_SW_OFF_IN,
   FMC130M_SET_SW_OFF_OUT},
  fmc130m_set_sw_off
};

#define FMC130M_GET_SW_ID 10
#define FMC130M_GET_SW_IN 0
#define FMC130M_GET_SW_OUT 4

uint8_t fmc130m_get_sw(uint8_t *input, uint8_t *output)
{
    uint32_t sw_state;
    
    printf(S"Getting Switching State!\n");
    fmc_config_130m_4ch_board_p->set_sw_on(&sw_state);
    *((uint32_t *)output) = sw_state;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_func = {
  {FMC130M_GET_SW_ID,
   FMC130M_GET_SW_IN,
   FMC130M_GET_SW_OUT},
  fmc130m_get_sw
};

/*************************************************/
/****** Set/Get Switching CLK DIV Functions ******/
/*************************************************/

#define FMC130M_SET_SW_DIVCLK_ID 11
#define FMC130M_SET_SW_DIVCLK_IN 4
#define FMC130M_SET_SW_DIVCLK_OUT 4

uint8_t fmc130m_set_sw_divclk(uint8_t *input, uint8_t *output)
{
    uint32_t divclk = *((uint32_t *) input);
    printf(S"Setting DIVCLK value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_divclk(divclk, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_divclk_func = {
  {FMC130M_SET_SW_DIVCLK_ID,
   FMC130M_SET_SW_DIVCLK_IN,
   FMC130M_SET_SW_DIVCLK_OUT},
  fmc130m_set_sw_divclk
};

#define FMC130M_GET_SW_DIVCLK_ID 12
#define FMC130M_GET_SW_DIVCLK_IN 0
#define FMC130M_GET_SW_DIVCLK_OUT 4

uint8_t fmc130m_get_sw_divclk(uint8_t *input, uint8_t *output)
{
    uint32_t divclk;
    
    printf(S"Getting DIVCLK value!\n");
    fmc_config_130m_4ch_board_p->set_sw_divclk(NULL, &divclk);
    *((uint32_t *)output) = divclk;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_divclk_func = {
  {FMC130M_GET_SW_DIVCLK_ID,
   FMC130M_GET_SW_DIVCLK_IN,
   FMC130M_GET_SW_DIVCLK_OUT},
  fmc130m_get_sw_divclk
};

/*************************************************/
/****** Set/Get Switching CLK Phase Functions *****/
/*************************************************/

#define FMC130M_SET_SW_PHASECLK_ID 13
#define FMC130M_SET_SW_PHASECLK_IN 4
#define FMC130M_SET_SW_PHASECLK_OUT 4

uint8_t fmc130m_set_sw_phaseclk(uint8_t *input, uint8_t *output)
{
    uint32_t phaseclk = *((uint32_t *) input);
    printf(S"Setting PHASECLK value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_phase(phaseclk, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_phaseclk_func = {
  {FMC130M_SET_SW_PHASECLK_ID,
   FMC130M_SET_SW_PHASECLK_IN,
   FMC130M_SET_SW_PHASECLK_OUT},
  fmc130m_set_sw_phaseclk
};

#define FMC130M_GET_SW_PHASECLK_ID 14
#define FMC130M_GET_SW_PHASECLK_IN 0
#define FMC130M_GET_SW_PHASECLK_OUT 4

uint8_t fmc130m_get_sw_phaseclk(uint8_t *input, uint8_t *output)
{
    uint32_t phaseclk;
    
    printf(S"Getting PHASECLK value!\n");
    fmc_config_130m_4ch_board_p->set_sw_phase(NULL, &phaseclk);
    *((uint32_t *)output) = phaseclk;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_phaseclk_func = {
  {FMC130M_GET_SW_PHASECLK_ID,
   FMC130M_GET_SW_PHASECLK_IN,
   FMC130M_GET_SW_PHASECLK_OUT},
  fmc130m_get_sw_phaseclk
};

/*************************************************/
/************ Set/Get ADC CLK Functions **********/
/*************************************************/

#define FMC130M_SET_ADC_CLK_ID 15
#define FMC130M_SET_ADC_CLK_IN 4
#define FMC130M_SET_ADC_CLK_OUT 4

uint8_t fmc130m_set_adc_clk(uint8_t *input, uint8_t *output)
{
    uint32_t adcclk = *((uint32_t *) input);
    printf(S"Setting ADC_CLK value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_adc_clk(adcclk, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_adc_clk_func = {
  {FMC130M_SET_ADC_CLK_ID,
   FMC130M_SET_ADC_CLK_IN,
   FMC130M_SET_ADC_CLK_OUT},
  fmc130m_set_adc_clk
};

#define FMC130M_GET_ADC_CLK_ID 16
#define FMC130M_GET_ADC_CLK_IN 0
#define FMC130M_GET_ADC_CLK_OUT 4

uint8_t fmc130m_get_adc_clk(uint8_t *input, uint8_t *output)
{
    uint32_t adcclk;
    
    printf(S"Getting ADC_CLK value!\n");
    fmc_config_130m_4ch_board_p->set_adc_clk(NULL, &adcclk);
    *((uint32_t *)output) = adcclk;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_adc_clk_func = {
  {FMC130M_GET_ADC_CLK_ID,
   FMC130M_GET_ADC_CLK_IN,
   FMC130M_GET_ADC_CLK_OUT},
  fmc130m_get_adc_clk
};

/*************************************************/
/****** Set/Get DDS frequency CLK Functions ******/
/*************************************************/

#define FMC130M_SET_DDS_FREQ_ID 17
#define FMC130M_SET_DDS_FREQ_IN 4
#define FMC130M_SET_DDS_FREQ_OUT 4

uint8_t fmc130m_set_dds_freq(uint8_t *input, uint8_t *output)
{
    uint32_t ddsfreq = *((uint32_t *) input);
    printf(S"Setting DDS_FREQ value...\n");
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_dds_freq(ddsfreq, NULL);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_dds_freq_func = {
  {FMC130M_SET_DDS_FREQ_ID,
   FMC130M_SET_DDS_FREQ_IN,
   FMC130M_SET_DDS_FREQ_OUT},
  fmc130m_set_dds_freq
};

#define FMC130M_GET_DDS_FREQ_ID 18
#define FMC130M_GET_DDS_FREQ_IN 0
#define FMC130M_GET_DDS_FREQ_OUT 4

uint8_t fmc130m_get_dds_freq(uint8_t *input, uint8_t *output)
{
    uint32_t ddsfreq;
    
    printf(S"Getting DDS_FREQ value!\n");
    fmc_config_130m_4ch_board_p->set_dds_freq(NULL, &ddsfreq);
    *((uint32_t *)output) = ddsfreq;

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_dds_freq_func = {
  {FMC130M_GET_DDS_FREQ_ID,
   FMC130M_GET_DDS_FREQ_IN,
   FMC130M_GET_DDS_FREQ_OUT},
  fmc130m_get_dds_freq
};

/*************************************************/
/****** Set Data Acquisition Functions ******/
/*************************************************/

#define FMC130M_SET_ACQ_PARAM_ID 18
#define FMC130M_SET_ACQ_PARAM_IN 8
#define FMC130M_SET_ACQ_PARAM_OUT 4

uint8_t fmc130m_set_acq_params(uint8_t *input, uint8_t *output)
{
    uint32_t nsamples = *((uint32_t *) input);
    uint32_t acq_chan = *((uint32_t *) input + sizeof(uint32_t));
    printf(S"Setting DATA_ACQ values...\n");

    // Do some checking
    if (acq_chan > END_CHAN_ID-1) {
        return -1; // invalid channel
    }
    
    if (nsamples > ddr3_acq_chan[acq_chan].max_samples) {
        return -2; // invalid number of samples
    }

    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_acq_params(&nsamples,
            &acq_chan, &ddr3_acq_chan[acq_chan].start_addr);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_acq_params_func = {
  {FMC130M_SET_ACQ_PARAM_ID,
   FMC130M_SET_ACQ_PARAM_IN,
   FMC130M_SET_ACQ_PARAM_OUT},
  fmc130m_set_acq_params
};

#define FMC130M_START_ACQ_ID 19
#define FMC130M_START_ACQ_IN 0
#define FMC130M_START_ACQ_OUT 4

uint8_t fmc130m_start_acq(uint8_t *input, uint8_t *output)
{
    printf(S"Starting ACQ...\n");

    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_data_acquire();

    return 0; // Success!!
}

static struct bsmp_func fmc130m_start_acq_func = {
  {FMC130M_START_ACQ_ID,
   FMC130M_START_ACQ_IN,
   FMC130M_START_ACQ_OUT},
  fmc130m_start_acq
};

int main(int argc, const char **argv) {

  cout << "FMC configuration software for FMC ADC 130M 4CH card (PASSIVE version)" << endl <<
                  "Author: Andrzej Wojenski" << endl;

  //WBInt_drv* int_drv;
  wb_data data;
  vector<uint16_t> amc_temp;
  vector<uint16_t> test_pattern;
  //commLink* _commLink = new commLink();
  //fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
  //tcp_server *tcp_server_p;
  uint32_t data_temp;
  int opt, error;

   /* Default command-line arguments */
  program = argv[0];
  quiet = 0;
  verbose = 0;
  error = 0;
  platform = PLATFORM_NOT_SET;

  /* Process the command-line arguments */
  while ((opt = getopt(argc, (char **)argv, "p:vqh")) != -1) {
    switch (opt) {
    case 'p':
      if (strlen(optarg) > MAX_PLATFORM_SIZE_ID) {
        fprintf(stderr, "%s: platform id length too big -- '%s'\n", program, optarg);
        return 1;
      }

      // Simple lookup table for checking the platform name (case insensitive)
      switch (platform = lookupstring_i(optarg)) {
        case ML605:
          delay_data_l = fmc_130m_ml605_delay_data_l;
          delay_clk_l = fmc_130m_ml605_delay_clk_l;
          platform_name = ML605_STRING;
          break;
        case KC705:
          delay_data_l = fmc_130m_kc705_delay_l;
          platform_name = KC705_STRING;
          break;
        case AFC:
          delay_data_l = fmc_130m_afc_delay_l;
          platform_name = AFC_STRING;
          break;
        case BAD_PLATFORM:
          fprintf(stderr, "%s: invalid platform -- '%s'\n", program, optarg);
          platform_name = NOPLAT_STRING;  // why care?
          return 1;
      }
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      quiet = 1;
      break;
    case 'h':
      help();
      return 1;
    case ':':
    case '?':
      error = 1;
      break;
    default:
      fprintf(stderr, "%s: bad option\n", program);
      help();
      return 1;
    }
  }

  if (error) return 1;

  if (platform == PLATFORM_NOT_SET) {
    fprintf(stderr, "%s: platform not set!\n", program);
    return 1;
  }

  fprintf(stdout, "%s: platform (%s) set!\n", program, platform_name);

  fmc_config_130m_4ch_board_p = new fmc_config_130m_4ch_board(new pcie_link_driver(0),
                                                  delay_data_l,
                                                  delay_clk_l);

  // Initialize TCP server and BSMP library

  tcp_server_p = new tcp_server(string("8080"), fmc_config_130m_4ch_board_p);
  
  // Register functions
  tcp_server_p->register_func(&fmc130m_blink_leds_func);
  tcp_server_p->register_func(&fmc130m_config_defaults_func);
  tcp_server_p->register_func(&fmc130m_set_kx_func);
  tcp_server_p->register_func(&fmc130m_get_kx_func);
  tcp_server_p->register_func(&fmc130m_set_ky_func);
  tcp_server_p->register_func(&fmc130m_get_ky_func);
  tcp_server_p->register_func(&fmc130m_set_ksum_func);
  tcp_server_p->register_func(&fmc130m_get_ksum_func);
  
  tcp_server_p->register_func(&fmc130m_set_sw_on_func);
  tcp_server_p->register_func(&fmc130m_set_sw_off_func);
  tcp_server_p->register_func(&fmc130m_get_sw_func);
  tcp_server_p->register_func(&fmc130m_set_sw_divclk_func);
  tcp_server_p->register_func(&fmc130m_get_sw_divclk_func);
  tcp_server_p->register_func(&fmc130m_set_sw_phaseclk_func);
  tcp_server_p->register_func(&fmc130m_get_sw_phaseclk_func);
  
  tcp_server_p->register_func(&fmc130m_set_adc_clk_func);
  tcp_server_p->register_func(&fmc130m_get_adc_clk_func);
  tcp_server_p->register_func(&fmc130m_set_dds_freq_func);
  tcp_server_p->register_func(&fmc130m_get_dds_freq_func);
  
  tcp_server_p->register_func(&fmc130m_set_acq_params_func);
  tcp_server_p->register_func(&fmc130m_start_acq_func);

  /* Endless tcp loop */
  tcp_server_p->start();

  return 0;
}
