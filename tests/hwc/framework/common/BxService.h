/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013-2015 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its suppliers
 * and licensors. The Material contains trade secrets and proprietary and confidential
 * information of Intel or its suppliers and licensors. The Material is protected by
 * worldwide copyright and trade secret laws and treaty provisions. No part of the
 * Material may be used, copied, reproduced, modified, published, uploaded, posted,
 * transmitted, distributed, or disclosed in any way without Intels prior express
 * written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel
 * or otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 */

#ifndef __BxService_h__
#define __BxService_h__

#include "iservice.h"
#include "HwcvalAbstractHwcServiceSubset.h"

#define IA_HWCREAL_SERVICE_NAME "hwc.info.real"

// BxService forwards most messages straight on
// to the real service.
class BxService : public android::BBinder, Hwcval::AbstractHwcServiceSubset
{
public:
    BxService();

    IBinder* onAsBinder();

    virtual android::status_t onTransact(uint32_t, const android::Parcel&, android::Parcel*, uint32_t);

    android::sp<hwcomposer::IService> Real();

private:
    enum {
        // ==============================================
        // Public APIs - try not to reorder these

        GET_HWC_VERSION = IBinder::FIRST_CALL_TRANSACTION,

        // Dump options and current settings to logcat.
        DUMP_OPTIONS,

        // Override an option.
        SET_OPTION,

        // Enable hwclogviewer output to logcat
        ENABLE_LOG_TO_LOGCAT = 99,

        // accessor for IBinder interface functions
        TRANSACT_GET_DIAGNOSTIC = 100,
        TRANSACT_GET_DISPLAY_CONTROL,
        TRANSACT_GET_VIDEO_CONTROL,
        TRANSACT_GET_MDS_EXT_MODE_CONTROL
    };

    void GetRealService();
    android::sp<android::IBinder> mRealBinder;
    android::sp<hwcomposer::IService> mRealService;
};


#endif // __BxService_h__
