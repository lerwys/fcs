//============================================================================
// Author      : Lucas Russo
// Version     : 1.0
// Description : Common definitons for supporting different boards
//============================================================================

#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include "commLink.h"

#include <stdint.h>

#define MAX_PLATFORM_SIZE_ID 8
#define ML605_STRING "ML605"
#define KC705_STRING "KC705"
#define AFC_STRING "AFC"
#define NOPLAT_STRING "NOPLAT"

// delay lines definitions
#define DELAY_LINES_NO_INIT 0
#define DELAY_LINES_INIT 1
#define DELAY_LINES_END 2

enum platform_t {
  ML605,
  KC705,
  AFC,
  /* add more platforms here */
  BAD_PLATFORM,
  PLATFORM_NOT_SET,
};

struct delay_lines {
  unsigned char init;
  uint32_t value;
};

struct sym_t {
  const char *key;
  enum platform_t val;
};

/* to be filled by main */
extern const char* program;
extern enum platform_t platform;
extern const char* platform_name;
extern int verbose;
extern int quiet;
extern const struct delay_lines *delay_l;

void help(void);
enum platform_t lookupstring_i(const char *key);
enum platform_t lookupstring_i(const char *key);
void set_fpga_delay(commLink* _commLink, uint32_t addr, uint32_t delay_val);
void set_fpga_delay_s(commLink* _commLink, uint32_t addr, const struct delay_lines *delay_val);

#endif
