#!/bin/bash
source ./conf
peptide_design=$peptide_design_dir

targetPDB=../input_files/1YCR.pdb
peptideChainID=B
seedBin=../1_generateSeeds/output/extendedfragments.bin
seedGraph=../3_buildSeedGraph/1YCR_seedGraph.adj
numPaths=500
minLength=13
config=../input_files/singlechainDB.configfile
base=1YCR

SECONDS=0

$peptide_design/bin/samplePaths --targetPDB $targetPDB --peptideChainID $peptideChainID --seedBin $seedBin --seedGraph $seedGraph \
--numPaths $numPaths --config $config --minLength $minLength \
--base $base --writeTopology --countContacts 2> stderr.txt

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

