//
//  findPaths.cpp
//  DummyTarget
//
//  Created by Venkatesh Sivaraman on 4/22/19.
//

#include <stdio.h>
#include "findfuseable.h"
#include "mstsystem_exts.h"
#include "structure_iter.h"
#include "seedgraph.h"
#include "seedscore.h"
#include "pathsampler.h"
#include "findpaths.h"
#include "utilities.h"
#include <unordered_set>

using namespace std;
using namespace std::chrono;

/**
 * Helper function that computes the corroboration score of a given path. The
 * corroboration score is the number of unique target residues that were used
 * as sources for the seeds that gave rise to the residues in the path.
 *
 * @param path the path to score
 * @param nearbyThreshold the number of residues to either side of each target
 *  residue to ignore in the final count. For example, if nearbyThreshold is 2
 *  and the target residue A53 is discovered, residues A51-A55 will be prevented
 *  from counting toward the corroboration score.
 */
int corroborationScore(PathResult &path, int nearbyThreshold) {
    int score = 0;
    vector<pair<string, int>> targetResidues;
    for (Residue *res: path.getOriginalResidues()) {
        string seedName = res->getStructure()->getName();
        auto code = getTargetResidueCode(seedName);
        targetResidues.push_back(code);
    }

    // Put target residues in sorted order before eliminating close neighbors
    sort(targetResidues.begin(), targetResidues.end(), [](const pair<string, int> &p1, const pair<string, int> &p2) {
        if (p1.first.compare(p2.first) < 0)    
            return true;
        return p1.second < p2.second;
    });
    unordered_set<pair<string, int>, pair_hash> coveredResidues;
    for (auto code: targetResidues) {
        if (coveredResidues.count(code) != 0) {
            continue;
        }
        score++;
        for (int i = code.second - nearbyThreshold; i < code.second + nearbyThreshold + 1; i++) {
            coveredResidues.insert(make_pair(code.first, i));
        }
    }

    return score;
}

int main (int argc, char *argv[]) {
    
    // Get command-line arguments
    MstOptions opts;
    opts.setTitle("Samples random paths from a seed graph/cluster tree and fuses the residues together into a peptide backbone structure.");
    opts.addOption("complex", "Path to a PDB structure file containing the target protein", true);
    opts.addOption("peptideChain", "Chain ID for the peptide chain in the target, if one exists - it will be removed (default false)", false);
    opts.addOption("seeds", "Path to a binary file containing seed structures", false);
    opts.addOption("seedChain", "Chain ID for the seed structures (default '0')", false);
    opts.addOption("seedGraph", "Path to a text file defining a seed graph", true);
    opts.addOption("numPaths", "Number of paths to generate. (default 200)", false);
    opts.addOption("minLength", "The minimum residue length of the sampled paths. (default 15)", false);
    opts.addOption("reqSeed", "The name of a seed in the binary file that all paths should extend from",false);
    opts.addOption("reqSeedSel", "A selection that specifies the residues in reqSeed that should always be included in sampled paths. Must be a continuous range: e.g. resid 3-5. (note: 'chain 0' is always assumed)",false);
    opts.addOption("fixedSeed", "If residues from the specified seed are included in a path, they will be fixed during fusing.",false);
    opts.addOption("ss", "Preferred secondary structure for paths (H, E, or O)", false);
    opts.addOption("score_paths", "Instead of sampling new paths from the graph, samples pre-defined paths, and scores. path format:q: seed_A:residue_i;seed_B:residue_j;etc...", false);
    opts.addOption("score_structures", "Instead of sampling new paths from the graph, loads structures, and scores.", false);
    opts.addOption("config", "The path to a configfile",true);
    opts.addOption("base", "Prepended to filenames",true);
    opts.addOption("noScore", "If provided, disable designability and contact scoring", false);
    opts.setOptions(argc, argv);

//    if (opts.isGiven("overlaps") == opts.isGiven("seedGraph")) MstUtils::error("Either 'overlaps' or 'seedGraph' must be provided, but not both.");
//    if (opts.isGiven("overlaps") == true && opts.isGiven("req_seed") == true) MstUtils::error("req_seed parameter not implemented for cluster tree path sampling");
    
    string complexPath = opts.getString("complex");
    string binaryFilePath = opts.getString("seeds");
    string overlapTreePath = opts.getString("overlaps");
    string configFilePath = opts.getString("config");
    string base = opts.getString("base");
    string seedChain = opts.getString("seedChain", "0");
    bool shouldScore = !opts.isGiven("noScore");
    
    //The base name sets the seed, since this varies between batches, this should give unique sampling
    hash<string> hash;
    int name_hash = hash(base);
    cout << "base name: " << base << " with hash: " << name_hash << endl;
    srand(name_hash);
    
    string outputPath = "./path_structures";
    if (!MstSys::fileExists(outputPath)) {
        MstSys::cmkdir(outputPath);
    }

    Structure complex(complexPath);

    // Remove native peptide from target
    if (opts.isGiven("peptideChain")) {
        Chain *peptide = complex.getChainByID(opts.getString("peptideChain", "B"));
        if (peptide != nullptr)
            complex.deleteChain(peptide);
    }

    // Set up scorer, if shouldScore provided
    StructureCompatibilityScorer *scorer = nullptr;
    if (shouldScore) {
        FragmentParams fParams(2, true);
        rmsdParams rParams(1.2, 15, 1);
        contactParams cParams;
        scorer = new StructureCompatibilityScorer(&complex, fParams, rParams, cParams, configFilePath, 0.4, 1, 8000, 0.7, true);
    }

    int numPaths = opts.getInt("numPaths", 200);
    int minLength = opts.getInt("minLength", 15);
    
    StructuresBinaryFile* seedFile;
    SeedGraphPathSampler* sampler;
//    SingleFragmentFetcher* fetcher;
//    ClusterTree* overlapTree;
    StructureCache* cache;
    SeedGraph* seedG;

    // Handle configuration options separately for each input type, since different
    // sets of options may be available
//    if (opts.isGiven("overlaps")) {
        //this is deprecated
//        cout << "Loading cluster tree..." << endl;
//        fetcher = new SingleFragmentFetcher(seedFile, 3, seedChain);
//        overlapTree = new ClusterTree(fetcher, 4, true); // 4 children per level; shared coordinate system
//        overlapTree->read(overlapTreePath);
//
//        // Stringent: 3-residue overlaps, 0.75A cutoff
//        // Permissive: 3-residue overlaps, 1.25A cutoff
//        ClusterTreePathSampler *cSampler = new ClusterTreePathSampler(&complex, fetcher, overlapTree, 3, 1.25, fetcher->getAllResidues());
//        if (opts.isGiven("ss")) {
//            cSampler->preferredSecondaryStructure = new string(opts.getString("ss"));
//        }
//
//        sampler = cSampler;
//        }
    if (opts.isGiven("seedGraph") && !opts.isGiven("score_structures")) {
        cout << "Loading seeds" << endl;
        seedFile = new StructuresBinaryFile(binaryFilePath);
        seedFile->scanFilePositions();
        
        cout << "Loading graph.." << endl;
        cache = new StructureCache(seedFile);
        seedG = new SeedGraph(opts.getString("seedGraph"), false, cache);
        
        int overlapLength = 1;
        sampler = new SeedGraphPathSampler(&complex,seedG,overlapLength);
        if (opts.isGiven("ss")) {
            sampler->preferredSecondaryStructure = new string(opts.getString("ss"));
        }
        if (opts.isGiven("reqSeed")) {
            string reqSeedName = opts.getString("reqSeed");
            Structure* reqSeed = cache->getStructure(reqSeedName);
            vector<Residue*> residues;
            if (opts.isGiven("reqSeedSel")) {
                string reqSeedSel = opts.getString("reqSeedSel");
                selector sel(*reqSeed);
                residues = sel.selectRes(reqSeedSel);
                cout << "Select residues: ";
                for (Residue* R : residues) {
                    cout << R->getChainID() << R->getNum() << " ";
                }
                cout << endl;
            } else {
                Chain* C = reqSeed->getChainByID(seedChain);
                residues = C->getResidues();
            }
            sampler->setStartingPathResidues(residues);
        }
        if (opts.isGiven("fixedSeed")) {
            string fixedSeedName = opts.getString("fixedSeed");
            sampler->addFixedSeed(fixedSeedName);
        }
        sampler->setMinimumLength(minLength);
    }
    
    // Sample paths
    ofstream out(base+"_fused_paths.csv", ios::out);
    if (!out.is_open())
        MstUtils::error("Could not open file stream");
    // CSV header
    out << "name,path,path_len,fuser_score,rmsd_score,total_rmsd_score,bond_score,angle_score,dihedral_score,interchain_clash,intrachain_clash,interchain_designability,num_interchain_contacts,num_designable_interchain_contacts,intrachain_designability,num_intrachain_contacts,num_designable_intrachain_contacts,corroboration" << endl;
    
    cout << "Sample, fuse, and score " << numPaths << " fused paths..." << endl;
    int pathIndex = 0;
    while (pathIndex < numPaths) {
        vector<PathResult> paths;
        if (opts.isGiven("score_paths")) {
            vector<string> path_strings = MstUtils::fileToArray(opts.getString("score_paths"));
            paths = sampler->fusePaths(path_strings);
            numPaths = paths.size();
        } else if (opts.isGiven("score_structures")) {
            vector<string> structure_paths = MstUtils::fileToArray(opts.getString("score_structures"));
            vector<Structure> structures;
            for (string structure_path : structure_paths) {
                Structure S(structure_path);
                S.setName(MstSystemExtension::fileName(S.getName()));
                //verify that there is a single chain with ID = 0
                if (S.chainSize() != 1) MstUtils::error("Structures provided for scoring should have a single chain");
                if (S.getChainByID("0") == NULL) MstUtils::error("Structures provided for scoring must have a chain with ID = 0");
                vector<Residue*> empty;
                paths.emplace_back(empty,S,0,fusionOutput(),0,0);
            }
            numPaths = paths.size();
        } else {
            paths = sampler->sample(min(100,numPaths-pathIndex));
        }
        for (PathResult &path_result: paths) {
            cout << "Path: " << pathIndex << endl;
            string name = base + "_fused-path_" + to_string(pathIndex);
            string name_whole = base + "_fused-path-and-context_" + to_string(pathIndex);
            out << name << ",";
            
            for (Residue *res: path_result.getOriginalResidues()) {
                out << res->getStructure()->getName() << ":" << res->getResidueIndex() << ";";
            }
            out << "," << path_result.size() << ",";
            
            //report the fusion score (and its individual components)
            fusionOutput fuserScore = path_result.getFuserScore();
            out << fuserScore.getScore() << "," << fuserScore.getRMSDScore() << "," << fuserScore.getTotRMSDScore() << ",";
            out << fuserScore.getBondScore() << "," << fuserScore.getAngleScore() << "," << fuserScore.getDihedralScore() << ",";
            
            //report clashes
            out << path_result.getInterchainClash() << "," << path_result.getIntrachainClash() << ",";
            
            //get the path_and_context structure (including target-aligned residues) and path only
            Structure& path_and_context = path_result.getFusedStructure();
            Structure path_only;
            path_result.getFusedPathOnly(path_only);
            path_only.setName(name);
            path_and_context.setName(name_whole);

            // Score the path
            // Score the path (looking at inter-chain contacts)
            mstreal totalScore = 0;
            int numContacts = 0;
            int numDesignable = 0;
            if (shouldScore) {
                cout << "Score inter-chain contacts" << endl;
                scorer->score(&path_only, totalScore, numContacts, numDesignable, false);
            }
            out << totalScore << "," << numContacts << "," << numDesignable << ",";
            
            if (shouldScore) {
                // Score the path (looking at intra-chain contacts)
                cout << "Score intra-chain contacts" << endl;
                scorer->score(&path_only, totalScore, numContacts, numDesignable, true);
            }
            out << totalScore << "," << numContacts << "," << numDesignable << ",";
            
            out << corroborationScore(path_result, 2) << endl;
            
            // Write out the PDBs
            path_and_context.writePDB(MstSystemExtension::join(outputPath,path_and_context.getName()+".pdb"));
            path_only.writePDB(MstSystemExtension::join(outputPath,path_only.getName()+".pdb"));
            
            pathIndex++;
            if (pathIndex >= numPaths) break;
        }
    }
    
    sampler->reportSamplingStatistics();
    
    // Deallocate all memory on the heap
//    if (opts.isGiven("overlaps")) {
//        delete fetcher;
//        delete overlapTree;
//    }
    if (opts.isGiven("seedGraph")) {
        delete cache;
        delete seedG;
    }
    delete sampler;
    if (shouldScore)
        delete scorer;

    cout << "Done" << endl;
    out.close();
    
    return 0;
}