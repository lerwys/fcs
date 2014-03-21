//============================================================================
// Author      : Andrzej Wojenski and Lucas Russo
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
#define SMON "bsmp_server_monit: "

fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
//tcp_server *tcp_server_p;

static char buffer[80];
static char * timestamp_str()
{
    time_t ltime; /* calendar time */
    ltime = time (NULL); /* get current cal time */
    strftime (buffer, 80, "%F %T", localtime(&ltime));
    return buffer;
}

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
     fprintf(stderr, "[%s] "S"blinking leds...\n", timestamp_str());
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
     fprintf(stderr,"[%s] "S"Configuring Defaults...\n", timestamp_str());
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
/********** Get FMC Temp Functions **********/
/*******************************************/

#define FMC130M_GET_TEMP1_ID 2
#define FMC130M_GET_TEMP1_IN 0
#define FMC130M_GET_TEMP1_OUT 8

uint8_t fmc130m_get_temp1(uint8_t *input, uint8_t *output)
{
    fmc_config_130m_4ch_board_p->get_fmc_temp1((uint64_t *)output);
     fprintf(stderr,"[%s] "S"Getting FMC Temp 1: %f\n", timestamp_str(), *(double *)output);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_temp1_func = {
  {FMC130M_GET_TEMP1_ID,
   FMC130M_GET_TEMP1_IN,
   FMC130M_GET_TEMP1_OUT},
  fmc130m_get_temp1
};

#define FMC130M_GET_TEMP2_ID 3
#define FMC130M_GET_TEMP2_IN 0
#define FMC130M_GET_TEMP2_OUT 8

uint8_t fmc130m_get_temp2(uint8_t *input, uint8_t *output)
{
    fmc_config_130m_4ch_board_p->get_fmc_temp2((uint64_t *)output);
     fprintf(stderr,"[%s] "S"Getting FMC Temp 2: %f\n", timestamp_str(), *(double *)output);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_temp2_func = {
  {FMC130M_GET_TEMP2_ID,
   FMC130M_GET_TEMP2_IN,
   FMC130M_GET_TEMP2_OUT},
  fmc130m_get_temp2
};

/********************************************/
/********** Set/Get KX  Function **********/
/*******************************************/

#define FMC130M_SET_KX_ID 4
#define FMC130M_SET_KX_IN 4
#define FMC130M_SET_KX_OUT 4

uint8_t fmc130m_set_kx(uint8_t *input, uint8_t *output)
{
    uint32_t kx = *((uint32_t *) input);

    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_kx(kx, NULL);
     fprintf(stderr,"[%s] "S"Setting Kx value to: %u\n", timestamp_str(), kx);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_kx_func = {
  {FMC130M_SET_KX_ID,
   FMC130M_SET_KX_IN,
   FMC130M_SET_KX_OUT},
  fmc130m_set_kx
};

#define FMC130M_GET_KX_ID 5
#define FMC130M_GET_KX_IN 0
#define FMC130M_GET_KX_OUT 4

uint8_t fmc130m_get_kx(uint8_t *input, uint8_t *output)
{
    uint32_t kx_out;

    fmc_config_130m_4ch_board_p->set_kx(NULL, &kx_out);
    *((uint32_t *)output) = kx_out;
     fprintf(stderr,"[%s] "S"Getting Kx value: %u\n", timestamp_str(), kx_out);

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

#define FMC130M_SET_KY_ID 6
#define FMC130M_SET_KY_IN 4
#define FMC130M_SET_KY_OUT 4

uint8_t fmc130m_set_ky(uint8_t *input, uint8_t *output)
{
    uint32_t ky = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_ky(ky, NULL);
     fprintf(stderr,"[%s] "S"Setting Ky value to: %u\n", timestamp_str(), ky);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_ky_func = {
  {FMC130M_SET_KY_ID,
   FMC130M_SET_KY_IN,
   FMC130M_SET_KY_OUT},
  fmc130m_set_ky
};

#define FMC130M_GET_KY_ID 7
#define FMC130M_GET_KY_IN 0
#define FMC130M_GET_KY_OUT 4

uint8_t fmc130m_get_ky(uint8_t *input, uint8_t *output)
{
    uint32_t ky_out;

     fmc_config_130m_4ch_board_p->set_ky(NULL, &ky_out);
    *((uint32_t *)output) = ky_out;
     fprintf(stderr,"[%s] "S"Getting Ky value: %u\n", timestamp_str(), ky_out);

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

#define FMC130M_SET_KSUM_ID 8
#define FMC130M_SET_KSUM_IN 4
#define FMC130M_SET_KSUM_OUT 4

uint8_t fmc130m_set_ksum(uint8_t *input, uint8_t *output)
{
    uint32_t ksum = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_ksum(ksum, NULL);
     fprintf(stderr,"[%s] "S"Setting Ksum value to: %u\n", timestamp_str(), ksum);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_ksum_func = {
  {FMC130M_SET_KSUM_ID,
   FMC130M_SET_KSUM_IN,
   FMC130M_SET_KSUM_OUT},
  fmc130m_set_ksum
};

#define FMC130M_GET_KSUM_ID 9
#define FMC130M_GET_KSUM_IN 0
#define FMC130M_GET_KSUM_OUT 4

uint8_t fmc130m_get_ksum(uint8_t *input, uint8_t *output)
{
    uint32_t ksum_out;

     fmc_config_130m_4ch_board_p->set_ksum(NULL, &ksum_out);
    *((uint32_t *)output) = ksum_out;
     fprintf(stderr,"[%s] "S"Getting Ksum value: %u\n", timestamp_str(), ksum_out);

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

#define FMC130M_SET_SW_ON_ID 10
#define FMC130M_SET_SW_ON_IN 0
#define FMC130M_SET_SW_ON_OUT 4

uint8_t fmc130m_set_sw_on(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_on(NULL);
     fprintf(stderr,"[%s] "S"Setting FPGA Deswitching ON!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_on_func = {
  {FMC130M_SET_SW_ON_ID,
   FMC130M_SET_SW_ON_IN,
   FMC130M_SET_SW_ON_OUT},
  fmc130m_set_sw_on
};

#define FMC130M_SET_SW_OFF_ID 11
#define FMC130M_SET_SW_OFF_IN 0
#define FMC130M_SET_SW_OFF_OUT 4

uint8_t fmc130m_set_sw_off(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_off(NULL);
     fprintf(stderr,"[%s] "S"Setting FPGA Deswitching OFF!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_off_func = {
  {FMC130M_SET_SW_OFF_ID,
   FMC130M_SET_SW_OFF_IN,
   FMC130M_SET_SW_OFF_OUT},
  fmc130m_set_sw_off
};

#define FMC130M_GET_SW_ID 12
#define FMC130M_GET_SW_IN 0
#define FMC130M_GET_SW_OUT 4

uint8_t fmc130m_get_sw(uint8_t *input, uint8_t *output)
{
    uint32_t sw_state;

    fmc_config_130m_4ch_board_p->set_sw_on(&sw_state);
    *((uint32_t *)output) = sw_state;
     fprintf(stderr,"[%s] "S"Getting Switching State: %u\n", timestamp_str(), sw_state);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_func = {
  {FMC130M_GET_SW_ID,
   FMC130M_GET_SW_IN,
   FMC130M_GET_SW_OUT},
  fmc130m_get_sw
};

/*************************************************/
/***** Set/Get Switching Clk Enable Functions ****/
/*************************************************/

#define FMC130M_SET_SW_CLK_EN_ON_ID 13
#define FMC130M_SET_SW_CLK_EN_ON_IN 0
#define FMC130M_SET_SW_CLK_EN_ON_OUT 4

uint8_t fmc130m_set_sw_clk_en_on(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_clk_en_on(NULL);
     fprintf(stderr,"[%s] "S"Setting Switching Clock ON!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_clk_en_on_func = {
  {FMC130M_SET_SW_CLK_EN_ON_ID,
   FMC130M_SET_SW_CLK_EN_ON_IN,
   FMC130M_SET_SW_CLK_EN_ON_OUT},
  fmc130m_set_sw_clk_en_on
};

#define FMC130M_SET_SW_CLK_EN_OFF_ID 14
#define FMC130M_SET_SW_CLK_EN_OFF_IN 0
#define FMC130M_SET_SW_CLK_EN_OFF_OUT 4

uint8_t fmc130m_set_sw_clk_en_off(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_clk_en_off(NULL);
     fprintf(stderr,"[%s] "S"Setting Switching Clock OFF!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_clk_en_off_func = {
  {FMC130M_SET_SW_CLK_EN_OFF_ID,
   FMC130M_SET_SW_CLK_EN_OFF_IN,
   FMC130M_SET_SW_CLK_EN_OFF_OUT},
  fmc130m_set_sw_clk_en_off
};

#define FMC130M_GET_SW_CLK_EN_ID 15
#define FMC130M_GET_SW_CLK_EN_IN 0
#define FMC130M_GET_SW_CLK_EN_OUT 4

uint8_t fmc130m_get_sw_clk_en(uint8_t *input, uint8_t *output)
{
    uint32_t sw_clk_en_state;

    fmc_config_130m_4ch_board_p->set_sw_clk_en_on(&sw_clk_en_state);
    *((uint32_t *)output) = sw_clk_en_state;
     fprintf(stderr,"[%s] "S"Getting Switching Enable State: %u\n", timestamp_str(), sw_clk_en_state);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_clk_en_func = {
  {FMC130M_GET_SW_CLK_EN_ID,
   FMC130M_GET_SW_CLK_EN_IN,
   FMC130M_GET_SW_CLK_EN_OUT},
  fmc130m_get_sw_clk_en
};

/*************************************************/
/****** Set/Get Switching CLK DIV Functions ******/
/*************************************************/

#define FMC130M_SET_SW_DIVCLK_ID 16
#define FMC130M_SET_SW_DIVCLK_IN 4
#define FMC130M_SET_SW_DIVCLK_OUT 4

uint8_t fmc130m_set_sw_divclk(uint8_t *input, uint8_t *output)
{
    uint32_t divclk = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_divclk(divclk, NULL);
     fprintf(stderr,"[%s] "S"Setting DIVCLK value to: %u\n", timestamp_str(), divclk);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_divclk_func = {
  {FMC130M_SET_SW_DIVCLK_ID,
   FMC130M_SET_SW_DIVCLK_IN,
   FMC130M_SET_SW_DIVCLK_OUT},
  fmc130m_set_sw_divclk
};

#define FMC130M_GET_SW_DIVCLK_ID 17
#define FMC130M_GET_SW_DIVCLK_IN 0
#define FMC130M_GET_SW_DIVCLK_OUT 4

uint8_t fmc130m_get_sw_divclk(uint8_t *input, uint8_t *output)
{
    uint32_t divclk;

    fmc_config_130m_4ch_board_p->set_sw_divclk(NULL, &divclk);
    *((uint32_t *)output) = divclk;
     fprintf(stderr,"[%s] "S"Getting DIVCLK value: %u\n", timestamp_str(), divclk);

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

#define FMC130M_SET_SW_PHASECLK_ID 18
#define FMC130M_SET_SW_PHASECLK_IN 4
#define FMC130M_SET_SW_PHASECLK_OUT 4

uint8_t fmc130m_set_sw_phaseclk(uint8_t *input, uint8_t *output)
{
    uint32_t phaseclk = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_sw_phase(phaseclk, NULL);
     fprintf(stderr,"[%s] "S"Setting PHASECLK value to: %u\n", timestamp_str(), phaseclk);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_sw_phaseclk_func = {
  {FMC130M_SET_SW_PHASECLK_ID,
   FMC130M_SET_SW_PHASECLK_IN,
   FMC130M_SET_SW_PHASECLK_OUT},
  fmc130m_set_sw_phaseclk
};

#define FMC130M_GET_SW_PHASECLK_ID 19
#define FMC130M_GET_SW_PHASECLK_IN 0
#define FMC130M_GET_SW_PHASECLK_OUT 4

uint8_t fmc130m_get_sw_phaseclk(uint8_t *input, uint8_t *output)
{
    uint32_t phaseclk;

    fmc_config_130m_4ch_board_p->set_sw_phase(NULL, &phaseclk);
    *((uint32_t *)output) = phaseclk;
     fprintf(stderr,"[%s] "S"Getting PHASECLK value: %u\n", timestamp_str(), phaseclk);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_sw_phaseclk_func = {
  {FMC130M_GET_SW_PHASECLK_ID,
   FMC130M_GET_SW_PHASECLK_IN,
   FMC130M_GET_SW_PHASECLK_OUT},
  fmc130m_get_sw_phaseclk
};

/*************************************************/
/*********** Set/Get Windowing Functions *********/
/*************************************************/

#define FMC130M_SET_WDW_ON_ID 20
#define FMC130M_SET_WDW_ON_IN 0
#define FMC130M_SET_WDW_ON_OUT 4

uint8_t fmc130m_set_wdw_on(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_wdw_on(NULL);
     fprintf(stderr,"[%s] "S"Setting Windowing ON!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_wdw_on_func = {
  {FMC130M_SET_WDW_ON_ID,
   FMC130M_SET_WDW_ON_IN,
   FMC130M_SET_WDW_ON_OUT},
  fmc130m_set_wdw_on
};

#define FMC130M_SET_WDW_OFF_ID 21
#define FMC130M_SET_WDW_OFF_IN 0
#define FMC130M_SET_WDW_OFF_OUT 4

uint8_t fmc130m_set_wdw_off(uint8_t *input, uint8_t *output)
{
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_wdw_off(NULL);
     fprintf(stderr,"[%s] "S"Setting Windowing OFF!\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_wdw_off_func = {
  {FMC130M_SET_WDW_OFF_ID,
   FMC130M_SET_WDW_OFF_IN,
   FMC130M_SET_WDW_OFF_OUT},
  fmc130m_set_wdw_off
};

#define FMC130M_GET_WDW_ID 22
#define FMC130M_GET_WDW_IN 0
#define FMC130M_GET_WDW_OUT 4

uint8_t fmc130m_get_wdw(uint8_t *input, uint8_t *output)
{
    uint32_t wdw_state;

    fmc_config_130m_4ch_board_p->set_wdw_on(&wdw_state);
    *((uint32_t *)output) = wdw_state;
     fprintf(stderr,"[%s] "S"Getting Windowing State: %u\n", timestamp_str(), wdw_state);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_wdw_func = {
  {FMC130M_GET_WDW_ID,
   FMC130M_GET_WDW_IN,
   FMC130M_GET_WDW_OUT},
  fmc130m_get_wdw
};

#define FMC130M_SET_WDW_DLY_ID 23
#define FMC130M_SET_WDW_DLY_IN 4
#define FMC130M_SET_WDW_DLY_OUT 4

uint8_t fmc130m_set_wdw_dly(uint8_t *input, uint8_t *output)
{
    uint32_t wdw_dly = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_wdw_dly(wdw_dly, NULL);
     fprintf(stderr,"[%s] "S"Setting Windowing Delay to: %u\n", timestamp_str(), wdw_dly);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_wdw_dly_func = {
  {FMC130M_SET_WDW_DLY_ID,
   FMC130M_SET_WDW_DLY_IN,
   FMC130M_SET_WDW_DLY_OUT},
  fmc130m_set_wdw_dly
};

#define FMC130M_GET_WDW_DLY_ID 24
#define FMC130M_GET_WDW_DLY_IN 0
#define FMC130M_GET_WDW_DLY_OUT 4

uint8_t fmc130m_get_wdw_dly(uint8_t *input, uint8_t *output)
{
    uint32_t wdw_dly;

    fmc_config_130m_4ch_board_p->set_wdw_dly(NULL, &wdw_dly);
    *((uint32_t *)output) = wdw_dly;
     fprintf(stderr,"[%s] "S"Getting Windowing Delay value: %u\n", timestamp_str(), wdw_dly);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_wdw_dly_func = {
  {FMC130M_GET_WDW_DLY_ID,
   FMC130M_GET_WDW_DLY_IN,
   FMC130M_GET_WDW_DLY_OUT},
  fmc130m_get_wdw_dly
};

/*************************************************/
/************ Set/Get ADC CLK Functions **********/
/*************************************************/

#define FMC130M_SET_ADC_CLK_ID 25
#define FMC130M_SET_ADC_CLK_IN 4
#define FMC130M_SET_ADC_CLK_OUT 4

uint8_t fmc130m_set_adc_clk(uint8_t *input, uint8_t *output)
{
    uint32_t adcclk = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_adc_clk(adcclk, NULL);
     fprintf(stderr,"[%s] "S"Setting ADC_CLK value to: %u\n", timestamp_str(), adcclk);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_adc_clk_func = {
  {FMC130M_SET_ADC_CLK_ID,
   FMC130M_SET_ADC_CLK_IN,
   FMC130M_SET_ADC_CLK_OUT},
  fmc130m_set_adc_clk
};

#define FMC130M_GET_ADC_CLK_ID 26
#define FMC130M_GET_ADC_CLK_IN 0
#define FMC130M_GET_ADC_CLK_OUT 4

uint8_t fmc130m_get_adc_clk(uint8_t *input, uint8_t *output)
{
    uint32_t adcclk;

    fmc_config_130m_4ch_board_p->set_adc_clk(NULL, &adcclk);
    *((uint32_t *)output) = adcclk;
     fprintf(stderr,"[%s] "S"Getting ADC_CLK value: %u\n", timestamp_str(), adcclk);

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

#define FMC130M_SET_DDS_FREQ_ID 27
#define FMC130M_SET_DDS_FREQ_IN 4
#define FMC130M_SET_DDS_FREQ_OUT 4

uint8_t fmc130m_set_dds_freq(uint8_t *input, uint8_t *output)
{
    uint32_t ddsfreq = *((uint32_t *) input);
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_dds_freq(ddsfreq, NULL);
     fprintf(stderr,"[%s] "S"Setting DDS_FREQ value to: %u\n", timestamp_str(), ddsfreq);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_dds_freq_func = {
  {FMC130M_SET_DDS_FREQ_ID,
   FMC130M_SET_DDS_FREQ_IN,
   FMC130M_SET_DDS_FREQ_OUT},
  fmc130m_set_dds_freq
};

#define FMC130M_GET_DDS_FREQ_ID 28
#define FMC130M_GET_DDS_FREQ_IN 0
#define FMC130M_GET_DDS_FREQ_OUT 4

uint8_t fmc130m_get_dds_freq(uint8_t *input, uint8_t *output)
{
    uint32_t ddsfreq;

    fmc_config_130m_4ch_board_p->set_dds_freq(NULL, &ddsfreq);
    *((uint32_t *)output) = ddsfreq;
     fprintf(stderr,"[%s] "S"Getting DDS_FREQ value: %u\n", timestamp_str(), ddsfreq);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_dds_freq_func = {
  {FMC130M_GET_DDS_FREQ_ID,
   FMC130M_GET_DDS_FREQ_IN,
   FMC130M_GET_DDS_FREQ_OUT},
  fmc130m_get_dds_freq
};

/*************************************************/
/****** Set/Get Data Acquisition Functions ******/
/*************************************************/

#define FMC130M_SET_ACQ_PARAM_ID 29
#define FMC130M_SET_ACQ_PARAM_IN 8
#define FMC130M_SET_ACQ_PARAM_OUT 4

uint8_t fmc130m_set_acq_params(uint8_t *input, uint8_t *output)
{
    uint32_t nsamples = *((uint32_t *) input);
    uint32_t acq_chan = *((uint32_t *) input + 1);
     fprintf(stderr,"[%s] "S"Setting DATA_ACQ values: ", timestamp_str());

    // Do some checking
    if (acq_chan > END_CHAN_ID-1) {
        return -1; // invalid channel
    }

    if (nsamples > ddr3_acq_chan[acq_chan].max_samples) {
        return -2; // invalid number of samples
    }

    fprintf(stderr,"acq_chan to %d, num_samples to %d and offset to 0x%X\n", acq_chan,
            nsamples, ddr3_acq_chan[acq_chan].start_addr);

    fmc_config_130m_4ch_board_p->set_acq_nsamples(nsamples);
    fmc_config_130m_4ch_board_p->set_acq_chan(acq_chan);
    fmc_config_130m_4ch_board_p->set_acq_offset(ddr3_acq_chan[acq_chan].start_addr);

    *((uint32_t *)output) = 0;
    return 0; // Success!!
}

static struct bsmp_func fmc130m_set_acq_params_func = {
  {FMC130M_SET_ACQ_PARAM_ID,
   FMC130M_SET_ACQ_PARAM_IN,
   FMC130M_SET_ACQ_PARAM_OUT},
  fmc130m_set_acq_params
};

#define FMC130M_GET_ACQ_NSAMPLES_ID 30
#define FMC130M_GET_ACQ_NSAMPLES_IN 0
#define FMC130M_GET_ACQ_NSAMPLES_OUT 4

uint8_t fmc130m_get_acq_nsamples(uint8_t *input, uint8_t *output)
{
    uint32_t nsamples;

    fmc_config_130m_4ch_board_p->get_acq_nsamples(&nsamples);
    *((uint32_t *)output) = nsamples;
     fprintf(stderr,"[%s] "S"Getting AACQ_NSAMPLES: %d\n", timestamp_str(), nsamples);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_acq_nsamples_func = {
  {FMC130M_GET_ACQ_NSAMPLES_ID,
   FMC130M_GET_ACQ_NSAMPLES_IN,
   FMC130M_GET_ACQ_NSAMPLES_OUT},
  fmc130m_get_acq_nsamples
};

#define FMC130M_GET_ACQ_CHAN_ID 31
#define FMC130M_GET_ACQ_CHAN_IN 0
#define FMC130M_GET_ACQ_CHAN_OUT 4

uint8_t fmc130m_get_acq_chan(uint8_t *input, uint8_t *output)
{
    uint32_t acq_chan;

    fmc_config_130m_4ch_board_p->get_acq_chan(&acq_chan);
    *((uint32_t *)output) = acq_chan;
     fprintf(stderr,"[%s] "S"Getting ACQ_CHAN: %u\n", timestamp_str(), acq_chan);

    return 0; // Success!!
}

static struct bsmp_func fmc130m_get_acq_chan_func = {
  {FMC130M_GET_ACQ_CHAN_ID,
   FMC130M_GET_ACQ_CHAN_IN,
   FMC130M_GET_ACQ_CHAN_OUT},
  fmc130m_get_acq_chan
};

#define FMC130M_START_ACQ_ID 32
#define FMC130M_START_ACQ_IN 0
#define FMC130M_START_ACQ_OUT 4

uint8_t fmc130m_start_acq(uint8_t *input, uint8_t *output)
{
     fprintf(stderr,"[%s] "S"Starting ACQ...\n", timestamp_str());
    *((uint32_t *)output) = fmc_config_130m_4ch_board_p->set_data_acquire();
     fprintf(stderr,"[%s] "S"ACQ finished...\n", timestamp_str());

    return 0; // Success!!
}

static struct bsmp_func fmc130m_start_acq_func = {
  {FMC130M_START_ACQ_ID,
   FMC130M_START_ACQ_IN,
   FMC130M_START_ACQ_OUT},
  fmc130m_start_acq
};

/*************************************************/
/************** Position Data Curves *************/
/*************************************************/

static void curve_read_block (struct bsmp_curve *curve, uint16_t block,
                              uint8_t *data, uint16_t *len)
{
    /* Let's check which curve we have so we can point to the right block. */

    /* Note: the use of strcmp here is unsafe, but bear in mind that this server
     * is just an example. A safer way to do it would to be to use strncmp or
     * not to use strings altogether as identifiers (as they are slow, too).
     */

    /* Note: the library will NOT request access to Curve blocks beyond the
     * specified limits. If you are paranoid or do not trust the library, you
     * can check the block limits yourself.
     */


    uint8_t *block_data;
    uint16_t block_size = curve->info.block_size;
    uint32_t curve_id = *((uint32_t*)curve->user);

    if (block == 0) {
      fprintf(stderr,"[%s] "S"Reading curve: ", timestamp_str());
      fprintf(stderr,"curve id = %d, block_size = %d\n", curve_id, block_size);
    }

    if (curve_id > END_CHAN_ID-1) {
      fprintf(stderr, "[%s] "S"Unexpected curve ID!\n", timestamp_str());
      return;
    }

    fmc_config_130m_4ch_board_p->get_acq_data_block(curve_id, block*block_size,
                        block_size, (uint32_t *)data, (uint32_t *)len);
}

//struct bsmp_curve_info
//{
//    uint8_t  id;                    // ID of the curve, used in the protocol.
//    bool     writable;              // Determine if the curve is writable.
//    uint32_t nblocks;               // How many blocks the curve contains.
//    uint16_t block_size;            // Maximum number of bytes in a block
//    uint8_t  checksum[16];          // MD5 checksum of the curve
//};
//
//struct bsmp_curve;
//
//typedef void (*bsmp_curve_read_t)  (struct bsmp_curve *curve, uint16_t block,
//                                    uint8_t *data, uint16_t *len);
//typedef void (*bsmp_curve_write_t) (struct bsmp_curve *curve, uint16_t block,
//                                    uint8_t *data, uint16_t len);
//struct bsmp_curve
//{
//    // Info about the curve identification
//    struct bsmp_curve_info info;
//
//    // Functions to read/write a block
//    void (*read_block)(struct bsmp_curve *curve, uint16_t block, uint8_t *data,
//                       uint16_t *len);
//
//    void (*write_block)(struct bsmp_curve *curve, uint16_t block, uint8_t *data,
//                        uint16_t len);
//
//    // The user can make use of this variable as he wishes. It is not touched by
//    // BSMP
//    void *user;
//};

static uint32_t adc_id = ADC_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve adc_curve = {
    {   // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        16384,  // 16384 blocks
        32768,  // 32768 bytes per block
        {0}     // checksum
    },
    curve_read_block, // read_block
    NULL,             // write_block
    (void*) &adc_id    //user
};

static uint32_t tbtamp_id = TBTAMP_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve tbtamp_curve = {
    {   // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        16384,  // 16384 blocks
        32768,  // 32768 bytes per block
        {0}     // checksum
    },
    curve_read_block, // read_block
    NULL,             // write_block
    (void*) &tbtamp_id    //user
};

static uint32_t tbtpos_id = TBTPOS_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve tbtpos_curve = {
    {   // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        16384,  // 16384 blocks
        32768,  // 32768 bytes per block
        {0}     // checksum
    },
    curve_read_block, // read_block
    NULL,             // write_block
    (void*) &tbtpos_id    //user
};

static uint32_t fofbamp_id = FOFBAMP_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve fofbamp_curve = {
    {   // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        16384,  // 16384 blocks
        32768,  // 32768 bytes per block
        {0}     // checksum
    },
    curve_read_block, // read_block
    NULL,             // write_block
    (void*) &fofbamp_id    //user
};

static uint32_t fofbpos_id = FOFBPOS_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve fofbpos_curve = {
    {  // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        16384,  // 16384 blocks
        32768,  // 32768 bytes per block
        {0}     // checksum
    },
    curve_read_block, // read_block
    NULL,             // write_block
    (void*) &fofbpos_id    //user
};

// Monit Amp
#define MONITAMP_CHAN_ID                 0

static void curve_read_monitamp_block (struct bsmp_curve *curve, uint16_t block,
                              uint8_t *data, uint16_t *len)
{

    DEBUGP(SMON"Reading curve monit. amp block...\n");

    uint8_t *block_data;
    uint16_t block_size = curve->info.block_size;
    uint32_t curve_id = *((uint32_t*)curve->user);

    if (curve_id != MONITAMP_CHAN_ID) {
      fprintf(stderr, "[%s] "SMON"Unexpected curve ID!\n", timestamp_str());
      return;
    }

    fmc_config_130m_4ch_board_p->get_acq_sample_monit_amp((uint32_t *)data,
            (uint32_t *)len);
}

static uint32_t monitamp_id = MONITAMP_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve monitamp_curve = {
    {  // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        1,      // 1 blocks
        16,     // 16 bytes per block
        {0}     // checksum
    },
    curve_read_monitamp_block, // read_block
    NULL,             // write_block
    (void*) &monitamp_id
};

// Monit Pos
#define MONITPOS_CHAN_ID                 1

static void curve_read_monitpos_block (struct bsmp_curve *curve, uint16_t block,
                              uint8_t *data, uint16_t *len)
{

    DEBUGP(SMON"Reading curve monit. pos block...\n");

    uint8_t *block_data;
    uint16_t block_size = curve->info.block_size;
    uint32_t curve_id = *((uint32_t*)curve->user);

    if (curve_id != MONITPOS_CHAN_ID) {
      fprintf(stderr, "[%s] "SMON"Unexpected curve ID!\n", timestamp_str());
      return;
    }

    fmc_config_130m_4ch_board_p->get_acq_sample_monit_pos((uint32_t *)data,
            (uint32_t *)len);
}

static uint32_t monitpos_id = MONITPOS_CHAN_ID;
// Whole cruve can get up to 512 MB
static struct bsmp_curve monitpos_curve = {
    {  // info
        0,      // Internal protocol id
        false,  // writable = Read-only
        1,      // 1 blocks
        16,     // 16 bytes per block
        {0}     // checksum
    },
    curve_read_monitpos_block, // read_block
    NULL,             // write_block
    (void*) &monitpos_id
};

int main(int argc, const char **argv) {

  cout << "FMC configuration software for FMC ADC 130M 4CH card (PASSIVE version)" << endl <<
                  "Author: Andrzej Wojenski (original idea) and Lucas Russo" << endl;

  //WBInt_drv* int_drv;
  wb_data data;
  vector<uint16_t> amc_temp;
  vector<uint16_t> test_pattern;
  //commLink* _commLink = new commLink();
  //fmc_config_130m_4ch_board *fmc_config_130m_4ch_board_p;
  tcp_server *tcp_server_p;
  tcp_server *tcp_server_monit_p;
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

  fprintf(stderr, "%s: platform (%s) set!\n", program, platform_name);

  fmc_config_130m_4ch_board_p = new fmc_config_130m_4ch_board(new pcie_link_driver(0),
                                                  delay_data_l,
                                                  delay_clk_l);

  // Initialize TCP server and BSMP library

  tcp_server_p = new tcp_server(string("8080")/*, fmc_config_130m_4ch_board_p*/);

  // Register functions
  tcp_server_p->register_func(&fmc130m_blink_leds_func);
  tcp_server_p->register_func(&fmc130m_config_defaults_func);
  tcp_server_p->register_func(&fmc130m_get_temp1_func);
  tcp_server_p->register_func(&fmc130m_get_temp2_func);

  tcp_server_p->register_func(&fmc130m_set_kx_func);
  tcp_server_p->register_func(&fmc130m_get_kx_func);
  tcp_server_p->register_func(&fmc130m_set_ky_func);
  tcp_server_p->register_func(&fmc130m_get_ky_func);
  tcp_server_p->register_func(&fmc130m_set_ksum_func);
  tcp_server_p->register_func(&fmc130m_get_ksum_func);

  tcp_server_p->register_func(&fmc130m_set_sw_on_func);
  tcp_server_p->register_func(&fmc130m_set_sw_off_func);
  tcp_server_p->register_func(&fmc130m_get_sw_func);

  // Regsiter get/set Switching Enable functions
  tcp_server_p->register_func(&fmc130m_set_sw_clk_en_on_func);
  tcp_server_p->register_func(&fmc130m_set_sw_clk_en_off_func);
  tcp_server_p->register_func(&fmc130m_get_sw_clk_en_func);

  tcp_server_p->register_func(&fmc130m_set_sw_divclk_func);
  tcp_server_p->register_func(&fmc130m_get_sw_divclk_func);
  tcp_server_p->register_func(&fmc130m_set_sw_phaseclk_func);
  tcp_server_p->register_func(&fmc130m_get_sw_phaseclk_func);

  tcp_server_p->register_func(&fmc130m_set_wdw_on_func);
  tcp_server_p->register_func(&fmc130m_set_wdw_off_func);
  tcp_server_p->register_func(&fmc130m_get_wdw_func);
  tcp_server_p->register_func(&fmc130m_set_wdw_dly_func);
  tcp_server_p->register_func(&fmc130m_get_wdw_dly_func);

  tcp_server_p->register_func(&fmc130m_set_adc_clk_func);
  tcp_server_p->register_func(&fmc130m_get_adc_clk_func);
  tcp_server_p->register_func(&fmc130m_set_dds_freq_func);
  tcp_server_p->register_func(&fmc130m_get_dds_freq_func);

  tcp_server_p->register_func(&fmc130m_set_acq_params_func);
  tcp_server_p->register_func(&fmc130m_get_acq_nsamples_func);
  tcp_server_p->register_func(&fmc130m_get_acq_chan_func);
  tcp_server_p->register_func(&fmc130m_start_acq_func);

  // Register curves functions
  tcp_server_p->register_curve(&adc_curve);
  tcp_server_p->register_curve(&tbtamp_curve);
  tcp_server_p->register_curve(&tbtpos_curve);
  tcp_server_p->register_curve(&fofbamp_curve);
  tcp_server_p->register_curve(&fofbpos_curve);

  // Register monit curves functions
  tcp_server_p->register_curve(&monitamp_curve);
  tcp_server_p->register_curve(&monitpos_curve);

  /* Endless tcp loop */
  tcp_server_p->start();

  return 0;
}
