/*
  Register definitions for slave core: BPM Swap Channels Interface Registers

  * File           : wb_bpm_swap_defs.h
  * Author         : auto-generated by wbgen2 from wb_bpm_swap.wb
  * Created        : Thu Mar 20 23:15:04 2014
  * Standard       : ANSI C

    THIS FILE WAS GENERATED BY wbgen2 FROM SOURCE FILE wb_bpm_swap.wb
    DO NOT HAND-EDIT UNLESS IT'S ABSOLUTELY NECESSARY!

*/

#ifndef __WBGEN2_REGDEFS_WB_BPM_SWAP_WB
#define __WBGEN2_REGDEFS_WB_BPM_SWAP_WB

#include <inttypes.h>

#if defined( __GNUC__)
#define PACKED __attribute__ ((packed))
#else
#error "Unsupported compiler?"
#endif

#ifndef __WBGEN2_MACROS_DEFINED__
#define __WBGEN2_MACROS_DEFINED__
#define WBGEN2_GEN_MASK(offset, size) (((1<<(size))-1) << (offset))
#define WBGEN2_GEN_WRITE(value, offset, size) (((value) & ((1<<(size))-1)) << (offset))
#define WBGEN2_GEN_READ(reg, offset, size) (((reg) >> (offset)) & ((1<<(size))-1))
#define WBGEN2_SIGN_EXTEND(value, bits) (((value) & (1<<bits) ? ~((1<<(bits))-1): 0 ) | (value))
#endif


/* definitions for register: Control Signals */

/* definitions for field: Reset in reg: Control Signals */
#define BPM_SWAP_CTRL_RST                     WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Mode Input 1 in reg: Control Signals */
#define BPM_SWAP_CTRL_MODE1_MASK              WBGEN2_GEN_MASK(1, 2)
#define BPM_SWAP_CTRL_MODE1_SHIFT             1
#define BPM_SWAP_CTRL_MODE1_W(value)          WBGEN2_GEN_WRITE(value, 1, 2)
#define BPM_SWAP_CTRL_MODE1_R(reg)            WBGEN2_GEN_READ(reg, 1, 2)

/* definitions for field: Mode Input 2 in reg: Control Signals */
#define BPM_SWAP_CTRL_MODE2_MASK              WBGEN2_GEN_MASK(3, 2)
#define BPM_SWAP_CTRL_MODE2_SHIFT             3
#define BPM_SWAP_CTRL_MODE2_W(value)          WBGEN2_GEN_WRITE(value, 3, 2)
#define BPM_SWAP_CTRL_MODE2_R(reg)            WBGEN2_GEN_READ(reg, 3, 2)

/* definitions for field: Swap Divisor in reg: Control Signals */
#define BPM_SWAP_CTRL_SWAP_DIV_F_MASK         WBGEN2_GEN_MASK(8, 16)
#define BPM_SWAP_CTRL_SWAP_DIV_F_SHIFT        8
#define BPM_SWAP_CTRL_SWAP_DIV_F_W(value)     WBGEN2_GEN_WRITE(value, 8, 16)
#define BPM_SWAP_CTRL_SWAP_DIV_F_R(reg)       WBGEN2_GEN_READ(reg, 8, 16)

/* definitions for field: Clock Swap Enable in reg: Control Signals */
#define BPM_SWAP_CTRL_CLK_SWAP_EN             WBGEN2_GEN_MASK(24, 1)

/* definitions for register: Delay */

/* definitions for field: Delay1 in reg: Delay */
#define BPM_SWAP_DLY_1_MASK                   WBGEN2_GEN_MASK(0, 16)
#define BPM_SWAP_DLY_1_SHIFT                  0
#define BPM_SWAP_DLY_1_W(value)               WBGEN2_GEN_WRITE(value, 0, 16)
#define BPM_SWAP_DLY_1_R(reg)                 WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Delay2 in reg: Delay */
#define BPM_SWAP_DLY_2_MASK                   WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_DLY_2_SHIFT                  16
#define BPM_SWAP_DLY_2_W(value)               WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_DLY_2_R(reg)                 WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Gain AA and AC */

/* definitions for field: Signal A by Channel A in reg: Gain AA and AC */
#define BPM_SWAP_A_A_MASK                     WBGEN2_GEN_MASK(0, 16)
#define BPM_SWAP_A_A_SHIFT                    0
#define BPM_SWAP_A_A_W(value)                 WBGEN2_GEN_WRITE(value, 0, 16)
#define BPM_SWAP_A_A_R(reg)                   WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Signal A by Channel C in reg: Gain AA and AC */
#define BPM_SWAP_A_C_MASK                     WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_A_C_SHIFT                    16
#define BPM_SWAP_A_C_W(value)                 WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_A_C_R(reg)                   WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Gain BB and BD */

/* definitions for field: Signal B by Channel B in reg: Gain BB and BD */
#define BPM_SWAP_B_B_MASK                     WBGEN2_GEN_MASK(0, 16)
#define BPM_SWAP_B_B_SHIFT                    0
#define BPM_SWAP_B_B_W(value)                 WBGEN2_GEN_WRITE(value, 0, 16)
#define BPM_SWAP_B_B_R(reg)                   WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Signal B by Channel D in reg: Gain BB and BD */
#define BPM_SWAP_B_D_MASK                     WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_B_D_SHIFT                    16
#define BPM_SWAP_B_D_W(value)                 WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_B_D_R(reg)                   WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Gain CC and CA */

/* definitions for field: Signal C by Channel C in reg: Gain CC and CA */
#define BPM_SWAP_C_C_MASK                     WBGEN2_GEN_MASK(0, 16)
#define BPM_SWAP_C_C_SHIFT                    0
#define BPM_SWAP_C_C_W(value)                 WBGEN2_GEN_WRITE(value, 0, 16)
#define BPM_SWAP_C_C_R(reg)                   WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Signal C by Channel A in reg: Gain CC and CA */
#define BPM_SWAP_C_A_MASK                     WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_C_A_SHIFT                    16
#define BPM_SWAP_C_A_W(value)                 WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_C_A_R(reg)                   WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Gain DD and DB */

/* definitions for field: Signal D by Channel D in reg: Gain DD and DB */
#define BPM_SWAP_D_D_MASK                     WBGEN2_GEN_MASK(0, 16)
#define BPM_SWAP_D_D_SHIFT                    0
#define BPM_SWAP_D_D_W(value)                 WBGEN2_GEN_WRITE(value, 0, 16)
#define BPM_SWAP_D_D_R(reg)                   WBGEN2_GEN_READ(reg, 0, 16)

/* definitions for field: Signal D by Channel B in reg: Gain DD and DB */
#define BPM_SWAP_D_B_MASK                     WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_D_B_SHIFT                    16
#define BPM_SWAP_D_B_W(value)                 WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_D_B_R(reg)                   WBGEN2_GEN_READ(reg, 16, 16)

/* definitions for register: Windowing Control */

/* definitions for field: Windowing Selection in reg: Windowing Control */
#define BPM_SWAP_WDW_CTL_USE                  WBGEN2_GEN_MASK(0, 1)

/* definitions for field: Use Windowing Swithing clock in reg: Windowing Control */
#define BPM_SWAP_WDW_CTL_SWCLK_EXT            WBGEN2_GEN_MASK(1, 1)

/* definitions for field: Reset Windowing module in reg: Windowing Control */
#define BPM_SWAP_WDW_CTL_RST_WDW              WBGEN2_GEN_MASK(2, 1)

/* definitions for field: Reserved in reg: Windowing Control */
#define BPM_SWAP_WDW_CTL_RESERVED_MASK        WBGEN2_GEN_MASK(3, 13)
#define BPM_SWAP_WDW_CTL_RESERVED_SHIFT       3
#define BPM_SWAP_WDW_CTL_RESERVED_W(value)    WBGEN2_GEN_WRITE(value, 3, 13)
#define BPM_SWAP_WDW_CTL_RESERVED_R(reg)      WBGEN2_GEN_READ(reg, 3, 13)

/* definitions for field: Windowing Delay in reg: Windowing Control */
#define BPM_SWAP_WDW_CTL_DLY_MASK             WBGEN2_GEN_MASK(16, 16)
#define BPM_SWAP_WDW_CTL_DLY_SHIFT            16
#define BPM_SWAP_WDW_CTL_DLY_W(value)         WBGEN2_GEN_WRITE(value, 16, 16)
#define BPM_SWAP_WDW_CTL_DLY_R(reg)           WBGEN2_GEN_READ(reg, 16, 16)

/* [0x0]: REG Control Signals */
#define BPM_SWAP_REG_CTRL (0x00000000 << WB_GR_SHIFT)
/* [0x4]: REG Delay */
#define BPM_SWAP_REG_DLY (0x00000001 << WB_GR_SHIFT)
/* [0x8]: REG Gain AA and AC */
#define BPM_SWAP_REG_A (0x00000002 << WB_GR_SHIFT)
/* [0xc]: REG Gain BB and BD */
#define BPM_SWAP_REG_B (0x00000003 << WB_GR_SHIFT)
/* [0x10]: REG Gain CC and CA */
#define BPM_SWAP_REG_C (0x00000004 << WB_GR_SHIFT)
/* [0x14]: REG Gain DD and DB */
#define BPM_SWAP_REG_D (0x00000005 << WB_GR_SHIFT)
/* [0x18]: REG Windowing Control */
#define BPM_SWAP_REG_WDW_CTL (0x00000006 << WB_GR_SHIFT)

///* [0x0]: REG Control Signals */
//#define BPM_SWAP_REG_CTRL 0x00000000
///* [0x4]: REG Delay */
//#define BPM_SWAP_REG_DLY 0x00000004
///* [0x8]: REG Gain AA and AC */
//#define BPM_SWAP_REG_A 0x00000008
///* [0xc]: REG Gain BB and BD */
//#define BPM_SWAP_REG_B 0x0000000c
///* [0x10]: REG Gain CC and CA */
//#define BPM_SWAP_REG_C 0x00000010
///* [0x14]: REG Gain DD and DB */
//#define BPM_SWAP_REG_D 0x00000014
///* [0x18]: REG Windowing Control */
//#define BPM_SWAP_REG_WDW_CTL 0x00000018
#endif
