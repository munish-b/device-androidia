/*******************************************************************************
**
** Copyright (c) Intel Corporation (2005-2013).
**
** DISCLAIMER OF WARRANTY
** NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
** CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
** OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
** EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
** FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
** THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
** BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
** ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
** SOURCE CODE IS LICENSED TO LICENSEE ON AN 'AS IS' BASIS AND NEITHER INTEL
** NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
** TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
** UPDATES, ENHANCEMENTS OR EXTENSIONS.
**
** File Name: gfxEscape.h
**
** Description:
**      This file defines the header that user of escape framework needs to
** prepend to its operation data buffer to be able to use the escape framework.
** It also provides the enum structure to uniquly specify the component control
** identifier.
**
** Environment:
**      Vista LDDM
**
** Notes:
**
*******************************************************************************/
#ifndef _GFXESCAPE_H_
#define _GFXESCAPE_H_

//==============================================================================
//
//  ESCAPE CODE
//
//  Desription:
//      Identifies the escape call. Based on escape code the DxgkDdiEscape(...)
//      will decide which escape code function to use to handle
//      EscapeCb(...) or OsThunkDDIEscape(...) call.
//------------------------------------------------------------------------------

typedef enum
{
    // IMPORTANT:- When adding new escape code, add it at the end of the list,
    // just before GFX_MAX_ESCAPE_CODES. the reason is that external test apps
    // depend on the current order.

    // DO NOT ADD NEGATIVE ENUMERATORS
    GFX_ESCAPE_CODE_DEBUG_CONTROL = 0L, // DO NOT CHANGE
    GFX_ESCAPE_CUICOM_CONTROL,
    GFX_ESCAPE_GMM_CONTROL,
    GFX_ESCAPE_CAMARILLO_CONTROL,
    GFX_ESCAPE_ROTATION_CONTROL,
    GFX_ESCAPE_PAVP_CONTROL,
    GFX_ESCAPE_UMD_GENERAL_CONTROL,
    GFX_ESCAPE_RESOURCE_CONTROL,
    GFX_ESCAPE_SOFTBIOS_CONTROL,
    GFX_ESCAPE_ACPI_CONTROL,
    GFX_ESCAPE_CODE_KM_DAF,
    GFX_ESCAPE_CODE_PERF_CONTROL,
    GFX_ESCAPE_IGPA_INSTRUMENTATION_CONTROL,
    GFX_ESCAPE_CODE_OCA_TEST_CONTROL,
    GFX_ESCAPE_AUTHCHANNEL,
    GFX_ESCAPE_SHARED_RESOURCE,
    GFX_ESCAPE_PWRCONS_CONTROL,
    GFX_ESCAPE_KMD,
    GFX_ESCAPE_DDE,
    GFX_ESCAPE_IFFS,
    GFX_ESCAPE_TOOLS_CONTROL, //Escape for Tools
    GFX_ESCAPE_ULT_FW,
    GFX_ESCAPE_HDCP_SRVC,
    GFX_ESCAPE_KM_GUC,
    GFX_ESCAPE_EVENT_PROFILING,
    GFX_ESCAPE_WAFTR,
    GFX_ESCAPE_KM_GUC_INTERNAL,

    GFX_ESCAPE_PERF_STATS = 100,


    GFX_ESCAPE_SW_DECRYPTION,

    GFX_ESCAPE_CHECK_PRESENT_DURATION_SUPPORT = 102,
    GFX_ESCAPE_GET_DISPLAYINFO_ESCAPE,
    // NOTE: WHEN YOU ADD NEW ENUMERATOR, PLEASE UPDATE
    //       InitializeEscapeCodeTable in miniport\LHDM\Display\AdapterEscape.c

    GFX_MAX_ESCAPE_CODES // MUST BE LAST
} GFX_ESCAPE_CODE_T;

C_ASSERT(GFX_ESCAPE_CODE_KM_DAF == 10); // If you're getting an error here, please...
// (1) IMPORTANT--Increment the KM_DAF_REV_ID #define in kmDaf.h--IMPORTANT!!!
// (2) Change the "10" in the above C_ASSERT to the new GFX_ESCAPE_CODE_KM_DAF enum index.
// (3) Change the "10" in both the above comment and this comment to the new index, also.  ;)

//==============================================================================
//
//  ESCAPE STRUCTURE HEADER
//
//  Description:
//      This should be the first member of GFX_ESCAPE_<Escape Code> structure.
//
//------------------------------------------------------------------------------

typedef struct GFX_ESCAPE_HEADER
{
    union
    {
        struct
        {
            unsigned int        Size;       // Size of operation specific data arguments
            unsigned int        CheckSum;   // ulong based sum of data arguments
            GFX_ESCAPE_CODE_T   EscapeCode; // code defined for each independent
                                            // component
            unsigned int        ulReserved;
        };
        //The new HEADER definition below is being added for the escape codes
        //in GFX_ESCAPE_CUICOM_CONTROL & GFX_ESCAPE_TOOLS_CONTROL
        struct
        {
            unsigned int        ulReserved1;
            unsigned int        ulMinorInterfaceVersion; // Currently this field is used only by SB Tool Escapes. For the rest, its don't care field
            GFX_ESCAPE_CODE_T   ulMajorEscapeCode; // code defined for each independent
                                            // component
            unsigned int        uiMinorEscapeCode; // Code defined for each sub component contained in the component
        };
    };        // ensure sizeof struct divisible by 8 to prevent padding on 64-bit builds
} GFX_ESCAPE_HEADER_T;

//==============================================================================
//
//  ESCAPE STRUCTURES
//
//  Description:
//      Each escape call has a structure associated with it.
//      First element of this structure must be GFX_ESCAPE_HEADER_T.
//------------------------------------------------------------------------------

#endif // _GFXESCAPE_H_
