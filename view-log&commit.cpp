// 3.
std::string generateCommitID(const std::string &message)
{

    // Gets the current timestamp
    std::time_t currentTime = std::time(nullptr);

    // Converts time to string and concatenate it with the message
    std::string base = std::to_string(currentTime) + message;

    return hashFunc(base);
}

std::vector<std::pair<std::string, std::string>> getStagedFiles()
{
    std::vector<std::pair<std::string, std::string>> staged;
    std::ifstream staging(".minigit/staging.txt");

    std::string line;
    // Reads file line-by-line
    while (std::getline(staging, line))
    {
        size_t colon = line.find(":");

        if (colon != std::string::npos)
        {
            std::string fileName = line.substr(0, colon);
            std::string fileHash = line.substr(colon + 1);
            staged.push_back({fileName, fileHash});
        }
    }

    staging.close();
    return staged;
}

std::string getParentHash()
{
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentHead;
    std::getline(headFile, currentHead);
    headFile.close();

    // If
    std::ifstream branches(".minigit/branches.txt");
    std::string line;
    while (std::getline(branches, line))
    {
        size_t colon = line.find(":");
        if (colon != std::string::npos)
        {
            std::string branch = line.substr(0, colon);
            std::string branchHash = line.substr(colon + 1);
            if (branch == currentHead)
            {
                branches.close();
                return branchHash;
            }
        }
    }

    branches.close();
    return "";
}

void createCommit(const std::string &message)
{
    // Gets staged files
    auto stagedFiles = getStagedFiles();
    if (stagedFiles.empty())
    {
        std::cout << "There is nothing inside staging.txt to commit.\n";
        return;
    }

    // Generates commit ID
    std::string commitID = generateCommitID(message);

    // Gets parent hash
    std::string parentHash = getParentHash();

    // Gets timestamp
    std::time_t now = std::time(nullptr);
    std::string timeStr = std::ctime(&now);
    timeStr.pop_back();

    // Writes commit data
    std::ofstream commits(".minigit/commits.txt", std::ios::app);
    commits << "COMMIT " << commitID << "\n";
    commits << "TIME " << timeStr << "\n";
    commits << "MESSAGE " << message << "\n";
    commits << "PARENT " << parentHash << "\n";
    for (const auto &[fileName, hash] : stagedFiles)
    {
        commits << "FILE " << fileName << ":" << hash << "\n";
    }
    commits << "END\n";
    commits.close();

    // Updates HEAD and branch pointers
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentBranch;
    std::getline(headFile, currentBranch);
    headFile.close();

    std::ifstream in(".minigit/branches.txt");
    std::ostringstream updatedBranches;
    std::string line;
    bool found = false;

    while (std::getline(in, line))
    {
        size_t colon = line.find(":");
        if (colon != std::string::npos)
        {
            std::string branch = line.substr(0, colon);
            if (branch == currentBranch)
            {
                updatedBranches << branch << ":" << commitID << "\n";
                found = true;
            }
            else
            {
                updatedBranches << line << "\n";
            }
        }
    }

    in.close();

    if (!found)
    {
        updatedBranches << currentBranch << ":" << commitID << "\n";
    }

    std::ofstream out(".minigit/branches.txt");
    out << updatedBranches.str();
    out.close();

    // Clear staging area
    std::ofstream staging(".minigit/staging.txt");
    staging.close();

    std::cout << "Commited! ID: " << commitID << "\n";
}

// 4.
void viewlog()
{

    // read HEAD, branches, and latest commit hash
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentBranch;
    std::getline(headFile, currentBranch);
    headFile.close();

    std::ifstream branchesFile(".minigit/branches.txt");
    std::string commitHash;
    std::string line;

    while (std::getline(branchesFile, line))
    {
        size_t colon = line.find(":");
        if (colon != std::string::npos)
        {
            std::string branch = line.substr(0, colon);
            if (branch == currentBranch)
            {
                commitHash = line.substr(colon + 1);
                break;
            }
        }
    }

    branchesFile.close();
    if (commitHash.empty())
    {
        std::cout << "No commits found on branch '" << currentBranch << "'.\n";
        return;
    }

    // Parse all commits from commits.txt
    std::ifstream commitsFile(".minigit/commits.txt");
    std::vector<Commit> allCommits;
    Commit current;

    while (std::getline(commitsFile, line))
    {
        if (line.rfind("COMMIT ", 0) == 0)
        {
            current.id = line.substr(7);
        }
        else if (line.rfind("TIME ", 0) == 0)
        {
            current.time = line.substr(5);
        }
        else if (line.rfind("MESSAGE ", 0) == 0)
        {
            current.message = line.substr(8);
        }
        else if (line.rfind("PARENT ", 0) == 0)
        {
            current.parent = line.substr(7);
        }
        else if (line == "END")
        {
            allCommits.push_back(current);
            current = Commit();
        }
    }

    commitsFile.close();

    // Follows the commit chain using parent hashes
    std::cout << "\nCommit history:\n";
    while (!commitHash.empty())
    {
        // Find the commit with matching ID
        bool found = false;
        for (const Commit &c : allCommits)
        {
            if (c.id == commitHash)
            {
                std::cout << "----------------------------\n";
                std::cout << "Commit ID: " << c.id << "\n";
                std::cout << "Time     : " << c.time << "\n";
                std::cout << "Message  : " << c.message << "\n";
                commitHash = c.parent; // Go to parent
                found = true;
                break;
            }
        }

        if (!found)
        {
            std::cout << "Error: Commit with ID " << commitHash << " not found.\n";
            break;
        }
    }
}