#include "bitmap_index/core/index_manager.h" // Include the IndexManager
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem> // Ensure included
#include <algorithm> // Required for std::transform

// Helper function to trim whitespace (could also use utils::trimCopy if linking allows)
std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(first, (last - first + 1));
}


// Function to parse query input for tag-based queries
bool parseTagQuery(const std::string& input, bitmap_index::core::StringTagSet& tags,
                   bitmap_index::core::BitmapOperation& op)
{
    tags.clear();
    std::stringstream ss(input);
    std::string segment;
    std::vector<std::string> parts;

    // Simple split by space
    while (ss >> segment)
    {
        parts.push_back(segment);
    }

    if (parts.size() < 2)
    {
        std::cerr << "Error: Query needs at least an operation (AND, OR, XOR, ANDNOT) and one tag." << std::endl;
        return false;
    }

    // Last part should be the operation
    std::string op_str = parts.back();
    std::transform(op_str.begin(), op_str.end(), op_str.begin(), ::toupper); // Convert to uppercase

    if (op_str == "AND")
    {
        op = bitmap_index::core::BitmapOperation::AND;
    }
    else if (op_str == "OR")
    {
        op = bitmap_index::core::BitmapOperation::OR;
    }
    else if (op_str == "XOR")
    {
        op = bitmap_index::core::BitmapOperation::XOR;
    }
    else if (op_str == "ANDNOT")
    {
        op = bitmap_index::core::BitmapOperation::ANDNOT;
    }
    else
    {
        std::cerr << "Error: Unknown operation '" << parts.back() << "'. Use AND, OR, XOR, or ANDNOT." << std::endl;
        return false;
    }

    // The rest are tags
    for (size_t i = 0; i < parts.size() - 1; ++i)
    {
        tags.push_back(parts[i]); // Assume tags don't contain spaces for this simple parser
    }

    return !tags.empty();
}


int main(int argc, char* argv[])
{
    // --- Configuration ---
    std::string data_file_path = "data.csv";
    std::string status_file_path = "index_status.txt";
    std::string index_save_dir = "index_data"; // Directory to save/load index
    char delimiter = '|';

    // Example: Allow overriding data file path via command line argument
    if (argc > 1)
    {
        data_file_path = argv[1];
        std::cout << "Using data file specified on command line: " << data_file_path << std::endl;
    }
    else
    {
        std::cout << "Using default data file: " << data_file_path
            << " (You can provide a path as a command line argument)" << std::endl;
    }


    // --- Initialization ---
    std::cout << "Initializing Index Manager..." << std::endl;
    bitmap_index::core::IndexManager manager(data_file_path, status_file_path, delimiter);

    // ---> Try to Load Existing Index <---
    std::cout << "Attempting to load index from " << index_save_dir << "..." << std::endl;
    if (manager.loadIndex(index_save_dir))
    {
        std::cout << "Existing index loaded successfully." << std::endl;
        // Optional: Verify counts after load if needed
        std::cout << "Documents: " << manager.getDocumentCount() << std::endl;
        std::cout << "Tags:      " << manager.getTagCount() << std::endl;
    }
    else
    {
        std::cout << "No existing index found or load failed. Will build from data file." << std::endl;
    }
    // ---> End Load Attempt <---


    std::cout << "Loading incremental data (if any)..." << std::endl;
    if (!manager.loadIncremental())
    {
        std::cerr << "Error loading incremental index data. State might be inconsistent." << std::endl;
        // Decide whether to exit or continue
    }
    std::cout << "Incremental load check complete." << std::endl;
    std::cout << "Current Documents: " << manager.getDocumentCount() << std::endl;
    std::cout << "Current Tags:      " << manager.getTagCount() << std::endl;
    std::cout << "----------------------------------------" << std::endl;


    // --- Query Loop ---
    std::string line;
    while (true)
    {
        std::cout << "\nEnter query type ('tagsfor <doc_id>', 'query <tag1> <tag2>... <OPERATION>', or 'quit'):" <<
            std::endl;
        std::cout << "> ";

        if (!std::getline(std::cin, line))
        {
            break; // End of input
        }

        line = trim(line);
        if (line.empty())
        {
            continue;
        }

        if (line == "quit")
        {
            break;
        }

        std::stringstream line_ss(line);
        std::string command;
        line_ss >> command;

        if (command == "tagsfor")
        {
            std::string doc_id;
            if (line_ss >> doc_id)
            {
                doc_id = trim(doc_id); // Trim potential whitespace around ID
                if (!doc_id.empty())
                {
                    std::cout << "Getting tags for document: '" << doc_id << "'" << std::endl;
                    bitmap_index::core::StringTagSet tags = manager.getTagsForDocument(doc_id);
                    if (tags.empty())
                    {
                        std::cout << "Document not found or has no tags." << std::endl;
                    }
                    else
                    {
                        std::cout << "Tags: ";
                        for (size_t i = 0; i < tags.size(); ++i)
                        {
                            std::cout << "'" << tags[i] << "'" << (i == tags.size() - 1 ? "" : ", ");
                        }
                        std::cout << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Error: Document ID cannot be empty." << std::endl;
                }
            }
            else
            {
                std::cerr << "Error: Missing document ID for 'tagsfor' command." << std::endl;
            }
        }
        else if (command == "query")
        {
            std::string rest_of_line;
            std::getline(line_ss, rest_of_line); // Get the rest of the line after "query "
            rest_of_line = trim(rest_of_line);

            bitmap_index::core::StringTagSet query_tags;
            bitmap_index::core::BitmapOperation query_op;

            if (!rest_of_line.empty() && parseTagQuery(rest_of_line, query_tags, query_op))
            {
                std::cout << "Performing query..." << std::endl;
                // Optionally print parsed query details
                // std::cout << "  Tags: {"; for(...) std::cout << t << ","; std::cout << "}" << std::endl;
                // std::cout << "  Operation: " << (query_op == ... ? "AND" : ...) << std::endl;

                bitmap_index::core::StringIdList results = manager.queryTags(query_tags, query_op);

                if (results.empty())
                {
                    std::cout << "No documents found matching the query." << std::endl;
                }
                else
                {
                    std::cout << "Found " << results.size() << " matching document(s):" << std::endl;
                    for (const auto& id : results)
                    {
                        std::cout << "  - " << id << std::endl;
                    }
                }
            }
            else
            {
                if (rest_of_line.empty())
                {
                    std::cerr << "Error: Missing tags and operation for 'query' command." << std::endl;
                    std::cerr << "Usage: query <tag1> [tag2...] <AND|OR|XOR|ANDNOT>" << std::endl;
                }
                // parseTagQuery prints specific errors
            }
        }
        else
        {
            std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
            std::cerr << "Available commands: 'tagsfor <doc_id>', 'query <tags...> <OPERATION>', 'quit'" << std::endl;
        }
    }

    // ---> Save Index Before Exiting <---
    std::cout << "\nSaving index state to " << index_save_dir << "..." << std::endl;
    if (manager.saveIndex(index_save_dir))
    {
        std::cout << "Index saved successfully." << std::endl;
    }
    else
    {
        std::cerr << "Error saving index state." << std::endl;
    }
    // ---> End Save Index <---


    std::cout << "\nExiting." << std::endl;
    return 0;
}
