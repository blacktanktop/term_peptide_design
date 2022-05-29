#!/bin/bash
# There is no source command in /bin/sh in ubuntu.
. ../input_files/conf
peptide_design=$peptide_design_dir

bin=../1_makePeptideBin/*bin

#get distances list
ls ../2_computeRMSD/distances*.csv > distances.list
distance_list=distances.list

SECONDS=0
$peptide_design/bin/buildPeptideRMSDMatrix --bin $bin --distance_list $distance_list 2> stderr.txt
ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

