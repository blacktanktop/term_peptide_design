#!/bin/bash
# There is no source command in /bin/sh in ubuntu.
. ../input_files/conf
peptide_design=$peptide_design_dir

seedBin=../1_generateSeeds/output/extendedfragments.bin
out=${PDBID}_seedGraph.adj
overlaps=../2_findOverlaps/output

SECONDS=0

$peptide_design/bin/buildSeedGraph --seedBin $seedBin --out $out --overlaps $overlaps 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

