/* --COPYRIGHT--,BSD
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Highly modified by me (iwasz).
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
/** @file UsbHid.c
 *  @brief Contains APIs related to HID (Human Interface Device) device class.
 */
//
//! \cond
//

/* 
 * ======== UsbHid.c ========
 */

#include <stdint.h>
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"
#include "USB_API/USB_Common/defMSP430USB.h"
#include "usbVendor.h"
#include <descriptors.h>
#include <string.h>

//function pointers
extern void *(*USB_TX_memcpy)(void * dest, const void * source, size_t count);
extern void *(*USB_RX_memcpy)(void * dest, const void * source, size_t count);

//Local Macros
#define INTFNUM_OFFSET(X)   (X - HID0_INTFNUM)  //Get the HID offset

extern uint8_t const report_len_input[HID_NUM_INTERFACES];

static struct _HidRead {
    uint8_t *pUserBuffer;                          // holds the current position of user's receiving buffer. If NULL- no receiving operation started
    uint8_t *pCurrentEpPos;                        // current positon to read of received data from curent EP
    uint16_t nBytesToReceive;                      // holds how many bytes was requested by receiveData() to receive
    uint16_t nBytesToReceiveLeft;                  // holds how many bytes is still requested by receiveData() to receive
    uint8_t * pCT1;                                // holds current EPBCTxx register - their MSB indicates NAK condition and the rest holds data count.
    uint8_t * pCT2;                                // holds next EPBCTxx register
    uint8_t * pEP2;                                // holds addr of the next EP buffer
    uint8_t nBytesInEp;                            // how many received bytes still available in current EP
    uint8_t bCurrentBufferXY;                      // indicates which buffer is used by host to transmit data via OUT endpoint
} HidReadCtrl[HID_NUM_INTERFACES];

extern uint16_t wUsbEventMask;

uint8_t hidProtocol[HID_NUM_INTERFACES] = {0};
uint8_t hidIdleRate[HID_NUM_INTERFACES] = {0};

/*----------------------------------------------------------------------------+
 | Global Variables                                                            |
 +----------------------------------------------------------------------------*/

extern __no_init tEDB __data16 tInputEndPointDescriptorBlock[];
extern __no_init tEDB __data16 tOutputEndPointDescriptorBlock[];


void vendorCopyUsbToBuff (uint8_t* pEP, uint8_t* pCT, uint8_t);

/*----------------------------------------------------------------------------+
 | Functions' implementatin                                                    |
 +----------------------------------------------------------------------------*/

//resets internal HID data structure
void vendorResetData ()
{
    int16_t i;

    //indicates which buffer is used by host to transmit data via OUT endpoint3 - X buffer is first
    //HidReadCtrl[intfIndex].bCurrentBufferXY = X_BUFFER;

    memset(&HidReadCtrl, 0, sizeof(HidReadCtrl));
    for (i = 0; i < HID_NUM_INTERFACES; i++){
        hidProtocol[i] = USB_REQ_HID_REPORT_PROTOCOL;
    }
}

//This function copies data from OUT endpoint into user's buffer
//Arguments:
//pEP - pointer to EP to copy from
//pCT - pointer to pCT control reg
//
void vendorCopyUsbToBuff (uint8_t* pEP, uint8_t* pCT, uint8_t intfNum)
{
        uint8_t nCount;

//        //how many byte we can get from one endpoint buffer
//        nCount = (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft > HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp) ?
//                        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp : HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft;

        nCount = HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp;

        USB_RX_memcpy(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer, pEP, nCount);   //copy data from OEPx X or Y buffer
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft -= nCount;
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer += nCount;                     //move buffer pointer
        //to read rest of data next time from this place

//        if (nCount == HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp) {                 //all bytes are copied from receive buffer?
                //switch current buffer
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY = (HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY + 1) & 0x01;
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = 0;

                //clear NAK, EP ready to receive data
                *pCT = 0;
//        } else {
//                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp -= nCount;
//                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos = pEP + nCount;
//        }
}

//
//! \endcond
//

//*****************************************************************************
//
//! Receives \b size Bytes Over HID Interface.
//!
//! \param data is an array to contain the data received.
//! \param size is the number of bytes to be received.
//! \param intfNum is which data interface to receive from.
//!
//! Receives \b size bytes over HID interface \b intfNum into memory starting at
//! address \b data. \b size has no inherent upper limit (beyond being a 16-bit
//! value).
//! 
//! The function may return with \b kUSBHID_receiveStarted, indicating that a
//! receive operation is underway. The operation completes when \b size bytes
//! are received. The application should ensure that the data memory buffer be
//! available during the whole of the receive operation.
//! 
//! The function may also return with \b kUSBHID_receiveCompleted. This means that
//! the receive operation was complete by the time the function returned.
//! 
//! If the bus is not connected when the function is called, the function
//! returns \b kUSBHID_busNotAvailable, and no operation is begun. If \b size is 0,
//! the function returns \b kUSBHID_generalError. If a previous receive operation
//! is already underway for this data interface, the function returns
//! \b kUSBHID_intfBusyError.
//! 
//! USB includes low-level mechanisms that ensure valid transmission of data.
//! 
//! See Sec. 7.2 of \e "Programmer's Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC" for a detailed discussion of
//! receive operations.
//!
//! \return Any of the following:
//! 		- \b kUSBHID_receiveStarted: A receive operation has been
//! 			successfully started.
//! 		- \b kUSBHID_receiveCompleted: The receive operation is already 
//! 			completed.
//! 		- \b kUSBHID_intfBusyError: a previous receive operation is
//! 			underway.
//! 		- \b kUSBHID_ busNotAvailable: the bus is either suspended or
//! 			disconnected.
//! 		- \b kUSBHID_generalError: size was zero, or other error.
//
//*****************************************************************************

uint8_t vendorReceiveData (uint8_t* data, uint16_t size, uint8_t intfNum)
{
    uint8_t nTmp1;
    uint16_t state;
    uint8_t edbIndex;

    edbIndex = stUsbHandle[intfNum].edb_Index;

    if ((size == 0) ||                                                      //read size is 0
        (data == NULL)){
        return (kUSBHID_generalError);
    }

    state = usbDisableOutEndpointInterrupt(edbIndex);

    //atomic operation - disable interrupts

    //do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE)){
    	usbRestoreOutEndpointInterrupt(state);
        return (kUSBHID_busNotAvailable);
    }

    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer != NULL){          //receive process already started
    	usbRestoreOutEndpointInterrupt(state);
        return (kUSBHID_receiveInProgress);
    }

    HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceive = size;            //bytes to receive
    HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft = size;        //left bytes to receive
    HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = data;                //set user receive buffer

    //read rest of data from buffer, if any
    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > 0){
        //copy data from pEP-endpoint into User's buffer
        vendorCopyUsbToBuff(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos,
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1,intfNum);

        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft == 0){ //the Receive opereation is completed
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;        //no more receiving pending
            USBHID_handleReceiveCompleted(intfNum);                         //call event handler in interrupt context
            usbRestoreOutEndpointInterrupt(state);
            return (kUSBHID_receiveCompleted);                              //receive completed
        }

        //check other EP buffer for data - exchange pCT1 with pCT2
        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 == &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX){
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos = (uint8_t*)stUsbHandle[intfNum].oep_Y_Buffer;
        } else {
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos = (uint8_t*)stUsbHandle[intfNum].oep_X_Buffer;
        }

        nTmp1 = *HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1;

        //try read data from second buffer
        if (nTmp1 & EPBCNT_NAK){                                            //if the second buffer has received data?
            nTmp1 = nTmp1 & 0x7f;                                           //clear NAK bit
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = *(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos + 1);  //holds how many valid bytes in the EP buffer

            if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > nTmp1 - 2){
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1 - 2;
            }

            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos += 2;        //here starts user data
            vendorCopyUsbToBuff(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos, HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1,intfNum);
        }

        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft == 0){ //the Receive opereation is completed
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;        //no more receiving pending
            if (wUsbEventMask & kUSB_receiveCompletedEvent){
                USBHID_handleReceiveCompleted(intfNum);                     //call event handler in interrupt context
            }
            usbRestoreOutEndpointInterrupt(state);
            return (kUSBHID_receiveCompleted);                              //receive completed
        }
    } //read rest of data from buffer, if any

    //read 'fresh' data, if available
    nTmp1 = 0;
    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY == X_BUFFER){ //this is current buffer
        if (tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX & EPBCNT_NAK){ //this buffer has a valid data packet
            //this is the active EP buffer
            //pEP1
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos =
                (uint8_t*)stUsbHandle[intfNum].oep_X_Buffer;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 =
                &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX;

            //second EP buffer
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 =
                (uint8_t*)stUsbHandle[intfNum].oep_Y_Buffer;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 =
                &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY;
            nTmp1 = 1;                                                      //indicate that data is available
        }
    } else {                                                                //Y_BUFFER
        if (tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY & EPBCNT_NAK){
            //this is the active EP buffer
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos =
                (uint8_t*)stUsbHandle[intfNum].oep_Y_Buffer;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 =
                &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY;

            //second EP buffer
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 =
                (uint8_t*)stUsbHandle[intfNum].oep_X_Buffer;
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 =
                &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX;
            nTmp1 = 1;                                                      //indicate that data is available
        }
    }

    if (nTmp1){
        //how many byte we can get from one endpoint buffer
        nTmp1 = *HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1;

        if (nTmp1 & EPBCNT_NAK){
            nTmp1 = nTmp1 & 0x7f;                                           //clear NAK bit
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp =
                *(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos + 1);  //holds how many valid bytes in the EP buffer
            if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > nTmp1 - 2){
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1 - 2;
            }
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCurrentEpPos += 2;        //here starts user data
            vendorCopyUsbToBuff(HidReadCtrl[INTFNUM_OFFSET(
                                             intfNum)].pCurrentEpPos,
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1,intfNum);

            nTmp1 = *HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2;
            //try read data from second buffer
            if ((HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft >
                 0) &&                                                      //do we have more data to receive?
                (nTmp1 & EPBCNT_NAK)){                                      //if the second buffer has received data?
                nTmp1 = nTmp1 & 0x7f;                                       //clear NAK bit
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp =
                    *(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 + 1);       //holds how many valid bytes in the EP buffer
                if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > nTmp1 -
                    2){
                    HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1 - 2;
                }
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 += 2;             //here starts user data
                vendorCopyUsbToBuff(HidReadCtrl[INTFNUM_OFFSET(
                                                 intfNum)].pEP2,
                    HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2,intfNum);
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 =
                    HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2;
            }
        }
    }

    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft == 0){     //the Receive opereation is completed
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;            //no more receiving pending
        if (wUsbEventMask & kUSB_receiveCompletedEvent){
            USBHID_handleReceiveCompleted(intfNum);                         //call event handler in interrupt context
        }
        usbRestoreOutEndpointInterrupt(state);
        return (kUSBHID_receiveCompleted);
    }

    //interrupts enable
    usbRestoreOutEndpointInterrupt(state);
    return (kUSBHID_receiveStarted);
}

//
//! \cond
//

//this function is used only by USB interrupt.
//It fills user receiving buffer with received data
int16_t vendorToBufferFromHost (uint8_t intfNum)
{
        uint8_t *pEP1;
        uint8_t nTmp1; // Temporary register value.
        uint8_t bWakeUp = FALSE;                                                   //per default we do not wake up after interrupt
        uint8_t edbIndex;

        edbIndex = stUsbHandle[intfNum].edb_Index;

        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft == 0) {     //do we have somtething to receive?
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;            //no more receiving pending
                return (bWakeUp);
        }

        //No data to receive...
        if (!((tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX | tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY) & 0x80)) {
                return (bWakeUp);
        }


        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY == X_BUFFER){ //X is current buffer
                //this is the active EP buffer
                pEP1 = (uint8_t*)stUsbHandle[intfNum].oep_X_Buffer; // Buffer
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX; // Its register

                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 = (uint8_t*)stUsbHandle[intfNum].oep_Y_Buffer; //second EP buffer
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY; // and its register
        } else {
                //this is the active EP buffer
                pEP1 = (uint8_t*)stUsbHandle[intfNum].oep_Y_Buffer;
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY;

                //second EP buffer
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 = (uint8_t*)stUsbHandle[intfNum].oep_X_Buffer;
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 = &tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX;
        }

        // how many byte we can get from one endpoint buffer
        // Max for interrupt is 64 bytes of payload.
        nTmp1 = *HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1;

        if (nTmp1 & EPBCNT_NAK) { // If true then :  1:Valid packet in buffer
                nTmp1 = nTmp1 & 0x7f;                                                   //clear NAK bit (nTmp1 = nTmp1 & ~EPBCNT_NAK;)
//                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = *(pEP1 + 1);          //holds how many valid bytes in the EP buffer
//
//                if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > nTmp1 - 2) {
//                        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1 - 2;
//                }
//
//                pEP1 += 2;                                                              //here starts user data

                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1;
                vendorCopyUsbToBuff(pEP1, HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1, intfNum);

//                nTmp1 = *HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2;

//                //try read data from second buffer
//                if ((HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft > 0) &&   //do we have more data to send?
//                                (nTmp1 & EPBCNT_NAK)) {                                              //if the second buffer has received data?
//
//                        nTmp1 = nTmp1 & 0x7f;                                               //clear NAK bit
//                        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = *(pEP1 + 1);      //holds how many valid bytes in the EP buffer
//                        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > nTmp1 - 2) {
//                                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = nTmp1 - 2;
//                        }
//
//                        HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 += 2;                     //here starts user data
//                        HidCopyUsbToBuff(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2, HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2, intfNum);
//                        HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT1 = HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2;
//                }
        }

//        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft == 0) {         //the Receive opereation is completed
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;                //no more receiving pending

                if (wUsbEventMask & kUSB_receiveCompletedEvent) {
                        bWakeUp = USBHID_handleReceiveCompleted(intfNum);
                }

                if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp) {                   //Is not read data still available in the EP?
                        if (wUsbEventMask & kUSB_dataReceivedEvent) {
                                bWakeUp = USBHID_handleDataReceived(intfNum);
                        }
                }
//        }

        return (bWakeUp);
}

//helper for USB interrupt handler
int16_t vendorIsReceiveInProgress (uint8_t intfNum)
{
    return (HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer != NULL);
}

//
//! \endcond
//

//*****************************************************************************
//
//! Aborts an Active Recieve Operation on HID Interface.
//!
//! \param size is the number of bytes that were received and are waiting at the
//! 	assigned address.
//! \param intfNum is the data interface for which the receive should be
//! 	aborted.
//!
//! Aborts an active receive operation on HID interface \b intfNum. Returns the
//! number of bytes that were received and transferred to the data location
//! established for this receive operation. The data moved to the buffer up to
//! that time remains valid.
//! 
//! An application may choose to call this function if it decides it no longer
//! wants to receive data from the USB host. It should be noted that if a
//! continuous stream of data is being received from the host, aborting the
//! operation is akin to pressing a "pause" button; the host will be NAK'ed
//! until another receive operation is opened.
//! 
//! See Sec. 7.2 of \e "Programmer's Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC" for a detailed discussion of
//! receive operations.
//!
//! \return \b kUSB_succeed
//
//*****************************************************************************

uint8_t vendorAbortReceive (uint16_t* size, uint8_t intfNum)
{
    uint16_t state;
    uint8_t edbIndex;
	
    edbIndex = stUsbHandle[intfNum].edb_Index;
    state = usbDisableOutEndpointInterrupt(edbIndex);

    *size = 0;                                                                  //set received bytes count to 0

    //is receive operation underway?
    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer){
        //how many bytes are already received?
        *size = HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceive -
                HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft;

        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = 0;
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer = NULL;
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceiveLeft = 0;
    }

    //restore interrupt status
    usbRestoreOutEndpointInterrupt(state);
    return (kUSB_succeed);
}

//*****************************************************************************
//
//! Rejects Data Received from the Host.
//!
//! This function rejects data that has been received from the host, for
//! interface \b intfNum, that does not have an active receive operation underway.
//! It resides in the USB endpoint buffer and blocks further data until a
//! receive operation is opened, or until rejected. When this function is
//! called, the buffer for this interface is purged, and the data lost. This
//! frees the USB path to resume communication.
//! 
//! See Sec. 7.2 of \e "Programmer's Guide: MSP430 USB API Stack for CDC/PHDC/HID/MSC" for a detailed discussion of
//! receive operations.
//!
//! \return \b kUSB_succeed
//
//*****************************************************************************

uint8_t vendorRejectData (uint8_t intfNum)
{
	uint16_t state;
    uint8_t edbIndex;

    edbIndex = stUsbHandle[intfNum].edb_Index;

    state = usbDisableOutEndpointInterrupt(edbIndex);

    //interrupts disable

    //do not access USB memory if suspended (PLL off). It may produce BUS_ERROR
    if (bFunctionSuspended){
    	usbRestoreOutEndpointInterrupt(state);
        return (kUSBHID_busNotAvailable);
    }

    //Is receive operation underway?
    //- do not flush buffers if any operation still active.
    if (!HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer){
        uint8_t tmp1 = tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX &
                    EPBCNT_NAK;
        uint8_t tmp2 = tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY &
                    EPBCNT_NAK;

        if (tmp1 ^ tmp2){                                                       //switch current buffer if any and only ONE of the
                                                                                //buffers is full
            //switch current buffer
            HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY =
                (HidReadCtrl[INTFNUM_OFFSET(intfNum)].bCurrentBufferXY +
                 1) & 0x01;
        }

        tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX = 0;                   //flush buffer X
        tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY = 0;                   //flush buffer Y
        HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp = 0;                    //indicates that no more data available in the EP
    }

    usbRestoreOutEndpointInterrupt(state);
    return (kUSB_succeed);
}

//*****************************************************************************
//
//! Indicates the Status of the HID Interface.
//!
//! \param intfNum is the interface number for which the status is being
//! 	retrieved.
//! \param bytesSent If a send operation is underway, the number of bytes
//! 	that have been transferred to the host is returned in this location. If 
//! 	no send operation is underway, this returns zero.
//! \param bytesReceived If a receive operation is underway, the number of
//! 	bytes that have been transferred to the assigned memory location is
//! 	returned in this location. If no receive operation is underway, this
//! 	returns zero.
//!
//! Indicates the status of the HID interface \b intfNum. If a send operation is
//! active for this interface, the function also returns the number of bytes
//! that have been transmitted to the host. If a receive operation is active for
//! this interface, the function also returns the number of bytes that have been
//! received from the host and are waiting at the assigned address.
//! 
//! Because multiple flags can be returned, the possible values can be masked
//! together - for example, \b kUSBHID_waitingForSend + \b kUSBHID_dataWaiting.
//!
//! \return Any combination of the following:
//! 		- \b kUSBHID_waitingForSend: Indicates that a send operation is open
//! 			on this interface.
//! 		- \b kUSBHID_waitingForReceive: Indicates that a receive operation
//! 			is open on this interface.
//! 		- \b kUSBHID_dataWaiting: Indicates that data has been received from
//! 			the host for this interface, waiting in the USB receive buffers,
//! 			lacking an open receive operation to accept it.
//! 		- \b kUSBHID_busNotAvailable: Indicates that the bus is either
//! 			suspended or disconnected. Any operations that had previously
//! 			been underway are now aborted.
//
//*****************************************************************************

uint8_t vendorIntfStatus (uint8_t intfNum, uint16_t* bytesSent, uint16_t* bytesReceived)
{
    uint8_t ret = 0;
    uint16_t stateIn, stateOut;
    uint8_t edbIndex;

    *bytesSent = 0;
    *bytesReceived = 0;

    edbIndex = stUsbHandle[intfNum].edb_Index;

    stateIn = usbDisableInEndpointInterrupt(edbIndex);
    stateOut = usbDisableOutEndpointInterrupt(edbIndex);

    //Is receive operation underway?
    if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].pUserBuffer != NULL){
        ret |= kUSBHID_waitingForReceive;
        *bytesReceived = HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesToReceive -
                         HidReadCtrl[INTFNUM_OFFSET(intfNum)].
                         nBytesToReceiveLeft;
    } else {                                                                    //not receive operation started
        //do not access USB memory if suspended (PLL off).
        //It may produce BUS_ERROR
        if (!bFunctionSuspended){
            if ((tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX &
                 EPBCNT_NAK)  |                                                 //any of buffers has a valid data packet
                (tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY &
                 EPBCNT_NAK)){
                ret |= kUSBHID_dataWaiting;
            }
        }
    }

    if ((bFunctionSuspended) ||
        (bEnumerationStatus != ENUMERATION_COMPLETE)){
        //if suspended or not enumerated  - report no other tasks pending
        ret = kUSBHID_busNotAvailable;
    }

    //restore interrupt status
    usbRestoreInEndpointInterrupt(stateIn);
    usbRestoreOutEndpointInterrupt(stateOut);

    return (ret);
}

//*****************************************************************************
//
//! Returns the Number of Bytes Waiting in the USB Endpoint Buffer.
//!
//! \param intfNum is the data interface whose buffer is to be checked.
//!
//! Returns the number of bytes waiting in the USB endpoint buffer for
//! \b intfNum. A non-zero value generally means that no receive operation is
//! open by which these bytes can be copied to a user buffer. If the value is
//! non-zero, the application should either open a receive operation so that the
//! data can be moved out of the endpoint buffer, or the data should be rejected
//! (USBHID_rejectData()).
//!
//! \return The number of bytes waiting in this buffer.
//
//*****************************************************************************

uint8_t vendorBytesInUSBBuffer (uint8_t intfNum)
{
        uint8_t bTmp1 = 0;
        uint8_t edbIndex;
        uint16_t state;

        edbIndex = stUsbHandle[intfNum].edb_Index;

        //interrupts disable
        state = usbDisableOutEndpointInterrupt(edbIndex);

        if ((bFunctionSuspended) || (bEnumerationStatus != ENUMERATION_COMPLETE)) {
                //if suspended or not enumerated - report 0 bytes available
                usbRestoreOutEndpointInterrupt(state);
                return (0);
        }

//        if (HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp > 0) {                   //If a RX operation is underway, part of data may
//                                                                                     //was read of the OEP buffer
//                bTmp1 = HidReadCtrl[INTFNUM_OFFSET(intfNum)].nBytesInEp;
//                if (*HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 & EPBCNT_NAK) {           //the next buffer has a valid data packet
//                        bTmp2 = *(HidReadCtrl[INTFNUM_OFFSET(intfNum)].pEP2 + 1);           //holds how many valid bytes in the EP buffer
//                        if (bTmp2 > (*HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 & 0x7F) - 2) {       //check if all data received correctly
//                                bTmp1 += (*HidReadCtrl[INTFNUM_OFFSET(intfNum)].pCT2 & 0x7F) - 2;
//                        } else {
//                                bTmp1 += bTmp2;
//                        }
//                }
//        } else {
                if (tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX & EPBCNT_NAK) {     //this buffer has a valid data packet
//                        bTmp2 = tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX & 0x7F;
//                        bTmp1 = *((uint8_t*) stUsbHandle[intfNum].oep_X_Buffer + 1);
//                        if (bTmp2 - 2 < bTmp1) {                                             //check if the count (second byte) is valid
//                                bTmp1 = bTmp2 - 2;
//                        }

                        bTmp1 = tOutputEndPointDescriptorBlock[edbIndex].bEPBCTX & 0x7F;
                }
//                if (tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY & EPBCNT_NAK) {     //this buffer has a valid data packet
//                        bTmp2 = tOutputEndPointDescriptorBlock[edbIndex].bEPBCTY & 0x7F;
//                        if (bTmp2 - 2 > *((uint8_t*) stUsbHandle[intfNum].oep_Y_Buffer + 1)) {   //check if the count (second byte) is valid
//                                bTmp1 += *((uint8_t*) stUsbHandle[intfNum].oep_Y_Buffer + 1);
//                        } else {
//                                bTmp1 += bTmp2 - 2;
//                        }
//                }
//        }

        //interrupts enable
        usbRestoreOutEndpointInterrupt(state);
        return (bTmp1);
}
