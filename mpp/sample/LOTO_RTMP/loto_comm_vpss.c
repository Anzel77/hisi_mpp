
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#include "loto_comm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "common.h"

HI_S32 LOTO_VPSS_Crop(VPSS_GRP VpssGrp, HI_S32 screenX, HI_S32 screenY, HI_U32 screenWidth, HI_U32 screenHeight)
{
    HI_S32 s32Ret;
    VPSS_CROP_INFO_S stVpssCropInfo = {0};

    s32Ret = HI_MPI_VPSS_GetGrpCrop(VpssGrp, &stVpssCropInfo);        //获取 VPSS CROP 功能属性
    if(s32Ret != HI_SUCCESS) 
    { 
        LOGE("HI_MPI_VPSS_GetGrpCrop(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    } 

    stVpssCropInfo.bEnable = HI_TRUE;
    stVpssCropInfo.enCropCoordinate = VPSS_CROP_ABS_COOR;

    stVpssCropInfo.stCropRect.s32X = screenX;
    stVpssCropInfo.stCropRect.s32Y = screenY;
    stVpssCropInfo.stCropRect.u32Width = screenWidth;
    stVpssCropInfo.stCropRect.u32Height = screenHeight;

    s32Ret = HI_MPI_VPSS_SetGrpCrop(VpssGrp, &stVpssCropInfo);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE("HI_MPI_VPSS_SetGrpCrop(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }
}

/*****************************************************************************
* function : start vpss grp.
*****************************************************************************/
HI_S32 LOTO_COMM_VPSS_Start(VPSS_GRP VpssGrp, HI_BOOL* pabChnEnable, VPSS_GRP_ATTR_S* pstVpssGrpAttr, VPSS_CHN_ATTR_S* pastVpssChnAttr)
{
    VPSS_CHN    VpssChn;
    HI_S32      s32Ret;
    HI_S32      j;

    s32Ret = HI_MPI_VPSS_CreateGrp(VpssGrp, pstVpssGrpAttr);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE("HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!\n", VpssGrp, s32Ret);
        return HI_FAILURE;
    }

#ifdef VPSS_CROP
#ifdef SNS_IMX335
    // s32Ret = LOTO_VPSS_Crop(VpssGrp, 756, 432, 1080, 1080);
    s32Ret = LOTO_VPSS_Crop(VpssGrp, 324, 0, 1944, 1944);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE("LOTO_VPSS_Crop failed!\n");
    }
#else
    s32Ret = LOTO_VPSS_Crop(VpssGrp, 420, 0 , 1080, 1080);
    if (s32Ret != HI_SUCCESS) 
    {
        LOGE("LOTO_VPSS_Crop failed!\n");
    }
#endif
#endif
    
    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++)
    {
        if(HI_TRUE == pabChnEnable[j])
        {
            VpssChn = j;
            s32Ret = HI_MPI_VPSS_SetChnAttr(VpssGrp, VpssChn, &pastVpssChnAttr[VpssChn]);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE("HI_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }

#ifdef VPSS_ROTATION_90
            /* Clockwise Rotation 90 */
            s32Ret = HI_MPI_VPSS_SetChnRotation (VpssGrp, VpssChn, ROTATION_90);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE("HI_MPI_VPSS_SetChnRotation failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }
            LOGI("=== Clockwise Rotation 90! ===\n");
#endif

            s32Ret = HI_MPI_VPSS_EnableChn(VpssGrp, VpssChn);
            if (s32Ret != HI_SUCCESS)
            {
                LOGE("HI_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
                return HI_FAILURE;
            }

        }
    }


    s32Ret = HI_MPI_VPSS_StartGrp(VpssGrp);
    if (s32Ret != HI_SUCCESS)
    {
        LOGE("HI_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return HI_FAILURE;
    }


    return HI_SUCCESS;
}

/*****************************************************************************
* function : stop vpss grp
*****************************************************************************/
HI_S32 LOTO_COMM_VPSS_Stop(VPSS_GRP VpssGrp, HI_BOOL* pabChnEnable)
{
    HI_S32 j;
    HI_S32 s32Ret = HI_SUCCESS;
    VPSS_CHN VpssChn;

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++)
    {
        if(HI_TRUE == pabChnEnable[j])
        {
            VpssChn = j;
            s32Ret = HI_MPI_VPSS_DisableChn(VpssGrp, VpssChn);

            if (s32Ret != HI_SUCCESS)
            {
                LOGE("failed with %#x!\n", s32Ret);
                return HI_FAILURE;
            }
        }
    }

    s32Ret = HI_MPI_VPSS_StopGrp(VpssGrp);

    if (s32Ret != HI_SUCCESS)
    {
        LOGE("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_VPSS_DestroyGrp(VpssGrp);

    if (s32Ret != HI_SUCCESS)
    {
        LOGE("failed with %#x!\n", s32Ret);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
