# `terminal_vga_hardware_cursor.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Οδηγός για τους καταχωρητές ελέγχου του **hardware text-mode cursor** και του **display start register** της κάρτας VGA, μέσω των κλασικών θυρών CRTC (CRT Controller). Ξεχωρίζει από τον `vga_text_buffer` (που διαχειρίζεται τα *δεδομένα* κειμένου στη μνήμη) — αυτό το αρχείο διαχειρίζεται αποκλειστικά τη *θέση* του αναβοσβήνοντος δρομέα που βλέπει ο χρήστης στην οθόνη, καθώς και ποια γραμμή εμφανίζεται πρώτη (scrolling μέσω CRTC start address).

## Ενσωματώσεις (Includes)

- `terminal_vga_hardware_cursor.h`: δηλώνει την κλάση/namespace `vga_hardware_cursor`.
- `internals/terminal_io_registers.h`: `outb`/`inb` για I/O θύρες.

## Σταθερές δεικτών καταχωρητών CRTC

```cpp
constexpr uint8_t cursor_register_low{0x0F}; constexpr uint8_t cursor_register_high{0x0E};
constexpr uint8_t display_register_low{0x0D}; constexpr uint8_t display_register_high{0x0C};
```

Ο ελεγκτής CRTC της VGA προσπελαύνεται μέσω **έμμεσης προσπέλασης** (indexed access): πρώτα γράφεις τον δείκτη (index) του καταχωρητή που θέλεις στη θύρα εντολών, μετά διαβάζεις/γράφεις την τιμή του στη θύρα δεδομένων. `0x0E`/`0x0F` είναι ο δείκτης θέσης cursor (high/low byte), και `0x0C`/`0x0D` ο δείκτης "αρχής προβολής" (start address) — η θέση στη μνήμη VGA από την οποία ξεκινά η ορατή οθόνη (χρησιμοποιείται για το scroll του κυκλικού buffer).

## `write_register(index, value)` — `[[gnu::regparm(2)]]`

```cpp
outb(command_port, index);
outb(data_port, value);
```

Η θεμελιώδης λειτουργία indexed I/O: επιλέγει τον καταχωρητή και γράφει την τιμή του.

## `read_register(index)` — `[[gnu::regparm(1)]]`

Συμμετρικό: επιλέγει τον καταχωρητή και διαβάζει την τρέχουσα τιμή του.

## `enable(start, end)`

```cpp
uint8_t cursor_start{static_cast<uint8_t>((read_register(0x0A) & 0xC0) | (start & 0x1F))};
write_register(0x0A, cursor_start);
uint8_t cursor_end{static_cast<uint8_t>((read_register(0x0B) & 0xE0) | (end & 0x1F))};
write_register(0x0B, cursor_end);
```

Ενεργοποιεί τον hardware cursor και ρυθμίζει το **σχήμα** του (ύψος, μέσω "scan lines" αρχής/τέλους — π.χ. λεπτή γραμμή στο κάτω μέρος του κελιού έναντι ολόκληρου γεμάτου block). Χρησιμοποιεί **διάβασε-τροποποίησε-γράψε (read-modify-write)** αντί για απευθείας εγγραφή: οι καταχωρητές `0x0A`/`0x0B` περιέχουν και άλλα, άσχετα bits (π.χ. bit απενεργοποίησης cursor στο `0x0A`) που **δεν** πρέπει να αλλοιωθούν — οι μάσκες `0xC0`/`0xE0` διατηρούν αυτά τα άλλα bits ανέγγιχτα, ενώ αντικαθιστούν μόνο τα bits του scan line εύρους.

## `set_position(position)` — `[[gnu::regparm(1)]]`

Γράφει τη θέση του cursor (γραμμικός δείκτης μέσα στον 80×25 buffer, `row * 80 + column`) στους δύο καταχωρητές θέσης, χωρισμένη σε χαμηλό και υψηλό byte — καλείται σε κάθε `sync_cursor()` από το `terminal::output`.

## `set_display_start(position)` — `[[gnu::regparm(1)]]`

Γράφει την "αρχή προβολής" στους δύο καταχωρητές display start — αυτή είναι η εντολή που κάνει την οθόνη να δείχνει διαφορετικό τμήμα του (μεγαλύτερου, κυκλικού) VGA buffer, υλοποιώντας το οπτικό scroll χωρίς να χρειάζεται να μετακινηθούν δεδομένα στη μνήμη σε κάθε νέα γραμμή· καλείται από τον `vga_text_buffer` όποτε αλλάζει το `base_row`.

## Σχεδιαστικές παρατηρήσεις

- Η ρητή διάκριση "θέση cursor" έναντι "αρχή προβολής" αντανακλά μια θεμελιώδη ιδιότητα της VGA hardware: ο δρομέας (πού αναβοσβήνει) και το scroll (τι φαίνεται) είναι **δύο ανεξάρτητοι** μηχανισμοί υλικού, και το ότι συνδυάζονται σωστά είναι ευθύνη του λογισμικού (εδώ, του `vga_text_buffer`, που καλεί και τα δύο στα κατάλληλα σημεία).
- Η χρήση read-modify-write στο `enable()` (αλλά **όχι** στα `set_position`/`set_display_start`, όπου γράφεται ολόκληρο το byte) δείχνει προσεκτική κατανόηση του ποιοι καταχωρητές μοιράζονται bits με άλλες λειτουργίες και ποιοι είναι αποκλειστικά αφιερωμένοι σε μία τιμή.
