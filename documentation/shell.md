# `shell.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί το `app::shell`: ένα στοιχειώδες διαδραστικό κέλυφος (interactive shell) γραμμής εντολών που τρέχει πάνω από τον `terminal::input`/`terminal::output`. Δέχεται μια εντολή, την αναγνωρίζει μέσω δυαδικής αναζήτησης (binary search) σε μια αλφαβητικά ταξινομημένη λίστα, και καλεί τη σχετική συνάρτηση εκτέλεσης μέσω πίνακα δεικτών συναρτήσεων.

## Ενσωματώσεις (Includes)

- `shell.h`: δηλώνει την κλάση `shell` και τα μέλη της (`m_input`, `m_output`, `m_is_running`) καθώς και τις δημόσιες μεθόδους εντολών.
- `io/output/terminal_output.h`: για έξοδο κειμένου.
- `internal/shell_commands_list.h`: X-macros `COMMAND_LIST` και `COMMAND_FUNCTIONS` — η **μοναδική πηγή αλήθειας** (single source of truth) για ποιες εντολές υπάρχουν.
- `cpu/features.h`: για την εντολή `flag`.
- `timer/kernel_timer.h`: για την εντολή `ticks`.

## Βοηθητική συνάρτηση σύγκρισης

### `str_compare(comparer, other)` — ανώνυμος χώρος ονομάτων

Υλοποιεί σύγκριση strings παρόμοια με το `strcmp` της C: επιστρέφει τη διαφορά (`int8_t`) των πρώτων χαρακτήρων που διαφέρουν, ή `0` αν είναι ίσα. Χρησιμοποιείται από τη δυαδική αναζήτηση εντολών, αφού χρειάζεται πρόσημο (θετικό/αρνητικό/μηδέν), όχι απλώς boolean ισότητα.

## Πίνακες εντολών, χτισμένοι σε compile time

### `command_list` → `g_command_list`

```cpp
constexpr uint8_t command_list_size{6};
struct command_list { const char* entries[command_list_size]; constexpr command_list(): entries{} { ... COMMAND_LIST ... } };
constexpr command_list g_command_list{};
```

Ένας πίνακας με τα **ονόματα** (strings) των 6 εντολών, γεμισμένος από το X-macro `COMMAND_LIST` — η σειρά τους **πρέπει** να είναι αλφαβητική, αφού πάνω σε αυτόν τον πίνακα εκτελείται δυαδική αναζήτηση.

### Wrapper συναρτήσεις εκτέλεσης

```cpp
inline void execute_clear(app::shell* shell) noexcept { shell->clear(); }
```

Κάθε δημόσια μέθοδος εντολής της κλάσης `shell` (π.χ. `clear()`, `exit()`) "τυλίγεται" σε μία ελεύθερη συνάρτηση `execute_<όνομα>` με ενιαία υπογραφή `void(app::shell*) noexcept`, ώστε να μπορεί να μπει σε πίνακα δεικτών συναρτήσεων — οι μέθοδοι μελών (member function pointers) της C++ δεν έχουν την ίδια, απλή υπογραφή δείκτη με τις ελεύθερες συναρτήσεις, οπότε αυτό το "άπλωμα" (flattening) απλοποιεί τον πίνακα αποστολής. Όλες σημειωμένες `[[gnu::always_inline]]` — δεν υπάρχει πραγματικό κόστος κλήσης, ενσωματώνονται πλήρως.

### `command_functions` → `g_command_functions`

```cpp
struct command_functions { command_list_functions entries[command_list_size]; constexpr command_functions(): entries{} { ... COMMAND_FUNCTIONS ... } };
constexpr command_functions g_command_functions{};
```

Πίνακας δεικτών συναρτήσεων (`execute_*`), γεμισμένος σε **αντιστοιχία θέσης (index)** με τον `g_command_list` — η θέση `i` στο `g_command_functions` πρέπει να αντιστοιχεί στην ίδια εντολή με τη θέση `i` στο `g_command_list`. Αυτή η αντιστοιχία διατηρείται από τα δύο X-macros (`COMMAND_LIST`, `COMMAND_FUNCTIONS`) που μοιράζονται τους ίδιους δείκτες (`index`).

## Δημόσιες μέθοδοι της κλάσης `shell` (namespace `app`)

### Κατασκευαστής

```cpp
shell::shell() noexcept: m_input{}, m_output{}, m_is_running{true}
```

Αρχικοποιεί τα εσωτερικά αντικείμενα εισόδου/εξόδου και θέτει τη σημαία εκτέλεσης σε ενεργή.

### `command_exists()` const

Υλοποιεί **δυαδική αναζήτηση (binary search)** πάνω στο `g_command_list.entries`, χρησιμοποιώντας τη συμβολοσειρά εισόδου του χρήστη (`m_input.read_buffer()`) ως κλειδί, μέσω `str_compare`. Επιστρέφει τον **δείκτη (index)** της εντολής αν βρεθεί ταίριασμα, ή `-1` αν όχι. Πολυπλοκότητα O(log n) αντί για γραμμική σύγκριση O(n) — σημαντικό ακόμη και για μικρό αριθμό εντολών, ως πρακτική καλής σχεδίασης που κλιμακώνεται (scales) όταν προστεθούν περισσότερες εντολές.

### `execute_command()`

Καλεί `command_exists()`· αν βρεθεί έγκυρος δείκτης, καλεί απευθείας `g_command_functions.entries[index](this)` — **καμία** αλυσίδα `if`/`else if` ή `switch` δεν χρειάζεται, η "αποστολή" (dispatch) γίνεται με μία μόνο προσπέλαση πίνακα και έμμεση κλήση συνάρτησης. Αν δεν βρεθεί εντολή, τυπώνει `"Command not found\n"`.

### `run()`

Ο κύριος βρόχος του κελύφους: όσο `m_is_running`, τυπώνει το prompt (`"my_OS:> "`), ξεκινά τη λήψη εισόδου (`m_input.start()`, μπλοκάρει μέχρι να πατηθεί Enter), εκτελεί την εντολή, και επαναφέρει το buffer εισόδου (`m_input.reset()`).

### `clear()`

Επαναφέρει το input buffer και καθαρίζει την οθόνη (`m_output.clear()`) — υλοποιεί την εντολή `clear`.

### `exit()`

Θέτει `m_is_running = 0`, τερματίζοντας τον βρόχο `run()` στην επόμενη επανάληψη, και τυπώνει μήνυμα τερματισμού.

### `flag()`

Τυπώνει το τρέχον επίπεδο SIMD υποστήριξης της CPU (`cpu::features::get()`) — χρήσιμο διαγνωστικό εργαλείο (diagnostic) για να επιβεβαιωθεί ποια υλοποίηση (fallback/SSE2/AVX2) χρησιμοποιεί ο VGA text buffer.

### `ticks()`

Τυπώνει τον τρέχοντα αριθμό timer ticks (`kernel::timer_ticks()`).

### `interrupt_stack()` / `kernel_stack()`

Υπολογίζουν και τυπώνουν το μέγεθος (σε bytes) του interrupt stack και του kernel stack αντίστοιχα, αφαιρώντας τις διευθύνσεις των linker-defined συμβόλων `_interrupt_stack_top`/`_bottom` και `_kernel_stack_top`/`_bottom` — διαγνωστικά εργαλεία για επαλήθευση ότι τα stacks έχουν το αναμενόμενο μέγεθος (σχετικό με το `diagnostic_tools/stack_calculator.cpp`, που υπολογίζει το θεωρητικό απαιτούμενο μέγεθος στατικά, πριν την εκτέλεση).

## Σχεδιαστικές παρατηρήσεις

- Η αρχιτεκτονική "πίνακας ονομάτων + παράλληλος πίνακας συναρτήσεων + binary search" είναι ένα πλήρως **data-driven** σχήμα διανομής εντολών: η προσθήκη μιας νέας εντολής απαιτεί μόνο μία γραμμή στο X-macro `COMMAND_LIST`/`COMMAND_FUNCTIONS` (διατηρώντας αλφαβητική σειρά) και μία νέα μέθοδο/wrapper — καμία αλλαγή στη λογική διανομής.
- Όλα τα δεδομένα διανομής (`g_command_list`, `g_command_functions`) είναι `constexpr`, άρα ζουν στο read-only τμήμα του binary (`.rodata`), χωρίς κόστος αρχικοποίησης κατά την εκκίνηση και χωρίς κίνδυνο ακούσιας τροποποίησης κατά το runtime.
