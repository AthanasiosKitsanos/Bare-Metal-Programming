# `terminal_input.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί την κλάση `terminal::input`: έναν επεξεργαστή γραμμής εντολών (line editor) με buffer σταθερού μεγέθους, δρομέα εισαγωγής (insertion cursor) στη μέση της γραμμής, και υποστήριξη για navigation keys (βέλη, Home, End) και control keys (Backspace, Enter, Tab, Escape). Καταναλώνει events από τον οδηγό πληκτρολογίου (`driver::keyboard`) και ενημερώνει ταυτόχρονα την οθόνη μέσω ενός εσωτερικού `terminal::output`.

## Ενσωματώσεις (Includes)

- `terminal_input.h`: δηλώνει την κλάση, το `input_buffer` (128 + 1 bytes), τους δείκτες `cursor`/`data_end`.
- `io/output/terminal_output.h`: για την ενσωματωμένη έξοδο (`m_output`).
- `internals/navigation_handlers.h`, `internals/control_input_handlers.h`: X-macros που παράγουν τον κώδικα διανομής (dispatch) για τα navigation/control keys.
- `keyboard/keyboard.h`: τύποι `keyboard_event`, `keyboard_key` και ταξινομητές γεγονότων (event classifiers).

## Βοηθητικές συναρτήσεις (ανώνυμος χώρος ονομάτων)

### `move_data_right(end, begin, step)` / `move_data_left(begin, end, step)`

Μετακινούν ένα εύρος χαρακτήρων μέσα στο buffer κατά `step` θέσεις, δεξιά ή αριστερά, byte προς byte. Χρησιμοποιούνται όταν εισάγεται ή διαγράφεται χαρακτήρας **στη μέση** της γραμμής (όχι μόνο στο τέλος): πρέπει να "ανοίξει" ή να "κλείσει" χώρος μετακινώντας ό,τι βρίσκεται μετά τον δρομέα.

### `string_length(string)` — `[[gnu::regparm(1)]]`

Απλή μέτρηση μήκους string μέχρι το `'\0'`, επιστρέφοντας `uint8_t` (αρκετό αφού η χωρητικότητα του buffer είναι μόλις 128 χαρακτήρες).

## Κατασκευαστής

```cpp
input::input() noexcept: cursor{input_buffer}, data_end{input_buffer}, input_buffer{}, m_output{}, input_ready{false}
```

Αρχικοποιεί `cursor` και `data_end` να δείχνουν στην αρχή ενός άδειου buffer.

## Λειτουργίες επεξεργασίας buffer

### `add_character(c)` — `[[gnu::regparm(2)]]`

Αν το buffer είναι γεμάτο (`buffer_full()`), επιστρέφει `false`. Αν ο δρομέας **δεν** βρίσκεται ήδη στο τέλος δεδομένων (`*cursor != '\0'`), μετακινεί τα υπόλοιπα δεδομένα προς τα δεξιά κατά μία θέση (`move_data_right`) ώστε να ανοίξει χώρος. Γράφει τον χαρακτήρα, προωθεί τον δρομέα, και θέτει τον νέο τερματικό `'\0'`.

### `add_string(string)` — `[[gnu::regparm(2)]]`

Ίδια λογική με το `add_character` αλλά για ολόκληρο string ταυτόχρονα (χρησιμοποιείται από το Tab, που εισάγει 4 κενά μαζί) — υπολογίζει το μήκος, ελέγχει χωρητικότητα, μετακινεί τα δεδομένα μία φορά για όλο το string (πιο αποδοτικό από το να καλείται `add_character` σε βρόχο, που θα μετακινούσε επαναληπτικά τα ίδια δεδομένα).

### `delete_character()` — `[[gnu::regparm(1)]]`

Αν ο δρομέας είναι ήδη στην αρχή (`buffer_begin()`), επιστρέφει `false`. Αλλιώς οπισθοχωρεί τον δρομέα και μετακινεί τα επόμενα δεδομένα προς τα αριστερά (`move_data_left`) για να "κλείσει" το κενό — υλοποιεί backspace στη μέση της γραμμής.

### `reset_buffer()` — `[[gnu::regparm(1)]]`

Επαναφέρει `cursor`/`data_end` στην αρχή και `input_ready = false` — καλείται μεταξύ διαδοχικών εντολών του shell.

### `trim_end()` — `[[gnu::regparm(1)]]`

Αφαιρεί τα κενά (`' '`) από το τέλος της γραμμής πριν την εκτέλεση της εντολής, μετακινώντας το `data_end` προς τα πίσω.

## Κύριος βρόχος εισαγωγής

### `start_data_receiving()` — `[[gnu::regparm(1)]]`

Αναμονή/επεξεργασία σε βρόχο μέχρι `input_ready == true`:

1. Όσο δεν υπάρχει εκκρεμές event πληκτρολογίου, ο επεξεργαστής "κοιμάται" με `hlt` (εξοικονόμηση ενέργειας — δεν κάνει busy-wait πολικής δειγματοληψίας/polling).
2. Παίρνει ένα event (`poll_keyboard_event`).
3. Αν το event αντιστοιχεί σε εκτυπώσιμο χαρακτήρα (`try_translate_text_event`), τον εισάγει (`add_character`) και ενημερώνει την οθόνη: γράφει τον χαρακτήρα, και αν ο δρομέας **δεν** βρίσκεται στο τέλος του κειμένου (δηλαδή εισήχθη στη μέση), ξανατυπώνει το υπόλοιπο κείμενο μετά τον δρομέα και μετακινεί τον hardware cursor πίσω στη σωστή θέση (`print_string_no_sync` + `move_cursor_left_n` + `call_cursor_sync`) — έτσι η οθόνη δείχνει σωστά το αποτέλεσμα ακόμη και όταν πληκτρολογείς στη μέση μιας γραμμής.
4. Αλλιώς, αν είναι "control" event (Backspace, Tab, Enter, Escape), καλεί `control_key_dispatch`.
5. Αλλιώς, αν είναι "navigation" event (βέλη, Home, End, PageUp/Down), καλεί `navigation_key_dispatch`.

## Χειριστές πλήκτρων ελέγχου (control handlers)

### `handle_escape()`

Μετακινεί τον hardware cursor στο τέλος της γραμμής, διαγράφει όλους τους χαρακτήρες οπτικά (`delete_last_char_no_sync` σε βρόχο μήκους `count()`), συγχρονίζει τον cursor, και επαναφέρει το buffer — υλοποιεί "καθάρισε ό,τι έγραψα".

### `handle_backspace()`

Καλεί `delete_character()`· αν πέτυχε, διαγράφει οπτικά τον τελευταίο χαρακτήρα και, αν ο δρομέας δεν ήταν στο τέλος, ξανατυπώνει το υπόλοιπο κείμενο με ένα επιπλέον κενό στο τέλος (για να "σβήσει" τον χαρακτήρα που πλέον περισσεύει οπτικά), μετά επαναφέρει τη θέση του δρομέα.

### `handle_tab()`

Εισάγει 4 κενά (`add_string("    ")`) με την ίδια λογική "ξανατύπωσε το υπόλοιπο αν χρειάζεται" όπως το `add_character`.

### `handle_enter()`

Αν υπάρχει περιεχόμενο (`count() > 0`), κόβει τα κενά στο τέλος (`trim_end()`). Γράφει νέα γραμμή και σηματοδοτεί `input_ready = true`, τερματίζοντας τον βρόχο του `start_data_receiving`.

### `control_key_dispatch(key)` — `[[gnu::regparm(2)]]`

Ένα `switch` που παράγεται από το X-macro `CONTROL_INPUT_HANDLERS` — για κάθε καταχωρημένο πλήκτρο ελέγχου καλεί την αντίστοιχη `handle_<key>()`.

## Χειριστές πλοήγησης (navigation handlers)

### `handle_home()` / `handle_end()`

Μετακινούν τον hardware cursor στην αρχή/τέλος της γραμμής (`move_cursor_left_n`/`move_cursor_right_n` στον buffer) και ενημερώνουν τον εσωτερικό δείκτη `cursor` αντίστοιχα.

### `handle_arrow_left()` / `handle_arrow_right()`

Καλούν `move_cursor_left()`/`move_cursor_right()` (inline μέθοδοι που απλά μετακινούν τον δείκτη `cursor` κατά μία θέση μέσα στο buffer, αν είναι δυνατόν) και, αν επιτύχει η μετακίνηση, ενημερώνουν οπτικά τον hardware cursor (`go_backwards()`/`go_forward()`).

### `handle_arrow_up()` / `handle_arrow_down()` / `handle_page_up()` / `handle_page_down()`

Επί του παρόντος **κενές υλοποιήσεις** (`return;`) — δεσμευμένες θέσεις (placeholders) για μελλοντική λειτουργικότητα (π.χ. ιστορικό εντολών/command history), χωρίς σήμερα καμία επίδραση.

### `navigation_key_dispatch(key)` — `[[gnu::regparm(2)]]`

Αντίστοιχο `switch`, παραγόμενο από το X-macro `NAVIGATION_HANDLERS`.

## Σχεδιαστικές παρατηρήσεις

- Το buffer έχει **σταθερό, στατικό μέγεθος** (`input_capacity = 128`) — καμία δυναμική δέσμευση μνήμης δεν εμπλέκεται στην επεξεργασία εισόδου, κατάλληλο για κώδικα πυρήνα πριν ακόμη υπάρχει πλήρως λειτουργικός heap.
- Η χρήση X-macros (`CONTROL_INPUT_HANDLERS`, `NAVIGATION_HANDLERS`) για την παραγωγή των `switch` blocks αποφεύγει την επανάληψη κώδικα (κάθε νέο πλήκτρο προστίθεται σε **ένα** σημείο, στη λίστα X-macro, και ο handler switch παράγεται αυτόματα).
- Ο διαχωρισμός "ενημέρωσε το buffer" / "ενημέρωσε την οθόνη" σε κάθε handler ακολουθεί το μοτίβο "πρώτα η λογική κατάσταση, μετά η οπτική αναπαράσταση", διατηρώντας τα δύο συστήματα συγχρονισμένα χωρίς να αναμειγνύεται η λογική τους.
