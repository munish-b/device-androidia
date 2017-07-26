/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <Debug.h>
#include "HwcTimeline.h"
#include "HwcTestUtil.h"

#if ANDROID_VERSION >= 430
#include SW_SYNC_H_PATH
#endif

#ifndef SYNC_FENCE_DEBUG
#define SYNC_FENCE_DEBUG 0
#endif

static const int MaxFenceNameLength = 32;

HwcTimeline::HwcTimeline() :
    mpchName( NULL ),
    mSyncTimeline( -1 ),
    mCurrentTime( 0 ),
    mNextFutureTime( 0 )
{
}

HwcTimeline::~HwcTimeline()
{
    uninit( );
}

// One-time initialisation.
// If initialisation is succesful then this will return true and the next
// call to acquire( ) will block until a subsequent release( ).
// Set pchName to a human-readable string that can be used to identify
// this SyncTimeline and all the Fences created from it.
// Pass explicit firstFutureTime to increase the initial delta between acquire/release.
// The SyncTimeline will be uninitialised on destruction.
bool HwcTimeline::init( const char* pchName, uint32_t firstFutureTime )
{
    if ( mSyncTimeline != -1 )
    {
        uninit( );
    }

    HWCLOGW_IF( !firstFutureTime, "Expected non-zero firstFutureTime" );

    if ( !pchName || !strlen( pchName ) )
    {
        pchName = "NO-NAME";
    }
    uint32_t nmLen = strlen( pchName );
    if ( nmLen > 32 )
        nmLen = 32;
    mpchName = new char [ nmLen + 1 ];
    if ( mpchName )
    {
        memset( mpchName, 0, nmLen + 1 );
        strncpy( mpchName, pchName, nmLen );
    }

    // Create the sync timeline for the main display
    mSyncTimeline = sw_sync_timeline_create( );

    if ( mSyncTimeline == -1 )
    {
        HWCERROR(eCheckInternalError, "Failed to create sync timeline." );
        return false;
    }

    mNextFutureTime = firstFutureTime;
    mCurrentTime = 0;

    return true;
}

// Uninitialise the timeline.
void HwcTimeline::uninit( void )
{
    if ( mSyncTimeline != -1 )
    {
        close( mSyncTimeline );
        mSyncTimeline = -1;
    }
    delete [] mpchName;
    mpchName = NULL;
}

// Create a Fence that can be passed to another subsystem to block progress until the future time is reached.
// If an existing Fence is provided then a combined Fence that represents completion of both is created to replace the existing Fence.
// If succesful, returns true and fence will be updated.
// The returned fence must be released using close( ).
bool HwcTimeline::createFence( Fence* pFence )
{
    ALOG_ASSERT( pFence );

    if ( mSyncTimeline == -1 )
    {
        HWCERROR(eCheckInternalError, "SyncTimeline is not initialised");
        return -1;
    }

    char fenceName[ MaxFenceNameLength + 32 ];

    // Build a Fence name from SyncTimeline name + SyncCounter.
    snprintf( fenceName, sizeof( fenceName ), "%s:%u", mpchName, mNextFutureTime );
    Fence newFence = sw_sync_fence_create( mSyncTimeline, fenceName, mNextFutureTime );
    if ( newFence < 0 )
    {
        HWCERROR(eCheckFenceAllocation, "HwcTimeline %d : Failed to create new fence [%s] : %s", mSyncTimeline, fenceName, strerror(errno));
        return false;
    }

    if ( *pFence < 0 )
    {
        HWCLOGD_IF(SYNC_FENCE_DEBUG, "SyncTimeline %d(%s) : Created new fence %d(%s)", mSyncTimeline, mpchName, newFence, fenceName);
        *pFence = newFence;
        return true;
    }

    // Merge newly created fence into existing.
    return mergeFence( pFence, &newFence );
}

// Combines another fence into this existing fence, returning a fence that represents completion of both.
// Returns true if succesful - in which case pFence will be updated and pOtherFence will be closed and reset to -1.
// The returned fence must be released using close( ).
bool HwcTimeline::mergeFence( Fence* pFence, Fence* pOtherFence )
{
    ALOG_ASSERT( pFence && pOtherFence );
    if ( *pFence < 0 )
    {
      if (*pOtherFence >= 0 )
      {
        *pFence = *pOtherFence;
        *pOtherFence = -1;
      }
      return true;
    }
    else if ( *pOtherFence < 0 )
    {
      return true;
    }

    char fenceName[ MaxFenceNameLength + 32 ];

    if (SYNC_FENCE_DEBUG)
    {
        // Create a syncpoint that merges the Fences.
        struct sync_fence_info_data* pInfo1 = sync_fence_info( *pFence );
        struct sync_fence_info_data* pInfo2 = sync_fence_info( *pOtherFence );

        if ( pInfo1 )
        {
            HWCLOGD_IF(pInfo1, "Fence Info1: %s status %d", pInfo1->name, pInfo1->status);
            struct sync_pt_info* pSyncPointInfo = NULL;
            while ( ( pSyncPointInfo = sync_pt_info( pInfo1, pSyncPointInfo ) ) != NULL )
            {
                HWCLOGD("  SyncPoint Driver %s Status %d Timestamp %.03f",
                    pSyncPointInfo->driver_name,
                    pSyncPointInfo->status,
                    (float)pSyncPointInfo->timestamp_ns * (1/1000000000.0f));
            }
        }
        if ( pInfo2 )
        {
            HWCLOGD_IF(pInfo2, "Fence Info2: %s status %d", pInfo2->name, pInfo2->status);
            struct sync_pt_info* pSyncPointInfo = NULL;
            while ( ( pSyncPointInfo = sync_pt_info( pInfo2, pSyncPointInfo ) ) != NULL )
            {
                HWCLOGD("  SyncPoint Driver %s Status %d Timestamp %.03f",
                        pSyncPointInfo->driver_name, pSyncPointInfo->status,
                        (float)pSyncPointInfo->timestamp_ns * (1/1000000000.0f) );
            }
        }
        if ( pInfo1 && pInfo2 )
        {
            // Create a combined Fence name from info names.
            snprintf( fenceName, sizeof( fenceName ), "[%s && %s]", pInfo1->name, pInfo2->name );
        }
        else
        {
            // Create a combined Fence name from handles.
            snprintf( fenceName, sizeof( fenceName ), "[F%d && F%d]", *pFence, *pOtherFence );
        }
        if ( pInfo1 )
            sync_fence_info_free( pInfo1 );
        if ( pInfo2 )
            sync_fence_info_free( pInfo2 );
    }
    else
    {
        // Create a combined Fence name from handles.
        snprintf( fenceName, sizeof( fenceName ), "[F%d && F%d]", *pFence, *pOtherFence );
    }

    // Merge the two component fences.
    Fence mergedFence = sync_merge( fenceName, *pFence, *pOtherFence );
    if ( mergedFence < 0 )
    {
        HWCERROR(eCheckFenceAllocation, "Failed to merge fence [%s] : %s", fenceName, strerror(errno));
        return false;
    }
    else
    {
        // Close the two component fences for the merge.
        HWCLOGD_IF(SYNC_FENCE_DEBUG, "HwcTimeline : Merged fence %d(%s)", mergedFence, fenceName);
        CloseFence( *pFence );
        CloseFence( *pOtherFence );
        *pFence = mergedFence;
        *pOtherFence = -1;
        return true;
    }
}

// Duplicate an existing fence to this fence.
// If an existing Fence is provided then a combined Fence that represents completion of both is created to replace the existing Fence.
// Returns true if succesful - in which case pFence will be updated.
// The returned fence must be released using close( ).
bool HwcTimeline::dupFence( Fence* pFence, const Fence otherFence )
{
    ALOG_ASSERT(pFence && otherFence >= 0);
    if ( *pFence < 0 )
    {
        *pFence = dup( otherFence );
        return ( *pFence >= 0 );
    }
    Fence mergedOther = dup( otherFence );
    if ( mergedOther < 0 )
    {
        return false;
    }
    return mergeFence( pFence, &mergedOther );
}

// Advance the 'current time' by N ticks.
// This will release all fences up to and including the new current time.
// By default, this will increase time by 1 tick.
void HwcTimeline::advance( uint32_t ticks )
{
    if ( mSyncTimeline == -1 )
    {
        HWCLOGW("SyncTimeline is not initialised");
        return;
    }

    HWCLOGD_IF(SYNC_FENCE_DEBUG, "Release mSyncTimeline %d(%s) +%u", mSyncTimeline, mpchName, ticks);

    int err = sw_sync_timeline_inc( mSyncTimeline, ticks );
    mCurrentTime += ticks;

    if ( err < 0 )
    {
        HWCERROR(eCheckInternalError, "Failed to advance sync timeline %d(%s)", mSyncTimeline, mpchName );
    }
}

void HwcTimeline::advanceTo( uint32_t absSync )
{
    if ( mSyncTimeline == -1 )
    {
        HWCLOGW("SyncTimeline is not initialised");
        return;
    }

    int32_t delta = int32_t( absSync - mCurrentTime );
    if ( delta > 0 )
    {
        HWCLOGD_IF(SYNC_FENCE_DEBUG, "advanceTo( %u ) mCurrentTime %u => delta %u", absSync, mCurrentTime, delta);
        advance( (uint32_t)delta );
    }
    else
    {
        HWCLOGW("Advance timeline delta is %d (expected +ve delta)", delta);
    }
}

// Dump trace for timeline status.
void HwcTimeline::dumpTimeline( const char* pchPrefix )
{
        HWCLOGD_IF(SYNC_FENCE_DEBUG, "%s - SyncTimeline %d(%s) mNextFutureTime %u",
                pchPrefix ? pchPrefix : "", mSyncTimeline, mpchName, mNextFutureTime);
}

// Dump trace for a fence.
void HwcTimeline::dumpFence( Fence fence, const char* pchPrefix )
{
    if (SYNC_FENCE_DEBUG)
    {
        if ( fence >= 0 )
        {
            struct sync_fence_info_data* pInfo = NULL;
            pInfo = sync_fence_info( fence );

            HWCLOGD("%s - Info: %s status %d", pchPrefix ? pchPrefix : "", pInfo ? pInfo->name : "n/a",
                    pInfo ? pInfo->status : -1);

            if ( pInfo )
            {
                struct sync_pt_info* pSyncPointInfo = NULL;
                while ( ( pSyncPointInfo = sync_pt_info( pInfo, pSyncPointInfo ) ) != NULL )
                {
                    HWCLOGD("  SyncPoint Driver %s Status %d Timestamp %.03f",
                            pSyncPointInfo->driver_name, pSyncPointInfo->status,
                            (float)pSyncPointInfo->timestamp_ns * (1/1000000000.0f));
                }
                sync_fence_info_free( pInfo );
            }
        }
    }
}
