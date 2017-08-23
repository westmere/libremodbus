/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mbfunccoils_m.c, v 1.60 2013/10/12 15:10:12 Armink Add Master Functions
 */
#include <mb.h>
#include <mb_master.h>

/* ----------------------- Defines ------------------------------------------*/
#define MB_PDU_REQ_READ_ADDR_OFF            (MB_PDU_DATA_OFF + 0)
#define MB_PDU_REQ_READ_COILCNT_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_READ_SIZE                (4)
#define MB_PDU_FUNC_READ_COILCNT_OFF        (MB_PDU_DATA_OFF + 0)
#define MB_PDU_FUNC_READ_VALUES_OFF         (MB_PDU_DATA_OFF + 1)
#define MB_PDU_FUNC_READ_SIZE_MIN           (1)

#define MB_PDU_REQ_WRITE_ADDR_OFF           (MB_PDU_DATA_OFF)
#define MB_PDU_REQ_WRITE_VALUE_OFF          (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_SIZE               (4)
#define MB_PDU_FUNC_WRITE_ADDR_OFF          (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_VALUE_OFF         (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_SIZE              (4)

#define MB_PDU_REQ_WRITE_MUL_ADDR_OFF       (MB_PDU_DATA_OFF)
#define MB_PDU_REQ_WRITE_MUL_COILCNT_OFF    (MB_PDU_DATA_OFF + 2)
#define MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF    (MB_PDU_DATA_OFF + 4)
#define MB_PDU_REQ_WRITE_MUL_VALUES_OFF     (MB_PDU_DATA_OFF + 5)
#define MB_PDU_REQ_WRITE_MUL_SIZE_MIN       (5)
#define MB_PDU_REQ_WRITE_MUL_COILCNT_MAX    (0x07B0)
#define MB_PDU_FUNC_WRITE_MUL_ADDR_OFF      (MB_PDU_DATA_OFF)
#define MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF   (MB_PDU_DATA_OFF + 2)
#define MB_PDU_FUNC_WRITE_MUL_SIZE          (5)

/* ----------------------- Static functions ---------------------------------*/
eMBException    prveMBError2Exception(mb_err_enum eErrorCode);

/* ----------------------- Start implementation -----------------------------*/
#if MB_MASTER > 0
#if MB_FUNC_READ_COILS_ENABLED > 0

/**
 * This function will request read coil.
 *
 * @param ucSndAddr salve address
 * @param usCoilAddr coil start address
 * @param usNCoils coil total number
 * @param lTimeOut timeout (-1 will waiting forever)
 *
 * @return error code
 */
mb_err_enum
eMBMasterReqReadCoils(mb_instance* inst, UCHAR ucSndAddr, USHORT usCoilAddr, USHORT usNCoils , LONG lTimeOut)
{
    UCHAR                 *ucMBFrame;
    //mb_err_enum    eErrStatus = MB_ENOERR;

    if (ucSndAddr > MB_ADDRESS_MAX)
    {
        return  MB_EINVAL;
    }
    if (inst->master_is_busy)
    {
        return MB_EBUSY;
    }
    //else if (xMBMasterRunResTake(lTimeOut) == FALSE) eErrStatus = MB_EBUSY; //FIXME
    inst->trmt->get_tx_frm(inst->transport, &ucMBFrame);
    inst->master_dst_addr = ucSndAddr;
    ucMBFrame[MB_PDU_FUNC_OFF]                 = MB_FUNC_READ_COILS;
    ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF]        = usCoilAddr >> 8;
    ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]    = usCoilAddr;
    ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF ]    = usNCoils >> 8;
    ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF + 1] = usNCoils;
    *(inst->pdu_snd_len) = (MB_PDU_SIZE_MIN + MB_PDU_REQ_READ_SIZE);
    (void)inst->pmt->evt_post(inst->port, EV_FRAME_SENT);
    //eErrStatus = eMBMasterWaitRequestFinish();
    return MB_ENOERR;
}

eMBException
eMBMasterFuncReadCoils(mb_instance* inst, UCHAR * pucFrame, USHORT * usLen)
{
    UCHAR          *ucMBFrame;
    USHORT          usRegAddress;
    USHORT          usCoilCount;
    UCHAR           ucByteCount;

    eMBException    eStatus = MB_EX_NONE;
    mb_err_enum    eRegStatus;

    /* If this request is broadcast, and it's read mode. This request don't need execute. */
    if (inst->trmt->rq_is_broadcast(inst->transport))
    {
        eStatus = MB_EX_NONE;
    }
    else if (*usLen >= MB_PDU_SIZE_MIN + MB_PDU_FUNC_READ_SIZE_MIN)
    {
        inst->trmt->get_tx_frm(inst->transport, &ucMBFrame);
        usRegAddress = (USHORT)(ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF] << 8);
        usRegAddress |= (USHORT)(ucMBFrame[MB_PDU_REQ_READ_ADDR_OFF + 1]);
        usRegAddress++;

        usCoilCount = (USHORT)(ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF] << 8);
        usCoilCount |= (USHORT)(ucMBFrame[MB_PDU_REQ_READ_COILCNT_OFF + 1]);

        /* Test if the quantity of coils is a multiple of 8. If not last
         * byte is only partially field with unused coils set to zero. */
        if ((usCoilCount & 0x0007) != 0)
        {
            ucByteCount = (UCHAR)(usCoilCount / 8 + 1);
        }
        else
        {
            ucByteCount = (UCHAR)(usCoilCount / 8);
        }

        /* Check if the number of registers to read is valid. If not
         * return Modbus illegal data value exception.
         */
        if ((usCoilCount >= 1) &&
                (ucByteCount == pucFrame[MB_PDU_FUNC_READ_COILCNT_OFF]))
        {
            /* Make callback to fill the buffer. */
            eRegStatus = eMBMasterRegCoilsCB(inst, &pucFrame[MB_PDU_FUNC_READ_VALUES_OFF], usRegAddress, usCoilCount);

            /* If an error occured convert it into a Modbus exception. */
            if (eRegStatus != MB_ENOERR)
            {
                eStatus = prveMBError2Exception(eRegStatus);
            }
        }
        else
        {
            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
        }
    }
    else
    {
        /* Can't be a valid read coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}
#endif

#if MB_FUNC_WRITE_COIL_ENABLED > 0

/**
 * This function will request write one coil.
 *
 * @param ucSndAddr salve address
 * @param usCoilAddr coil start address
 * @param usCoilData data to be written
 * @param lTimeOut timeout (-1 will waiting forever)
 *
 * @return error code
 *
 * @see eMBMasterReqWriteMultipleCoils
 */
mb_err_enum
eMBMasterReqWriteCoil(mb_instance* inst, UCHAR ucSndAddr, USHORT usCoilAddr, USHORT usCoilData, LONG lTimeOut)
{
    UCHAR                 *ucMBFrame;
//    mb_err_enum    eErrStatus = MB_ENOERR;

    if (ucSndAddr > MB_ADDRESS_MAX)
    {
        return MB_EINVAL;
    }
    if ((usCoilData != 0xFF00) && (usCoilData != 0x0000))
    {
        return MB_EINVAL;
    }
    if (inst->master_is_busy)
    {
        return MB_EBUSY;
    }
    // else if (xMBMasterRunResTake(lTimeOut) == FALSE) eErrStatus = MB_EBUSY; //FIXME
    inst->trmt->get_tx_frm(inst->transport, &ucMBFrame);
    inst->master_dst_addr = ucSndAddr;
    ucMBFrame[MB_PDU_FUNC_OFF]                = MB_FUNC_WRITE_SINGLE_COIL;
    ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF]      = usCoilAddr >> 8;
    ucMBFrame[MB_PDU_REQ_WRITE_ADDR_OFF + 1]  = usCoilAddr;
    ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF ]    = usCoilData >> 8;
    ucMBFrame[MB_PDU_REQ_WRITE_VALUE_OFF + 1] = usCoilData;
    *(inst->pdu_snd_len) = (MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_SIZE);
    (void)inst->pmt->evt_post(inst->port, EV_FRAME_SENT);
    //eErrStatus = eMBMasterWaitRequestFinish();
    return MB_ENOERR;
}

eMBException
eMBMasterFuncWriteCoil(mb_instance* inst, UCHAR * pucFrame, USHORT * usLen)
{
//    USHORT          usRegAddress;
//    UCHAR           ucBuf[2];

    eMBException    eStatus = MB_EX_NONE;
//    mb_err_enum    eRegStatus;

    if (*usLen == (MB_PDU_FUNC_WRITE_SIZE + MB_PDU_SIZE_MIN))
    {
//        usRegAddress = (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF] << 8);
//        usRegAddress |= (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_ADDR_OFF + 1]);
//        usRegAddress++;
//
//        if ((pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF + 1] == 0x00) &&
//            ((pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF) ||
//              (pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0x00)))
//        {
//            ucBuf[1] = 0;
//            if (pucFrame[MB_PDU_FUNC_WRITE_VALUE_OFF] == 0xFF)
//            {
//                ucBuf[0] = 1;
//            }
//            else
//            {
//                ucBuf[0] = 0;
//            }
//            eRegStatus =
//                eMBMasterRegCoilsCB(inst, &ucBuf[0], usRegAddress, 1, MB_REG_WRITE);
//
//            /* If an error occured convert it into a Modbus exception. */
//            if (eRegStatus != MB_ENOERR)
//            {
//                eStatus = prveMBError2Exception(eRegStatus);
//            }
//        }
//        else
//        {
//            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
//        }
        eStatus = MB_EX_NONE;
    }
    else
    {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}

#endif

#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0

/**
 * This function will request write multiple coils.
 *
 * @param ucSndAddr salve address
 * @param usCoilAddr coil start address
 * @param usNCoils coil total number
 * @param usCoilData data to be written
 * @param lTimeOut timeout (-1 will waiting forever)
 *
 * @return error code
 *
 * @see eMBMasterReqWriteCoil
 */
mb_err_enum
eMBMasterReqWriteMultipleCoils(mb_instance* inst, UCHAR ucSndAddr,
                               USHORT usCoilAddr, USHORT usNCoils, UCHAR * pucDataBuffer, LONG lTimeOut)
{
    UCHAR                 *ucMBFrame;
    USHORT                 usRegIndex = 0;
    UCHAR                  ucByteCount;
//    mb_err_enum    eErrStatus = MB_ENOERR;

    if (ucSndAddr > MB_ADDRESS_MAX)
    {
        return MB_EINVAL;
    }
    if (usNCoils > MB_PDU_REQ_WRITE_MUL_COILCNT_MAX)
    {
        return MB_EINVAL;
    }
    if (inst->master_is_busy)
    {
        return MB_EBUSY;
    }
    //else if (xMBMasterRunResTake(lTimeOut) == FALSE) eErrStatus = MB_EBUSY; //FIXME
    inst->trmt->get_tx_frm(inst->transport, &ucMBFrame);
    inst->master_dst_addr = ucSndAddr;
    ucMBFrame[MB_PDU_FUNC_OFF]                      = MB_FUNC_WRITE_MULTIPLE_COILS;
    ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF]        = usCoilAddr >> 8;
    ucMBFrame[MB_PDU_REQ_WRITE_MUL_ADDR_OFF + 1]    = usCoilAddr;
    ucMBFrame[MB_PDU_REQ_WRITE_MUL_COILCNT_OFF]     = usNCoils >> 8;
    ucMBFrame[MB_PDU_REQ_WRITE_MUL_COILCNT_OFF + 1] = usNCoils ;
    if((usNCoils & 0x0007) != 0)
    {
        ucByteCount = (UCHAR)(usNCoils / 8 + 1);
    }
    else
    {
        ucByteCount = (UCHAR)(usNCoils / 8);
    }
    ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF]     = ucByteCount;
    ucMBFrame += MB_PDU_REQ_WRITE_MUL_VALUES_OFF;
    while(ucByteCount > usRegIndex)
    {
        *ucMBFrame++ = pucDataBuffer[usRegIndex++];
    }
    *(inst->pdu_snd_len) = (MB_PDU_SIZE_MIN + MB_PDU_REQ_WRITE_MUL_SIZE_MIN + ucByteCount);
    (void)inst->pmt->evt_post(inst->port, EV_FRAME_SENT);
    //eErrStatus = eMBMasterWaitRequestFinish();
    return MB_ENOERR;
}

eMBException
eMBMasterFuncWriteMultipleCoils(mb_instance* inst, UCHAR * pucFrame, USHORT * usLen)
{
//    USHORT          usRegAddress;
//    USHORT          usCoilCnt;
//    UCHAR           ucByteCount;
//    UCHAR           ucByteCountVerify;
//    UCHAR          *ucMBFrame;

    eMBException    eStatus = MB_EX_NONE;
//    mb_err_enum    eRegStatus;

    /* If this request is broadcast, the *usLen is not need check. */
    if ((*usLen == MB_PDU_FUNC_WRITE_MUL_SIZE) || inst->trmt->rq_is_broadcast(inst->transport))
    {
//    	inst->trmt->get_tx_frm(inst->transport, &ucMBFrame);
//        usRegAddress = (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF] << 8);
//        usRegAddress |= (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_MUL_ADDR_OFF + 1]);
//        usRegAddress++;
//
//        usCoilCnt = (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF] << 8);
//        usCoilCnt |= (USHORT)(pucFrame[MB_PDU_FUNC_WRITE_MUL_COILCNT_OFF + 1]);
//
//        ucByteCount = ucMBFrame[MB_PDU_REQ_WRITE_MUL_BYTECNT_OFF];
//
//        /* Compute the number of expected bytes in the request. */
//        if ((usCoilCnt & 0x0007) != 0)
//        {
//            ucByteCountVerify = (UCHAR)(usCoilCnt / 8 + 1);
//        }
//        else
//        {
//            ucByteCountVerify = (UCHAR)(usCoilCnt / 8);
//        }
//
//        if ((usCoilCnt >= 1) && (ucByteCountVerify == ucByteCount))
//        {
//            eRegStatus =
//                eMBMasterRegCoilsCB(inst, &ucMBFrame[MB_PDU_REQ_WRITE_MUL_VALUES_OFF],
//                               usRegAddress, usCoilCnt, MB_REG_WRITE);
//
//            /* If an error occured convert it into a Modbus exception. */
//            if (eRegStatus != MB_ENOERR)
//            {
//                eStatus = prveMBError2Exception(eRegStatus);
//            }
//        }
//        else
//        {
//            eStatus = MB_EX_ILLEGAL_DATA_VALUE;
//        }
        eStatus = MB_EX_NONE;
    }
    else
    {
        /* Can't be a valid write coil register request because the length
         * is incorrect. */
        eStatus = MB_EX_ILLEGAL_DATA_VALUE;
    }
    return eStatus;
}

#endif
#endif
