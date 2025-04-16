#!/bin/sh
bindir=$(pwd)
cd /mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/TP1/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/TP1 
	else
		"/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/TP1"  
	fi
else
	"/mnt/c/Users/nighc/OneDrive/Documents/GitHub/Game-Engine-Project/TP1_code/build/TP1"  
fi
