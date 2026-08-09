# `keyboard.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί τον οδηγό (driver) του PS/2 πληκτρολογίου: αρχικοποίηση της συσκευής, μετάφραση raw scancodes (Scancode Set 1) σε λογικά πλήκτρα (`keyboard_key`), παρακολούθηση της κατάστασης modifiers (Shift/Ctrl/Alt/CapsLock), και μια ουρά κυκλικού buffer (ring buffer) γεγονότων πληκτρολογίου που γεμίζει το interrupt handler και αδειάζει ο υπόλοιπος πυρήνας.

## Ενσωματώσεις (Includes)

- `pic/kernel_pic.h`: PIC (Programmable Interrupt Controller) λειτουργίες.
- `keyboard.h`: δημόσιο API και τύποι.
- `internals/terminal_io_registers.h`: `inb`/`outb` για I/O θύρες.
- `internal/keyboard_key_list_n_map.h`: X-macros με τους πλήρεις πίνακες αντιστοίχισης scancode→πλήκτρο και πλήκτρο→χαρακτήρα.
- `internal/kernel_interrupt_frame.h`, `internal/kernel_interrupt_guard.h`: τύπος `interrupt_frame` και RAII φύλακας διακοπών (interrupt guard).
- `logger/kernel_logger.h`: αναφορά σφαλμάτων αρχικοποίησης.

## Σταθερές θυρών και πρωτοκόλλου

```cpp
constexpr uint16_t data_port{0x60};
constexpr uint16_t status_port{0x64};
constexpr uint8_t output_buffer_full{0x01};
constexpr uint8_t input_buffer_full{0x02};
```

Αυτές είναι οι κλασικές θύρες του PS/2 controller (i8042): `0x60` για δεδομένα, `0x64` για κατάσταση/εντολές. Τα bits `output_buffer_full`/`input_buffer_full` δείχνουν αν υπάρχουν δεδομένα προς ανάγνωση από το πληκτρολόγιο ή αν ο controller είναι έτοιμος να δεχτεί νέα εντολή.

## Πίνακες αντιστοίχισης χτισμένοι σε compile time

Τέσσερις δομές, όλες χτισμένες μέσω `constexpr` κατασκευαστή που εκτελεί ένα X-macro:

- **`key_list` → `normal_key_map`**: scancode (0–127) → `keyboard_key` (χωρίς extended prefix).
- **`normal_character_map_table` → `normal_characters_table`**: `keyboard_key` → χαρακτήρας χωρίς Shift.
- **`shifted_character_map_table` → `shifted_characters_table`**: `keyboard_key` → χαρακτήρας με Shift.
- **`extended_key_map_table` → `extended_key_table`**: scancode (με extended prefix `0xE0`) → `keyboard_key`.

Επειδή είναι όλες `constexpr`, οι πίνακες αυτοί γεμίζουν **κατά τη μεταγλώττιση** και ενσωματώνονται στο binary ως έτοιμα δεδομένα (data section) — καμία αρχικοποίηση δεν χρειάζεται κατά την εκκίνηση (runtime), και η αναζήτηση γίνεται πάντα με O(1) προσπέλαση πίνακα.

## Βοηθητικές συναρτήσεις μετάφρασης

### `map_scancode_set_1_key(key_code, extended)` — `[[gnu::regparm(2)]]`

Επιλέγει τον σωστό πίνακα (`extended_key_table` ή `normal_key_map`) ανάλογα με το αν προηγήθηκε το byte `0xE0`.

### `get_normal_character(key)` — `[[gnu::regparm(1)]]` / `get_shifted_character(key)`

Απλή προσπέλαση στους αντίστοιχους πίνακες χαρακτήρων.

## Πρωτόκολλο επικοινωνίας με τον controller

### `wait_input_buffer_clear()` / `wait_output_buffer_full()`

Πολικές αναμονές (polling loops) με άνω όριο `keyboard_timeout = 100000` προσπάθειες, καλώντας `kernel::io_wait()` σε κάθε επανάληψη (μικρή καθυστέρηση μεταξύ διαδοχικών προσπελάσεων I/O, απαραίτητη σε παλιό υλικό PS/2). Επιστρέφουν `false` αν λήξει το timeout — προστασία από μόνιμο "κρέμασμα" (hang) αν ο controller δεν απαντήσει ποτέ.

### `read_keyboard_ack()`

Περιμένει δεδομένα και ελέγχει αν η απάντηση είναι `keyboard_ack (0xFA)`.

### `send_keyboard_byte_and_wait_ack(byte)`

Στέλνει ένα byte εντολής στον controller και επιβεβαιώνει ότι έγινε αποδεκτό (ACK).

### `flush_keyboard_output_buffer()`

Αδειάζει τυχόν "μπαγιάτικα" (stale) bytes από προηγούμενες καταστάσεις πριν ξεκινήσει η κανονική λειτουργία — αποτρέπει το πρώτο interrupt να επεξεργαστεί ένα ξεχασμένο byte από το BIOS.

## Ουρά γεγονότων (Event Queue)

```cpp
struct alignas(64) keyboard_event_queue
{
    driver::keyboard::keyboard_event entries[64];
    uint8_t head; uint8_t tail; uint8_t count;
};
```

Κυκλικό buffer (ring buffer) χωρητικότητας 64 στοιχείων (δύναμη του 2, ώστε το wraparound να γίνεται με bitwise AND αντί για modulo — βλέπε `next_keyboard_event`). Η ευθυγράμμιση `alignas(64)` τοποθετεί τη δομή σε όριο cache line, αποτρέποντας το "false sharing" και βελτιώνοντας τη χωρική τοπικότητα (locality) κατά τις προσπελάσεις.

- **`next_keyboard_event(index)`** — `[[gnu::always_inline]]`: `(index + 1) & keyboard_event_queue_mask` — γρήγορο wraparound modulo χωρίς πραγματική διαίρεση, εφικτό επειδή το μέγεθος είναι δύναμη του 2 (ελεγμένο με `static_assert`).
- **`commit_keyboard_event()`**: προωθεί το `tail` και αυξάνει το `count` — καλείται αφού γραφτεί ένα νέο γεγονός στην ουρά.

## Παρακολούθηση κατάστασης modifiers

### `update_modifier_state(key, state)` — `[[gnu::regparm(2)]]`

Ένα `switch` που ενημερώνει το bitmask `g_modifier_state` (τύπος `modifier_state = uint8_t`) για κάθε πλήκτρο modifier. Ιδιαίτερη προσοχή στο `caps_lock`: επειδή το πλήκτρο Caps Lock είναι "toggle" (εναλλαγή κατάστασης) και όχι "κράτα πατημένο", ο χειρισμός του διακρίνει ρητά μεταξύ **κατάστασης πατήματος** (`caps_lock_down`, sticky bit που αποτρέπει επανειλημμένη εναλλαγή όσο το πλήκτρο παραμένει κάτω) και **κατάστασης ενεργοποίησης** (`caps_lock_on`, το πραγματικό on/off): μόνο στην **πρώτη** στιγμή που ανιχνεύεται πάτημα (`!is_caps_down`) αναστρέφεται το `caps_lock_on`.

## Δημόσιο API

### `driver::initialize_keyboard()`

Καθαρίζει την κατάσταση modifiers, αδειάζει το output buffer, και στέλνει εντολή απενεργοποίησης όλων των LEDs (`set_leds_command` + `all_leds_off`). Αν αποτύχει, καταγράφει προειδοποίηση (`kernel::logger::warning`) αλλά **δεν** σταματά την εκκίνηση — η αποτυχία ρύθμισης LEDs δεν είναι κρίσιμη.

### `try_translate_text_event(event, out_character)` — `[[gnu::regparm(2)]]`

Ελέγχει πρώτα αν το event είναι υποψήφιο για εισαγωγή κειμένου (`is_text_input_candidate_event`). Για γράμματα, εφαρμόζει **XOR λογική** μεταξύ Shift και Caps Lock: `shift_pressed != caps_on` επιλέγει το shifted γράμμα — αυτό υλοποιεί σωστά τον κανόνα ότι το Caps Lock αντιστρέφει το Shift **μόνο** για γράμματα (π.χ. Shift+γράμμα με Caps ενεργό δίνει πεζό), ενώ για μη-γράμματα (ψηφία, σύμβολα) εξαρτάται μόνο από το Shift.

### `handle_keyboard_interrupt(frame)` — `[[gnu::regparm(1)]]`

Ο πραγματικός IRQ1 handler, καλείται από τον interrupt dispatcher:
1. Ελέγχει ότι όντως υπάρχουν δεδομένα (`output_buffer_full`) — αλλιώς επιστρέφει αμέσως.
2. Διαβάζει το scancode byte.
3. Αν είναι το extended prefix (`0xE0`), θέτει σημαία `g_extended_pending` και επιστρέφει (το πραγματικό scancode έρχεται στο **επόμενο** interrupt).
4. Αν η ουρά είναι γεμάτη, αγνοεί το γεγονός (drop) — αποτρέπει buffer overflow· προτιμάται η απώλεια ενός γεγονότος από καταστροφή μνήμης.
5. Αποκωδικοποιεί: `key_code = scancode & 0x7F`, κατάσταση (`pressed`/`released` από το bit `0x80`), αναζητά το λογικό πλήκτρο, ενημερώνει τους modifiers, και γράφει όλα τα πεδία του `keyboard_event` απευθείας στη θέση `tail` της ουράς πριν καλέσει `commit_keyboard_event()`.

### `current_keyboard_modifier_state()`

Επιστρέφει το `g_modifier_state`, προστατευμένο από `kernel::interrupt_guard` (RAII, απενεργοποιεί τις διακοπές όσο διαρκεί η ανάγνωση) — αποτρέπει μια ενδιάμεση κατάσταση αν συμβεί interrupt ενώ διαβάζεται η μεταβλητή.

### `poll_keyboard_event(out_event)` — `[[gnu::regparm(1)]]`

Αν η ουρά είναι άδεια, επιστρέφει `false`. Αλλιώς αντιγράφει το event στη θέση `head`, προωθεί το `head` και μειώνει το `count`. Επίσης προστατευμένο με `interrupt_guard`, αφού η ουρά είναι κοινόχρηστη μεταξύ interrupt context (writer) και κανονικού κώδικα (reader) — κλασικό producer/consumer πρόβλημα ταυτοχρονισμού (concurrency) σε περιβάλλον χωρίς threads αλλά με interrupts.

### `has_pending_keyboard_event()`

Επιστρέφει το `count`, επίσης προστατευμένο.

## Σχεδιαστικές παρατηρήσεις

- Όλη η στατική κατάσταση (πίνακες, ουρά) είναι περιορισμένη στον ανώνυμο χώρο ονομάτων — καμία εξωτερική μεταγλωττιστική μονάδα δεν μπορεί να τη διαβάσει ή να την αλλοιώσει απευθείας.
- Το `interrupt_guard` γύρω από κάθε προσπέλαση της κοινής ουράς είναι το βασικό μοτίβο συγχρονισμού (synchronization) του πυρήνα, αντικαθιστώντας locks/mutexes (άσκοπα σε μονοπύρηνο, μη-preemptive πλαίσιο) με απλή απενεργοποίηση διακοπών.
- Η επιλογή "απόρριψη γεγονότος όταν η ουρά είναι γεμάτη" αντί για αναμονή είναι σωστή επιλογή μέσα σε interrupt handler: ο handler πρέπει να επιστρέψει γρήγορα και ποτέ να μην μπλοκάρει.
