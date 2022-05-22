#!/bin/bash
source ./conf
peptide_design=$peptide_design_dir

seedBin=../1_generateSeeds/output/extendedfragments.bin
out="1YCR_seedGraph.adj"
overlaps=../2_findOverlaps/output

SECONDS=0

$peptide_design/bin/buildSeedGraph --seedBin $seedBin --out $out --overlaps $overlaps 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

