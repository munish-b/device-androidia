#!/usr/bin/env python3

'''
PyReplay: A Tool for Comparing Hardware Composer Logs
'''

import sys
import re
import tkinter as tk
import tkinter.scrolledtext as tkst
import tkinter.filedialog as tkfd
import tkinter.messagebox as tkmb
from   tkinter import StringVar

__author__     = 'James Pascoe'
__copyright__  = 'Copyright 2015 Intel Corporation'
__version__    = '1.1'
__maintainer__ = 'James Pascoe'
__email__      = 'james.pascoe@intel.com'
__status__     = 'Alpha'

### Regex patterns - place them here for export  ###

onset_pattern = "^(\d+)s (\d+)ms (?:(\d+)ns )?(?:TID:(\d+) )?D(\d) onSet Entry " \
                "(?:frame:(\d+) )?Fd:(-?\d{1,2}) " \
                "outBuf:0x(.{1,8}) outFd:(-?\d{1,2}) [fF]lags:(\d+)(:?.*)$";

layer_hdr_pattern = r"^\s*(\d+) (\w{2}) *0x(.{1,8}): ?(?:--|(\d{1,2})): ?(\d) ?(\d+) " \
                    "(\w{2}): ?(.{1,2}) ([?\w]{1,5}) *(\d{1,4})x(\d{1,4}) * " \
                    " *(-?\d+\.?\d*), *(-?\d+\.?\d*), *(-?\d+\.?\d*), *(-?\d+\.?\d*)" \
                    " *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4}) " \
                    "-?\d+ -?\d+ " \
                    "V: *\d{1,4}, *\d{1,4}, *\d{1,4}, *\d{1,4} "

onset_1533_pattern = r"^(\d+)s (\d+)ms ((\d+)ns)? D(\d) onSet Entry " \
                     "Fd:(-?\d{1,2}) outBuf:0x(.{1,8}) outFd:(-?\d{1,2}) Flags:(\d+)(:?.*)$"

layer_vbr_pattern = r" *(\d{1,4}), *(\d{1,4}), *(\d{1,4}), *(\d{1,4})"

layer_trl_pattern = r" *U:.{1,8} * Hi:\d+\w* Fl:.{1,8}[ \w]*$"

layer_1533_pattern = r"^\s*(\d+) (\w{2}) *0x(.{1,8}): ?(\d{1,2}): ?(\d{1,2}) " \
                     "(\w{2}): ?(.{1,2}) ([?A-Za-z0-9]{1,5})  *(\d{1,4})x(\d{1,4}) * " \
                     " *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4})->" \
                     " *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4}), *(-?\d{1,4}) " \
                     "(-?\d+) (-?\d+) " \
                     "V: *(\d{1,4}), *(\d{1,4}), *(\d{1,4}), *(\d{1,4}) "

skip_pattern = r"SKIP"

error_pattern = r"HWCVAL:E"

### Class Definitions ###

class Parser:
    '''This is the HWC log-file parser. Note that the regular
    expressions have been taken from HwchReplayParser.h
    (i.e. the Replay tool).'''

    def isOnSet(self, line=None):
        '''Matches an onSet Entry (frame) header'''
        match = self.onset_regex.search(line)
        match_1533 = self.onset_1533_regex.search(line)

        if (match):
            return match.groups()
        elif (match_1533):
            return match_1533.groups()

    def isLayerHeader(self, line=None):
        '''Matches an HWC-Next or a 15.33 layer'''
        header = self.layer_hdr_regex.search(line)
        trailer = self.layer_trl_regex.search(line)

        match_1533 = self.layer_1533_regex.search(line)

        if (header and trailer):
            return header.groups() + trailer.groups()
        elif (match_1533):
            return match_1533.groups()

    def isLayerSkip(self, line=None):
        '''Matches a skip layer i.e. a layer that includes SKIP'''
        return self.skip_regex.search(line)

    def isError(self, line=None):
        '''Matches an HWCVAL:E flag'''
        return self.error_regex.search(line)

    def __init__(self):
        '''This function compiles the regular expressions ready for use'''
        self.onset_regex = re.compile(onset_pattern)
        self.layer_hdr_regex = re.compile(layer_hdr_pattern)
        self.layer_vbr_regex = re.compile(layer_vbr_pattern)
        self.layer_trl_regex = re.compile(layer_trl_pattern)

        self.onset_1533_regex = re.compile(onset_1533_pattern)
        self.layer_1533_regex = re.compile(layer_1533_pattern)

        self.skip_regex = re.compile(skip_pattern)
        self.error_regex = re.compile(error_pattern)

class LogFileFrame(tk.Frame):
    '''This class provides the Tk frame for each log file. This class also
    defines the loading mechanism and uses the parser to interpret the log
    correctly.'''

    file_str_default = "No file loaded."
    info_str_default = "(frames: 0, layers: 0, errors: 0, lines: 0)"

    def loadFile(self, window=None, prompt=True):
        '''Loads a file by prompting the user with a File Dialog box. If
        prompt is set to False, then the file is reloaded.'''
        if prompt:
            self.file_name = tkfd.askopenfilename(defaultextension='.log',
                filetypes=(('HWC logs', '*.log'), ('All files', '*.*')))

        if self.file_name:
            file = open(self.file_name, "r")
            if file:
                self.clear(window)
                num_frames = 0
                num_layers = 0
                num_errors = 0
                self.num_lines = 0

                for line in file:
                    self.num_lines += 1
                    window.insert(tk.INSERT,
                        (str(self.num_lines) + ": ").ljust(7), 'line_number')

                    onSetMatch = parser.isOnSet(line)
                    layerMatch = parser.isLayerHeader(line)

                    if (onSetMatch):
                        window.insert(tk.INSERT, line, 'onSet')
                        num_frames += 1
                        seen_onset = True
                    elif (layerMatch and seen_onset):
                        if (layerMatch[1] != "TG" and not parser.isLayerSkip(line)):
                            self.layers.append((self.num_lines, layerMatch))
                        window.insert(tk.INSERT, line, 'layer')
                        num_layers += 1
                    elif (parser.isError(line)):
                        window.insert(tk.INSERT, line, 'error')
                        num_errors += 1
                    else:
                        seen_onset = False
                        window.insert(tk.INSERT, line)

                self.file_str.set(self.file_name)
                self.info_str.set("(frames: {0}, layers: {1}, errors: {2}, lines: {3})"
                    .format(num_frames, num_layers, num_errors, self.num_lines))

    def clear(self, window=None):
        '''Clears a window in preparation for a load or a reload.'''
        window.delete("1.0", tk.END)
        del self.layers[:]
        self.file_str.set(self.file_str_default)
        self.info_str.set(self.info_str_default)

    def createWidgets(self):
        '''Creates a scrollable text window and the widgets for the file-name
        and information string.'''
        self.text_window = tkst.ScrolledText(self)
        self.text_window.tag_config('onSet', foreground='blue')
        self.text_window.tag_config('layer', foreground='green')
        self.text_window.tag_config('error', foreground='red')
        self.text_window.tag_config('highlight', background='yellow')
        self.text_window.tag_config('line_number', foreground='grey')
        self.text_window.pack(side="top", expand=tk.YES, fill='both')

        self.file_str = StringVar()
        self.file_str.set(self.file_str_default)
        file_label = tk.Label(self, textvariable=self.file_str)
        file_label.pack(side="left")

        self.info_str = StringVar()
        self.info_str.set(self.info_str_default)
        info_label = tk.Label(self, textvariable=self.info_str)
        info_label.pack(side="right")

    def __init__(self, master=None):
        '''Initialises some instance variables and creates the widgets.'''
        tk.Frame.__init__(self, master)
        self.file_name = None
        self.layers = []
        self.num_lines = 0
        self.pack(expand=tk.YES, fill='both', side="bottom")
        self.createWidgets()


class MenuBar(tk.Menu):
    '''Defines and handles the menu-bar'''

    def handleCompare(self):
        '''This is the compare function that is called when two files have been loaded
        and are being compared. Note that mismatches are highlighted in the text windows
        as well as being printed to stdout.'''
        for left, right in zip(leftLogFileWindow.layers, rightLogFileWindow.layers):
            if (left[1] != right[1]):
                leftLogFileWindow.text_window.tag_add("highlight",
                    str(left[0]) + ".0", str(left[0]) + ".end")
                rightLogFileWindow.text_window.tag_add("highlight",
                    str(right[0]) + ".0", str(right[0]) + ".end")

                print("Mismatch ({0}, {1}):\n{2}\n{3}"
                      .format(left[0], right[0],
                        leftLogFileWindow.text_window.get(str(left[0]) + ".0",
                            str(left[0]) + ".end"),
                        rightLogFileWindow.text_window.get(str(right[0]) + ".0",
                            str(right[0]) + ".end")))

    def handleHelp(self):
        '''Displays a pop-up dialog box giving information about the tool.'''
        file_name = tkmb.showinfo("About PyReplay", "PyReplay: A Tool for Visualising "
        "Hardware Composer Logs\n\nFor help and support contact:\n\n{0}\n{1}\n\n{2}"
        .format(__maintainer__, __email__, __copyright__))

    def createMenu(self):
        ''' Creates the main menu, which consists of the file menu, an edit menu
        and a help menu.'''

        # Create the file menu (load/reload/compare/exit)
        file_menu = tk.Menu(self, tearoff=0)
        file_menu.add_command(label="Load left", command=lambda:
            leftLogFileWindow.loadFile(leftLogFileWindow.text_window))
        file_menu.add_command(label="Load right", command=lambda:
            rightLogFileWindow.loadFile(rightLogFileWindow.text_window))
        file_menu.add_separator()
        file_menu.add_command(label="Reload left", command=lambda:
            leftLogFileWindow.loadFile(leftLogFileWindow.text_window, False))
        file_menu.add_command(label="Reload right", command=lambda:
            rightLogFileWindow.loadFile(rightLogFileWindow.text_window, False))
        file_menu.add_separator()
        file_menu.add_command(label="Compare layers", command=lambda:
            self.handleCompare())
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=root.quit)
        self.add_cascade(label="File", menu=file_menu)

        # Create and add the edit menu (clear)
        edit_menu = tk.Menu(self, tearoff=0)
        edit_menu.add_command(label="Clear left", command=lambda:
            leftLogFileWindow.clear(leftLogFileWindow.text_window))
        edit_menu.add_command(label="Clear right", command=lambda:
            rightLogFileWindow.clear(rightLogFileWindow.text_window))
        self.add_cascade(label="Edit", menu=edit_menu)

        # Create and add the help menu (about)
        help_menu = tk.Menu(self, tearoff=0)
        help_menu.add_command(label="About", command=lambda:
            self.handleHelp())
        self.add_cascade(label="Help", menu=help_menu)

    def __init__(self, master=None):
        '''Creates the main menu and adds it to the root window.'''
        tk.Menu.__init__(self, master)
        self.createMenu()
        master.config(menu=self)

### Main Code ###

if __name__ == '__main__':
    root = tk.Tk()
    root.title("PyReplay: A Tool for Comparing Hardware Composer Logs")

    # Create the main log file windows
    paned_window = tk.PanedWindow(master=root)
    paned_window.pack(fill='both', expand=1)
    leftLogFileWindow = LogFileFrame(master=paned_window)
    rightLogFileWindow = LogFileFrame(master=paned_window)
    paned_window.add(leftLogFileWindow)
    paned_window.add(rightLogFileWindow)

    # Ensure that the 'sash' remains in the middle when the window is resized
    paned_window.bind("<Configure>", lambda e:
        paned_window.sash_place(0, e.width // 2, e.height)
    )

    # Create the top-level menu
    MenuBar(root)

    # Instantiate the parser
    parser = Parser()

    # The setup is complete - start handling events
    root.mainloop()
