#!/bin/bash

MST_install="../../../Mosaist"

p="../4_samplePaths/path_structures/XXXXX_fused-path-and-context_?????.pdb" # the PDB structure of the designed peptide and target protein
c=../input_files/dTERMen.configfile #see Mosaist/fasstDB to construct a fasstDB for dTERMen
s="chain Y" # select the peptide by chain
o="XXXXX" # the name of the complex

SECONDS=0

echo $MST_install/MST/bin/design --p $p --c $c --s "$s" --o $o
srun /scratch/users/swans/MST_workspace/MST/bin/design --p $p --c $c --s "$s" --o $o

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

