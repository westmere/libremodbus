/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id: mbfuncother.c, v 1.8 2006/12/07 22:10:34 wolti Exp $
 */

#include <mb.h>

#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0

/* ----------------------- Static variables ---------------------------------*/

#define ucMBSlaveID inst->slave_id
#define usMBSlaveIDLen inst->slave_id_len

/* ----------------------- Start implementation -----------------------------*/

mb_err_enum
mb_set_slv_id(mb_instance* inst, UCHAR slv_id, BOOL is_running,
               UCHAR const *slv_idstr, USHORT slv_idstr_len)
{
    mb_err_enum    eStatus = MB_ENOERR;

    /* the first byte and second byte in the buffer is reserved for
     * the parameter slv_id and the running flag. The rest of
     * the buffer is available for additional data. */
    if (slv_idstr_len + 2 < MB_FUNC_OTHER_REP_SLAVEID_BUF)
    {
        usMBSlaveIDLen = 0;
        ucMBSlaveID[usMBSlaveIDLen++] = slv_id;
        ucMBSlaveID[usMBSlaveIDLen++] = (UCHAR)(is_running ? 0xFF : 0x00);
        if (slv_idstr_len > 0)
        {
            memcpy(&ucMBSlaveID[usMBSlaveIDLen], slv_idstr,
                    (size_t)slv_idstr_len);
            usMBSlaveIDLen += slv_idstr_len;
        }
    }
    else
    {
        eStatus = MB_ENORES;
    }
    return eStatus;
}

mb_exception_enum
eMBFuncReportSlaveID(mb_instance* inst, UCHAR * frame_ptr, USHORT * usLen)
{
    memcpy(&frame_ptr[MB_PDU_DATA_OFF], &ucMBSlaveID[0], (size_t)usMBSlaveIDLen);
    *usLen = (USHORT)(MB_PDU_DATA_OFF + usMBSlaveIDLen);
    return MB_EX_NONE;
}

#endif
