#!/bin/bash
# There is no source command in /bin/sh in ubuntu.
. ../input_files/conf
peptide_design=$peptide_design_dir

targetPDB=../input_files/${PDBID}.pdb
peptideChainID=${p_chain}
paramsFile=genSeeds.params

SECONDS=0

$peptide_design/bin/generateSeeds --targetPDB $targetPDB --paramsFile $paramsFile --peptideChainID $peptideChainID

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0
