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
constexpr const char* stack_result{"C:/Users/thano/OneDrive/Desktop/C++/my_OS/diagnostic_tools/calc_results.txt"};
constexpr const char* relationships{"C:/Users/thano/OneDrive/Desktop/C++/my_OS/diagnostic_tools/relationships.txt"};

constexpr uint32_t undepended_interrupt_methods_size{12};
constexpr const char* undepended_interrupt_methods[undepended_interrupt_methods_size] =
{
    "_ZN6kernel22handle_timer_interruptEPNS_15interrupt_frameE",
    "_ZN6driver8keyboard25handle_keyboard_interruptEPN6kernel15interrupt_frameE",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L9use_sse_2EPVmtm",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L9use_avx_2EPVmtm",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L13fallback_fillEPVmtm",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L14use_sse_2_copyEPVKmPVmt",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L14use_avx_2_copyEPVKmPVmt",
    "utilities/vga/vga_text_buffer/terminal_vga_text_buffer.cpp:_ZN12_GLOBAL__N_1L13fallback_copyEPVKmPVmt",
    "_ZN8terminal3decERNS_6outputE",
    "_ZN8terminal3hexERNS_6outputE",
    "_ZN8terminal10bool_alphaERNS_6outputE",
    "_ZN8terminal13bool_no_alphaERNS_6outputE",
};

constexpr uint32_t depended_interrupt_methods_size{2};
constexpr const char* depended_interrupt_methods[depended_interrupt_methods_size] =
{
    "kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L20handle_cpu_exceptionEPN6kernel15interrupt_frameE",
    "kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L25default_interrupt_handlerEPN6kernel15interrupt_frameE"
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

// Find the worst case scenario of memory consuption of a method
uint64_t get_subtree_depth(const graph_ci* node) noexcept
{
    uint64_t kernel_dist{0};
    const std::string* children{node->children.data()};
    const std::string* const childern_end{children + node->children.size()};
    for(; children < childern_end; ++children)
    {
        kernel_dist = std::max(kernel_dist , get_subtree_depth(&u_map.at(*children)));
    }
    return node->frame_size + kernel_dist;
}



// Find the worst case scenario of memory consumption while interrupt_happen
uint64_t get_interrupt_stack_size() noexcept
{
    uint64_t stack_size{0};
    graph_ci* indirect_call{&u_map.at("__indirect_call")};
    const char* const* current_interrupt{undepended_interrupt_methods};
    const char* const* current_interrupt_end{current_interrupt + undepended_interrupt_methods_size};
    for(; current_interrupt < current_interrupt_end; ++current_interrupt)
    {
        stack_size = std::max(stack_size, get_subtree_depth(&u_map.at(*current_interrupt)));
    }
    
    indirect_call->frame_size = static_cast<uint32_t>(stack_size);

    current_interrupt = depended_interrupt_methods;
    current_interrupt_end = current_interrupt + depended_interrupt_methods_size;
    for(; current_interrupt < current_interrupt_end; ++current_interrupt)
    {
        stack_size = std::max(stack_size, get_subtree_depth(&u_map.at(*current_interrupt)));
    }
    indirect_call->frame_size = static_cast<uint32_t>(stack_size);
    return stack_size;
}

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::vector<graph_ci*> topological_order{};
    
    {   // We read and divide nodes from edges and fill our unorder_map
        // when a method is marked as an edge, it means that it is a node that is called by another node
        std::filesystem::directory_iterator iterator{std::filesystem::path{path}};
        std::ifstream input_file{};
        
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
    }
    
    // We reserve space equal to our unordered_map size
    // That avoids asking the OS for more memory when an addition happens at the current capacity does not cover for it
    topological_order.reserve(u_map.size());
    {
        // We check if we passed all the methods once
        // and find if the calls are acyclic, meaning the is no A calls B, which calls C, which calls A again. This makes a cycle
        bool loop_found{false};
        const std::pair<const std::string, graph_ci>* temp{nullptr};
        for(std::pair<const std::string, graph_ci>& pair: u_map)
        {
            if(pair.second.col == color::white)
            {
                loop_found |= fdp_unorderd_map(&pair.second, &topological_order);
            }
        }
        if(loop_found) std::cout << "The methods are cyclic\n";
    }
    
    // we reverse the vector, so there order is the same as the unordered map's
    std::reverse(topological_order.begin(), topological_order.end());

    {   // Here we find how much memory a method will occupy while running
        // meaning by the variables it creates and other methods that it may call
        // i.e. method A creates some variables, then calls another method, in which it creates its own varibales
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

    const uint64_t interrupt_stack{get_interrupt_stack_size()};
    const uint64_t kernel_stack{get_subtree_depth(&u_map.at("kernel_main"))};
    std::ofstream out{stack_result, std::ios::binary};
    out << "#Kernel Stack\n" << kernel_stack
    << "\n#Interrupt Stack\n" << interrupt_stack << '\n';
}