#!/bin/bash

s="../4_samplePaths/path_structures/XXXXX_fused-path_?????_aa.pdb"
nstruct=1
resfile=./resfile.txt
extraflags="-ex1 -ex1aro -ex2 -ex2aro"
SECONDS=0
echo $rosetta/fixbb.default.linuxgccrelease -s $s -nstruct $nstruct -resfile $resfile $extraflags
$rosetta/fixbb.default.linuxgccrelease -s $s -nstruct $nstruct -resfile $resfile $extraflags 1>stdout.txt 2>stderr.txt 

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0
