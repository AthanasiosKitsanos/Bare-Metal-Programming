# `terminal_vga_text_buffer.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί τη λογική του **VGA text-mode buffer**: γέμισμα (fill), αντιγραφή (copy/scroll), τοποθέτηση χαρακτήρων και μετακίνηση δρομέα (cursor) μέσα στο κείμενο 80×25 του VGA. Το κεντρικό χαρακτηριστικό του αρχείου είναι ο **πίνακας αποστολής (dispatch table) SIMD συναρτήσεων**: την ίδια λειτουργία (fill ή copy) μπορεί να την εκτελέσει είτε ένα scalar fallback, είτε SSE2 (128-bit), είτε AVX2 (256-bit) — και η επιλογή γίνεται δυναμικά κατά το runtime, ανάλογα με τις δυνατότητες της CPU.

## Ενσωματώσεις (Includes)

- `terminal_vga_text_buffer.h`: δηλώνει την κλάση `vga_text_buffer`, τις ιδιωτικές (private) βοηθητικές μεθόδους της (`begin_32`, `cell_32`, `make_entry` κ.λπ.) και τις σταθερές διαστάσεων (`vga_width = 80`, `vga_height = 25`).
- `vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h`: για ενημέρωση του hardware scroll register (`set_display_start`).
- `cpu/features.h`: για το `cpu::features::get()`, τον δείκτη (index) που λέει ποιο SIMD επίπεδο υποστηρίζει η τρέχουσα CPU.
- `<immintrin.h>`: τα SSE2/AVX2 intrinsics (`_mm_set1_epi32`, `_mm256_store_si256` κ.λπ.).

## Οι τρεις υλοποιήσεις "γεμίσματος" (fill)

Όλες μοιράζονται την ίδια υπογραφή: `void(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) [[gnu::regparm(3)]]`. Το `entry` είναι μια 32-bit τιμή που περιέχει **δύο** VGA text cells (χαρακτήρας + χρώμα) πακεταρισμένα μαζί, ώστε κάθε εγγραφή 32-bit να γεμίζει δύο διαδοχικές θέσεις οθόνης ταυτόχρονα.

### `use_sse_2` — `[[gnu::target("sse2")]]`

Μετατρέπει το `entry` σε ένα SSE register 128 bits (`__m128i`) με τέσσερις επαναλήψεις της ίδιας 32-bit τιμής (`_mm_set1_epi32`), δηλαδή 8 VGA cells ανά εγγραφή. Το `bytes >>= 4` μετατρέπει το μήκος σε bytes σε πλήθος 16-byte (128-bit) εγγραφών. Γράφει με `_mm_store_si128` — **ευθυγραμμισμένη (aligned)** εγγραφή, που απαιτεί ο δείκτης προορισμού να είναι πολλαπλάσιο των 16 bytes.

### `use_avx_2` — `[[gnu::target("avx2")]]`

Ίδια λογική αλλά με 256-bit registers (`__m256i`, `_mm256_set1_epi32`, `_mm256_store_si256`), δηλαδή 16 VGA cells ανά εγγραφή. Το `bytes >>= 5` αντιστοιχεί σε 32-byte (256-bit) εγγραφές.

### `fallback_fill`

Απλός scalar βρόχος (`bytes >>= 2`, δηλαδή 4-byte εγγραφές, γράφοντας ένα `uint32_t` τη φορά) — χρησιμοποιείται όταν η CPU δεν υποστηρίζει ούτε SSE2 ούτε AVX2 (σπάνιο σε σύγχρονο υλικό, αλλά απαραίτητο για ορθότητα/φορητότητα).

### `[[gnu::target("...")]]` γιατί υπάρχει

Επειδή τα project-wide compile flags **δεν** περιλαμβάνουν `-mavx2` γενικά (θα προκαλούσε VEX-encoded εντολές να παραχθούν πριν αρχικοποιηθεί το AVX hardware στην εκκίνηση, οδηγώντας σε `#UD` fault), το `[[gnu::target("avx2")]]` επιτρέπει σε **αυτές τις συγκεκριμένες συναρτήσεις** να μεταγλωττιστούν με AVX2 εντολές, χωρίς να επηρεάζεται ο υπόλοιπος κώδικας του πυρήνα. Το ίδιο ισχύει και για το `sse2` target, αν και το SSE2 είναι baseline σε κάθε x86-64/i686 CPU με FPU· εδώ διατηρείται ρητό για σαφήνεια και συνέπεια.

## `g_dispatch` — Ο πίνακας αποστολής γεμίσματος

```cpp
using fill_fn = void(*)(volatile uint32_t*, uint16_t, const uint32_t) [[gnu::regparm(3)]];
struct fill_functions { fill_fn entries[3]; constexpr fill_functions(): entries{fallback_fill, use_sse_2, use_avx_2} {} };
constexpr fill_functions g_dispatch{};
```

Ένας πίνακας δεικτών συναρτήσεων, γεμισμένος σε **compile time** (`constexpr`), στη σειρά `{fallback, sse2, avx2}`. Ο δείκτης `cpu::features::get()` επιστρέφει έναν αριθμό 0/1/2 που αντιστοιχεί ακριβώς σε αυτή τη σειρά, οπότε η επιλογή SIMD υλοποίησης γίνεται με **μία μόνο προσπέλαση πίνακα (O(1))** αντί για αλυσίδα από `if`/`switch` — αποφεύγοντας branch misprediction σε κάθε κλήση.

## Οι τρεις υλοποιήσεις "αντιγραφής" (copy)

Ίδια φιλοσοφία με το fill, αλλά για αντιγραφή δεδομένων από πηγή σε προορισμό (χρησιμοποιείται στο scroll):

- **`use_sse_2_copy`** / **`use_avx_2_copy`**: `_mm_load_si128`/`_mm256_load_si256` από την πηγή, ακολουθούμενο από `_mm_store_si128`/`_mm256_store_si256` στον προορισμό. Και οι δύο επιστρέφουν τον **ενημερωμένο δείκτη προορισμού** (`destination`) στο τέλος, ώστε ο καλών να ξέρει πού σταμάτησε η εγγραφή.
- **`fallback_copy`**: scalar βρόχος `uint32_t` τη φορά.

Ο αντίστοιχος πίνακας αποστολής είναι το `g_dispatch_cpy` (`fill_copy_function`), με ίδια δομή `{fallback, sse2, avx2}`.

## Δημόσιες μέθοδοι της `vga_text_buffer` (namespace `terminal`)

### `reset()`

Καλείται όταν ο "παράθυρο προβολής" (viewport) του κυκλικού buffer φτάνει στο μέγιστο επιτρεπτό `base_row` (`base_row_max = 179`). Αντιγράφει τις τελευταίες `vga_height - 1` γραμμές πίσω στην **αρχή** της φυσικής μνήμης VGA (μέσω `g_dispatch_cpy`) και μηδενίζει το `base_row` σε `1`. Αυτό υλοποιεί το **double-buffer ring scheme**: αντί να μετακινεί δεδομένα σε κάθε γραμμή (scroll), ο buffer προωθεί απλώς έναν δείκτη βάσης (`base_row`) μέσα σε μια μεγαλύτερη εικονική περιοχή, και μόνο όταν φτάσει στο άκρο αυτής της περιοχής αντιγράφει το ορατό τμήμα πίσω στην αρχή — μια σπάνια, "ακριβή" λειτουργία αντί για μια λειτουργία ανά κάθε νέα γραμμή. Μετά την αντιγραφή, καθαρίζει τη νέα τελευταία γραμμή (`g_dispatch.entries[idx]`).

### `clear()`

Γεμίζει **ολόκληρο** τον buffer (`length` bytes) με το προεπιλεγμένο κενό cell (space character, `default_color`), μηδενίζει `base_row`, `row`, `column`, και μηδενίζει το hardware scroll register (`vga_hardware_cursor::set_display_start(0)`). Χρησιμοποιείται στην εντολή shell `clear` και στην αρχικοποίηση.

### `clear_row()` — `[[gnu::regparm(1)]]`

Γεμίζει μόνο μία γραμμή (`vga_width << 1` bytes) στη θέση `cell_32()` — χρησιμοποιείται όταν προστίθεται μια νέα γραμμή στο κάτω μέρος του ορατού παραθύρου, χωρίς να χρειάζεται πλήρες `reset()`.

### `put(c)` — `[[gnu::regparm(2)]]`

Γράφει έναν χαρακτήρα στη τρέχουσα θέση του δρομέα (`make_entry(c, active_color)`) και προωθεί τη θέση με `move_forward()`.

### `remove_last_char()` — `[[gnu::regparm(1)]]`

Οπισθοχωρεί με `move_backwards()` και γράφει ένα κενό στη νέα θέση — υλοποιεί backspace.

### `move_forward()` — `[[gnu::regparm(1)]]`

Προωθεί `column`, με **branchless "carry" λογική**:

```cpp
++column;
bool overflowed{column == vga_width};
column -= overflowed * vga_width;
row += overflowed;

overflowed = (row == vga_height);
base_row += overflowed;
row -= overflowed;
```

Αντί για ένθετα `if`, το overflow από στήλη σε γραμμή, και από γραμμή σε "νέα γραμμή στο τέλος της οθόνης", υπολογίζεται με boolean → αριθμητική μετατροπή (`true`/`false` → `1`/`0`) πολλαπλασιαζόμενη με το βήμα υπερχείλισης. Αν προκύψει υπερχείλιση γραμμής, ελέγχει αν χρειάζεται πλήρες `reset()` (έφτασε στο άκρο του κυκλικού buffer) ή απλό `clear_row()`, και ενημερώνει το hardware scroll register ώστε η οθόνη να "κυλήσει" οπτικά.

### `move_backwards()` — `[[gnu::regparm(1)]]`

Συμμετρικό του `move_forward`, με ίδια branchless φιλοσοφία, για μετακίνηση προς τα πίσω (backspace σε αρχή γραμμής, ξετύλιγμα σε προηγούμενη γραμμή).

### `move_to_next_line()` — `[[gnu::regparm(1)]]`

Χρησιμοποιείται για το `'\n'`: μηδενίζει τη στήλη και προωθεί τη γραμμή, με ίδια λογική overflow/reset/clear_row με το `move_forward`.

### `move_cursor_left_n(count)` / `move_cursor_right_n(count)` — `[[gnu::regparm(2)]]`

Μετατρέπουν την τρέχουσα θέση σε "απόλυτη" θέση μέσα στην ορατή οθόνη (`row * vga_width + column`), αφαιρούν/προσθέτουν `count`, και ξαναϋπολογίζουν `row`/`column` με διαίρεση/υπόλοιπο ακέραιας αριθμητικής (`/` και υπολειπόμενη αφαίρεση), ενημερώνοντας κατάλληλα και το `base_row` αν η γραμμή άλλαξε. Χρησιμοποιούνται από το `terminal::input` για τα βέλη αριστερά/δεξιά και τα Home/End.

## Σχεδιαστικές παρατηρήσεις

- Ο **δείκτης SIMD dispatch** (`cpu::features::get()`) υπολογίζεται μία φορά κατά την εκκίνηση (CPU feature detection) και επαναχρησιμοποιείται σε κάθε κλήση fill/copy — δεν γίνεται επανέλεγχος CPUID σε κάθε χαρακτήρα.
- Η επιλογή `volatile uint32_t*` για τους δείκτες αντανακλά ότι η μνήμη VGA (`0xB8000`) είναι memory-mapped I/O· το `volatile` αποτρέπει τον compiler από το να εξαλείψει (optimize away) ή να αναδιατάξει εγγραφές που θεωρητικά "δεν έχουν επίδραση" στο πρόγραμμα, ενώ στην πραγματικότητα έχουν ορατό αποτέλεσμα στην οθόνη.
- Όλες οι γεωμετρικές μεταβάσεις (overflow γραμμής/στήλης) είναι σκόπιμα branchless, σύμφωνα με την αρχή του project ότι ο κώδικας πυρήνα σε hot path δεν πρέπει να χρησιμοποιεί έλεγχο ροής με `if` όπου αποφεύγεται.
