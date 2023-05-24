#ifndef LOTO_VENC_H
#define LOTO_VENC_H

#include "loto_comm.h"

HI_S32 LOTO_VENC_CheckSensor(SAMPLE_SNS_TYPE_E enSnsType, SIZE_S stSize);
HI_S32 LOTO_VENC_ModifyResolution(SAMPLE_SNS_TYPE_E enSnsType, PIC_SIZE_E *penSize, SIZE_S *pstSize);
HI_S32 LOTO_VENC_VI_Init(SAMPLE_VI_CONFIG_S *pstViConfig, HI_BOOL bLowDelay, HI_U32 u32SupplementConfig);
HI_S32 LOTO_VENC_FramerateDown(HI_BOOL enable);

HI_VOID *LOTO_VENC_CLASSIC(void *p);

HI_S32 LOTO_RGN_InitCoverRegion() ;
HI_S32 LOTO_RGN_UninitCoverRegion();
HI_S32 LOTO_VENC_AttachCover();
HI_S32 LOTO_VENC_DetachCover();
HI_S32 LOTO_VENC_AddCover();
HI_S32 LOTO_VENC_RemoveCover();

#endif // LOTO_VENC_H