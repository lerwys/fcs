//============================================================================
// Author      : Andrzej Wojenski
// Version     : 1.0
// Description : Software driver for Si570/ Si571 chip (clock generator)
//============================================================================
#include "si570.h"
#include "inttypes.h"

#define SI570_ADDR 0x55
#define MAX_REPEAT 10

#define ULLONG_MAX      (~0ULL)

// Define PRIx64 print macros
//#define __STDC_FORMAT_MACROS

wb_data Si570_drv::data_;
commLink* Si570_drv::commLink_;
string Si570_drv::i2c_id_;
string Si570_drv::gpio_id_;

static uint64_t __fxtal = 0;
static unsigned int __n1 = 0;
static unsigned int __hs_div = 0;
static uint64_t __rfreq = 0;
static uint64_t __frequency = SI570_FOUT_FACTORY_DFLT;

#define ARRAY_SIZE(ARRAY) (sizeof(ARRAY)/sizeof((ARRAY)[0]))

// From Linux kernel
static inline uint64_t div_u64_rem(uint64_t dividend, uint32_t divisor, uint32_t *remainder)
{
  *remainder = dividend % divisor;
  return dividend / divisor;
}

// From Linux kernel
static inline uint64_t div_u64(uint64_t dividend, uint32_t divisor)
{
  uint32_t remainder;
  return div_u64_rem(dividend, divisor, &remainder);
}

// From Linux kernel
static inline uint64_t div64_u64(uint64_t dividend, uint64_t divisor)
{
  return dividend / divisor;
}

// From Linux kernel
static inline int64_t div64_s64(int64_t dividend, int64_t divisor)
{
  return dividend / divisor;
}

static int __si570_calc_divs(unsigned long frequency,
                uint64_t *out_rfreq, unsigned int *out_n1, unsigned int *out_hs_div);

void Si570_drv::si570_setCommLink(commLink* comm, string i2c_id, string gpio_id) {

  commLink_ = comm;
  i2c_id_ = i2c_id;
  gpio_id_ = gpio_id;
  data_.data_send.resize(2);
  data_.extra.resize(1);
}

// get info about startup setting so user can calculate needed values
int Si570_drv::si570_read_freq(wb_data* data, uint64_t *rfreq,
                                unsigned int *n1, unsigned int *hs_div) {

#if 0
  uint32_t HS_DIV, N1;
  uint32_t RFFREQ_INTEGER; // 10 bits
  uint32_t RFFREQ_INTEGER_FLOAT;
  //uint64_t RFFREQ = 0; // 38 bits
  //uint64_t fdco = 0;
  int err = 0, std_read = 0;
  //int data_size = data->data_send.size();

  data->data_send.resize(1);

  //cout << showbase << internal << setfill('0') << setw(8);

//  if (data->extra.size() < 2) {
//    data->extra.resize(2);
//    data->extra[0] = SI570_ADDR;
//    data->extra[1] = 6; // number of registers to read
//    data->data_send[0] = 0x07; // starting register
//    std_read = 1;
//  }

  //if (data->data_send.size() == 0)
  //  data->data_send[0] = 0x07; // starting register

  err = commLink_->fmc_send_read(i2c_id_, data);

  //data->data_send.resize(data_size);

  if (err != 0)
    return err;

  // parsing data
  for (unsigned int i = 0; i < data->extra[1]; i++)
    cout << "Si570: data read [" << i << "] : 0x" << hex << (data->data_read[i] & 0xFF) << endl;

  if (std_read == 1) {
    //HS_DIV = data->data_read[0] >> 5;
    HS_DIV = (data->data_read[0] & 0xE0) >> 5;
    //N1 = ( (data->data_read[0] & 0x1F) << 2) | (data->data_read[1] >> 6);
    N1 = ( (data->data_read[0] & 0x1F) << 2) | ((data->data_read[1] & 0xC0) >> 6);

    RFFREQ_INTEGER = ((data->data_read[1] & 0x3F) << 4) | ((data->data_read[2] & 0xF0) >> 4);

    RFFREQ_INTEGER_FLOAT = (data->data_read[2] & 0x0F) << 3*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[3] << 2*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[4] << 1*8;
    RFFREQ_INTEGER_FLOAT |= data->data_read[5];

    //RFFREQ = data->data_read[1] & 0x3F;
    //
    //RFFREQ = (RFFREQ << 8) + data->data_read[2];
    //RFFREQ = (RFFREQ << 8) + data->data_read[3];
    //RFFREQ = (RFFREQ << 8) + data->data_read[4];
    //RFFREQ = (RFFREQ << 8) + data->data_read[5];

    cout << "Si570: Read parameters: " << endl <<
        "HS_DIV: 0x" << hex << HS_DIV << endl <<
        "N1: 0x" << hex << N1 << endl <<
        "RFFREQ_INTEGER: 0x" << hex << RFFREQ_INTEGER << endl <<
        "RFFREQ_INTEGER_FLOAT: 0x" << hex << RFFREQ_INTEGER_FLOAT << endl;

    //cout << "Si570: Read parameters: " << endl <<
    //    "HS_DIV: 0x" << hex << HS_DIV << endl <<
    //    "N1: 0x" << hex << N1 << endl <<
    //    "RFFREQ (LSB): 0x" << hex << (RFFREQ & 0x00000000FFFFFFFF) << endl <<
    //    "RFFREQ (MSB): 0x" << hex << ((RFFREQ & 0xFFFFFFFF00000000) >> 32) << endl;
  }
#endif

  int err;
  uint64_t tmp;

  data->data_send.resize(1);

  err = commLink_->fmc_send_read(i2c_id_, data);

  if (err != 0)
    return err;

  // parsing data
  for (unsigned int i = 0; i < data->extra[1]; i++)
    cout << "Si570: data read [" << i << "] : 0x" << hex << (data->data_read[i] & 0xFF) << endl;

  *hs_div = ((data->data_read[0] & HS_DIV_MASK) >> HS_DIV_SHIFT) + HS_DIV_OFFSET;
  *n1 = ((data->data_read[0] & N1_6_2_MASK) << 2) + ((data->data_read[1] & N1_1_0_MASK) >> 6) + 1;
  /* Handle invalid cases */
  if (*n1 > 1)
          *n1 &= ~1;

  tmp = data->data_read[1] & RFREQ_37_32_MASK;
  tmp = (tmp << 8) + data->data_read[2];
  tmp = (tmp << 8) + data->data_read[3];
  tmp = (tmp << 8) + data->data_read[4];
  tmp = (tmp << 8) + data->data_read[5];
  *rfreq = tmp;

  cout << "RFREQ = " << *rfreq << endl;
  cout << "HS_DIV = " << *hs_div << endl;
  cout << "N1 = " << *n1 << endl;

  return err;
}

int Si570_drv::si570_get_defaults(uint32_t addr)
{
  int err;
  uint64_t fdco;
  wb_data data;

  data_.extra[0] = addr; // chip addr

  data_.data_send[0] = SI570_REG_CONTROL;
  data_.data_send[1] = SI570_CNTRL_RECALL;

  cout << "Returning si571 to initial conditions" << endl;
  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  usleep(30000); // 30ms

  cout << "Reading default configurations" << endl;

  data.data_send.clear();
  data.data_read.clear();
  data.extra.clear();
  data.data_send.resize(1);
  data.data_read.resize(10);
  data.extra.resize(2);
  data.extra[0] = addr;
  data.extra[1] = 6;
  data.data_send[0] = 0x07; // starting register

  err = si570_read_freq(&data, &__rfreq, &__n1, &__hs_div);
  if (err)
          return err;

  /*
   * Accept optional precision loss to avoid arithmetic overflows.
   * Acceptable per Silicon Labs Application Note AN334.
   */
  fdco = SI570_FOUT_FACTORY_DFLT * __n1 * __hs_div;
  if (fdco >= (1LL << 36))
          __fxtal = div64_u64(fdco << 24, __rfreq >> 4);
  else
          __fxtal = div64_u64(fdco << 28, __rfreq);

  __frequency = SI570_FOUT_FACTORY_DFLT;

  return 0;
}

// From the linux kernel
static int __si570_calc_divs(unsigned long frequency,
                uint64_t *out_rfreq, unsigned int *out_n1, unsigned int *out_hs_div)
{
  int i;
  unsigned int n1, hs_div;
  uint64_t fdco, best_fdco = ULLONG_MAX;
  static const uint8_t si570_hs_div_values[] = { 11, 9, 7, 6, 5, 4 };

  for (i = 0; i < ARRAY_SIZE(si570_hs_div_values); i++) {
          hs_div = si570_hs_div_values[i];
          /* Calculate lowest possible value for n1 */
          n1 = div_u64(div_u64(FDCO_MIN, hs_div), frequency);
          if (!n1 || (n1 & 1))
                  n1++;
          while (n1 <= 128) {
                  fdco = (uint64_t)frequency * (uint64_t)hs_div * (uint64_t)n1;
                  if (fdco > FDCO_MAX)
                          break;
                  if (fdco >= FDCO_MIN && fdco < best_fdco) {
                          *out_n1 = n1;
                          *out_hs_div = hs_div;
                          *out_rfreq = div64_u64(fdco << 28, __fxtal);
                          best_fdco = fdco;
                  }
                  n1 += (n1 == 1 ? 1 : 2);
          }
  }

  if (best_fdco == ULLONG_MAX)
    return -1;

  return 0;
}

int Si570_drv::si570_set_freq(wb_data* data) {

  int i;
  int err = 0, repeat = 0;

/*  if (data->extra.size() < 1) {
    data_.extra[0] = SI570_ADDR;
    data->extra[0] = SI570_ADDR;
  }
  else*/

  data_.extra[0] = data->extra[0]; // chip addr

  // freeze DCO - reg 137 bit 4
  data_.data_send[0] = 0x89;
  data_.data_send[1] = 0x10;

  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  // wait 10 ms
  usleep(10000);

  // write data (for 20ppm and 50ppm devices) - regs 7 - 12
  for (i = 0; i < 6; i++) {
    data_.data_send[0] = 0x07 + i;
    data_.data_send[1] = data->data_send[i];
    err =  commLink_->fmc_send(i2c_id_, &data_);
    if (err != 0)
      return err;
  }

  // unfreeze DCO + append new freq - bit 6 reg 137
  data_.data_send[0] = 0x89;
  data_.data_send[1] = 0x00; // unfreeze DCO
  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  data_.data_send[0] = 0x87;
  data_.data_send[1] = 0x40; // apply new freq (NewFreq bit)
  err = commLink_->fmc_send(i2c_id_, &data_);
  if (err != 0)
    return err;

  // wait 10 ms
  usleep(10000);

  // check if newfreq bit is cleared (new frequency applied)
  while(1) { // bit automatically cleared

    data_.data_send[0] = 0x87; // reg 135
    data_.extra[1] = 1;
    err = commLink_->fmc_send_read(i2c_id_, &data_); //i2c_int->int_send_read_data(&data_);
//cout << "data: " << hex << data_.data_read[0] << endl;
    if ( ( (data_.data_read[0] & 0xBF) >> 6 ) == 0)
      break;
    sleep(1);

    repeat++;

    if (repeat > MAX_REPEAT) {
      cout << "Si570: Error: Frequency not set." << endl;
      err = 1;
      return err;
    }

  }

  cout << "Si570: Setup new frequency completed" << endl;

  //dev_info(fmc->hwdev, "Si570: Setup new frequency completed\n");

  return err;

}

int Si570_drv::si570_set_freq_hl(unsigned long frequency, uint32_t addr) {

  int err;
  wb_data data;

  data.data_send.clear();
  data.data_read.clear();
  data.extra.clear();
  data.data_send.resize(1);
  data.data_read.resize(10);
  data.extra.resize(2);

  err = __si570_calc_divs(frequency, &__rfreq, &__n1, &__hs_div);
  if (err)
          return err;

  uint8_t reg[6];
  reg[0] = ((__hs_div - HS_DIV_OFFSET) << HS_DIV_SHIFT) |
                         (((__n1 - 1) >> 2) & N1_6_2_MASK);
  reg[1] = ((__n1 - 1) << 6) |
          ((__rfreq >> 32) & RFREQ_37_32_MASK);
  reg[2] = (__rfreq >> 24) & 0xff;
  reg[3] = (__rfreq >> 16) & 0xff;
  reg[4] = (__rfreq >> 8) & 0xff;
  reg[5] = __rfreq & 0xff;

  data.data_send.clear();
  data.data_send.push_back(reg[0]);
  data.data_send.push_back(reg[1]);
  data.data_send.push_back(reg[2]);
  data.data_send.push_back(reg[3]);
  data.data_send.push_back(reg[4]);
  data.data_send.push_back(reg[5]);

  data.extra[0] = addr;
  data.extra[1] = 6;

  err = si570_set_freq(&data);
  if (err)
    return err;

  sleep(1);

  data.data_send.clear();
  data.data_read.clear();

  return 0;
}

int Si570_drv::si570_outputEnable(uint32_t addr) {

  data_.wb_addr = addr;
  commLink_->fmc_read(gpio_id_, &data_);
  data_.data_send[0] = data_.data_read[0] | 0x1;
  commLink_->fmc_send(gpio_id_, &data_);

  usleep(30000); // 30ms

  cout << "Si571 output enabled" << endl;

  return 0;
}

int Si570_drv::si570_outputDisable(uint32_t addr) {

  data_.wb_addr = addr;
  commLink_->fmc_read(gpio_id_, &data_);
  data_.data_send[0] = data_.data_read[0] & 0xFFFFFFFE;
  commLink_->fmc_send(gpio_id_, &data_);

  usleep(30000); // 30ms

  cout << "Si571 output disabled" << endl;

  return 0;
}

void Si570_drv::si570_assert(uint32_t chip_addr, uint8_t reg, uint8_t val) {

  data_.extra[0] = chip_addr;
  data_.extra[1] = 1;
  data_.data_send[0] = reg; // starting register

   unsigned int n1;
   unsigned int hs_div;
   uint64_t rfreq;

  si570_read_freq(&data_, &rfreq, &n1, &hs_div);

  //cout << "Si571 assert, reg: 0x" << hex << reg <<
  //    " val: 0x" << hex << unsigned(data_.data_read[0] & 0xFF) << " =? 0x" << hex << unsigned(val) << "...";

  printf("Si571 assert, reg: 0x%02x val: 0x%02x =? 0x%02x...", reg, data_.data_read[0] & 0xFF, val);

  // compare data read data from I2C with expected value
  assert( (data_.data_read[0] & 0xFF) == val);

  printf("passed!\n");

  //cout << "passed!" << endl;

}
