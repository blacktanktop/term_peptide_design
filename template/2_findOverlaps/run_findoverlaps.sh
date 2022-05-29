#!/bin/bash
# There is no source command in /bin/sh in ubuntu.
. ../input_files/conf
peptide_design=$peptide_design_dir

worker=$1
numWorkers=10

seedBin=../1_generateSeeds/output/extendedfragments.bin
dir=./output
overlapSize=4
deviation=1.0
minCosAngle=0.5
batchSize=10000

out=$dir/overlaps$1

#make a directory for the output, if it doesn't already exist
if [ ! -d $dir ]; then
        mkdir $dir
fi

SECONDS=0

$peptide_design/bin/findOverlaps --seedBin $seedBin --out $out --overlapSize $overlapSize --deviation $deviation --minCosAngle $minCosAngle --worker $worker  --numWorkers $numWorkers --batchSize $batchSize 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

# excute command example
# seq 10 | parallel --joblog joblog.txt --results std -j 10 --workdir $PWD sh ./run_findoverlaps.sh {}
