#include <iostream>
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <stddef.h>
#include <algorithm>

constexpr const char* path{"C:/Users/thano/OneDrive/Desktop/C++/my_OS/ci_files"};
constexpr const char* exc_txt{"C:/Users/thano/OneDrive/Desktop/C++/my_OS/diagnostic_tools/inderect_calls.txt"};

constexpr uint32_t indirect_call_list_size{4};
constexpr const char* exception_list[indirect_call_list_size] =
{
    "_ZN6kernel22handle_timer_interruptEPNS_15interrupt_frameE",
    "kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L20handle_cpu_exceptionEPN6kernel15interrupt_frameE",
    "kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L25default_interrupt_handlerEPN6kernel15interrupt_frameE",
    "_ZN6driver8keyboard25handle_keyboard_interruptEPN6kernel15interrupt_frameE"
};

[[gnu::always_inline]]
inline void reset_input_file(std::ifstream* const input_file) noexcept
{
    input_file->close();
    input_file->clear();
}

enum class color: uint8_t
{
    white = 0x00,
    gray = 0x01,
    black = 0x02
};

struct alignas(8) graph_ci
{
    std::vector<std::string> children;
    uint32_t frame_size;
    uint32_t dist;
    color col;
    std::string title;

    graph_ci() noexcept = default;
    graph_ci(graph_ci&& other) noexcept: children{std::move(other.children)}, frame_size{other.frame_size}, dist{other.dist}, col{other.col}, title{std::move(other.title)}
    {
        other.frame_size = 0;
        other.dist = 0;
        other.col = color::white;
    }
    
    graph_ci& operator=(const graph_ci& other) noexcept
    {
        children = other.children;
        frame_size = other.frame_size;
        col = other.col;
        dist = other.dist;
        title = other.title;
        return *this;
    }
};

static_assert(sizeof(graph_ci) == 72);

struct node_ci
{
    std::string source_name;
    std::string target_name;
};

std::unordered_map<std::string, graph_ci> u_map{};

bool fdp_unorderd_map(graph_ci* graph, std::vector<graph_ci*>* vec) noexcept
{
    graph->col = color::gray;
    graph_ci* temp{nullptr};
    const std::string* vector_end{graph->children.data() + graph->children.size()};
    for(std::string* start{graph->children.data()}; start < vector_end; ++start)
    {
        temp = &u_map[start->data()];
        if(temp->col == color::white)
        {
            if(!fdp_unorderd_map(temp, vec)) continue;
            return true;
        }
        else if(temp->col == color::gray) return true;
    }
    graph->col = color::black;
    vec->emplace_back(graph);
    return false;
}

// TODO need to complete it
uint32_t fdp_unordered_set(const graph_ci* child) noexcept
{
    const std::string* children{child->children.data()};
    const std::string* const childern_end{children + child->children.size()};
    uint32_t total_dist{0};
    for(; children < childern_end; ++children)
    {
        if(children->empty()) return static_cast<uint32_t>(std::max(total_dist, child->dist));
    }
}

int main()
{
    std::ifstream input_file{};
    std::vector<graph_ci*> topological_order{};
        
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

                if(u_map.contains(graph.title))
                {
                    graph_ci* temp{&u_map[graph.title]};
                    temp->frame_size += graph.frame_size;
                    temp->dist = temp->frame_size;
                }
                else
                {
                    graph.dist = graph.frame_size;
                    u_map.emplace(graph.title, std::move(graph));
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


    topological_order.reserve(u_map.size());
    bool loop_found{false};
    for(std::pair<const std::string, graph_ci>& pair: u_map)
    {
        if(pair.second.col == color::white)
        {
            loop_found |= fdp_unorderd_map(&pair.second, &topological_order);
        }
    }
    
    std::reverse(topological_order.begin(), topological_order.end());

    graph_ci* const indirect_call_node{&u_map.at("__indirect_call")};
    {
        uint32_t indirect_worse_case{0};
        const char* const* currrent_call{exception_list};
        const char* const* list_end{currrent_call + indirect_call_list_size};
        for(; currrent_call < list_end; ++currrent_call)
        {   
            indirect_worse_case = std::max(indirect_worse_case, u_map.at(*currrent_call).frame_size);
        }

        indirect_call_node->frame_size = indirect_worse_case;
    }


    {
        graph_ci* const* topological_current{topological_order.data()};
        const graph_ci* const* topological_end{topological_current + topological_order.size()};
        const std::string* child{nullptr};
        const std::string* child_end{nullptr};
        graph_ci* temp_child{};
        
        uint32_t topological_dist{0};
        uint32_t biggest_dist{0};
    
        for(; topological_current < topological_end; ++topological_current)
        {
            topological_dist = (*topological_current)->dist;
            biggest_dist = std::max(biggest_dist, topological_dist);
    
            child = (*topological_current)->children.data();
            child_end = child + (*topological_current)->children.size();
            for(; child < child_end; ++child)
            {
                temp_child = &u_map.at(*child);
                temp_child->dist = std::max(temp_child->dist, topological_dist + temp_child->frame_size);
            }
        }
    }

    // interrupt_stack D + (52 + d) * 17
    
    //kernel_stack
    // const graph_ci* kernel_main{&u_map.at("kernel_main")};
    // const std::string* kernel_current_child{kernel_main->children.data()};
    // const std::string* const kernel_children_end{kernel_current_child + kernel_main->children.size()};
    // std::cout << "Kernel Color: " << static_cast<unsigned>(kernel_main->col) << '\n';

    // for(; kernel_current_child < kernel_children_end; ++kernel_current_child)
    // {
    //     std::cout << *kernel_current_child << ": " << static_cast<unsigned>(u_map.at(*kernel_current_child).col) << "\n\n";
    // }

    graph_ci* kernel_main{&u_map.at("kernel_main")};
    uint32_t kernel_greater_dist{fdp_unordered_set(kernel_main)};
    kernel_main->dist = kernel_greater_dist;
}