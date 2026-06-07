#include "shell.h"
#include "io/output/terminal_output.h"

namespace
{

}

namespace app
{
    shell::shell(terminal::output* scr) noexcept: m_input{}, m_output(scr), m_command_ready{false}
    {}
}