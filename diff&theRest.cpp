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