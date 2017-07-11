/****************************************************************************
 *
 * Copyright (c) Intel Corporation (2014).
 *
 * DISCLAIMER OF WARRANTY
 * NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
 * CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
 * OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
 * EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
 * THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
 * BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
 * ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
 * SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
 * NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
 * TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.
 *
 * @file    ReplayParser.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    12th June 2014
 * @brief   Top-level header file for the replay parser.
 *
 * @details Top-level header for the replay parser. This uses the Google Regular
 * Expression library (RE2) to implement RAII parsing for log files generated
 * by the HWC and the Android dumpsys command. The following snippets are taken
 * from files processed by the parser. The ellipses indicate line continuation:
 *
 * HWC-Next entries:
 *
 * 60494s 715ms 414796ns TID:8782 D0 onSet Entry frame:0 Fd:-1 outBuf:0x0...
 *     0 OV 0xb7af3070:22:0 60 BL:FF RGBA 1920x1200    0.0,   0.0,1920.0,1200.0
 *     0,   0,1920,1200 38 -1 V:   0,   0,1920,1200 U:20000900 Hi:0 Fl:0 A B
 *
 * HWC 15.33 entries:
 *
 * 2159s 244ms D1 onSet Entry Fd:-1 outBuf:0x0 outFd:-1 Flags:0
 * 2060s 064ms D0 onSet Entry Fd:-1 outBuf:0x0 outFd:-1 Flags:1(GEOMETRY...
 *     0 OV 0xb91e51e0:69:0 OP:FF 422i  1280x960     0,   0,1280, 960-> ...
 *     141, 112,1144, 864 -1 -1 V: 141, 112,1144, 864 U:00000b02 Hi:0 Fl:0
 *
 * dumpsys SurfaceFlinger entries:
 *
 *   Display[0] : 1920x1200, xdpi=254.000000, ydpi=254.000000, refresh=16...
 *   numHwLayers=4, flags=00000000
 *     type    |  handle  |   hints  |   flags  | tr | blend |  format  |...
 * ------------+----------+----------+----------+----+-------+----------+...
 *        GLES | b871f920 | 00000000 | 00000000 | 00 | 00100 | 00000001 |...
 *        GLES | b8834e40 | 00000000 | 00000000 | 00 | 00105 | 00000001 |...
 *
 *           source crop            |           frame           name
 * ---------------------------------+--------------------------------
 *  [    0.0,   38.0, 1920.0, 1128.0] | [    0,   38, 1920, 1128] com.and...
 *  [    0.0,    0.0, 1920.0,   38.0] | [    0,    0, 1920,   38] StatusBar
 *
 * The canonical reference for RE2 is the RE2 wiki at: code.google.com/p/re2
 *
 * The parser currently matches the following regular expressions:
 *
 * 1. hwcl_onset     : "onSet Entry" lines in HWC-Next log files.
 * 2. hwcl_layer_hdr : everything upto the end of the first visible region
 *                     in the layer description of a HWC-Next log.
 * 3. hwcl_layer_vbr : a visbile region entry. Layers in HWC logs can have
 *                     an arbitray number of visible regions (that should be
 *                     captured), so this is why HWC layers have to be parsed
 *                     in stages.
 * 4. hwcl_layer_trl : the trailer in a HWC layer (i.e. the usage, hints and
 *                     flags).
 * 5. ds_display     : matches the "Display" headers in dumpsys files.
 * 6. ds_layer       : process the layer descriptors in dumpsys files.
 *
 * Note: the regular expressions with '1533' in the titles are the 15.33
 * versions. These are applied if the default (i.e. the HWC-Next) versions do
 * not match.
 *
 *****************************************************************************/

#ifndef __HwchReplayParser_h__
#define __HwchReplayParser_h__

#include <fstream>
#include <string>
#include <utils/RefBase.h>

#include "re2/re2.h"

#include "HwchFrame.h"
#include "HwchLayer.h"

/**
 * Flag to enable log messages from the parser. This is useful when making
 * changes to the regular expressions.
 */
#define PARSER_DEBUG 1

namespace Hwch
{

    class ReplayParser :
        public android::RefBase
    {
        private:

            // Regular expressions for matching HWC log entries
            //
            // See the RE2 wiki for details of the syntax: code.google.com/p/re2
            const std::string hwcl_onset_string =
                "^(\\d+)s (\\d+)ms (?:(\\d+)ns )?(?:TID:(\\d+) )?D(\\d) onSet Entry "
                "(?:frame:(\\d+) )?Fd:(-?\\d{1,2}) "
                "outBuf:0x(.{1,8}) outFd:(-?\\d{1,2}) [fF]lags:(\\d+)(:?.*)$";

            const std::string hwcl_onset_1533_string =
                "^(\\d+)s (\\d+)ms D(\\d) onSet Entry Fd:(-?\\d{1,2}) outBuf:"
                "0x(.{1,8}) outFd:(-?\\d{1,2}) Flags:(\\d+)(:?.*)$";

            const std::string hwcl_layer_string_hdr =
                "^\\s*(\\d+) (\\w{2}) *0x(.{1,12}): ?(?:--|(\\d{1,3})): ?(\\d) ?(\\d+) "
                "(\\w{2}): ?(.{1,2}) ([[:alnum:]]{1,5}) *:[XLY]  *(\\d{1,4})x(\\d{1,4}) * "
                " *(-?\\d+\\.?\\d*), *(-?\\d+\\.?\\d*), *(-?\\d+\\.?\\d*), *(-?\\d+\\.?\\d*)"
                " *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4}) "
                "(-?\\d+) (-?\\d+) "
                "V: *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4}) ";

            const std::string hwcl_layer_1533_string_hdr =
                "^\\s*(\\d+) (\\w{2}) *0x(.{1,8}): ?(\\d{1,2}): ?(\\d{1,2}) "
                "(\\w{2}): ?(.{1,2}) ([?[:alnum:]]{1,5})  *(\\d{1,4})x(\\d{1,4}) * "
                " *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4})->"
                " *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4}), *(-?\\d{1,4}) "
                "(-?\\d+) (-?\\d+) "
                "V: *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4}) ";

            const std::string hwcl_layer_string_vbr =
                " *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4}), *(\\d{1,4})";

            const std::string hwcl_layer_string_trl =
                " *U:(.{1,8}) * Hi:(\\d+)(:?[[:alpha:]]*) "
                "Fl:(\\d+)(:?[ [:alnum:]]*).*";

            // Patterns for matching the output from dumpsys SurfaceFlinger
            const std::string ds_display_string =
                "^\\s*Display\\[(\\d+)\\] : (\\d{1,4})x(\\d{1,4}), "
                "xdpi=(.*), ydpi=(.*), refresh=(\\d*).*$";

            const std::string ds_layer_string =
                "^\\s*([ [:alnum:]]+) \\| (.{1,8}) \\| (\\d+) \\| (\\d+) \\| "
                "(\\d+) \\| (\\d+) \\| (.{1,8}) \\| \\[ *([\\.\\d]*), *([\\.\\d]*), "
                "*([\\.\\d]*), *([\\.\\d]*)] \\| \\[ *([\\.\\d]*), *([\\.\\d]*), "
                "*([\\.\\d]*), *([\\.\\d]*)] *(:?[\\._/[:alpha:]]*) *"
                "([\\.\\d]*)? *([[:alpha:]]*)?\\s*$";

            // Patterns for matching hotplug events
            const std::string hotplug_connected_string = ".+ HotPlug connected$";
            const std::string hotplug_disconnected_string = ".+ HotPlug disconnected$";

            // Patterns for matching blanking events
            const std::string blank_string = ".+ HardwareManager::onBlank Display (\\d) Blank.*$";
            const std::string unblank_string = ".+ HardwareManager::onBlank Display (\\d) Unblank.*$";

            // Patterns related to Protected Content
            const std::string encrypted_layer_string = "ENCRYPT\\(S:(\\d+), I:(\\d+)\\)";
            const std::string protected_enable_string = "Hwc service enable protected sessionID:(\\d+) instanceID:(\\d+)";
            const std::string protected_disable_session_string = "Hwc service disable protected sessionID:(\\d+)";
            const std::string protected_disable_all_string = "Hwc service disable all protected sessions";

            // RE2 compiled Regex data structures
            RE2 hwcl_onset_regex, hwcl_onset_1533_regex, hwcl_layer_regex_hdr,
                hwcl_layer_1533_regex_hdr, hwcl_layer_regex_vbr, hwcl_layer_regex_trl,
                ds_display_regex, ds_layer_regex,
                hotplug_connected_regex, hotplug_disconnected_regex,
                blank_regex, unblank_regex, encrypted_layer_regex,
                protected_enable_regex, protected_disable_session_regex,
                protected_disable_all_regex;

            // Flag to denote whether the parser is ready
            bool mRegexCompilationSuccess;

            // Default update frequency for the dumpsys replay layers
            const float mDefaultDSUpdateFreq = 60.0;

            // Width for hex prints (including the '0x' leader)
            const uint32_t mHexPrintWidth = 10;

        public:

            /** The constructor compiles the regexs into RE2 structures. */
            ReplayParser();

            /** Default destructor. */
            ~ReplayParser() = default;

            /** The parser is not copyable (see above). */
            ReplayParser(const ReplayParser& rhs) = delete;

            /** Disable move semantics (see above). */
            ReplayParser(ReplayParser&& rhs) = delete;

            /** Parser is not copy assignable (see above). */
            ReplayParser& operator=(const ReplayParser& rhs) = delete;

            /** Disable move semantics (see default constructor). */
            ReplayParser& operator=(const ReplayParser&& rhs) = delete;

            /** Returns whether the Regular Expression Compilation was successful. */
            bool IsReady()
            {
                return mRegexCompilationSuccess;
            }

            /*
             * Funtions for operating on HWC log files
             */

            /** Parses frame headers (i.e. 'onSet Entry') lines in HWC log files. */
            bool ParseHwclOnSet(const std::string& line, int32_t& secs, int32_t& msecs,
                                int32_t& nsecs, int32_t& frame, int32_t& disp, int32_t& flags);

            /** Parses layer lines in HWC log files. */
            bool ParseHwclLayer(const std::string& line, Layer& layer);

            /*
             * Functions for parsing HWC string fields
             */

            /** Parses the index field in layer entries. */
            bool ParseHwclLayerIndex(const std::string& str, uint32_t& val);

            /** Parses the buffer handle field in layer entries. */
            bool ParseHwclLayerHandle(const std::string& str, uint64_t& val);

            /** Parses the blending field in layer entries. */
            bool ParseHwclLayerBlending(const std::string& str, uint32_t& val);

            /** Parses the transform field in layer entries. */
            bool ParseHwclLayerTransform(const std::string& str, uint32_t& val);

            /** Parses the colour space field in layer entries. */
            bool ParseHwclLayerColourSpace(const std::string& str, uint32_t& val);

            /*
             * Predicates for testing HWC layer properties
             */

            /** Tests whether a string matches the pattern for a HWC layer entry */
            bool IsHwclLayer(const std::string& line);

            /** Tests if a layer is an HWC_FRAMEBUFFER_TARGET */
            bool IsHwclLayerFramebufferTarget(const std::string& line);

            /** Tests if a layer is a skip layer */
            bool IsHwclLayerSkip(const std::string& line);

            /** Tests if a layer has an unsupported colour space */
            bool IsHwclLayerUnsupported(const std::string& line);

            /*
             * Functions for operating on dumpsys files
             */

            /** Parses the display, width and height from a dumpsys line. */
            bool ParseDSDisplay(const std::string& line, int32_t& disp,
                                int32_t& width, int32_t& height);

            /** Parses the layer lines in a dumpsys file. */
            bool ParseDSLayer(const std::string& line, Layer& layer);

            /** Parses and extracts the 'profile' field at the end of the layer. */
            bool ParseDSProfile(const std::string& line, std::string& profile);

            /*
             * Predicates for testing HWC layer properties
             */

            /** Tests if a line matches the pattern for a dumpsys layer. */
            bool IsDSLayer(const std::string& line);

            /** Tests if a layer is an HWC_FRAMEBUFFER_TARGET. */
            bool IsDSLayerFramebufferTarget(const std::string& line);

            /*
             * Functions for parsing special events
             */

            /** Parses hot plug events in the log. The parameter 'connected' returns
                'true' if the hot plug event is a connection and 'false' otherwise. */
            bool ParseHotPlug(const std::string& line, bool& connected);

            /** Parses blanking (i.e. blank / unblank) requests */
            bool ParseBlanking(const std::string& line, bool& blank, int32_t& disp);

            /** Parses encrypted layers */
            bool ParseEncrypted(const std::string& line, int32_t& session, int32_t& instance);
            bool ParseProtectedEnable(const std::string& line, int32_t& session, int32_t& instance);
            bool ParseProtectedDisableSession(const std::string& line, int32_t& session);
            bool ParseProtectedDisableAll(const std::string& line);

            /*
             * Unit tests
             */

            /** Unit-tests to prevent regressions in the regular expressions. */
            bool RunParserUnitTests();
    };
}

#endif // __HwchReplayParser_h__
