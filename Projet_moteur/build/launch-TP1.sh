#!/bin/sh
bindir=$(pwd)
cd /home/clementsaperes/Documents/Projet_moteur/Game-Engine-Project/Projet_moteur/TP1/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "xYES" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		/usr/bin/gdb -batch -command=$bindir/gdbscript --return-child-result /home/clementsaperes/Documents/Projet_moteur/Game-Engine-Project/Projet_moteur/build/TP1 
	else
		"/home/clementsaperes/Documents/Projet_moteur/Game-Engine-Project/Projet_moteur/build/TP1"  
	fi
else
	"/home/clementsaperes/Documents/Projet_moteur/Game-Engine-Project/Projet_moteur/build/TP1"  
fi
