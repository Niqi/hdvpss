/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


#include "scLink_priv.h"


#pragma DATA_ALIGN(gScLink_tskStack, 32)
#pragma DATA_SECTION(gScLink_tskStack, ".bss:taskStackSection")
UInt8 gScLink_tskStack[SC_LINK_OBJ_MAX][SCALAR_LINK_TSK_STACK_SIZE];

ScLink_Obj gScLink_obj[SC_LINK_OBJ_MAX];


Void ScLink_tskMain(struct VpsUtils_TskHndl *pTsk, VpsUtils_MsgHndl *pMsg)
{
    UInt32 cmd = VpsUtils_msgGetCmd(pMsg);
    Bool ackMsg, done;
    Int32 status;
    ScLink_Obj *pObj;

    pObj = (ScLink_Obj *)pTsk->appData;

    if(cmd!=SYSTEM_CMD_CREATE)
    {
        VpsUtils_tskAckOrFreeMsg(pMsg, FVID2_EFAIL);
        return;
    }

    status = ScLink_drvCreate(pObj, VpsUtils_msgGetPrm(pMsg));
    GT_assert( GT_DEFAULT_MASK, status==FVID2_SOK);
    VpsUtils_tskAckOrFreeMsg(pMsg, status);

    done = FALSE;
    ackMsg = FALSE;

    while(!done)
    {
        status = VpsUtils_tskRecvMsg(pTsk, &pMsg, BIOS_WAIT_FOREVER);
        if(status!=FVID2_SOK)
            break;

        cmd = VpsUtils_msgGetCmd(pMsg);

        switch(cmd)
        {
            case SYSTEM_CMD_NEW_DATA:
                VpsUtils_tskAckOrFreeMsg(pMsg, status);

                ScLink_drvProcessData(pObj);
                break;

            case SC_LINK_CMD_GET_PROCESSED_DATA:
                VpsUtils_tskAckOrFreeMsg(pMsg, status);

                ScLink_drvGetProcessedData(pObj);
                break;

            case SYSTEM_CMD_STOP:
                ScLink_drvStop(pObj);
                VpsUtils_tskAckOrFreeMsg(pMsg, status);
                break;

            case SYSTEM_CMD_DELETE:
                ScLink_drvStop(pObj);
                done = TRUE;
                ackMsg = TRUE;
                break;

            default:
                VpsUtils_tskAckOrFreeMsg(pMsg, status);
                break;
        }
    }

    ScLink_drvDelete(pObj);

    if(ackMsg && pMsg!=NULL)
        VpsUtils_tskAckOrFreeMsg(pMsg, status);

    return;
}

Int32 ScLink_init()
{
    Int32 status;
    System_LinkObj linkObj;
    ScLink_Obj *pObj;
    char name[32];
    UInt32 objId;

    for(objId=0; objId<SC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gScLink_obj[objId];

        memset(pObj, 0, sizeof(*pObj));

        pObj->linkId = SYSTEM_LINK_ID_SCALAR_0 + objId;

        linkObj.pTsk = &pObj->tsk;
        linkObj.linkGetFullFrames  = ScLink_getFullFrames;
        linkObj.linkPutEmptyFrames = ScLink_putEmptyFrames;
        linkObj.getLinkInfo = ScLink_getInfo;

        sprintf(name, "Sc%d   ", objId);

        System_registerLink(pObj->linkId, &linkObj);

        status = VpsUtils_tskCreate(
                &pObj->tsk,
                ScLink_tskMain,
                SCALAR_LINK_TSK_PRI,
                gScLink_tskStack[objId],
                SCALAR_LINK_TSK_STACK_SIZE,
                pObj,
                name
                );
        GT_assert( GT_DEFAULT_MASK, status==FVID2_SOK);
    }

    return status;
}


Int32 ScLink_deInit()
{
    UInt32 objId;
    ScLink_Obj *pObj;

    for(objId=0; objId<SC_LINK_OBJ_MAX; objId++)
    {
        pObj = &gScLink_obj[objId];

        VpsUtils_tskDelete(&pObj->tsk);
    }

    return FVID2_SOK;
}

System_LinkInfo *ScLink_getInfo(VpsUtils_TskHndl *pTsk)
{
    ScLink_Obj *pObj = (ScLink_Obj *)pTsk->appData;

    return &pObj->info;
}

Int32 ScLink_getFullFrames(VpsUtils_TskHndl *pTsk, UInt16 queId, FVID2_FrameList *pFrameList)
{
    ScLink_Obj *pObj = (ScLink_Obj *)pTsk->appData;

    GT_assert( GT_DEFAULT_MASK, queId < SC_LINK_MAX_OUT_QUE);

    return VpsUtils_bufGetFull(&pObj->outObj[queId].bufOutQue, pFrameList, BIOS_NO_WAIT);
}

Int32 ScLink_putEmptyFrames(VpsUtils_TskHndl *pTsk, UInt16 queId, FVID2_FrameList *pFrameList)
{
    ScLink_Obj *pObj = (ScLink_Obj *)pTsk->appData;

    GT_assert( GT_DEFAULT_MASK, queId < SC_LINK_MAX_OUT_QUE);

    return VpsUtils_bufPutEmpty(&pObj->outObj[queId].bufOutQue, pFrameList);
}
