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
 * File: $Id: mb.c, v 1.28 2010/06/06 13:54:40 wolti Exp $
 */
#include <mb.h>

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif

#define ucMBAddress inst->address
#define eMBCurrentMode inst->cur_mode
#define eMBCurrentState inst->cur_state

#define pvMBFrameStartCur               inst->trmt->frm_start
#define peMBFrameSendCur                inst->trmt->frm_send
#define pvMBFrameStopCur                inst->trmt->frm_stop
#define peMBFrameReceiveCur             inst->trmt->frm_rcv

#define pvMBGetRxFrame                  inst->trmt->get_rx_frm
#define pvMBGetTxFrame                  inst->trmt->get_tx_frm

#define pbMBMasterRequestIsBroadcastCur inst->trmt->rq_is_broadcast

#define xFuncHandlers inst->xFuncHandlers

#define usLength inst->len
#define PDUSndLength inst->pdu_snd_len
#define rxFrame inst->rx_frame
#define txFrame inst->tx_frame

#define pvMBFrameCloseCur  (inst->pmt->frm_close)
#define pvPortEventPostCur (inst->pmt->evt_post)
#define pvPortEventGetCur  (inst->pmt->evt_get)

//master variables
#define ucMBMasterDestAddress inst->master_dst_addr
#define xMBRunInMasterMode    inst->master_mode_run
#define eMBMasterCurErrorType inst->master_err_cur

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler defaultFuncHandlers[MB_FUNC_HANDLERS_MAX] =
{
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, (void*)eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, (void*)eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, (void*)eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, (void*)eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, (void*)eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, (void*)eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, (void*)eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, (void*)eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, (void*)eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, (void*)eMBFuncReadDiscreteInputs},
#endif
};

#if MB_MASTER >0
static xMBFunctionHandler xMasterFuncHandlers[MB_FUNC_HANDLERS_MAX] =
{
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    //TODO Add Master function define
    {MB_FUNC_OTHER_REPORT_SLAVEID, (void*)eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, (void*)eMBMasterFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, (void*)eMBMasterFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, (void*)eMBMasterFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, (void*)eMBMasterFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, (void*)eMBMasterFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, (void*)eMBMasterFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, (void*)eMBMasterFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, (void*)eMBMasterFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, (void*)eMBMasterFuncReadDiscreteInputs},
#endif
};
#endif

/* ----------------------- Start implementation -----------------------------*/
#if MB_RTU_ENABLED > 0
mb_err_enum
eMBInitRTU(mb_instance* inst, mb_rtu_tr* transport, UCHAR ucSlaveAddress, mb_port_base * port_obj, ULONG ulBaudRate, eMBParity eParity)
{
    return eMBInit(inst, (void*)transport, MB_RTU, FALSE, ucSlaveAddress, port_obj, ulBaudRate, eParity);
}
#endif

#if MB_ASCII_ENABLED > 0
mb_err_enum
eMBInitASCII(mb_instance* inst, mb_ascii_tr* transport, UCHAR ucSlaveAddress, mb_port_base * port_obj, ULONG ulBaudRate, eMBParity eParity)
{

    return eMBInit(inst, (void*)transport, MB_ASCII, FALSE, ucSlaveAddress, port_obj, ulBaudRate, eParity);
}

#endif


#if MB_MASTER >0

#if MB_RTU_ENABLED > 0
mb_err_enum
eMBMasterInitRTU(mb_instance* inst, mb_rtu_tr* transport, mb_port_base * port_obj, ULONG ulBaudRate, eMBParity eParity)
{
    return eMBInit(inst, (void*)transport, MB_RTU, TRUE, 0, port_obj, ulBaudRate, eParity);
}
#endif

#if MB_ASCII_ENABLED > 0
mb_err_enum
eMBMasterInitASCII(mb_instance* inst, mb_ascii_tr* transport, mb_port_base * port_obj, ULONG ulBaudRate, eMBParity eParity)
{
    return eMBInit(inst, (void*)transport, MB_ASCII, TRUE, 0, port_obj, ulBaudRate, eParity);
}

#endif

#if MB_TCP_ENABLED > 0
mb_err_enum eMBMasterInitTCP(mb_instance* inst, mb_tcp_tr* transport, USHORT ucTCPPort, SOCKADDR_IN hostaddr)
{
    return eMBTCPInit(inst, transport, ucTCPPort, hostaddr, TRUE);
}

#endif

#endif //MASTER

#if MB_RTU_ENABLED || MB_ASCII_ENABLED
mb_err_enum
eMBInit(mb_instance *inst, mb_trans_union *transport, mb_mode_enum eMode, BOOL is_master, UCHAR ucSlaveAddress, mb_port_base * port_obj, ULONG ulBaudRate, eMBParity eParity)
{
    eMBCurrentState = STATE_NOT_INITIALIZED;
    mb_err_enum    eStatus = MB_ENOERR;

    inst->transport          = (mb_trans_base *)transport;
    inst->port               = port_obj;
    transport->base.port_obj = port_obj;

    switch (eMode)
    {
#if MB_RTU_ENABLED > 0
    case MB_RTU:
    {
        inst->trmt = (mb_tr_mtab *)&mb_rtu_mtab;
        PDUSndLength = &(transport->rtu.snd_pdu_len);
        eStatus = eMBRTUInit((mb_rtu_tr*)transport, is_master, ucMBAddress, ulBaudRate, eParity);
        break;
    }
#endif //RTU
#if MB_ASCII_ENABLED > 0
    case MB_ASCII:
    {
        inst->trmt = (mb_tr_mtab *)&mb_ascii_mtab;
        PDUSndLength = &(transport->ascii.snd_pdu_len);
        eStatus = eMBASCIIInit((mb_ascii_tr*)transport, is_master, ucMBAddress, ulBaudRate, eParity);
        break;
    }
#endif//ASCII
    default:
        eStatus = MB_EINVAL;
    }

#if (MB_PORT_HAS_CLOSE > 0)
#   define MB_SERIAL_CLOSE vMBPortClose
#else
#   define MB_SERIAL_CLOSE NULL
#endif // MB_PORT_HAS_CLOSE
    static const mb_port_mtab mb_serial_mtab =
    {
        .frm_close = (pvMBFrameClose) MB_SERIAL_CLOSE,
        .evt_post  = (pvPortEventPost)xMBPortEventPost,
        .evt_get   = (pvPortEventGet) xMBPortEventGet
    };
    inst->pmt = (mb_port_mtab *)&mb_serial_mtab;

    pvMBGetTxFrame(inst->transport, (void*)&(txFrame));      //Можно было прописать сразу.

#if MB_MASTER > 0
    if (is_master == TRUE)
    {
        inst->master_is_busy = FALSE;
        xMBRunInMasterMode   = TRUE;
        xFuncHandlers        = xMasterFuncHandlers;
    }
    else
#endif //MB_MASTER
    {
        xFuncHandlers = defaultFuncHandlers;
    }

    /* check preconditions */
    if (((ucSlaveAddress == MB_ADDRESS_BROADCAST) ||
            (ucSlaveAddress < MB_ADDRESS_MIN) || (ucSlaveAddress > MB_ADDRESS_MAX))&& (is_master == FALSE))
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        ucMBAddress = ucSlaveAddress;



        if (eStatus == MB_ENOERR)
        {
            if (!xMBPortEventInit((mb_port_ser*)inst->port))
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
                eMBCurrentMode = eMode;
                eMBCurrentState = STATE_DISABLED;
            }
        }
    }

    //if(isMaster == TRUE)
    //vMBMasterOsResInit(); //FIXME: what is it?

    return eStatus;
}
#endif

#if MB_TCP_ENABLED > 0
mb_err_enum
eMBTCPInit(mb_instance* inst, mb_tcp_tr* transport, USHORT ucTCPPort, SOCKADDR_IN hostaddr, BOOL bMaster)
{
    mb_err_enum    eStatus = MB_ENOERR;

    inst->transport = transport;
    transport->parent = (void*)(inst);
    int i;

    if (transport->tcpMaster ==  TRUE)
    {
        inst->xMBRunInMasterMode = TRUE;
        xFuncHandlers = xMasterFuncHandlers;
    }
    else
    {
        xFuncHandlers = defaultFuncHandlers;
    }

    if ((eStatus = eMBTCPDoInit(transport, ucTCPPort, hostaddr, bMaster)) != MB_ENOERR)
    {
        inst->eMBCurrentState = STATE_DISABLED;
    }
    else if (!xMBPortEventInit(&(transport->tcp_port)))
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        //TODO: place const tu mb_ascii.c
        inst->trmt = (mb_tr_mtab *)&mb_tcp_mtab;

        inst->ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        inst->cur_mode = MB_TCP;
        inst->eMBCurrentState = STATE_DISABLED;
        inst->pdu_snd_len = &(transport->tcp.usSendPDULength);
        //inst->port = (void*) &(((mb_tcp_tr*)transport)->tcp_port);

        inst->trmt->get_rx_frm(inst->transport, &inst->rxFrame);
        inst->trmt->get_tx_frm(inst->transport, &inst->txFrame);//Зачем 2 раза???

#if (MB_PORT_HAS_CLOSE > 0)
#   define MB_TCP_CLOSE vMBPortClose
#else
#   define MB_TCP_CLOSE NULL
#endif // MB_PORT_HAS_CLOSE
        static const mb_port_mtab mb_tcp_mtab =
        {
            .frm_close = (pvMBFrameClose) MB_TCP_CLOSE,
            .evt_post  = (pvPortEventPost)xMBPortEventPost,
            .evt_get   = (pvPortEventGet) xMBPortEventGet
        };
        inst->pmt = (mb_port_mtab *)&mb_tcp_mtab;
    }
    return eStatus;
}
#endif

mb_err_enum
eMBClose(mb_instance* inst)
{
    mb_err_enum    eStatus = MB_ENOERR;

    if (eMBCurrentState == STATE_DISABLED)
    {
        if (pvMBFrameCloseCur != NULL)
        {
            pvMBFrameCloseCur((mb_port_base *)inst->transport);
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

mb_err_enum
eMBEnable(mb_instance* inst)
{
    mb_err_enum    eStatus = MB_ENOERR;

    if (eMBCurrentState == STATE_DISABLED)
    {
        /* Activate the protocol stack. */
        pvMBFrameStartCur(inst->transport);
        eMBCurrentState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

mb_err_enum
eMBDisable(mb_instance* inst)
{
    mb_err_enum    eStatus;

    if (eMBCurrentState == STATE_ENABLED)
    {
        pvMBFrameStopCur(inst->transport);
        eMBCurrentState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if (eMBCurrentState == STATE_DISABLED)
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

mb_err_enum
eMBPoll(mb_instance* inst)
{
    static UCHAR    ucRcvAddress;
    static UCHAR    ucFunctionCode;
    static eMBException eException;
#if MB_MASTER > 0
    //eMBMasterErrorEventType errorType;
#endif
    int             i, j;
    mb_err_enum    eStatus = MB_ENOERR;
    mb_event_enum    eEvent;

    /* Check if the protocol stack is ready. */
    if (eMBCurrentState != STATE_ENABLED)
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    BOOL gotEvent;

    gotEvent = pvPortEventGetCur(inst->port, inst, &eEvent);

    if (gotEvent  == TRUE)
    {
        switch (eEvent)
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = peMBFrameReceiveCur(inst->transport, &ucRcvAddress, (UCHAR**) &rxFrame, (USHORT*)&usLength);
            if (eStatus == MB_ENOERR)
            {
#if MB_MASTER > 0
                if (xMBRunInMasterMode == TRUE)
                {
                    if (ucRcvAddress == ucMBMasterDestAddress || eMBCurrentMode== MB_TCP) //All addresses work in tcp mode
                    {
                        (void)pvPortEventPostCur(inst->port, EV_EXECUTE);
                    }
                }
                else
#endif // MB_MASTER
                {
                    /* Check if the frame is for us. If not ignore the frame. */
                    if ((ucRcvAddress == ucMBAddress) || (ucRcvAddress == MB_ADDRESS_BROADCAST))
                    {
                        (void)pvPortEventPostCur(inst->port, EV_EXECUTE);
                    }
                }

            }
            else
            {
#if MB_MASTER > 0
                if (xMBRunInMasterMode)
                {
//                    eMBMasterCurErrorType = ERR_EV_ERROR_RECEIVE_DATA;
//                    (void)pvPortEventPostCur(inst->port, EV_ERROR_PROCESS);
                    (void)pvPortEventPostCur(inst->port, EV_MASTER_ERROR_RECEIVE_DATA);
                }
#endif // MB_MASTER
            }
            break;

        case EV_EXECUTE:
        {
            ucFunctionCode = rxFrame[MB_PDU_FUNC_OFF];
            eException = MB_EX_ILLEGAL_FUNCTION;

#if MB_MASTER > 0
            if ((ucFunctionCode >> 7) && xMBRunInMasterMode)
            {
                eException = (eMBException)rxFrame[MB_PDU_DATA_OFF];
            }
            else
#endif // MB_MASTER
            {
                for (i = 0; i < MB_FUNC_HANDLERS_MAX; i++)
                {
                    /* No more function handlers registered. Abort. */
                    if (xFuncHandlers[i].ucFunctionCode == 0)
                    {
                        break;
                    }
                    else if (xFuncHandlers[i].ucFunctionCode == ucFunctionCode)
                    {
#if MB_MASTER > 0
                        if (xMBRunInMasterMode == FALSE)
#endif// MB_MASTER
                        {
                            for (j=0; j<usLength; j++)
                            {
                                txFrame[j]=rxFrame[j];
                            }
                            eException = xFuncHandlers[i].pxHandler(inst, (UCHAR*)(txFrame), (USHORT*)&usLength);
                        }
#if MB_MASTER > 0
                        else
                        {
                            eException = xFuncHandlers[i].pxHandler(inst, (UCHAR*)(rxFrame), (USHORT*)&usLength);
                        }
#endif// MB_MASTER
                        break;
                    }
                }
            }

            /* If the request was not sent to the broadcast address we
             * return a reply. */
#if MB_MASTER > 0
            if (xMBRunInMasterMode)
            {
                if (eException != MB_EX_NONE)
                {
//                    eMBMasterCurErrorType =  ERR_EV_ERROR_EXECUTE_FUNCTION;
//                    (void)pvPortEventPostCur(inst->port, EV_ERROR_PROCESS);
                    (void)pvPortEventPostCur(inst->port, EV_MASTER_ERROR_EXECUTE_FUNCTION);
                }
                else
                {
                    vMBMasterCBRequestSuccess(inst);
                    inst->master_is_busy = FALSE;
                }
            }
            else
#endif
            {
                if (ucRcvAddress != MB_ADDRESS_BROADCAST)
                {
                    if (eException != MB_EX_NONE)
                    {
                        /* An exception occured. Build an error frame. */
                        usLength = 0;
                        txFrame[usLength++] = (UCHAR)(ucFunctionCode | MB_FUNC_ERROR);
                        txFrame[usLength++] = eException;
                    }
                    if ((eMBCurrentMode == MB_ASCII) && MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS)
                    {
                        vMBPortTimersDelay((mb_port_ser *)inst->port, MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS);///WTF?????????
                    }
                    eStatus = peMBFrameSendCur(inst->transport, ucMBAddress, (UCHAR*)(txFrame), usLength);///Where eStatus is used?
                }
            }
        }
        break;

        case EV_FRAME_SENT:
#if MB_MASTER > 0
            if (xMBRunInMasterMode)
            {
                if (FALSE == inst->master_is_busy)
                {
                    pvMBGetTxFrame(inst->transport, (UCHAR**)(&txFrame));
                    eStatus = peMBFrameSendCur(inst->transport, ucMBMasterDestAddress, (UCHAR*)(txFrame), *PDUSndLength);
                    if (MB_ENOERR == eStatus)
                    {
                        inst->master_is_busy = TRUE; /* Master is busy now. */
                    }
                    else
                    {
                        return MB_EIO;
                    }
                }
            }
#endif
            break;

//        case EV_ERROR_PROCESS:
#if MB_MASTER > 0
        case EV_MASTER_ERROR_RESPOND_TIMEOUT:
        {
            vMBMasterErrorCBRespondTimeout(inst, ucMBMasterDestAddress, (const UCHAR*)txFrame, *PDUSndLength);
            inst->master_is_busy = FALSE;
            //eStatus = MB_ETIMEDOUT;
        }
        break;
        case EV_MASTER_ERROR_RECEIVE_DATA:
        {
            vMBMasterErrorCBReceiveData(inst, ucMBMasterDestAddress, (const UCHAR*)txFrame, *PDUSndLength);
            inst->master_is_busy = FALSE;
            //eStatus = MB_EIO;
        }
        break;
        case EV_MASTER_ERROR_EXECUTE_FUNCTION:
        {
            vMBMasterErrorCBExecuteFunction(inst, ucMBMasterDestAddress, (const UCHAR*)txFrame, *PDUSndLength);
            inst->master_is_busy = FALSE;
            //eStatus = MB_EILLFUNC;
        }
        break;
#endif
        }
    }
    return MB_ENOERR;//eStatus;
}

