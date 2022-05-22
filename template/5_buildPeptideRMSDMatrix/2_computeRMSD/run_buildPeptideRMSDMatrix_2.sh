#!/bin/bash
source ./conf
peptide_design=$peptide_design_dir

bin=../1_makePeptideBin/*bin
numWorkers=1
worker=1

SECONDS=0

$peptide_design/bin/buildPeptideRMSDMatrix --bin $bin --numWorkers $numWorkers --worker $worker 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

