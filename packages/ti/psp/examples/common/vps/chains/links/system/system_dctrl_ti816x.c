/*******************************************************************************
 *                                                                             *
 * Copyright (c) 2009 Texas Instruments Incorporated - http://www.ti.com/      *
 *                        ALL RIGHTS RESERVED                                  *
 *                                                                             *
 ******************************************************************************/


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include <ti/psp/examples/common/vps/chains/links/system/system_priv.h>
#include <ti/psp/devices/vps_sii9022a.h>
#include <ti/psp/devices/vps_thsfilters.h>
#include <ti/psp/examples/common/vps/chains/src/chains.h>

#define ONCHIP_HDMI_ENABLE
//#define SWITCH_BP0_BP1
//#define CIG_ENABLE

#ifdef CIG_ENABLE

/* since CIG can be enable only on HDDAC path we need to alter
  the DC Config when CIG is ON
*/
#define SWITCH_BP0_BP1

#endif

#ifdef SWITCH_BP0_BP1

/* tied VENCs has some application problem when
    BP0/BP1 is switched hence disabling it
*/
#undef ONCHIP_HDMI_ENABLE
#endif

/* Display Controller Configuration */
Vps_DcConfig gSystem_dctrlTriDisplayConfig =
{
    VPS_DC_USERSETTINGS,                /* Use Case */
    /* Edge information */
    {
        {VPS_DC_BP0_INPUT_PATH, VPS_DC_HDCOMP_MUX},
        {VPS_DC_HDCOMP_MUX, VPS_DC_CIG_PIP_INPUT},
#ifdef ONCHIP_HDMI_ENABLE
        {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDMI_BLEND},
#endif
#ifdef SWITCH_BP0_BP1
        {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_DVO2_BLEND},
#else
        {VPS_DC_CIG_PIP_OUTPUT, VPS_DC_HDCOMP_BLEND},
#endif
        {VPS_DC_BP1_INPUT_PATH, VPS_DC_VCOMP_MUX},
        {VPS_DC_VCOMP_MUX, VPS_DC_VCOMP},
#ifdef SWITCH_BP0_BP1
        {VPS_DC_CIG_CONSTRAINED_OUTPUT, VPS_DC_HDCOMP_BLEND},
#else
        {VPS_DC_CIG_NON_CONSTRAINED_OUTPUT, VPS_DC_DVO2_BLEND},
#endif
        {VPS_DC_SEC1_INPUT_PATH, VPS_DC_SDVENC_MUX},
        {VPS_DC_SDVENC_MUX, VPS_DC_SDVENC_BLEND},
#ifdef ONCHIP_HDMI_ENABLE
        {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDMI_BLEND},
#endif
        {VPS_DC_GRPX0_INPUT_PATH, VPS_DC_HDCOMP_BLEND},
        {VPS_DC_GRPX1_INPUT_PATH, VPS_DC_DVO2_BLEND},
        {VPS_DC_GRPX2_INPUT_PATH, VPS_DC_DVO2_BLEND},
    },
#ifdef ONCHIP_HDMI_ENABLE
    13,
#else
    11u,                                /* Number of edges */
#endif
    /* VENC information */
    {
        /* Mode information */
        {
#ifdef ONCHIP_HDMI_ENABLE
            {VPS_DC_VENC_HDMI, {FVID2_STD_1080P_60}},   /* 1080p30 is mode is overwritten later inside System_displayCtrlInit */
#endif
            {VPS_DC_VENC_HDCOMP, {FVID2_STD_1080P_60}},   /* 1080p30 is mode is overwritten later inside System_displayCtrlInit */
            {VPS_DC_VENC_DVO2, {FVID2_STD_1080P_60}},   /* 1080p30 is mode is overwritten later inside System_displayCtrlInit */
            {VPS_DC_VENC_SD, {FVID2_STD_NTSC}}
        },
#ifdef ONCHIP_HDMI_ENABLE
        (VPS_DC_VENC_HDMI|VPS_DC_VENC_HDCOMP),   /* Tied VENC bit mask */
        4u                              /* Number of VENCs */
#else
        0,
        3u                              /* Number of VENCs */
#endif

    }
};

Int32 System_displayCtrlInit(UInt32 displayRes, UInt32 sdDisplayRes,
    Bool edeEnable)
{
    Int32                   driverRetVal, retVal;
    UInt32                  vencMode, sdVencMode;
    Vps_DcCreateConfig      dcCreateCfg;
    Vps_DcEdeConfig         dcEdeCfg;
    Vps_CprocConfig         cprocCfg;
    Vps_CscConfig           dcHdcompCscConfig;
    Vps_CscConfig           dcSdCscConfig;
    Vps_PlatformBoardId     boardId;
    Vps_PlatformId          platformId;
    Vps_PlatformCpuRev      cpuRev;
#ifndef PLATFORM_SIM
    Vps_DcVencClkSrc        clkSrc;
    Vps_DcOutputInfo        dcOutputInfo;
#endif
#ifdef CIG_ENABLE
    Vps_DcCigMainConfig     dcCigMainConfig;
    Vps_DcCigPipConfig      dcCigPipConfig;
#endif


    System_displayUnderflowCheck(TRUE);

    boardId = Vps_platformGetBoardId();

    if(boardId>=VPS_PLATFORM_BOARD_MAX)
    {
        Vps_printf(" %d: SYSTEM: UNKNOWN board detected, defaulting to VS board\r\n",
                Clock_getTicks()
            );
        boardId = VPS_PLATFORM_BOARD_VS;
    }

    /* Get platform type, board and CPU revisions */
    platformId = Vps_platformGetId();
    if ((platformId == VPS_PLATFORM_ID_UNKNOWN) ||
    (platformId >= VPS_PLATFORM_ID_MAX))
    {
        Vps_printf("%s: Error Unrecognized platform @ line %d\n",
          __FUNCTION__, __LINE__);
        return FVID2_EFAIL;
    }

    cpuRev = Vps_platformGetCpuRev();
    if (cpuRev >= VPS_PLATFORM_CPU_REV_MAX)
    {
        Vps_printf("%s: Error Unrecognized CPU version @ line %d\n",
          __FUNCTION__, __LINE__);
        return FVID2_EFAIL;
    }

    if (SYSTEM_DISPLAY_RES_1080P30 == displayRes)
    {
        vencMode = FVID2_STD_1080P_30;
        if (Vps_platformIsEvm())
        {
            gSystem_obj.vpllCfg.outputClk = 74250u;
        }
    }
    else if (SYSTEM_DISPLAY_RES_1080I60 == displayRes)
    {
        vencMode = FVID2_STD_1080I_60;
        if (Vps_platformIsEvm())
        {
            gSystem_obj.vpllCfg.outputClk = 74250u;
        }
    }
    else if (SYSTEM_DISPLAY_RES_1080P60 == displayRes)
    {
        vencMode = FVID2_STD_1080P_60;
        if (Vps_platformIsEvm())
        {
            gSystem_obj.vpllCfg.outputClk = 148500u;
        }
    }
    else if (SYSTEM_DISPLAY_RES_720P60 == displayRes)
    {
        vencMode = FVID2_STD_720P_60;
        if (Vps_platformIsEvm())
        {
            gSystem_obj.vpllCfg.outputClk = 74250u;
        }
    }
    else
    {
        GT_assert( GT_DEFAULT_MASK, 0);
    }

    if (SYSTEM_DISPLAY_RES_NTSC == sdDisplayRes)
    {
        sdVencMode = FVID2_STD_NTSC;
    }
    else if (SYSTEM_DISPLAY_RES_PAL == sdDisplayRes)
    {
        sdVencMode = FVID2_STD_PAL;
    }
    else
    {
        GT_assert(GT_DEFAULT_MASK, 0);
    }

    /* Configure pixel clock */
    retVal = System_dispSetPixClk();
    GT_assert( GT_DEFAULT_MASK, FVID2_SOK == retVal);

    memcpy(
        &gSystem_obj.displayCtrlCfg, 
        &gSystem_dctrlTriDisplayConfig, 
        sizeof(Vps_DcConfig));

    gSystem_obj.displayCtrlCfg.vencInfo.modeInfo[0].mInfo.standard = vencMode;
    gSystem_obj.displayCtrlCfg.vencInfo.modeInfo[1].mInfo.standard = vencMode;
    #ifdef ONCHIP_HDMI_ENABLE
    gSystem_obj.displayCtrlCfg.vencInfo.modeInfo[2].mInfo.standard = vencMode;
    #endif
    gSystem_obj.displayCtrlCfg.vencInfo.modeInfo[3].mInfo.standard = sdVencMode;

    dcHdcompCscConfig.bypass =
    dcSdCscConfig.bypass     = FALSE;
    dcHdcompCscConfig.coeff  =
    dcSdCscConfig.coeff      = NULL;
    if (VPS_PLATFORM_BOARD_VS == boardId)
    {
        dcHdcompCscConfig.mode =
        dcSdCscConfig.mode     = VPS_CSC_MODE_SDTV_GRAPHICS_Y2R;
    }
    else
    {
        dcHdcompCscConfig.mode =
        dcSdCscConfig.mode     = VPS_CSC_MODE_HDTV_GRAPHICS_Y2R;
    }

    dcEdeCfg.ltiEnable = TRUE;
    dcEdeCfg.horzPeaking = TRUE;
    dcEdeCfg.ctiEnable = TRUE;
    dcEdeCfg.transAdjustEnable = TRUE;
    dcEdeCfg.lumaPeaking = TRUE;
    dcEdeCfg.chromaPeaking = TRUE;
    dcEdeCfg.minClipLuma = 0;
    dcEdeCfg.maxClipLuma = 1023;
    dcEdeCfg.minClipChroma = 0;
    dcEdeCfg.maxClipChroma = 1023;
    if (TRUE == edeEnable)
        dcEdeCfg.bypass = FALSE;
    else
        dcEdeCfg.bypass = TRUE;

#ifdef CIG_ENABLE
    dcCigMainConfig.enableContraining   = TRUE;
    dcCigMainConfig.nonCtrInterlace     = FALSE;
    dcCigMainConfig.ctrInterlace        = FALSE;

    dcCigPipConfig.pipInterlace         = FALSE;
#endif

    memset(&dcCreateCfg, 0, sizeof(dcCreateCfg));

    dcCreateCfg.edeConfig       = &dcEdeCfg;
    dcCreateCfg.hdcompCscConfig = &dcHdcompCscConfig;
    dcCreateCfg.sdCscConfig     = &dcSdCscConfig;

#ifdef CIG_ENABLE
    dcCreateCfg.cigMainConfig = &dcCigMainConfig;
    dcCreateCfg.cigPipConfig  = &dcCigPipConfig;
#endif

    memset(&cprocCfg, 0, sizeof(cprocCfg));
    if (VPS_PLATFORM_BOARD_VS == boardId)
    {
        cprocCfg.ciecamMode = VPS_CPROC_CIECAM_MODE_BT601;
    }
    else
    {
        cprocCfg.ciecamMode = VPS_CPROC_CIECAM_MODE_BT709;
    }
    dcCreateCfg.cprocConfig = &cprocCfg;

    /* Open and configure display controller */
    gSystem_obj.fvidDisplayCtrl = FVID2_create(
                      FVID2_VPS_DCTRL_DRV,
                      VPS_DCTRL_INST_0,
                      &dcCreateCfg,
                      &driverRetVal,
                      NULL);
    GT_assert( GT_DEFAULT_MASK, NULL != gSystem_obj.fvidDisplayCtrl );

#ifndef PLATFORM_SIM
    /* Set output in display controller */
    dcOutputInfo.vencNodeNum = VPS_DC_VENC_DVO2;
    dcOutputInfo.aFmt = VPS_DC_A_OUTPUT_COMPOSITE;
    dcOutputInfo.dvoFidPolarity = VPS_DC_POLARITY_ACT_HIGH;
    dcOutputInfo.dvoVsPolarity = VPS_DC_POLARITY_ACT_HIGH;
    dcOutputInfo.dvoHsPolarity = VPS_DC_POLARITY_ACT_HIGH;
    dcOutputInfo.dvoActVidPolarity = VPS_DC_POLARITY_ACT_HIGH;

    if (VPS_PLATFORM_BOARD_VS == boardId)
    {
        dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_DOUBLECHAN;
        dcOutputInfo.dataFormat = FVID2_DF_YUV422SP_UV;
    }
    else if (VPS_PLATFORM_BOARD_VC == boardId)
    {
        dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_TRIPLECHAN_DISCSYNC;
        dcOutputInfo.dataFormat = FVID2_DF_RGB24_888;
    }
    else if (VPS_PLATFORM_BOARD_CATALOG == boardId)
    {
        dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_TRIPLECHAN_DISCSYNC;
        dcOutputInfo.dataFormat = FVID2_DF_RGB24_888;
    }
    else
    {
        dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_DOUBLECHAN;
        dcOutputInfo.dataFormat = FVID2_DF_YUV422SP_UV;
    }
    retVal = FVID2_control(
                gSystem_obj.fvidDisplayCtrl,
                IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                &dcOutputInfo,
                NULL);
    GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);

    /* Set the Clock source for VENC_DVO2 */
    clkSrc.venc = VPS_DC_VENC_DVO2;
    if ((VPS_PLATFORM_ID_EVM_TI816x == platformId) &&
        (VPS_PLATFORM_CPU_REV_1_0   == cpuRev))
    {
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD_DIV2;
    }
    else
    {
        clkSrc.clkSrc = VPS_DC_CLKSRC_VENCD;
    }

    retVal = FVID2_control(
                 gSystem_obj.fvidDisplayCtrl,
                 IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                 &clkSrc,
                 NULL);
    GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);

#ifdef ONCHIP_HDMI_ENABLE
    dcOutputInfo.vencNodeNum = VPS_DC_VENC_HDMI;
    dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_TRIPLECHAN_DISCSYNC;
    dcOutputInfo.aFmt = VPS_DC_A_OUTPUT_COMPOSITE;
    dcOutputInfo.dataFormat = FVID2_DF_RGB24_888;
    retVal = FVID2_control(
                gSystem_obj.fvidDisplayCtrl,
                IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                &dcOutputInfo,
                NULL);
    GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);

    /* Set the Clock source for VENC_HDMI */
    clkSrc.venc = VPS_DC_VENC_HDMI;
    // clkSrc is the same as DVO2 for this App
    retVal = FVID2_control(
                 gSystem_obj.fvidDisplayCtrl,
                 IOCTL_VPS_DCTRL_SET_VENC_CLK_SRC,
                 &clkSrc,
                 NULL);
    GT_assert(GT_DEFAULT_MASK, retVal == FVID2_SOK);
#endif

#ifndef PLATFORM_ZEBU
    {
        Vps_Ths7360SfCtrl       thsCtrl;

        dcOutputInfo.vencNodeNum = VPS_DC_VENC_HDCOMP;
        dcOutputInfo.dvoFmt = VPS_DC_DVOFMT_TRIPLECHAN_EMBSYNC;
        dcOutputInfo.aFmt = VPS_DC_A_OUTPUT_COMPONENT;
        dcOutputInfo.dataFormat = FVID2_DF_YUV444P;
        retVal = FVID2_control(
                    gSystem_obj.fvidDisplayCtrl,
                    IOCTL_VPS_DCTRL_SET_VENC_OUTPUT,
                    &dcOutputInfo,
                    NULL);
        GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);

        switch (vencMode)
        {
            case FVID2_STD_720P_60:
            case FVID2_STD_720P_50:
            case FVID2_STD_1080I_60:
            case FVID2_STD_1080I_50:
            case FVID2_STD_1080P_30:
                thsCtrl = VPS_THS7360_SF_HD_MODE;
                break;

            default:
            case FVID2_STD_1080P_60:
            case FVID2_STD_1080P_50:
                thsCtrl = VPS_THS7360_SF_TRUE_HD_MODE;
                break;
        }
        Vps_ths7360SetSfParams(thsCtrl);
        Vps_ths7360SetSdParams(VPS_THSFILTER_ENABLE_MODULE);
    }
#endif
#endif

    retVal = FVID2_control(
                 gSystem_obj.fvidDisplayCtrl,
                 IOCTL_VPS_DCTRL_SET_CONFIG,
                 &gSystem_obj.displayCtrlCfg,
                 NULL);
    GT_assert( GT_DEFAULT_MASK, retVal==FVID2_SOK);

    #ifdef SYSTEM_USE_OFF_CHIP_HDMI
    if (VPS_PLATFORM_BOARD_VS == boardId || VPS_PLATFORM_BOARD_VC == boardId ||
        VPS_PLATFORM_BOARD_CATALOG == boardId)
    {
        System_hdmiStart(displayRes, boardId);
    }
    #endif

    return retVal;
}

