#!/bin/bash
# There is no source command in /bin/sh in ubuntu.
. ../input_files/conf
peptide_design=$peptide_design_dir
target=../input_files/${PDBID}_${t_chain}__.pdb
complex=../input_files/${PDBID}.pdb
peptide=${p_chain}
structures=../5_buildPeptideRMSDMatrix/1_makePeptideBin
out=scores
config=../input_files/multichainDB.configfile
worker=$1
numWorkers=10

SECONDS=0

$peptide_design/bin/scoreStructures --target $target --structures $structures --out $out \
--config $config --worker $worker --numWorkers $numWorkers

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0

# excute command example
# seq 10 | parallel --joblog joblog.txt --results std -j 10 --workdir $PWD sh ./run_scoreStructures.sh {}

