/* --COPYRIGHT--,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/*
 * ======== hal.c ========
 *
 */
#include "msp430.h"
#include "driverlib.h"
#include "USB_API/USB_Common/device.h"
#include "usbConfig/descriptors.h"
#include "hal.h"
#include "inc/hw_regaccess.h"
#include "inc/hw_memmap.h"

#define GPIO_ALL	GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3| \
					GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7

/*
 * This function drives all the I/O's as output-low, to avoid floating inputs
 * (which cause extra power to be consumed).  This setting is compatible with
 * TI FET target boards, the F5529 Launchpad, and F5529 Experimenters Board;  
 * but may not be compatible with custom hardware, which may have components  
 * attached to the I/Os that could be affected by these settings.  So if using
 * other boards, this function may need to be modified.
 */
void initPorts (void)
{
#ifdef __MSP430_HAS_PORT1_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT2_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT3_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT4_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT5_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT6_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT7_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT8_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORT9_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_ALL);
#endif

#ifdef __MSP430_HAS_PORTJ_R__
        GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_ALL);
        GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_ALL);
#endif
}

/* Configures the system clocks:
 * MCLK = SMCLK = DCO/FLL = mclkFreq (expected to be expressed in Hz)
 * ACLK = FLLref = REFO=32kHz
 *
 * XT2 is not configured here.  Instead, the USB API automatically starts XT2
 * when beginning USB communication, and optionally disables it during USB
 * suspend.  It's left running after the USB host is disconnected, at which
 * point you're free to disable it.  You need to configure the XT2 frequency
 * in the Descriptor Tool (currently set to 4MHz in this example).
 * See the Programmer's Guide for more information.
 */
void initClocks (uint32_t mclkFreq)
{
#ifndef DRIVERLIB_LEGACY_MODE

////        TODO wywalić to.
////        UCS_bypassXT2 ();
//
//        UCS_clockSignalInit(
//                UCS_FLLREF,
//                UCS_REFOCLK_SELECT,
//                UCS_CLOCK_DIVIDER_1);
//
//        UCS_clockSignalInit(
//                UCS_ACLK,
//                UCS_REFOCLK_SELECT,
//                UCS_CLOCK_DIVIDER_1);
////
////        UCS_clockSignalInit(
////                UCS_SMCLK,
////                UCS_VLOCLK_SELECT,
////                UCS_CLOCK_DIVIDER_32);
//
//        UCS_initFLLSettle (mclkFreq / 1000, mclkFreq / 32768);

        GPIO_setAsPeripheralModuleFunctionInputPin(
                GPIO_PORT_P5,
                GPIO_PIN2 + GPIO_PIN3
                );

        //Initialize XT2. Returns STATUS_SUCCESS if initializes successfully
        UCS_XT2StartWithTimeout(
                UCS_XT2DRIVE_4MHZ_8MHZ,
                50000
                );


        /*
         * FLL
         * Patrz rysunek po kolei:
         * Ustawienie SELREF, czyli źródla FLL (domyślnie XT1CLK)
         * FLLREFDIV__1    : Reference Divider: f(LFCLK)/1
         * FLLREFDIV__2    : Reference Divider: f(LFCLK)/2
         * FLLREFDIV__4    : Reference Divider: f(LFCLK)/4
         * FLLREFDIV__8    : Reference Divider: f(LFCLK)/8
         * FLLREFDIV__12   : Reference Divider: f(LFCLK)/12
         * FLLREFDIV__16   : Reference Divider: f(LFCLK)/16
         *                 :
         * SELREF__XT1CLK  : Multiply Selected Loop Freq. By XT1CLK (DEFAULT)
         * SELREF__REFOCLK : Multiply Selected Loop Freq. By REFOCLK
         * SELREF__XT2CLK  : Multiply Selected Loop Freq. By XT2CLK
         *
         * Znów operator "=", bo nie ma w tym rejestrze nic innego prócz tych 2 ustawień
         */
        HWREG16 (UCS_BASE | OFS_UCSCTL3) = SELREF__XT2CLK | FLLREFDIV__4;

//        // Zapamiętaj stan rejestru SG0
//        uint16_t srRegisterState = __get_SR_register() & SCG0;

        //Set DCO to lowest Tap
        //HWREG8 (UCS_BASE | OFS_UCSCTL0_H) = 0x0000;

        /*
         * FLLN = 24 oraz FLLD = 0; Mnożnik 6 oznacza 1 * (24 + 1) = 25MHz
         * Pierwsze 1 jest bo FLLREFDIV__4 dzieli 4MHz na 4, czyli 1MHz.
         */
        HWREG16 (UCS_BASE | OFS_UCSCTL2) = (24 & 0x03ff) | 0 << 10;
//        HWREG16 (UCS_BASE | OFS_UCSCTL2) = (0 & 0x03ff) | 6 << 10;

        // Przedział częstotliwości
        HWREG8 (UCS_BASE | OFS_UCSCTL1) = DCORSEL_6;

//        // Re-enable FLL
//         __bic_SR_register(SCG0);

        while (HWREG8(UCS_BASE | OFS_UCSCTL7_L) & DCOFFG)
        {
            //Clear OSC flaut Flags
            HWREG8(UCS_BASE | OFS_UCSCTL7_L) &= ~(DCOFFG);

            //Clear OFIFG fault flag
            HWREG8(SFR_BASE + OFS_SFRIFG1) &= ~OFIFG;
        }

//        // Restore previous SCG0
//        __bis_SR_register(srRegisterState);






/**
 * Ustalamy źródło dla każdego z trzech sygnałów zegarowych:
 *
 * SELM__XT1CLK    : MCLK Source Select XT1CLK
 * SELM__VLOCLK    : MCLK Source Select VLOCLK
 * SELM__REFOCLK   : MCLK Source Select REFOCLK
 * SELM__DCOCLK    : MCLK Source Select DCOCLK
 * SELM__DCOCLKDIV : MCLK Source Select DCOCLKDIV
 * SELM__XT2CLK    : MCLK Source Select XT2CLK
 *
 * SELS__XT1CLK    : SMCLK Source Select XT1CLK
 * SELS__VLOCLK    : SMCLK Source Select VLOCLK
 * SELS__REFOCLK   : SMCLK Source Select REFOCLK
 * SELS__DCOCLK    : SMCLK Source Select DCOCLK
 * SELS__DCOCLKDIV : SMCLK Source Select DCOCLKDIV
 * SELS__XT2CLK    : SMCLK Source Select XT2CLK
 *                 :
 * SELA__XT1CLK    : ACLK Source Select XT1CLK
 * SELA__VLOCLK    : ACLK Source Select VLOCLK
 * SELA__REFOCLK   : ACLK Source Select REFOCLK
 * SELA__DCOCLK    : ACLK Source Select DCOCLK
 * SELA__DCOCLKDIV : ACLK Source Select DCOCLKDIV
 * SELA__XT2CLK    : ACLK Source Select XT2CLK
 *
 * Uwaga! Użyłem operatora =, a nie |=, żeby nadpisać wykluczające się ustawienia!
 */
HWREG16 (UCS_BASE | OFS_UCSCTL4) = SELM__DCOCLK | SELS__DCOCLK | SELA__REFOCLK;








#else

        UCS_clockSignalInit(
                        UCS_BASE,
                        UCS_FLLREF,
                        UCS_REFOCLK_SELECT,
                        UCS_CLOCK_DIVIDER_1);

        UCS_clockSignalInit(
                        UCS_BASE,
                        UCS_ACLK,
                        UCS_REFOCLK_SELECT,
                        UCS_CLOCK_DIVIDER_1);

        UCS_initFLLSettle(
                        UCS_BASE,
                        mclkFreq/1000,
                        mclkFreq/32768);

#endif

}
//Released_Version_4_10_02
