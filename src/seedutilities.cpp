//
//  seedutilities.cpp
//  TPD_dummytarget
//
//  Created by Sebastian Swanson on 4/30/20.
//  Copyright © 2020 Sebastian Swanson. All rights reserved.
//

#include "seedutilities.h"

/* --------- generateRandomSeed --------- */

generateRandomSeed::generateRandomSeed(const string& dbFile, int _max_len) : classifier() {
  readDatabase(dbFile,1);
  max_len = _max_len;
  
  //build windows
  for (int l = 1; l <= max_len; l++) {
    windows.insert(map<int,vector<pair<int,int>>>::value_type(l,vector<pair<int,int>>()));
  }
  
  int target_num = numTargets();
  for (int target_id = 0; target_id < target_num; target_id++) {
    Structure* target = getTarget(target_id);
    for (int l = 1; l <= max_len; l++) {
      int window_num = target->residueSize() - l + 1;
      for (int res_id = 0; res_id < window_num; res_id++) {
        windows[l].push_back(make_pair(target_id,res_id));
      }
    }
  }
};

pair<Structure*,string> generateRandomSeed::getSeed(int len) {
  if (windows.find(len) == windows.end()) MstUtils::error("Seed length must be in [1,max_length] ("+MstUtils::toString(max_len)+")");
  //sample a random window
  int window_id = MstUtils::randInt(windows[len].size());
  int target_id = windows[len][window_id].first;
  int nterm_res_id = windows[len][window_id].second;
  
  //copy the residues from the target protein
  Structure* target = getTarget(target_id);
  Structure* seed = new Structure;
  Chain* C = seed->appendChain("0",false);
  vector<int> res_idx; //for getting the secondary structure in the next section
  for (int i = 0; i < len; i++) {
    res_idx.push_back(nterm_res_id+i);
    Residue* R = new Residue(target->getResidue(res_idx[i]));
    C->appendResidue(R);
  }
  
  //get secondary structure
  string sec_structure = classifier.classifyResInStruct(target,res_idx);
  
  //name the new seed
  //targetid_resid_length
  string seed_name = MstUtils::toString(target_id) + "_" + MstUtils::toString(nterm_res_id) + "_" + MstUtils::toString(len);
  seed->setName(seed_name);
  
  return make_pair(seed,sec_structure);
}

/* --------- structureBoundingBox --------- */

structureBoundingBox::structureBoundingBox(Chain * C, mstreal _pad) : pad(_pad){
  Structure Chain_structure = *C;
  AtomPointerVector structure_atoms = RotamerLibrary::getBackbone(Chain_structure);
  construct_structureBoundingBox(structure_atoms);
}

structureBoundingBox::structureBoundingBox(vector<Residue*> residues, mstreal _pad) : pad(_pad){
  vector<Atom*> atoms;
  for (Residue* R: residues) {
    if (!RotamerLibrary::hasFullBackbone(R)) MstUtils::error("Provided residue is missing backbone atoms");
    vector<Atom*> R_bb_atoms = RotamerLibrary::getBackbone(R);
    atoms.insert(atoms.end(),R_bb_atoms.begin(),R_bb_atoms.end());
  }
  construct_structureBoundingBox(atoms);
}

void structureBoundingBox::construct_structureBoundingBox(AtomPointerVector atoms) {
  cout << "Trying to construct bounding box containing all seed centroids..." << endl;
  xlo = INFINITY;
  ylo = INFINITY;
  zlo = INFINITY;
  xhi = -INFINITY;
  yhi = -INFINITY;
  zhi = -INFINITY;
  
  cout << "Backbone with " << atoms.size() << " number atoms" << endl;
  for (Atom* A : atoms) {
    if (A->getX() < xlo) xlo = A->getX();
    if (A->getX() > xhi) xhi = A->getX();
    if (A->getY() < ylo) ylo = A->getY();
    if (A->getY() > yhi) yhi = A->getY();
    if (A->getZ() < zlo) zlo = A->getZ();
    if (A->getZ() > zhi) zhi = A->getZ();
  }
  
  //add the pad to each boundary
  xlo -= pad; xhi += pad;
  ylo -= pad; yhi += pad;
  zlo -= pad; zhi += pad;
  
  cout << "Bounding box complete with the following boundaries: ";
  cout << "Xlo: " << xlo << ", Xhi: " << xhi << "\t";
  cout << "Ylo: " << ylo << ", Yhi: " << yhi << "\t";
  cout << "Zlo: " << zlo << ", Zhi: " << zhi << "\t";
  cout << endl;
}

/* --------- naiveSeedsfromBin --------- */
naiveSeedsFromBin::naiveSeedsFromBin(Structure& S, string p_id) : complex(S){
  //get target structure
  peptide = complex.getChainByID(p_id);
  vector<Residue*> target_residues;
  for (Residue* R : complex.getResidues()) if (R->getChainID() != p_id) target_residues.push_back(R);
  target = Structure(target_residues);
  
  //generate proximity search of target backbone
  cout << "Extracting target backbone to construct a promixity search object..." << endl;
  if (!RotamerLibrary::hasFullBackbone(target)) cout << "warning: target structure is missing backbone atoms!" << endl;
  target_BB_atoms = RotamerLibrary::getBackbone(target);
  target_BB_structure = Structure(target_BB_atoms);
  target_PS = new ProximitySearch(target_BB_atoms, vdwRadii::maxSumRadii()/2);
  
  max_attempts = 10;
}

void naiveSeedsFromBin::newPose(string seedBinaryPath_in, string output_path, string out_name, bool position, bool orientation, vector<Residue*> binding_site) {
  
  if ((position == false) && (orientation == false)) MstUtils::error("Neither position or orientation have been selected to be randomized. Nothing to do!");
  cout << "Will randomize: ";
  if (position) cout << "position\t";
  if (orientation) cout << "orientation";
  cout << endl;
  //construct bounding box
  structureBoundingBox bounding_box = structureBoundingBox(peptide); //only use if position is randomized
  
  StructuresBinaryFile bin(seedBinaryPath_in);
  
  //prepare for writing new seed binary file
  string seedBinaryPath_out = output_path + "/" + out_name + ".bin";
  fstream bin_out;
  MstUtils::openFile(bin_out, seedBinaryPath_out, fstream::out | fstream::binary, "naiveSeedsFromBin::newPose");
  
  //open seed data file
  string seedRetry_out = output_path + "/" + out_name + "_newpose_attempts.out";
  fstream retry_out;
  MstUtils::openFile(retry_out, seedRetry_out, fstream::out, "naiveSeedsFromBin::newPose");
  //header
  retry_out << "name\tattempts" << endl;
  
  //randomize seeds and write to file
  int count = 0;
  while (bin.hasNext()) {
    Structure* extended_fragment = bin.next();
    Chain* seed_C = extended_fragment->getChainByID("0");
    Structure* seed_structure = new Structure(*seed_C);
    CartesianPoint seed_centroid = AtomPointerVector(seed_structure->getAtoms()).getGeometricCenter();
    
    //randomize position/orientation
    int attempts = transform(seed_structure, bounding_box, position, orientation, seed_centroid);
    
    //write transformed seed to binary file
    seed_structure->setName(extended_fragment->getName());
    seed_structure->writeData(bin_out);
    
    //write number of attempts to info file
    retry_out << seed_structure->getName() << "\t" << attempts << endl;
    
    delete extended_fragment;
    delete seed_structure;
    count++;
  }
  int num_seeds = bin.structureCount();
  cout << "There are " << num_seeds << " seeds in the input binary file. After randomization, there are " << count << " seeds in the output binary file" << endl;
  bin_out.close();
  retry_out.close();
}


int naiveSeedsFromBin::transform(Structure* seed, structureBoundingBox& bounding_box, bool position, bool orientation, CartesianPoint new_centroid) {
  //make a copy of the atoms in the structure, in case multiple transformations must be sampled
  vector<Atom*> original_seed_atoms = seed->getAtoms(); vector<Atom*> original_seed_atoms_copy;
  for (Atom* A : original_seed_atoms) {
    Atom* A_copy = new Atom(A);
    original_seed_atoms_copy.push_back(A_copy);
  }
  
  TransformFactory tf;
  //initial transform to bring the seed centroid to the origin
  CartesianPoint seed_original_centroid = AtomPointerVector(original_seed_atoms_copy).getGeometricCenter();
  CartesianPoint centroid_translate = CartesianPoint(0,0,0) - seed_original_centroid; //this vector should cancel with seed_original_centroid
  Transform translate_to_origin = tf.translate(centroid_translate);
  
  //construct all other transforms
  //only used if orientation is selected
  Transform rotate_around_x, rotate_around_y, rotate_around_z;
  
  //only used if translation is selected
  Transform translate_to_bounding_box_origin;
  Transform translate_into_box;
  
  //only used if translation is not selected
  Transform translate_to_centroid;
  
  //final transform
  Transform sample_new_pose;
  
  int attempts = 0;
  bool seed_clash = true;
  while (seed_clash) {
    if (attempts > 0) {
      generalUtilities::copyAtomCoordinates(seed, original_seed_atoms_copy);
    }
    attempts++;
    
    if (orientation) {
      //generate a random rotation
      mstreal x_angle = MstUtils::randUnit() * 360;
      mstreal y_angle = MstUtils::randUnit() * 360;
      mstreal z_angle = MstUtils::randUnit() * 360;
      rotate_around_x = tf.rotateAroundX(x_angle);
      rotate_around_y = tf.rotateAroundY(y_angle);
      rotate_around_z = tf.rotateAroundZ(z_angle);
    }
    
    //generate the translation
    if (position) {
      //randomize
      translate_to_bounding_box_origin = tf.translate(bounding_box.xlo, bounding_box.ylo, bounding_box.zlo);
      mstreal x_pos = MstUtils::randUnit() * (bounding_box.xhi - bounding_box.xlo);
      mstreal y_pos = MstUtils::randUnit() * (bounding_box.yhi - bounding_box.ylo);
      mstreal z_pos = MstUtils::randUnit() * (bounding_box.zhi - bounding_box.zlo);
      translate_into_box = tf.translate(x_pos,y_pos,z_pos);
    } else {
      //to the provided centroid
      translate_to_centroid = tf.translate(new_centroid);
    }
    
    //combine all transformations
    if (position && orientation) {
      sample_new_pose = translate_into_box * translate_to_bounding_box_origin * rotate_around_z * rotate_around_y * rotate_around_x * translate_to_origin; //matrix multiplication progresses right to left.... right? lol
    } else if (!position && orientation) {
      sample_new_pose = translate_to_centroid * rotate_around_z * rotate_around_y * rotate_around_x * translate_to_origin;
    } else if (position && !orientation) {
      sample_new_pose = translate_into_box * translate_to_bounding_box_origin * translate_to_origin;
    }
    sample_new_pose.apply(seed);
    
    //check if seed clashes
    AtomPointerVector transformed_seed_atoms = seed->getAtoms();
    seed_clash = isClash(*target_PS, target_BB_atoms, transformed_seed_atoms); //from structgen
    if (attempts > max_attempts) seed_clash = false;
    if (seed_clash) {
      //reset transformations
      sample_new_pose.makeIdentity();
      if (orientation) {
        rotate_around_x.makeIdentity(); rotate_around_y.makeIdentity(); rotate_around_z.makeIdentity();
      }
      if (position) {
        translate_to_bounding_box_origin.makeIdentity(); translate_into_box.makeIdentity();
      } else {
        translate_to_centroid.makeIdentity();
      }
    }
  }
  for (Atom* A_copy : original_seed_atoms_copy) delete A_copy; //the atom copies are no longer necessary
  return attempts;
}


/* --------- naiveSeedsfromDB --------- */

void naiveSeedsFromDB::newPose(string seedBinaryPath_in, string output_path, string out_name, bool position, bool orientation, vector<Residue*> binding_site) {
  
  if ((position == false) && (orientation == false)) MstUtils::error("Neither position or orientation have been selected to be randomized. Nothing to do!");
  cout << "Will randomize: ";
  if (position) cout << "position\t";
  if (orientation) cout << "orientation";
  cout << endl;
  
  //construct bounding box
  structureBoundingBox bounding_box = structureBoundingBox(peptide);
  //  structureBoundingBox bounding_box = structureBoundingBox(binding_site);
  StructuresBinaryFile bin(seedBinaryPath_in);
  
  //open new seed binary file
  string seedBinaryPath_out = output_path + "/" + out_name + ".bin";
  fstream bin_out;
  MstUtils::openFile(bin_out, seedBinaryPath_out, fstream::out | fstream::binary, "Fragmenter::writeExtendedFragmentsBin");
  
  //open seed data file
  string seedRetry_out = output_path + "/" + out_name + "_newpose_attempts.out";
  fstream retry_out;
  MstUtils::openFile(retry_out, seedRetry_out, fstream::out, "naiveSeedsFromBin::newPose");
  //header
  retry_out << "name\tattempts" << endl;
  
  //open seed secondary structure file
  string seedSecStruct_out = output_path + "/" + out_name + "_secondary_structure.out";
  fstream secstruct_out;
  MstUtils::openFile(secstruct_out, seedSecStruct_out, fstream::out, "naiveSeedsFromBin::newPose");
  //header
  secstruct_out << "name\tsecondary_structure" << endl;
  
  //randomize seeds and write to file
  int count = 0;
  while (bin.hasNext()) {
    //read original seed from binary file
    Structure* extended_fragment = bin.next();
    Chain* seed_C = extended_fragment->getChainByID("0");
    CartesianPoint seed_original_centroid = AtomPointerVector(seed_C->getAtoms()).getGeometricCenter();
    int seed_length = seed_C->residueSize();
    
    //sample new seed from DB
    pair<Structure*,string> seed_data = seedSampler.getSeed(seed_length);
    Structure* new_seed = seed_data.first;
    
    //randomize position/orientation
    int attempts = transform(new_seed, bounding_box, position, orientation, seed_original_centroid);
    
    //write seed to new binary file
    new_seed->writeData(bin_out);
    
    //write attempts to info file
    retry_out << new_seed->getName() << "\t" << attempts << endl;
    
    //write secondary structure
    secstruct_out << new_seed->getName() << "\t" << seed_data.second << endl;
    
    delete extended_fragment;
    delete new_seed;
    count++;
  }
  int num_seeds = bin.structureCount();
  cout << "There are " << num_seeds << " seeds in the input binary file. After randomization, there are " << count << " seeds in the output binary file" << endl;
  bin_out.close();
  retry_out.close();
  secstruct_out.close();
};

/* --------- seedStatistics --------- */

seedStatistics::seedStatistics(Structure& S, string p_id) : complex(S) {
  neigborhood = 10.0;
  
  //get target structure
  peptide = complex.getChainByID(p_id);
  vector<Residue*> target_residues;
  for (Residue* R : complex.getResidues()) if (R->getChainID() != p_id) target_residues.push_back(R);
  target = Structure(target_residues);

  //generate proximity search of target backbone
  cout << "Extracting target backbone to construct a promixity search object..." << endl;
  if (!RotamerLibrary::hasFullBackbone(target)) cout << "warning: target structure is missing backbone atoms!" << endl;
  target_BB_atoms = RotamerLibrary::getBackbone(target);
  target_BB_structure = Structure(target_BB_atoms);
  target_PS = new ProximitySearch(target_BB_atoms, neigborhood/2);
}

void seedStatistics::writeStatisticstoFile(string seedBinaryPath_in, string output_path, string output_name, int num_final_seeds) {
  
  //open seed info file
  string seed_info = output_path + "/" + output_name + "_statistics.info";
  fstream seed_out;
  MstUtils::openFile(seed_out, seed_info, fstream::out, "seedStatistics::writeStatisticstoFile");
  //header
  seed_out << "name\tbounding_sphere_radius\tmin_distance_centroid_protein\tseed_length" << endl;
  
  StructuresBinaryFile bin_file(seedBinaryPath_in);
  
  long num_seeds = bin_file.structureCount();
  mstreal skip_probability = 1 - min(mstreal(1),mstreal(num_final_seeds)/num_seeds);
  cout << "There are " << num_seeds << " seeds in the input binary file. Skip probability is " << skip_probability << endl;
  
  int count = 0;
  while (bin_file.hasNext() == true) {
    mstreal sampled_value = MstUtils::randUnit();
    if (sampled_value <= skip_probability) {
      bin_file.skip();
      continue;
    }
    Structure* extfrag = bin_file.next();
    Chain* seed_C = extfrag->getChainByID("0");
    Structure* seed = new Structure(*seed_C);
    mstreal bounding_sphere_radius = boundingSphereRadius(seed);
    mstreal distance = centroid2NearestProteinAtom(seed);
    seed_out << extfrag->getName() << "\t" << bounding_sphere_radius << "\t" << distance << "\t" << seed->residueSize() << endl;
    delete extfrag;
    delete seed;
    count++;
  }
  cout << "In the end, wrote the info for " << count << " seeds" << endl;
  seed_out.close();
}

mstreal seedStatistics::boundingSphereRadius(Structure *seed) {
  vector<Atom*> seed_atoms = seed->getAtoms();
  CartesianPoint centroid = AtomPointerVector(seed_atoms).getGeometricCenter();
  mstreal max_distance = 0;
  for (Atom* A : seed_atoms) {
    mstreal new_distance = centroid.distance(A);
    if (new_distance > max_distance) max_distance = new_distance;
  }
  return max_distance;
}

mstreal seedStatistics::centroid2NearestProteinAtom(Structure* seed) {
  vector<Atom*> seed_atoms = seed->getAtoms();
  CartesianPoint centroid = AtomPointerVector(seed_atoms).getGeometricCenter();
  
  //get nearby points
  vector<int> closest_points;
  mstreal max_d = neigborhood;
  while (closest_points.empty()) {
    closest_points = target_PS->getPointsWithin(centroid,0.0,max_d);
    max_d = max_d*2;
    if (max_d > 80.0) MstUtils::error("Dude this seed is nowhere near the protein...");
  }
  
  //find the closest one;
  mstreal min_distance = INFINITY;
  for (int i : closest_points) {
    Atom* A = target_BB_atoms[i];
    mstreal new_distance = centroid.distance(A);
    if (new_distance < min_distance) min_distance = new_distance;
  }
  return min_distance;
}
