void createBranch(const std::string& newBranchName) {
    //Reads current branch from HEAD
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentBranch;
    std::getline(headFile, currentBranch);
    headFile.close();

    // Gets current commit hash from branhces.txt
    std::ifstream in(".minigit/branches.txt");
    std::string currentCommitHash;
    std::string line;
    std::ostringstream existingBranches;
    bool alreadyExists = false;

    while (std::getline(in, line))
    {
        size_t colon = line.find(":");
        if (colon != std::string::npos)
        {
            std::string branch = line.substr(0, colon);
            std::string hash = line.substr(colon + 1);
            if (branch == newBranchName)
            {
                alreadyExists = true;
            }
            if (branch == currentBranch)
            {
                currentCommitHash = hash;
            }
            existingBranches << line << "\n";
        }
        
    }

    in.close();

    // Error if branch already exists
    if (alreadyExists) {
        std::cout << "Branch '" << newBranchName << "' already exists.\n";
        return;
    }
    
    // Add a new branch
    existingBranches << newBranchName << ":" << currentCommitHash << "\n";

    std::ofstream out(".minigit/branches.txt");
    out << existingBranches.str();
    out.close();

    std::cout << "Branch '" << newBranchName << "' was created successfully\n";
}


//this is the part where we bypass the firewall