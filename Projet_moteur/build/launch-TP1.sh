#!/bin/sh
bindir=$(pwd)
cd /home/clementsaperes/Documents/Cours/TP_M1/S2/HAI819I_Moteur_de_jeu/Projet_moteur/TP1/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "xYES" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		/usr/bin/gdb -batch -command=$bindir/gdbscript --return-child-result /home/clementsaperes/Documents/Cours/TP_M1/S2/HAI819I_Moteur_de_jeu/Projet_moteur/build/TP1 
	else
		"/home/clementsaperes/Documents/Cours/TP_M1/S2/HAI819I_Moteur_de_jeu/Projet_moteur/build/TP1"  
	fi
else
	"/home/clementsaperes/Documents/Cours/TP_M1/S2/HAI819I_Moteur_de_jeu/Projet_moteur/build/TP1"  
fi
