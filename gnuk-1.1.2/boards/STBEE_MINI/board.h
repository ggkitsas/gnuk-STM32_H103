/*
    ChibiOS/RT - Copyright (C) 2006,2007,2008,2009,2010 Giovanni Di Sirio.

    This file is part of ChibiOS/RT.

    ChibiOS/RT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS/RT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

                                      ---

    A special exception to the GPL can be applied should you wish to distribute
    a combined work that includes ChibiOS/RT, without being obliged to provide
    the source code for any proprietary components. See the file exception.txt
    for full details of how and when the exception can be applied.
*/

#ifndef _BOARD_H_
#define _BOARD_H_

#include "config.h"
/*
 * Setup for the STBee Mini board.
 */
#define	SET_USB_CONDITION(en) (en)	/* To connect USB, call palSetPad */
#define	SET_LED_CONDITION(on) (!on)	/* To emit light, call palClearPad */
#define GPIO_USB	GPIOA_USB_ENABLE
#define IOPORT_USB	GPIOA
#define GPIO_LED	GPIOA_LED1
#define IOPORT_LED	GPIOA

#define NEUG_ADC_SETTING2_SMPR1 0
#define NEUG_ADC_SETTING2_SMPR2 ADC_SMPR2_SMP_AN1(ADC_SAMPLE_1P5)    \
                              | ADC_SMPR2_SMP_AN2(ADC_SAMPLE_1P5)
#define NEUG_ADC_SETTING2_SQR3  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN1)      \
                              | ADC_SQR3_SQ2_N(ADC_CHANNEL_IN2)
#define NEUG_ADC_SETTING2_NUM_CHANNELS 2

/*
 * Board identifier.
 */
#define BOARD_STBEE_MINI
#define BOARD_NAME "STBee Mini"

#if defined(PINPAD_CIR_SUPPORT) || defined(PINPAD_DIAL_SUPPORT)
#define HAVE_7SEGLED	1
/*
 * Timer assignment for CIR
 */
#define TIMx	TIM3
#endif

/*
 * Board frequencies.
 */
#define STM32_LSECLK            32768
#define STM32_HSECLK            12000000

/*
 * MCU type, this macro is used by both the ST library and the ChibiOS/RT
 * native STM32 HAL.
 */
#define STM32F10X_MD

/*
 * IO pins assignments.
 */
#define GPIOA_LED1              13
#define GPIOA_USB_ENABLE        14
#define GPIOA_LED2              15

#define GPIOC_BUTTON            13

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 *
 * The digits have the following meaning:
 *   0 - Analog input.
 *   1 - Push Pull output 10MHz.
 *   2 - Push Pull output 2MHz.
 *   3 - Push Pull output 50MHz.
 *   4 - Digital input.
 *   5 - Open Drain output 10MHz.
 *   6 - Open Drain output 2MHz.
 *   7 - Open Drain output 50MHz.
 *   8 - Digital input with PullUp or PullDown resistor depending on ODR.
 *   9 - Alternate Push Pull output 10MHz.
 *   A - Alternate Push Pull output 2MHz.
 *   B - Alternate Push Pull output 50MHz.
 *   C - Reserved.
 *   D - Alternate Open Drain output 10MHz.
 *   E - Alternate Open Drain output 2MHz.
 *   F - Alternate Open Drain output 50MHz.
 * Please refer to the STM32 Reference Manual for details.
 */

#if defined(PINPAD_CIR_SUPPORT) || defined(PINPAD_DIAL_SUPPORT)
/*
 * Port A setup.
 * PA1  - Digital input with PullUp.  AN1 for NeuG
 * PA2  - Digital input with PullUp.  AN2 for NeuG
 * PA6  - (TIM3_CH1) input with pull-up
 * PA7  - (TIM3_CH2) input with pull-down
 * PA11 - input with pull-up (USBDM)
 * PA12 - input with pull-up (USBDP)
 * Everything input with pull-up except:
 * PA13 - Open Drain output (LED1 0:ON 1:OFF)
 * PA14 - Push pull output  (USB ENABLE 0:DISABLE 1:ENABLE)
 * PA15 - Open Drain output (LED2 0:ON 1:OFF)
 */
#define VAL_GPIOACRL            0x88888888      /*  PA7...PA0 */
#define VAL_GPIOACRH            0x63688888      /* PA15...PA8 */
#define VAL_GPIOAODR            0xFFFFFF7F

/* Port B setup. */
#define GPIOB_CIR		0
#define GPIOB_BUTTON            2
#define GPIOB_ROT_A             6
#define GPIOB_ROT_B             7

#define GPIOB_7SEG_DP           15
#define GPIOB_7SEG_A            14
#define GPIOB_7SEG_B            13
#define GPIOB_7SEG_C            12
#define GPIOB_7SEG_D            11
#define GPIOB_7SEG_E            10
#define GPIOB_7SEG_F            9
#define GPIOB_7SEG_G            8

#define VAL_GPIOBCRL            0x88888888      /*  PB7...PB0 */
#define VAL_GPIOBCRH            0x66666666      /* PB15...PB8 */
#define VAL_GPIOBODR            0xFFFFFFFF
#else
/*
 * Port A setup.
 * PA1  - Digital input with PullUp.  AN1 for NeuG
 * PA2  - Digital input with PullUp.  AN2 for NeuG
 * PA11 - input with pull-up (USBDM)
 * PA12 - input with pull-up (USBDP)
 * Everything input with pull-up except:
 * PA13 - Open Drain output (LED1 0:ON 1:OFF)
 * PA14 - Push pull output  (USB ENABLE 0:DISABLE 1:ENABLE)
 * PA15 - Open Drain output (LED2 0:ON 1:OFF)
 */
#define VAL_GPIOACRL            0x88888888      /*  PA7...PA0 */
#define VAL_GPIOACRH            0x63688888      /* PA15...PA8 */
#define VAL_GPIOAODR            0xFFFFFFFF

/* Port B setup. */
/* Everything input with pull-up */
#define VAL_GPIOBCRL            0x88888888      /*  PB7...PB0 */
#define VAL_GPIOBCRH            0x88888888      /* PB15...PB8 */
#define VAL_GPIOBODR            0xFFFFFFFF
#endif

/*
 * Port C setup.
 * Everything input with pull-up except:
 * PC13 - Normal input.
 * PC14 - Normal input.
 * PC15 - Normal input.
 */
#define VAL_GPIOCCRL            0x88888888      /*  PC7...PC0 */
#define VAL_GPIOCCRH            0x44488888      /* PC15...PC8 */
#define VAL_GPIOCODR            0xFFFFFFFF

/*
 * Port D setup.
 * Everything input with pull-up except:
 * PD0  - Normal input (XTAL).
 * PD1  - Normal input (XTAL).
 */
#define VAL_GPIODCRL            0x88888844      /*  PD7...PD0 */
#define VAL_GPIODCRH            0x88888888      /* PD15...PD8 */
#define VAL_GPIODODR            0xFFFFFFFF

/*
 * Port E setup.
 * Everything input with pull-up except:
 */
#define VAL_GPIOECRL            0x88888888      /*  PE7...PE0 */
#define VAL_GPIOECRH            0x88888888      /* PE15...PE8 */
#define VAL_GPIOEODR            0xFFFFFFFF

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* _BOARD_H_ */
