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
//#include "reg_map/fmc_config_130m_4ch.h"

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

#include "config.h"
#include "plat_opts.h"
#include "data.h"
#include "wishbone/rs232_syscon.h"
#include "wishbone/pcie_link.h"
#include "platform/fmc130m_plat.h"
#include "common.h"
#include "tcp_server.h"

using namespace std;

#define S "sllp_server: "

fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
tcp_server *tcp_server_p;

/***************************************************************/ 
/**********************   SLLP methods  **********************/
/***************************************************************/

//struct sllp_func_info
//{
//    uint8_t id;                     // ID of the function, used in the protocol
//    uint8_t input_size;             // How many bytes of input
//    uint8_t output_size;            // How many bytes of output
//};
//
//typedef uint8_t (*sllp_func_t) (uint8_t *input, uint8_t *output);
//struct sllp_func
//{
//    struct sllp_func_info info;     // Information about the function
//    sllp_func_t           func_p;   // Pointer to the function to be executed
//};

#define FMC130M_BLINK_LEDS_ID 0
#define FMC130M_BLINK_LEDS_IN 0
#define FMC130M_BLINK_LEDS_OUT 4

uint8_t fmc130m_blink_leds(uint8_t *input, uint8_t *output)
{
    printf(S"blinking leds...\n");
    fmc_config_130m_4ch_board_p->blink_leds();

    *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct sllp_func fmc130m_blink_leds_func = {
  {FMC130M_BLINK_LEDS_ID,
   FMC130M_BLINK_LEDS_IN,
   FMC130M_BLINK_LEDS_OUT},
  fmc130m_blink_leds
};

#define FMC130M_CONFIG_DEFAULTS_ID 1
#define FMC130M_CONFIG_DEFAULTS_IN 0
#define FMC130M_CONFIG_DEFAULTS_OUT 4

uint8_t fmc130m_config_defaults(uint8_t *input, uint8_t *output)
{
    printf(S"Configuring Defaults...\n");
    fmc_config_130m_4ch_board_p->config_defaults();

     *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct sllp_func fmc130m_config_defaults_func = {
  {FMC130M_CONFIG_DEFAULTS_ID,
   FMC130M_CONFIG_DEFAULTS_IN,
   FMC130M_CONFIG_DEFAULTS_OUT},
  fmc130m_config_defaults
};

#define FMC130M_SET_KX_ID 2
#define FMC130M_SET_KX_IN 4
#define FMC130M_SET_KX_OUT 4

uint8_t fmc130m_set_kx(uint8_t *input, uint8_t *output)
{
    uint32_t kx = *((uint32_t *) input);
    printf(S"Setting Kx value...\n");
    fmc_config_130m_4ch_board_p->set_kx(kx);

     *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct sllp_func fmc130m_set_kx_func = {
  {FMC130M_SET_KX_ID,
   FMC130M_SET_KX_IN,
   FMC130M_SET_KX_OUT},
  fmc130m_set_kx
};

#define FMC130M_SET_KY_ID 3
#define FMC130M_SET_KY_IN 4
#define FMC130M_SET_KY_OUT 4

uint8_t fmc130m_set_ky(uint8_t *input, uint8_t *output)
{
    uint32_t ky = *((uint32_t *) input);
    printf(S"Setting Ky value...\n");
    fmc_config_130m_4ch_board_p->set_ky(ky);

     *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct sllp_func fmc130m_set_ky_func = {
  {FMC130M_SET_KY_ID,
   FMC130M_SET_KY_IN,
   FMC130M_SET_KY_OUT},
  fmc130m_set_kx
};
#define FMC130M_SET_KSUM_ID 4
#define FMC130M_SET_KSUM_IN 4
#define FMC130M_SET_KSUM_OUT 4

uint8_t fmc130m_set_ksum(uint8_t *input, uint8_t *output)
{
    uint32_t ksum = *((uint32_t *) input);
    printf(S"Setting Ksum value...\n");
    fmc_config_130m_4ch_board_p->set_ksum(ksum);

     *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct sllp_func fmc130m_set_ksum_func = {
  {FMC130M_SET_KSUM_ID,
   FMC130M_SET_KSUM_IN,
   FMC130M_SET_KSUM_OUT},
  fmc130m_set_ksum
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

  // Initialize TCP server and SLLP library

  tcp_server_p = new tcp_server(string("8080"), fmc_config_130m_4ch_board_p);
  
  // Register functions
  tcp_server_p->register_func(&fmc130m_blink_leds_func);
  tcp_server_p->register_func(&fmc130m_config_defaults_func);
  tcp_server_p->register_func(&fmc130m_set_kx_func);
  tcp_server_p->register_func(&fmc130m_set_ky_func);
  tcp_server_p->register_func(&fmc130m_set_ksum_func);
  
  /* Endless tcp loop */
  tcp_server_p->start();

  return 0;
}
