#!/bin/bash
. ../input_files/conf
peptide_design=$peptide_design_dir
#Runs fast relax as per https://www.rosettacommons.org/docs/latest/application_documentation/structure_prediction/relax


protein="../7_rosettaFixbb/XXXXX_fused-path_?????_0001.pdb"#path to structure
nstruct=10 #select the structure with the lowest energy

SECONDS=0

echo $rosetta/relax.default.linuxgccrelease -relax:default_repeats 5 -in:file:s $protein -nstruct $nstruct  
$rosetta/relax.default.linuxgccrelease -relax:default_repeats 5 -in:file:s $protein -nstruct $nstruct  

ELAPSED="Elapsed: $(($SECONDS / 3600))hrs $((($SECONDS / 60) % 60))min $(($SECONDS % 60))sec"
echo $ELAPSED

exit 0
