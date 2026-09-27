#!/usr/bin/python

from __future__ import with_statement

import sys
import os

drivlist = []
extra_drivlist = []

def parse_file(srcfile):
    try:
        fp = open(srcfile, 'rb')
    except IOError:
        sys.stderr.write("Unable to open source file '%s'\n" % srcfile)
        return 1
    in_comment = 0
    linenum = 0
    for line in fp.readlines():
        drivname = ''
        linenum+=1
        srcptr = 0
        while srcptr < len(line):
            c = line[srcptr]
            srcptr+=1
            if c==13 or c==10:
                if c==13 and line[srcptr]==10:
                    srcptr+=1
                continue
            if c==' ':
                continue
            if in_comment==1 and c=='*' and line[srcptr]=='/' :
                srcptr+=1
                in_comment = 0
                continue
            if in_comment:
                continue
            if c=='/' and line[srcptr]=='*' :
                srcptr+=1
                in_comment = 1
                continue
            if c=='/' and line[srcptr]=='/' :
                break
            if c=='#' and (line[srcptr]==' ' or line[srcptr]=='|') :
                break
            drivname += c
        drivname = drivname.strip()
        if len(drivname)>0:
            if drivname[0]=='#':
               sys.stderr.write("Importing drivers from '%s'\n" % drivname[1:])
               parse_file(drivname[1:])
               continue
            if not all(((c >='a' and c<='z') or (c>='0' and c<='9') or c=='_') for c in drivname):
               sys.stderr.write("%s:%d - Invalid character in driver \"%s\"\n" % (srcfile,  linenum,  drivname))
               return 1
            else:			   
               drivlist.append(drivname)
               extra_drivlist.append(drivname)
    return 0


if len(sys.argv) < 3:
    print('Usage:')
    print('  makelist <is_driver_switch> <source.lst>')
    sys.exit(0)

header_outputed = False
if sys.argv[1].isdigit() :
    is_driver_switch = int(sys.argv[1])
else:
    is_driver_switch = 0
for i in range(2, len(sys.argv)-1):
    filename = sys.argv[i]
    name, ext = os.path.splitext(os.path.basename(filename))
    extra_drivlist = []
    sys.stderr.write("%s\n" % filename)
    if parse_file(filename) :
        sys.exit(1)

    if is_driver_switch != 0 :
        # output a count
        if (len(extra_drivlist)==0) :
            sys.stderr.write("No drivers found\n")
            sys.exit(1)

        # add a reference to the ___empty driver
        extra_drivlist.append("___empty")

        # start with a header
        if not header_outputed :
            print('#include "emu.h"\n');
            print('#include "drivenum.h"\n');
            header_outputed = True

        #output the list of externs first
        for drv in sorted(extra_drivlist):
            print("GAME_EXTERN(%s);" % drv)
        print("")

        # then output the array
        print("const game_driver * const driver_switch::%sdrivers[%d] =" % (name, len(extra_drivlist)+1))
        print("{")
        for drv in sorted(extra_drivlist):
            print("\t&GAME_NAME(%s)," % drv)
        print("0");
        print("};");
        print("");

# output a count
if len(drivlist)==0 :
    sys.stderr.write("No drivers found\n")
    sys.exit(1)

sys.stderr.write("%d drivers found\n" % len(drivlist))

# add a reference to the ___empty driver
drivlist.append("___empty")

# start with a header
if not header_outputed :
    print('#include "emu.h"\n')
    print('#include "drivenum.h"\n')
    header_outputed = True

#output the list of externs first
for drv in sorted(drivlist):
    print("GAME_EXTERN(%s);" % drv)
print("")

# then output the array
if is_driver_switch != 0 :
    print("const game_driver * driver_list::s_drivers_sorted[%d] =" % len(drivlist))
else:
    print("const game_driver * const driver_list::s_drivers_sorted[%d] =" % len(drivlist))
print("{")
for drv in sorted(drivlist):
    print("\t&GAME_NAME(%s)," % drv)
print("};")
print("")

# also output a global count
print("int driver_list::s_driver_count = %d;\n" % len(drivlist))
