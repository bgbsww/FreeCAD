#!/usr/bin/env python3

# SPDX-License-Identifier: LGPL-2.1-or-later
# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2023 <bgbsww@gmail.com>                                 *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

"""
This utility tries to find all the default settings set in .ui files and the source code, and flag potential conflicts.


Usage:
    uidefaults.py <directory>

"""

import glob
import re
import os
from pprint import pprint as p
import sys
import xml.etree.ElementTree as ET  # Yes, this is vulnerable to bad XML, but we should't have any

def get_ui_files(dir_path='.'):
    uipat = re.compile(".*\.ui")
    files = []
    for (dir_path, dir_names, file_names) in os.walk(dir_path):
        files.extend([ (dir_path,file_name) for file_name in file_names if uipat.match(file_name) ])
    return files

def find_defaults(filelist):
    forms = []
    defaults = []
    for file_path, file_name in filelist:
        whole_name = os.path.join(file_path,file_name)
        xmltree = ET.parse(whole_name)
        xmlroot = xmltree.getroot()
        if xmlroot.tag != 'ui' or float(xmlroot.attrib['version']) != 4.0:
            print(f"Not processing {whole_name} with {xmlroot.tag} of {xmlroot.attrib}: wrong version.")
        classtag = xmlroot.find('class')
        if classtag == None:
            print(f"Not processing {whole_name} without a class name.")
        class_name = classtag.text
        forms.extend((file_path, file_name, class_name))
        # walk the form for default elements
        for widget in xmlroot.iter('widget'):
            for prop in widget.findall('property'):
                if 'name' in prop.attrib and prop.attrib['name'] in ['value','name']:
                    defaults.append((file_path, file_name, class_name, widget.attrib['name'], prop[0].tag, prop[0].text))

    return forms, defaults

def locate_cpp_files(dir, module):
    dialog_name = module.split('::')[-1]
    incs = glob.glob('**/'+dialog_name+'.h',root_dir=dir, recursive=True)
    cpps = glob.glob('**/'+dialog_name+'.cpp',root_dir=dir, recursive=True)
    # Should we use cpps as potentials and double check?  Yes, because we're getting both App and Gui files?
    # Or check if we include the gui files in the app?
    if True: #len(cpps) == 0:
        ui_str = f"ui(new Ui_{dialog_name})"
        if len(cpps) == 0:
            potentials = glob.glob("**/*.cpp",root_dir=dir, recursive=True)
        else:
            potentials = cpps
        cpps = []
        for file_name in potentials:
            with open(os.path.join(dir,file_name)) as f:
                if ui_str in f.read():
                    cpps.append(file_name)
    if len(incs) == 0 and len(cpps) == 0:
        pass
        # These are the ui modules that are likely loaded rather than compiled 
    return (incs,cpps)

def is_equal(val1,val2,type):
    try:
        if not type in ['number','double']:
            sys.stderr.write(f"Unknown property type: {type}\n")
        if type == 'number':
            return int(val1) == int(val2)
        if type == 'double':
            return float(val1) == float(val2)
        return val1 == val2
    except:
        return False
    
def find_value_set_in_cpp_file(filepath,prop,value, type):
    # This RE failed on multiline situations until removed the closing paren, but aren't any anyway.
    code_value_pat = re.compile(".*set(?:Value|CheckState|Text|PlainText)\((.*)")
    code_value_pat2 = re.compile(".*set(?:Value|CheckState|Text|PlainText)\((.*)\)")
    with open(filepath) as f:
        lines = f.readlines()
        for lineno, line in enumerate(lines):
            if prop in line: # and ("setValue(" in line or "setChecked(" in line):
                match = code_value_pat.match(line)
                if match:
                    match2 = code_value_pat2.match(line)
                    if match2:
                        match = match2
                    code_value = match.group(1)
                    eq = is_equal(value,code_value,type)
                    if not eq:
                        print(f"{filepath}:{lineno} {prop}  {value} != {code_value}")


def main(dir='.'):
    files = get_ui_files(dir) # find all ui files
    forms, defaults = find_defaults(files) # for each file, parse XML for defaults
    # p(defaults)
    props = [ f + locate_cpp_files(dir, f[2]) for f in defaults]
    # p(props)
    sys.stderr.write(f"All {len(props)} property defaults in ui files located.   Checking code for bad matches.\n")
    for pathdir, uifile, module, prop, proptype, propvalue, hfiles, cppfiles in props:
        for filepath in cppfiles:
            find_value_set_in_cpp_file(os.path.join(dir,filepath), prop, propvalue, proptype)

if __name__ == "__main__":
    dir = "."
    args = sys.argv[1:]
    if args:
        dir = args[0]
    main(dir)
