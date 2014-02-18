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

void debug_print (const char *fmt, ...) __attribute__((format(printf,1,2)));

// Debug print
#ifdef DEBUG
#define dbg_print(fmt, ...) \
    debug_print(fmt, ## __VA_ARGS__)
#else
#define dbg_print(fmt, ...)
#endif /* Debug print */

#ifdef DEBUG
#define DEBUGP(fmt, ...)                 \
    do{                                     \
            dbg_print(fmt, ## __VA_ARGS__); \
        }                                   \
    } while(0)
#else
#define DEBUGP(fmt, ...)
#endif

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

enum delay_type_t
{
  DLY_DATA = 1,
  DLY_CLK = 2,
  DLY_ALL = 3     // DLY_ALL = DLY_DATA | DLY_CLK
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
extern const struct delay_lines *delay_data_l;
extern const struct delay_lines *delay_clk_l;

void help(void);
enum platform_t lookupstring_i(const char *key);
enum platform_t lookupstring_i(const char *key);
void set_fpga_delay(commLink* _commLink, uint32_t addr, uint32_t delay_val,
                        enum delay_type_t dly_type);
void set_fpga_delay_s(commLink* _commLink, uint32_t addr, const struct delay_lines *delay_val,
                        enum delay_type_t dly_type);

#endif
