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

enum class color: uint32_t
{
    white = 0x00,
    gray = 0x01,
    black = 0x02
};

struct alignas(64) graph_ci
{
    std::string title;
    std::vector<std::string> children;
    uint32_t frame_size;
    color col;

    graph_ci() noexcept = default;
    graph_ci(graph_ci&& other) noexcept: title{std::move(other.title)}, children{std::move(other.children)}, frame_size{other.frame_size}, col{other.col}
    {
        other.frame_size = 0;
        other.col = color::white;
    }
};

static_assert(sizeof(graph_ci) == sizeof(double) * sizeof(double));

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

    constexpr const char* keyboard_interrupt{"_ZN6driver8keyboard25handle_keyboard_interruptEPN6kernel15interrupt_frameE"};
    constexpr const char* timer_interrupt{"_ZN6kernel22handle_timer_interruptEPNS_15interrupt_frameE"};
    constexpr const char* cpu_exception{"kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L20handle_cpu_exceptionEPN6kernel15interrupt_frameE"};
    constexpr const char* handle_exception{"kernel/exceptions/kernel_exceptions.cpp:_ZN12_GLOBAL__N_1L16handle_exceptionEPKcS1_PN6kernel15interrupt_frameE"};
    constexpr const char* put_method{"_ZN8terminal15vga_text_buffer3putEc"};
    
    const graph_ci* const k_interrupt{&u_map[keyboard_interrupt]};
    const graph_ci* const t_interrupt{&u_map[timer_interrupt]};
    const graph_ci* const c_exception{&u_map[cpu_exception]};
    const graph_ci* const h_exception{&u_map[handle_exception]};
    const graph_ci* const p_method{&u_map[put_method]};

    std::cout << "handle_keyboard_interrupt: " << k_interrupt->frame_size
    << " bytes\n\ntimer_interrupt: " << t_interrupt->frame_size
    << " bytes\n\ncpu_exceptions: " << c_exception->frame_size
    << " bytes\n\nhandle_exceptions: " << h_exception->frame_size << " bytes, with " << h_exception->children.size() << " childern\n\n"
    << "put(): " << p_method->frame_size << " bytes, with " << p_method->children.size() << " children\n\n";
}