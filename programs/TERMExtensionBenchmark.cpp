//
// TERMExtensionBenchmark.cpp
//  TPD_target
//
//  Created by Sebastian Swanson on 2/24/19.
//


//mst dependencies
#include "mstsystem.h"
#include "mstoptions.h"

//tpd dependencies
#include "utilities.h"
#include "coverage.h" //to directly assess coverage
#include "benchmarkutilities.h"
#include "termextension.h"
#include "secondarystructure.h"

int main(int argc, char *argv[]) {
  MstOptions op;
  op.setTitle("Generates structural fragments that are potentially compatible with the surface of a target protein binding site through TERM Extension. We refer to these fragments as 'seeds'. These seeds are mapped to the peptide and various metrics are reported.");
  op.addOption("pdb", "Structure file containing the whole complex", true);
  op.addOption("peptide", "Peptide chain ID", true);
  op.addOption("flanking_res","The number of residues flanking a contact to include when creating a fragment.", false);
  op.addOption("max_rmsd", "The max RMSD threshold used when determining whether a seed aligns to the peptide or not.",false);
  op.addOption("config","Path to the configuration file (specifies fasst database and rotamer library)",true);
  op.addOption("hist","Path to the histogram file with the seed distance distribution that will be matched in the null model seeds");
  op.addOption("match_req", "The fragmenter will attempt to create the largest (ranked by number of residues) fragments that have at least this many matches. During TERM Extension, even if the fragment has more than this number match_num_req matches, only this number will be used to generate seeds.  If not defined, defaults to CEN_RES.");
  op.addOption("bin_path", "If a binary file of extended fragment structures already exists, will skip creating fragments/extending them and just compare the provided ones to the peptide. This assumes the null model binary files are also in the same directory. e.g. /path/to/bins/");
  op.setOptions(argc, argv);
  
  /*
   Set up:
   Import user input, set up basic parameters, make directories
   */
  MstTimer timer;
  
  // Variables set at the time of compilation
  int max_seed_length = 10; //this doesn't limit seed length, but limits the length of kmers that are compared between the peptide and a seed
  int num_final_seeds = 100000; //this is approximately the number of seeds that will be sampled when writing line clouds

  // Variables provided by user
  configFile config(op.getString("config"));
  string p_cid = op.getString("peptide");
  int flanking_res = op.getInt("flanking_res",2);
  mstreal max_rmsd = op.getReal("max_rmsd",2.0);
  Structure complex(op.getString("pdb"));
  
  // Set the sequence of the peptide to "unknown"
  selector sel(complex);
  vector<Residue*> peptide_res = sel.selectRes("chain "+p_cid);
  cout << "Selected " << peptide_res.size() << " peptide residues to be renamed to 'UNK'" << endl;
  for (Residue* R : peptide_res) {
    R->setName("UNK");
  }
  
  // Make directories
  bool makeParents = true;
  // Fragment output folder
  string outDir = "termextension_output/";
  string covDir = "coverage_output/";
  MstSys::cmkdir(outDir,makeParents);
  MstSys::cmkdir(covDir,makeParents);
  
  /*
   Main program:
   1) TERM extension to generate seeds (if not provided)
   2) Generate naive seeds, randomize orientations and position/orientation
   3) Assess coverage by both sets of seeds
   4) Write out the seed coordinates for visualization
   */
  
  // Write out the secondary structure of each residue in the complex
  fstream out;
  string output_path = "secondarystructure.tsv";
  MstUtils::openFile(out, output_path, fstream::out);
  
  secondaryStructureClassifier classifier;
  classifier.writeResClassificationtoTSV(complex, out);
  out.close();
  
  //Initialize the interface coverage class now, so that the binding site residues can be used in TERM Extension (if necessary).
  interfaceCoverage IC(complex, p_cid, max_rmsd, max_seed_length, config.getRL());
  vector<Residue*> bindingSiteRes = IC.getBindingSiteRes();
  
  //TERM Extension
  if (!op.isGiven("bin_path")) {
    cout << "No extended fragments binary provided, will generate anew" << endl;
    
    TermExtension TE(config.getDB(), config.getRL(), bindingSiteRes);
    TE.setFlankingNumber(flanking_res);
    if (op.isGiven("match_req")) {
      TE.setMatchReq(op.getInt("match_req"));
      TE.generateFragments(TermExtension::MATCH_NUM_REQ);
    }
    else TE.generateFragments(TermExtension::CEN_RES);
    TE.extendFragmentsandWriteStructures(seedTERM::MANY_CONTACT,outDir);
    
    //write fragments
    cout << "Writing fragment pdbs..." << endl;
    TE.writeFragmentPDBs(outDir);
    TE.writeFragmentClassification(outDir);
  }
  
  //Randomize seeds
  /*
   Two types of randomized seeds
   Type 1) randomized orientation, position sampled from previous seed
   Type 2) randomized position and orientation
   */
  string bin_path = op.getString("bin_path",outDir);
  string extfrag_bin = bin_path + "extendedfragments.bin";
  string type1_name = "type1_seeds";
  string type1_bin = bin_path + "type1_seeds.bin";
  string type2_name = "type2_seeds";
  string type2_bin = bin_path + "type2_seeds.bin";
  bool position = false;
  bool orientation = true;
  
  
  if (op.isGiven("hist") && !op.isGiven("bin_path")) {
    cout << "generating null model seeds..." << endl;
    string hist_file = op.getString("hist");
    
    naiveSeedsFromDB naiveSeeds(complex, p_cid, config.getDB(), extfrag_bin, hist_file);
    
    timer.start();
    naiveSeeds.newPose(outDir, type1_name, position, orientation);
    timer.stop();
    cout << "Generated type 1 seeds in " << timer.getDuration() << " seconds" << endl;
    
    position = true;
    timer.start();
    naiveSeeds.newPose(outDir, type2_name, position, orientation);
    timer.stop();
    cout << "Generated type 2 seeds in " << timer.getDuration() << " seconds" << endl;
  }
  
  //Write Info
  seedStatistics stats(complex, p_cid);
  //Extended fragments
  stats.writeStatisticstoFile(extfrag_bin, outDir, "extended_fragments", num_final_seeds);
  
  if (op.isGiven("bin_path") || op.isGiven("hist")) {
    //Type 1 seeds
    stats.writeStatisticstoFile(type1_bin, outDir, type1_name, num_final_seeds);
    //Type 2 seeds
    stats.writeStatisticstoFile(type2_bin, outDir, type2_name, num_final_seeds);
  }
  
  if (!op.isGiven("bin_path")) {
    
    string lcloud_out;
    cout << "Writing a line cloud file (TERM Extension) for visualization" << endl;
    lcloud_out = outDir + "termext_seed_ca.lcloud";
    MstUtils::openFile(out, lcloud_out, fstream::out);
    classifier.writeCaInfotoLineFile(extfrag_bin, num_final_seeds, out, 1);
    out.close();
    
    if (op.isGiven("hist")) {
      cout << "Writing a line cloud file (type 1) for visualization" << endl;
      lcloud_out = outDir + "type1_seed_ca.lcloud";
      MstUtils::openFile(out, lcloud_out, fstream::out);
      classifier.writeCaInfotoLineFile(type1_bin, num_final_seeds, out, 1);
      out.close();
      
      cout << "Writing a line cloud file (type 2) for visualization" << endl;
      lcloud_out = outDir + "type2_seed_ca.lcloud";
      MstUtils::openFile(out, lcloud_out, fstream::out);
      classifier.writeCaInfotoLineFile(type2_bin, num_final_seeds, out, 1);
      out.close();
    }
  }
  

  //Map the seed coverage
  /*
   Vary parameters that filter the seeds
   - match RMSD. The RMSD of the match that generated the seed (depends on the complexity of the fragment)
   - sequence constraint. When applied, only matches with the same residue at the central position are accepted
   */
  
  vector<mstreal> match_rmsds = {0.2,0.4,0.6,0.8,1.0,1.2};
  vector<bool> seq_consts = {false,true};
  
  for (mstreal match_rmsd : match_rmsds) {
    for (bool seq_const : seq_consts) {
      cout << "sequence constraint: " << seq_const << endl;
      cout << "match_rmsd_cutoff: " << match_rmsd << endl;
      //make directory for this output
      string dir_name = "params_"+MstUtils::toString(match_rmsd)+"_"+MstUtils::toString(seq_const)+"/";
      string path_to_covDir = covDir+dir_name;
      MstSys::cmkdir(path_to_covDir);
      
      IC.setSeqConst(seq_const);
      IC.setMatchRMSDConst(match_rmsd);
      
      cout << "Search for segments of seed chains (TERM Extension) that map to the peptide..." << endl;
      IC.findCoveringSeeds(extfrag_bin);
      cout << "Write coverage to files..." << endl;
      IC.writePeptideResidues(path_to_covDir+"termext_");
      IC.writeContacts(path_to_covDir+"termext_");
      IC.writeAllAlignedSeedsInfo(path_to_covDir+"termext_");
      IC.writeBestAlignedSeeds(path_to_covDir+"termext_",10);
      
      if (op.isGiven("bin_path") || op.isGiven("hist")) {
        //Type 1 seeds
        cout << "Search for segments of seed chains (randomized) that map to the peptide..." << endl;
        IC.findCoveringSeeds(type1_bin);
        cout << "Write coverage to files..." << endl;
        IC.writePeptideResidues(path_to_covDir+"type1_");
        IC.writeContacts(path_to_covDir+"type1_");
        IC.writeBestAlignedSeeds(path_to_covDir+"type1_",10);
        
        //Type 2 seeds
        cout << "Search for segments of seed chains (randomized) that map to the peptide..." << endl;
        IC.findCoveringSeeds(type2_bin);
        cout << "Write coverage to files..." << endl;
        IC.writePeptideResidues(path_to_covDir+"type2_");
        IC.writeContacts(path_to_covDir+"type2_");
        IC.writeBestAlignedSeeds(path_to_covDir+"type2_",10);
      }
    }
  }
  cout << "done" << endl;
  return 0;
}
