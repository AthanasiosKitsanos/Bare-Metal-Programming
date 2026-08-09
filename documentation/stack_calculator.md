# `stack_calculator.cpp` — Documentation

## File Purpose

This is a **host-side** tool (runs on Windows, compiled with a regular desktop compiler — **not** part of the kernel image) that computes the **worst-case** stack size that the kernel might require, both for the regular stack (`kernel_main` and everything it calls) and for the interrupt stack (during interrupt handling). The tool reads `.ci` files (call-graph files produced by GCC via `-fdump-ipa-cgraph` or an equivalent flag) and performs a topological sort over the call graph to compute, for every function, the sum of its own stack frame plus the deepest call-chain path starting from it.

## Includes

- `<iostream>`, `<fstream>`, `<filesystem>`: file and directory I/O — acceptable here since this is a host tool, **not** freestanding kernel code (hence the project's guidance that the kernel's branchless rules/constraints do **not** apply to this tool; clarity via plain `if` statements is preferred here).
- `<vector>`, `<unordered_map>`, `<unordered_set>`: call-graph data structures.
- `<charconv>`: `std::from_chars` for fast, exception-free number parsing from text.
- `<algorithm>`: `std::reverse`, `std::max`.

## Path constants and special cases

```cpp
constexpr const char* path{".../ci_files"};
constexpr const char* exc_txt{".../inderect_calls.txt"};
constexpr const char* stack_result{".../calc_results.txt"};
constexpr const char* relationships{".../relationships.txt"};
```

Hardcoded absolute paths to the input folder (`ci_files`, where the `.ci` files produced by GCC for each compiled unit live) and the output/helper files. Acceptable here since this is an internal development tool that always runs on the same development machine.

```cpp
constexpr uint32_t undepended_interrupt_methods_size{12};
constexpr const char* undepended_interrupt_methods[...] = { ... };
constexpr uint32_t depended_interrupt_methods_size{2};
constexpr const char* depended_interrupt_methods[...] = { ... };
```

These arrays contain the (mangled) names of the **roots** of the call graph that correspond to actual interrupt handlers (e.g. `handle_timer_interrupt`, `handle_keyboard_interrupt`, the VGA buffer's SIMD dispatch functions, etc.) — the starting point for computing the "worst-case interrupt stack". The split into **"undepended"** and **"depended"** reflects the known circular-dependency issue described below, around `__indirect_call`.

## `reset_input_file(input_file)` — `[[gnu::always_inline]]`

A simple helper that calls `close()` + `clear()` on an `ifstream`, so the same stream object can be reused for the next file in the reading loop, without leftover error-state bits from the previous open.

## `enum class color`

```cpp
enum class color: uint8_t { white = 0x00, gray = 0x01, black = 0x02 };
```

The classic **three colors of the DFS cycle-detection algorithm** for graphs: `white` = not yet visited, `gray` = currently on the active recursion path (if a gray node is encountered again, a cycle exists), `black` = fully processed.

## `struct graph_ci` — A call-graph node

```cpp
struct alignas(8) graph_ci
{
    std::vector<std::string> children;
    uint32_t frame_size;
    uint32_t dist;
    color col;
    std::string title;
    ...
};
static_assert(sizeof(graph_ci) == 72);
```

Each node represents a function: `children` is the list of names of functions it calls, `frame_size` is the size of its own stack frame (as reported by GCC in the `.ci` file), `dist` is the **computed** worst-case distance (deepest possible total stack) up to this node during topological traversal, and `col` is the DFS state. The move constructor is explicitly defined and zeroes out the source object's fields (`other.frame_size = 0`, etc.) — good clean move-semantics practice, so the "empty" object after the move is left in a predictable state. The `static_assert(sizeof(graph_ci) == 72)` is an **empirical verification** of the structure's size — exactly the "verify, don't assume" principle that runs through the whole project: if some future change adds/removes a field, the compiler will fail immediately instead of letting the size silently change.

## `struct node_ci`

A simple helper structure of two strings (`source_name`, `target_name`) — a temporary representation of an edge during parsing, before it's added to the `children` list of the correct node.

## `std::unordered_map<std::string, graph_ci> u_map` — The main graph

All nodes are stored here, keyed by the function's mangled name. The choice of `unordered_map` (over `vector`) is deliberate: the **pointer stability** of `unordered_map` elements during rehashing/insertion is guaranteed by the C++ standard, which **does not** hold for `std::vector` (growing its capacity moves all elements). This allows safely using raw pointers (`graph_ci*`) inside the `topological_order` vector throughout execution, even as new nodes continue to be added to `u_map`.

## `fdp_unorderd_map(graph, vec)` — Depth-First Search with cycle detection

```cpp
bool fdp_unorderd_map(graph_ci* graph, std::vector<graph_ci*>* vec) noexcept
```

A classic three-color recursive DFS:
1. Marks the current node as `gray` (on the active path).
2. For each child: if it's `white`, recurse into it; if the recursion finds a cycle, propagate `true` upward. If the child is already `gray`, **a cycle has been found** — return `true` immediately.
3. If all children complete without a cycle, marks the node `black` and adds it to the result vector `vec` — this insertion order (nodes are inserted **after** all their children finish) is the basis of the topological sort: once reversed (`std::reverse`) at the end of `main`, it yields an ordering where every node appears **before** anyone who calls it.
4. Uses raw pointers into the `children` vector (`start`, `vector_end`) instead of range-based iterators — plain, explicit pointer arithmetic, consistent with the style of the rest of the project.

## `get_subtree_depth(node)` — Worst-case stack of a subtree

```cpp
uint64_t get_subtree_depth(const graph_ci* node) noexcept
```

Recursively computes the **maximum** sum of stack frame sizes along any call path starting from the given node, via `std::max` over all children. It's used explicitly (rather than the precomputed `dist` from the topological traversal) for interrupt handler roots and for `kernel_main` — since these are the points of interest (there's no need for "the sum from the start of the program", but rather "the depth from this point onward").

## `get_interrupt_stack_size()` — The `__indirect_call` problem

```cpp
graph_ci* indirect_call{&u_map.at("__indirect_call")};
...
indirect_call->frame_size = static_cast<uint32_t>(stack_size);
```

Here the known **circular dependency** is dealt with: some interrupt handlers indirectly call (via function pointers, e.g. `operator<<` in logging) back into the compiler's own `__indirect_call` mechanism, which in turn could theoretically call back into one of the handlers — a genuine cycle in the call graph that would make a plain topological sort impossible.

The solution here is a **two-pass procedure**:
1. **First pass**: computes `stack_size` over the independent nodes (`undepended_interrupt_methods`) — those that don't depend on `__indirect_call`. The result is assigned as the `frame_size` of the `__indirect_call` node itself.
2. **Second pass**: now that `__indirect_call` has a concrete (upper-bound) `frame_size` value, the nodes that **do** depend on it are computed (`depended_interrupt_methods`, e.g. the default exception handlers that call through `operator<<`), giving a safe, conservative estimate without needing to actually resolve the real cycle.

## `main()`

### Phase 1 — Parsing the `.ci` files

Walks every file inside the `ci_files` folder and reads line by line:
- Lines starting with `"node"`: parses the function's name (inside quotes) and the stack frame size (after the `"\\n"` literal embedded in the line, characteristic of GCC's dump format), using manual character-pointer parsing (`current_char`) and `std::from_chars` for the number conversion — faster and exception-free compared to `std::stoi`. If the name already exists (the same function appears in multiple `.ci` files, e.g. due to inlining or multiple dump files), it **sums** the frame sizes rather than replacing them.
- Lines starting with `"edge"`: parses the source and target of a call edge, and — **only if the source is already a known node** — appends the target to the source's `children` list.

### Phase 2 — Topological sort

Calls `fdp_unorderd_map` for every unvisited node (`color::white`), collecting the result into `topological_order`, and reports whether a cycle was found. It then reverses the order (`std::reverse`) so every node precedes the nodes that call it.

### Phase 3 — Computing `dist` via edge relaxation

```cpp
for(; topological_current < topological_end; ++topological_current)
{
    topological_dist = (*topological_current)->dist;
    ...
    for(each child) temp_child->dist = std::max(temp_child->dist, topological_dist + temp_child->frame_size);
}
```

A classic **longest path in a DAG (Directed Acyclic Graph) via topological sort** algorithm: because the processing order guarantees every node is processed **before** anyone it calls, each child's `dist` can be safely updated based on its parent's already-finalized `dist` — the same logic as the Bellman-Ford shortest-path algorithm on a DAG, but for the **maximum** instead of the minimum path.

### Phase 4 — Final computation and writing the results

Computes `interrupt_stack` (via `get_interrupt_stack_size()`) and `kernel_stack` (via `get_subtree_depth` starting from `"kernel_main"`), and writes them to a text file (`calc_results.txt`), explicitly opened in **binary mode** (`std::ios::binary`) — this is the known protection against CRLF corruption on Windows: without binary mode, `std::ofstream` would automatically convert every `'\n'` into `"\r\n"`, which would confuse the number-reading tool (shell tools/Makefile) that expects plain `'\n'`.

## Design notes

- Although part of the same project, this file deliberately **does not** follow the kernel's branchless/freestanding constraints — it freely uses STL containers, exceptions (via `std::filesystem`), and plain `if` statements, as appropriate for a host-side development tool where clarity is explicitly preferred over extreme performance.
- `std::ios::sync_with_stdio(false)` and `std::cin.tie(nullptr)` at the start of `main` are standard I/O optimizations for stream-heavy C++ programs (here, though, mostly file I/O rather than stdin/stdout is used, so the benefit is limited but harmless).
- The use of `.at()` instead of `operator[]` at every read site (`u_map.at(...)`) — as opposed to the write sites, where `operator[]` is used deliberately — reflects the project's well-known principle: `operator[]` on an `unordered_map` silently inserts a "phantom" node if the key doesn't exist, which is undesirable during reads.
