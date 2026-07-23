#include <iostream>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <stddef.h>

constexpr const char* path{"C:/Users/thano/OneDrive/Desktop/C++/my_OS/ci_files"};

[[gnu::always_inline]]
inline void reset_input_file(std::ifstream* const input_file) noexcept
{
    input_file->close();
    input_file->clear();
}

struct graph_ci
{
    std::string title;
    std::vector<std::string> children;
    uint32_t frame_size;

    graph_ci() noexcept = default;
    graph_ci(graph_ci&& other) noexcept: title(std::move(other.title)), children(std::move(other.children)), frame_size(other.frame_size)
    {
        other.frame_size = 0;
    }
};

struct node_ci
{
    std::string source_name;
    std::string target_name;
};


int main()
{
    std::unordered_map<std::string, graph_ci> u_map{};
    std::ifstream input_file{};
        
    std::filesystem::directory_iterator iterator{std::filesystem::path{path}};
    for(const std::filesystem::directory_entry& entry: iterator)
    {
        input_file.open(entry.path().string());
        for(std::string line{}; std::getline(input_file, line);)
        {
            if(line.starts_with("node"))
            {
                graph_ci graph{};
                const char* current_char{(line.data() + 4)};
                const char* const end{(line.data() + line.size())};
                while(current_char < end && *current_char != '\"') ++current_char;
                current_char += (current_char < end);
    
                while(*current_char != '\"')
                {
                    graph.title.push_back(*current_char);
                    ++current_char;
                }

                size_t new_line_index{line.rfind("\\n")};
                current_char = (line.data() + new_line_index + 2);
                std::from_chars_result result{std::from_chars(current_char, end, graph.frame_size)};

                const bool exists{u_map.contains(graph.title)};
                if(!exists) u_map.emplace(graph.title, std::move(graph));
                else
                {
                    u_map[graph.title].frame_size += graph.frame_size;
                }
            }
            else if(line.starts_with("edge"))
            {
                node_ci node{};
                const char* current_char{(line.data() + 4)};
                const char* const end{(line.data() + line.size())};

                while(current_char < end && *current_char != '\"') ++current_char;
                current_char += (current_char < end);

                while(*current_char != '\"')
                {
                    node.source_name.push_back(*current_char);
                    ++current_char;
                }
                current_char += (current_char < end);

                if(u_map.contains(node.source_name))
                {
                    while(current_char < end && *current_char != '\"') ++current_char;
                    current_char += (current_char < end);
                    while(*current_char != '\"')
                    {
                        node.target_name.push_back(*current_char);
                        ++current_char;
                    }

                    u_map[node.source_name].children.emplace_back(std::move(node.target_name));
                }

            }
        }
        reset_input_file(&input_file);
    }

    size_t sum{0};
    for(const auto& pair : u_map)
    {
        std::cout << pair.first << " Size: " << pair.second.frame_size << '\n';
        sum += pair.second.frame_size;
    }

    std::cout << sum << '\n';
}