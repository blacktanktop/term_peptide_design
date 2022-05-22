#!/bin/bash
source ./conf
peptide_design=$peptide_design_dir

# get a list of all peptide structures
ls ../../4_samplePaths/path_structures/*_fused-path_*.pdb > peptideStructures.list
list=peptideStructures.list

SECONDS=0

$peptide_design/bin/buildPeptideRMSDMatrix --list $list 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0
