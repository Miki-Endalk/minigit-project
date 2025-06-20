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

void addFile(const std::string &fileName)
{

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
