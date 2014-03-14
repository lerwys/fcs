//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Common definitons for supporting different boards
//============================================================================

#include "common.h"
#include "data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NKEYS (sizeof(lookuptable)/sizeof(struct sym_t))

const char* program;
enum platform_t platform;
const char* platform_name;
int verbose;
int quiet;
const struct delay_lines *delay_data_l;
const struct delay_lines *delay_clk_l;
const char *build_revision = "";
const char *build_date = __DATE__ " " __TIME__;

static const struct sym_t lookuptable[] = {
    { ML605_STRING, ML605 },
    { KC705_STRING, KC705 },
    { AFC_STRING, AFC }
};

void help(void) {
  fprintf(stderr, "FCS Server Program\n");
  fprintf(stderr, "Git commit ID: %s.\n", build_revision);
  fprintf(stderr, "Build date: %s.\n\n", build_date);
  fprintf(stderr, "Usage: %s [OPTION]\n", program);
  fprintf(stderr, "\n");
  fprintf(stderr, "  -p <platform>  supportted platforms (ML605/KC705/AFC)\n");
  fprintf(stderr, "  -v             verbose operation\n");
  fprintf(stderr, "  -q             quiet: do not display warnings\n");
  fprintf(stderr, "  -h             display this help and exit\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Report bugs to <a.wojenski@elka.pw.edu.pl> and/or <lucas.russo@lnls.br>\n");
  fprintf(stderr, "Version (%s). Licensed under the GPL v3.\n", VERSION);
}

/* Based on http://stackoverflow.com/questions/1102542/how-to-define-an-enumerated-type-enum-in-c */
enum platform_t lookupstring(const char *key)
{
    int i;

    for (i=0; i < NKEYS; i++) {
      const struct sym_t *sym = lookuptable + i;

      if (strcmp(sym->key, key) == 0) {
        return sym->val;
      }
    }

    return BAD_PLATFORM;
}

// lookup string insensitive case
enum platform_t lookupstring_i(const char *key)
{
  char *p, *conv_str;
  enum platform_t ret;

  p = conv_str = strdup(key);

  for( ; *p; ++p)
    *p = toupper(*p);

  ret = lookupstring(conv_str);
  free(conv_str);

  return ret;
}

void set_fpga_delay(commLink* _commLink, uint32_t addr, uint32_t delay_val,
                      enum delay_type_t dly_type)
{
  wb_data data;

  data.data_send.resize(2);
  data.extra.resize(2);
  data.data_send[0] = 0;
  data.data_send[1] = 0;
  data.wb_addr = addr;

  if (dly_type == DLY_DATA) {
    data.data_send[0] = IDELAY_DATA_LINES;
    printf ("dly type DLY_DATA\n");
  }

  if (dly_type == DLY_CLK) {
    data.data_send[0] |= IDELAY_CLK_LINE;
    printf ("dly type DLY_CLK\n");
  }

  //data.data_send[0] = IDELAY_ALL_LINES | IDELAY_TAP(delay_val) | IDELAY_UPDATE; // should be 0x0050003f
  //data.data_send[0] |= IDELAY_TAP(delay_val) | IDELAY_UPDATE; // should be 0x0050003f
  data.data_send[0] |= IDELAY_TAP(delay_val);
  _commLink->fmc_config_send(&data);
  data.data_send[0] |= IDELAY_UPDATE;
  _commLink->fmc_config_send(&data);
  data.data_send[0] |= IDELAY_UPDATE;
  _commLink->fmc_config_send(&data);
  data.data_send[0] |= IDELAY_UPDATE;
  _commLink->fmc_config_send(&data);
  // check data
  _commLink->fmc_config_read(&data);
  printf ("dly data: 0x%X\n", data.data_read[0]);
  //assert(data.data_read[0] == (IDELAY_ALL_LINES | IDELAY_TAP(delay_val) | IDELAY_UPDATE));
  usleep(1000);
  //data.data_send[0] = (IDELAY_ALL_LINES | IDELAY_TAP(delay_val)) & 0xFFFFFFFE; // should be 0x0050003f
  //_commLink->fmc_config_send(&data);
}

// safer set FPGA delay
void set_fpga_delay_s(commLink* _commLink, uint32_t addr, const struct delay_lines *delay_val,
                        enum delay_type_t dly_type)
{
  if (delay_val->init == DELAY_LINES_NO_INIT)
    return;

  if (delay_val->init == DELAY_LINES_END)
    return;

  set_fpga_delay(_commLink, addr, delay_val->value, dly_type);
}

void enum_to_string(enum platform_t platform, char *platform_name, int len)
{
  strncpy(platform_name, lookuptable[platform].key, len);
}
