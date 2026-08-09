# `kernel_exceptions.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Το κεντρικό "νευρικό σύστημα" διακοπών (interrupts) του πυρήνα: εγκαθιστά όλους τους handlers CPU exceptions (π.χ. Divide Error, Page Fault) και hardware IRQs (Timer, Keyboard) στο IDT, και υλοποιεί τον ενιαίο **interrupt dispatcher** που καλείται από το assembly stub (`common_interrupt_entry.S`) για **κάθε** διακοπή, ανεξαρτήτως vector.

## Ενσωματώσεις (Includes)

- `idt/kernel_idt.h`: `set_interrupt_gate`, `load_idt`.
- `logger/kernel_logger.h`: αναφορά exceptions στην οθόνη.
- `kernel_exceptions.h`: δημόσιο API.
- `internal/kernel_interrupt_frame.h`: δομή `interrupt_frame` (καταχωρητές CPU κατά τη στιγμή της διακοπής).
- `pic/kernel_pic.h`: `pic_remap`, `send_eoi`, `mask_all_except_timer_and_keyboard`.
- `timer/kernel_timer.h`, `keyboard/keyboard.h`: οι πραγματικοί handlers IRQ0/IRQ1.
- `internal/kernel_cpu_interrupts_list.h`, `internal/kernel_hardware_interrupts_list.h`: X-macros `CPU_INTERRUPT_LIST`/`HARDWARE_INTERRUPT_LIST` — η πλήρης λίστα όλων των γνωστών exceptions/IRQs με vector, όνομα, τίτλο, mnemonic.

## Σταθερές

```cpp
constexpr uint16_t interrupt_vector_count{256};
constexpr uint16_t kernel_code_selector{0x08};
constexpr uint8_t interrupt_gate_attributes{0x8E};
constexpr uint8_t irq_base{32};
constexpr uint8_t irq_max{47};
```

Το x86 IDT έχει πάντα 256 πιθανά vectors. Το `0x08` είναι ο επιλογέας (selector) του kernel code segment στο GDT. Το `0x8E` κωδικοποιεί: παρόν (present), προνόμιο (privilege) ring 0, 32-bit interrupt gate. Τα IRQs υλικού, μετά το PIC remap, καταλαμβάνουν τα vectors 32–47.

## Δομές περιγραφής exceptions

### `exception_descriptor`

```cpp
struct exception_descriptor { uint8_t vector; exception_handler_ptr stub; const char* name; const char* mnemonic; };
```

Ενοποιεί όλες τις πληροφορίες που χρειάζονται για να εγκατασταθεί ένα IDT entry: το vector, τον δείκτη στο assembly stub, το ανθρώπινα-αναγνώσιμο όνομα και το τυπικό mnemonic (π.χ. `"#DE"`, `"#PF"`).

### Παραγωγή δηλώσεων και descriptors μέσω X-macros

```cpp
#define X(vector, name, title, mnemonic) extern "C" void isr_##vector() noexcept;
CPU_INTERRUPT_LIST
#undef X
```

Αυτό το μοτίβο επαναλαμβάνεται τέσσερις φορές (δύο για CPU exceptions, δύο για hardware IRQs): μία φορά για να δηλωθούν τα εξωτερικά assembly σύμβολα (`isr_N`/`irq_N`, ορισμένα στο `common_interrupt_entry.S`), και μία φορά για να χτιστούν οι αντίστοιχοι `constexpr exception_descriptor`. Έτσι, η **μία και μοναδική** λίστα ορισμών στο `.h` αρχείο παράγει αυτόματα τόσο τις δηλώσεις όσο και τα δεδομένα περιγραφής — καμία επανάληψη, μηδενική πιθανότητα το vector ενός exception να μην ταιριάζει με το stub του.

## Ο πίνακας εγγραφών χειρισμού (dispatch table)

```cpp
using interrupt_handler = void (*)(kernel::interrupt_frame*) noexcept [[gnu::regparm(1)]];
struct g_interrupt_handlers_table { interrupt_handler entries[interrupt_vector_count]; constexpr g_interrupt_handlers_table() ... };
constexpr g_interrupt_handlers_table g_interrupt_handlers{};
```

Ένας πίνακας 256 δεικτών συναρτήσεων, γεμισμένος σε compile time:
1. Αρχικά, **όλα** τα 256 στοιχεία γεμίζουν με `default_interrupt_handler` (fallback για κάθε άγνωστο/απρόσμενο vector).
2. Στη συνέχεια, τα vectors των γνωστών CPU exceptions αντικαθίστανται με `handle_cpu_exception`.
3. Τέλος, τα vectors των γνωστών hardware IRQs αντικαθίστανται με τον **πραγματικό** handler τους (`kernel::handle_timer_interrupt`, `driver::keyboard::handle_keyboard_interrupt`), μέσω `name_space::handle_##name` δυναμικά παραγόμενου από το X-macro.

Αυτό σημαίνει ότι κάθε νέο IRQ που προστίθεται στη λίστα `HARDWARE_INTERRUPT_LIST` "συνδέεται" αυτόματα με τον σωστό handler του, χωρίς να χρειάζεται αλλαγή στη λογική διανομής.

## Χειρισμός CPU exceptions

### `handle_exception(name, mnemonic, frame)` — `[[gnu::regparm(1)]] [[noreturn]]`

Καταγράφει πλήρη κατάσταση καταχωρητών CPU (EIP, EFLAGS, error code, EAX/ECX/EDX/EBX/ESP/EBP/ESI/EDI, vector) στο log επιπέδου σφάλματος (`log.error()`), σε δεκαεξαδική μορφή, και μετά καλεί `log.panic(...)`, το οποίο σταματά τον επεξεργαστή οριστικά. Χρησιμοποιείται όταν συμβεί ένα **αναγνωρισμένο** CPU exception — δεν υπάρχει ασφαλής τρόπος συνέχισης, οπότε ο πυρήνας "πανικοβάλλεται" ελεγχόμενα, δίνοντας στον προγραμματιστή όλα τα διαγνωστικά δεδομένα.

### `handle_cpu_exception(frame)` — `[[gnu::regparm(1)]] [[noreturn]]`

`switch` πάνω στο `frame->vector`, παραγόμενο από το X-macro `CPU_INTERRUPT_LIST` — κάθε γνωστό vector καλεί το `handle_exception` με το σωστό όνομα/mnemonic. Ένα `default` case καλύπτει την (θεωρητικά αδύνατη, αλλά ασφαλή) περίπτωση ενός vector που δεν αναγνωρίζεται, καταγράφοντας προειδοποίηση και σταματώντας τον επεξεργαστή.

### `default_interrupt_handler(frame)` — `[[gnu::regparm(1)]]`

Χειρίζεται **κάθε** vector που δεν έχει ρητά καταχωρημένο handler (ούτε CPU exception, ούτε γνωστό IRQ): καταγράφει προειδοποίηση με το vector και το EIP. Αν το vector ανήκει στο εύρος IRQ (`irq_base`–`irq_max`), επιστρέφει κανονικά (ώστε το EOI να σταλεί ούτως ή άλλως και το hardware να μη "φρακάρει"). Αλλιώς, σταματά τον επεξεργαστή — ένα άγνωστο, μη-IRQ interrupt είναι σοβαρό σφάλμα.

## `interrupt_dispatcher(frame)` — `extern "C"`

```cpp
extern "C" void interrupt_dispatcher(kernel::interrupt_frame* frame) noexcept
{
    uint32_t vector{frame->vector};
    g_interrupt_handlers.entries[vector](frame);
    if(vector >= irq_base && vector <= irq_max) kernel::send_eoi(static_cast<uint8_t>(vector - irq_base));
}
```

Το **μοναδικό, καθολικό σημείο εισόδου** που καλείται από το assembly (`common_interrupt_entry.S`) για κάθε είδος διακοπής, ανεξάρτητα από τον τύπο της. Η αποστολή γίνεται με μία απλή προσπέλαση πίνακα (O(1), καμία αλυσίδα `if`), και αν το vector είναι hardware IRQ, στέλνεται End-Of-Interrupt (EOI) στο PIC μετά την επεξεργασία — απαραίτητο ώστε το PIC να ξέρει ότι μπορεί να στείλει το επόμενο IRQ ίδιας ή χαμηλότερης προτεραιότητας.

## `kernel::initialize_exceptions()`

1. Αναδιάταξη (remap) του PIC ώστε τα IRQs να μεταφερθούν από τα συγκρουόμενα vectors 0–15 (που επικαλύπτονται με CPU exceptions) στα 32–47.
2. Εγκατάσταση **όλων** των CPU exception και IRQ gates στο IDT, μέσω `install_exception` σε βρόχο X-macro.
3. `kernel::mask_all_except_timer_and_keyboard()` — απενεργοποιεί όλα τα άλλα IRQs στο PIC, αφού ο πυρήνας δεν έχει ακόμη handlers γι' αυτά.
4. `load_idt()` — φορτώνει το IDTR με τη διεύθυνση του πίνακα IDT μέσω `lidt`.

## Σχεδιαστικές παρατηρήσεις

- Η φιλοσοφία "μία λίστα X-macro, πολλαπλές παραγόμενες όψεις (δηλώσεις, descriptors, dispatch table, switch cases)" εξαλείφει ολόκληρες κατηγορίες σφαλμάτων συγχρονισμού μεταξύ IDT εγγραφών, handler tables και human-readable ονομάτων.
- Η επιλογή vector-indexed πίνακα 256 στοιχείων αντί για hash map ή αλυσίδα ελέγχων είναι η φυσική επιλογή εδώ, αφού το εύρος του vector είναι ήδη γνωστό, μικρό και πυκνό (0–255) — τέλειο σενάριο για άμεση ευρετηρίαση (direct indexing).
