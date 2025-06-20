// 6.
void checkoutBranch(const std::string& branchName) {
    // Load branches.txt and find the commit hash for the target branch
    std::ifstream branches(".minigit/branches.txt");
    std::string line;
    std::string targetCommitHash;
    bool foundBranch = false;

    while(std::getline(branches, line))
    {
        size_t colon = line.find(":");
        if(colon != std::string::npos)
        {
            std::string name = line.substr(0, colon);
            std::string hash = line.substr(colon + 1);
            if(name == branchName)
            {
                targetCommitHash = hash;
                foundBranch = true;
                break;
            }
        }
    }
    
    branches.close();

    if(!foundBranch)
    {
        std::cout << "Branch '" << branchName << "' does not exist.\n";
        return;
    }

    // Find the matching commit in commits.txt
    std::ifstream commits(".minigit/commits.txt");
    bool foundCommit = false;
    std::vector<std::pair<std::string, std::string>> filesToRestore;

    while (std::getline(commits, line))
    {
        if (line.rfind("COMMIT ", 0) == 0 && line.substr(7) == targetCommitHash)
        {
            foundCommit = true;
        } else if (foundCommit && line.rfind("FILE ", 0) == 0)
        {
            size_t colon = line.find(":");
            if(colon != std::string::npos ){
                std::string file = line.substr(5, colon - 5);
                std::string hash = line.substr(colon + 1);
                filesToRestore.push_back({file, hash});
            }
        } else if (foundCommit && line == "END")
        {
            break;
        }
    }
    
    commits.close();

    if (!foundCommit)
    {
        std::cout << "Error: Commit '" << targetCommitHash << "' was not found.\n";
        return;
    }
    
    // Restores each file from its blob
    for(const auto& [fileName, blobHash] : filesToRestore) {
        std::ifstream blob(".minigit/objects/" + blobHash);
        if (!blob.is_open())
        {
            std::cout << "Error: Blob for file " << fileName << " not found.\n";
            continue;
        }
        
        std::ofstream outFile(fileName); // This overwritess the file in working dir
        outFile << blob.rdbuf();
        blob.close();
        outFile.close();
    }

    // Updates HEAD.txt
    std::ofstream head(".minigit/HEAD.txt");
    head << branchName;
    head.close();

    std::cout << "Switched to branch '" << branchName << "'.\n";
}

// 7.
std::map<std::string, Commit> loadAllCommits() {
    std::ifstream commitsFile(".minigit/commits.txt");
    std::map<std::string, Commit> commits;
    std::string line;

    Commit current;
    while (std::getline(commitsFile, line))
    {
        if (line.rfind("COMMIT ", 0) == 0)
        {
            current = Commit(); // This lets you start fresh
            current.id = line.substr(7);
        } else if(line.rfind("TIME ", 0) == 0) {
            current.time = line.substr(5);
        } else if(line.rfind("MESSAGE ", 0) == 0) {
            current.message = line.substr(8);
        }  else if(line.rfind("PARENT ", 0) == 0) {
            current.parent = line.substr(7);
        } else if(line.rfind("FILE ", 0) == 0) {
            size_t colon = line.find(":");
            std::string file = line.substr(5, colon - 5);
            std::string hash = line.substr(colon + 1);
            current.files[file] = hash;
        } else if (line == "END") {
            commits[current.id] = current;
        }
    }
    
    return commits;
}

void mergeBranch(const std::string& targetBranch) {
    // Load current branch and commits
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentBranch;
    std::getline(headFile, currentBranch);
    headFile.close();

    std::ifstream branchesFile(".minigit/branches.txt");
    std::map<std::string, std::string> branches;
    std::string line;

    while(std::getline(branchesFile, line)) {
        size_t colon = line.find(":");
        if(colon != std::string::npos) {
            branches[line.substr(0, colon)] = line.substr(colon + 1);
        }
    }

    branchesFile.close();

    if(branches.find(targetBranch) == branches.end()) {
        std::cout << "Branch '" << targetBranch << "' was not found.\n";
        return;
    }

    std::string currentCommit = branches[currentBranch];
    std::string targetCommit = branches[targetBranch];

    // Load all commits
    auto allCommits = loadAllCommits();

    if (allCommits.find(currentCommit) == allCommits.end() || allCommits.find(targetCommit) == allCommits.end())
    {
        std::cout << "One of the commits could not be found.\n";
        return;
    }
    
    Commit curr = allCommits[currentCommit];
    Commit targ = allCommits[targetCommit];
    Commit merged;

    merged.parent = curr.id;
    merged.message = "Merged branch '" + targetBranch + "'";

    std::time_t now = std::time(nullptr);
    merged.time = std::ctime(&now);
    merged.time.pop_back(); // Removes newline for us

    merged.id = hashFunc(merged.time + merged.message);

    // Merge files
    for(auto& [file, hash] : curr.files) {
        merged.files[file] = hash;
    }

    for(auto& [file, targetHash] : targ.files) {
        if(merged.files.count(file) == 0) {
            //file only in target -> add
            merged.files[file] = targetHash;
        } else if(merged.files[file] != targetHash) {
            // Conflict
            std::cout << "CONFLICT: both modified " << file << "\n";
            // keep current version but notify
        }
    }

    // Write commit
    std::ofstream commits(".minigit/commits.txt", std::ios::app);
    commits << "COMMIT " << merged.id << "\n";
    commits << "TIME " << merged.time << "\n";
    commits << "MESSAGE " << merged.message << "\n";
    commits << "PARENT " << merged.parent << "\n";
    for(const auto& [file, hash] : merged.files) {
        commits << "FILE " << file << ":" << hash << "\n";
    }
    commits << "END\n\n";
    commits.close();


    // Update current branch pointer
    std::ofstream branchesOut(".minigit/branches.txt");
    for(auto& [branch, hash] : branches) {
        if (branch == currentBranch) 
        {
            branchesOut << branch << ":" << merged.id << "\n";
        } else {
            branchesOut << branch << ":" << hash << "\n";
        }
    }

    branchesOut.close();

    std::cout << "Merge complete! Commit ID: " << merged.id << "\n";
}
