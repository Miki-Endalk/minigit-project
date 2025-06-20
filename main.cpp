#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <vector>
#include <map>
#include <set>

namespace fs = std::filesystem;

struct Commit
{
    std::string id;
    std::string time;
    std::string message;
    std::string parent;
    std::map<std::string, std::string> files;
};

// 1.
void initMiniGit()
{
    std::string repoPath = ".minigit";

    if (fs::exists(repoPath))
    {
        std::cout << "Repository already initialized. \n";
        return;
    }

    // Create .minigit folder
    fs::create_directory(repoPath);

    // Create subfolder for blobs
    fs::create_directory(repoPath + "/objects");

    // Create commits file
    std::ofstream commitsFile(repoPath + "/commits.txt");

    // Create branches file with default "main" branch
    std::ofstream branchesFile(repoPath + "/branches.txt");
    branchesFile << "main:\n"; // start with an empty main branch
    branchesFile.close();

    // Create HEAD file pointing to main
    std::ofstream headFile(repoPath + "/HEAD.txt");
    headFile << "main"; // HEAD is now pointing to the main branch
    headFile.close();

    std::cout << "MiniGit repository initilized successfully!\n";
}

// 2.
std::string hashFunc(const std::string &content)
{
    unsigned long hash = 5381;
    for (char c : content)
    {
        hash = ((hash << 5) + hash) + c; // hash * 33 + 1;
    }

    return std::to_string(hash);
}

std::string readFileContent(const std::string &fileName)
{
    std::ifstream file(fileName);
    if (!file.is_open())
    {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void addFile(const std::string &fileName){

    // Checks if the file exists
    if (!fs::exists(fileName))
    {
        std::cout << "The file '" << fileName << "' does not exist.\n";
        return;
    }

    // Reads the content of the file
    std::string content = readFileContent(fileName);
    if (content.empty())
    {
        std::cout << "The file is empty.";
        return;
    }

    // Generates a hash of the content
    std::string hashedContent = hashFunc(content);

    // Saves blob as .minigit/object/hash
    std::string blobPath = ".minigit/objects/" + hashedContent;
    std::ofstream blobFile(blobPath);
    blobFile << content;
    blobFile.close();

    // Add to .minigit/staging.txt
    std::ofstream staging(".minigit/staging.txt", std::ios::app); // append new content at the end
    staging << fileName << ":" << hashedContent << "\n";
    staging.close();

    std::cout << fileName << " has been seccussfully added!";
}

// 3.
std::string generateCommitID(const std::string& message) {

    // Gets the current timestamp
    std::time_t currentTime = std::time(nullptr);

    // Converts time to string and concatenate it with the message
    std::string base = std::to_string(currentTime) + message;

    return hashFunc(base);
}

std::vector<std::pair<std::string, std::string>> getStagedFiles() {
    std::vector<std::pair<std::string, std::string>> staged;
    std::ifstream staging(".minigit/staging.txt");

    std::string line;
    //Reads file line-by-line
    while(std::getline(staging, line)) {
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

std::string getParentHash() {
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

void createCommit(const std::string& message) {
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
    for(const auto& [fileName, hash] : stagedFiles) {
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

    while(std::getline(in, line)) {
        size_t colon = line.find(":");
        if(colon != std::string::npos) {
            std::string branch = line.substr(0, colon);
            if (branch == currentBranch)
            {
                updatedBranches << branch << ":" << commitID << "\n";
                found = true;
            } else {
                updatedBranches << line << "\n";
            }
        }
    }

    in.close();

    if(!found) {
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
void viewlog() {

    // read HEAD, branches, and latest commit hash
    std::ifstream headFile(".minigit/HEAD.txt");
    std::string currentBranch;
    std::getline(headFile, currentBranch);
    headFile.close();

    std::ifstream branchesFile(".minigit/branches.txt");
    std::string commitHash;
    std::string line;

    while (std::getline(branchesFile, line)) {
        size_t colon = line.find(":");
        if (colon != std::string::npos) {
            std::string branch = line.substr(0, colon);
            if (branch == currentBranch) {
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
        } else if(line.rfind("TIME ", 0) == 0) {
            current.time = line.substr(5);
        } else if(line.rfind("MESSAGE ", 0) == 0) {
            current.message = line.substr(8);
        }  else if(line.rfind("PARENT ", 0) == 0) {
            current.parent = line.substr(7);
        } else if(line == "END") {
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
        for(const Commit& c : allCommits) {
            if(c.id == commitHash) {
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

// 5.
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

// 8.
Commit loadCommitByID(const std::string& id) {
    std::ifstream file(".minigit/commits.txt");
    std::string line;
    Commit commit;
    bool found = false;

    while (std::getline(file, line)) {
        if (line.rfind("COMMIT ", 0) == 0) {
            if (line.substr(7) == id) {
                commit.id = id;
                found = true;
            }
        } else if (found && line.rfind("TIME ", 0) == 0) {
            commit.time = line.substr(5);
        } else if (found && line.rfind("MESSAGE ", 0) == 0) {
            commit.message = line.substr(8);
        } else if (found && line.rfind("PARENT ", 0) == 0) {
            commit.parent = line.substr(7);
        } else if (found && line.rfind("FILE ", 0) == 0) {
            size_t colon = line.find(':');
            std::string file = line.substr(5, colon - 5);
            std::string hash = line.substr(colon + 1);
            commit.files[file] = hash;
        } else if (found && line == "END") {
            break;
        }
    }

    return commit;
}

std::vector<std::string> getBlobLines(const std::string& hash) {
    std::ifstream file(".minigit/objects/" + hash);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

void diffCommits(const std::string& id1, const std::string& id2) {
    Commit c1 = loadCommitByID(id1);
    Commit c2 = loadCommitByID(id2);

    if (c1.id.empty() || c2.id.empty()) {
        std::cout << "One or both commits were not found.\n";
        return;
    }

    std::cout << "Comparing " << id1 << " and " << id2 << "\n";

    // Union of file names
    std::set<std::string> allFiles;
    for (auto& [f, _] : c1.files) allFiles.insert(f);
    for (auto& [f, _] : c2.files) allFiles.insert(f);

    for (const auto& file : allFiles) {
        bool inC1 = c1.files.count(file);
        bool inC2 = c2.files.count(file);

        std::cout << "\n File: " << file << "\n";

        if (!inC1) {
            std::cout << "+ Added in commit " << id2 << "\n";
            continue;
        }

        if (!inC2) {
            std::cout << "- Removed in commit " << id2 << "\n";
            continue;
        }

        if (c1.files[file] == c2.files[file]) {
            std::cout << "No changes.\n";
            continue;
        }

        // Hashes differ → Show line-by-line diff
        auto lines1 = getBlobLines(c1.files[file]);
        auto lines2 = getBlobLines(c2.files[file]);

        std::cout << "Changes:\n";

        size_t i = 0, j = 0;
        while (i < lines1.size() || j < lines2.size()) {
            if (i < lines1.size() && j < lines2.size()) {
                if (lines1[i] == lines2[j]) {
                    ++i; ++j;
                } else {
                    std::cout << "- " << lines1[i++] << "\n";
                    std::cout << "+ " << lines2[j++] << "\n";
                }
            } else if (i < lines1.size()) {
                std::cout << "- " << lines1[i++] << "\n";
            } else {
                std::cout << "+ " << lines2[j++] << "\n";
            }
        }
    }
}

int main()
{

    std::string command;
 
    std::cout << "Enter command: ";
    std::cin >> command;

    if (command == "init")
    {
        initMiniGit();
    }
    else if (command == "add") {
        std::string fileName;
        std::cout << "Enter the file name: ";
        std::cin >> fileName;

        addFile(fileName);
    } else if (command == "commit") {
        std::string flag, message;
        std::cout << "Enter '-m': ";
        std::cin >> flag;                // This should be "-m"

        if (flag != "-m")
        {
            std::cout << "Usage: commit -m \"your message\"\n";
            return 1;
        }

        // Clear input buffer before reading message
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::cout << "Enter commit message: ";
        std::getline(std::cin, message);

        if ((message.size() > 0) && (message[0] == ' '))
        {
            message = message.substr(1);
        }

        createCommit(message);        
    } else if (command == "log")
    {
        viewlog();
    } else if (command == "branch")
    {
        std::string newBranch;
        std::cout << "Enter new branch name: ";
        std::cin >> newBranch;
        createBranch(newBranch);
    } else if (command == "checkout")
    {
        std::string targetBranch;
        std::cout << "Enter branch to checkout: ";
        std::cin >> targetBranch;
        checkoutBranch(targetBranch);
    } else if (command == "merge")
    {
        std::string target;
        std::cout <<  "Enter branch to merge into the current one: ";
        std::cin >> target;
        mergeBranch(target);
    } else if (command == "diff") {
        std::string id1, id2;
        std::cout << "Enter first commit ID: ";
        std::cin >> id1;
        std::cout << "Enter second commit ID: ";
        std::cin >> id2;
        diffCommits(id1, id2);
    } else
    {
        std::cout << "Unknown command.\n";
    }

    return 0;
}