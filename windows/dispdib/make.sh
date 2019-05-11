#!/bin/bash
rel=../..
if [ x"$TOP" == x ]; then TOP=`pwd`/$rel; fi
. $rel/linux-ow.sh

win30=1 # Windows 3.0
win31=1 # Windows 3.1
winnt=1 # Windows NT
win32=1 # Windows 9x/NT/XP/Vista/etc.
win32s=1 # Windows 3.1 + Win32s
#win386=1 # Windows 3.1 + Watcom win386        <- FIXME: Ok great, it produces some proprietary EXE with an 'MQ' signature, how the fuck do I run it?

if [ "$1" == "clean" ]; then
    do_clean
    rm -fv test.dsk v86kern.map
    exit 0
fi

if [ "$1" == "disk" ]; then
    # boot data disk
    make_msdos_data_disk test.dsk || exit 1
    mcopy -i test.dsk win302l/hello.exe ::he286_30.exe

    mcopy -i test.dsk win313l/hello.exe ::he386_31.exe

    mcopy -i test.dsk win32s3/hello.exe ::hewin32s.exe

    mcopy -i test.dsk win32/hello.exe ::helwin32.exe

    mcopy -i test.dsk winnt/hello.exe ::helwinnt.exe
fi

if [[ "$1" == "build" || "$1" == "" ]]; then
    make_buildlist
    begin_bat

    what=all
    if [ x"$2" != x ]; then what="$2"; fi

    for name in $build_list; do
        do_wmake $name "$what" || exit 1
        bat_wmake $name "$what" || exit 1
    done

    end_bat
fi

