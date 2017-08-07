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
 * File: $Id: mbtcp.h,v 1.2 2006/12/07 22:10:34 wolti Exp $
 */

#ifndef _MB_TCP_H
#define _MB_TCP_H

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif

#include "mb_types.h"

typedef struct
{
    MBTCPPortInstance tcp_port;
    BOOL tcpMaster;
    USHORT usSendPDULength;
    BOOL xFrameIsBroadcast;
} MBTCPInstance;

/* ----------------------- Defines ------------------------------------------*/
#define MB_TCP_PSEUDO_ADDRESS   255

/* ----------------------- Function prototypes ------------------------------*/
eMBErrorCode	eMBTCPDoInit       (MBTCPInstance* inst, USHORT ucTCPPort, SOCKADDR_IN hostaddr, BOOL bMaster        );
void            eMBTCPStart        (MBTCPInstance* inst                                                              );
void            eMBTCPStop         (MBTCPInstance* inst                                                              );
eMBErrorCode    eMBTCPReceive      (MBTCPInstance* inst, UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength);
eMBErrorCode    eMBTCPSend         (MBTCPInstance* inst, UCHAR _unused, const UCHAR * pucFrame, USHORT usLength      );
#if MB_MASTER > 0
void vMBTCPMasterGetPDURcvBuf      (MBTCPInstance* inst, UCHAR ** pucFrame                                           );
void vMBTCPMasterGetPDUSndBuf      (MBTCPInstance* inst, UCHAR ** pucFrame                                           );
BOOL xMBTCPMasterRequestIsBroadcast(MBTCPInstance* inst                                                              );
#endif

#ifdef __cplusplus
PR_END_EXTERN_C
#endif
#endif