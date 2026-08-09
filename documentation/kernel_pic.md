# `kernel_pic.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Οδηγός για το ζεύγος **8259 PIC (Programmable Interrupt Controller)** — master και slave. Υλοποιεί την κλασική διαδικασία "remap" (μετατόπιση των IRQ vectors ώστε να μη συγκρούονται με τα CPU exceptions), αποστολή End-Of-Interrupt (EOI), και επιλεκτική απόκρυψη (masking) IRQs.

## Ενσωματώσεις (Includes)

- `kernel_pic.h`: δηλώσεις `inb`/`outb` (μέσω `terminal::` namespace) και το δημόσιο API.
- `<stdint.h>`.

## Θύρες και σταθερές πρωτοκόλλου (ανώνυμος χώρος ονομάτων)

```cpp
constexpr uint16_t master_command{0x20}; constexpr uint16_t master_data{0x21};
constexpr uint16_t slave_command{0xA0};  constexpr uint16_t slave_data{0xA1};
constexpr uint8_t enable{0x11};
constexpr uint8_t master_bit{0x04}; constexpr uint8_t slave_bit{0x02};
constexpr uint8_t x86_mode{0x01};
constexpr uint8_t eoi_command{0x20};
```

Αυτές είναι οι κλασικές θύρες και τιμές πρωτοκόλλου του i8259: `0x20`/`0x21` για τον master PIC, `0xA0`/`0xA1` για τον slave. Το `0x11` είναι η εντολή αρχικοποίησης ICW1 (Initialization Control Word 1) που ζητά ICW4. Τα `0x04`/`0x02` (ICW3) δηλώνουν στον master ότι ο slave είναι συνδεδεμένος στη γραμμή IRQ2, και στον slave τη δική του "cascade identity". Το `0x01` (ICW4) ενεργοποιεί τη λειτουργία 8086/88 mode.

## `pic_remap(offset_1, offset_2)`

Η κλασική ακολουθία τεσσάρων ζευγών ICW (Initialization Control Words), με `io_wait()` μετά από κάθε εγγραφή (απαραίτητο επειδή ο παλιός PIC ενδέχεται να μην προλαβαίνει να επεξεργαστεί διαδοχικές εγγραφές αρκετά γρήγορα σε σύγχρονο υλικό):

1. **Διάβασμα και προσωρινή αποθήκευση των τρεχουσών μασκών (masks)** και για τους δύο ελεγκτές, ώστε να μπορούν να επαναφερθούν στο τέλος χωρίς να χαθεί καμία προϋπάρχουσα ρύθμιση.
2. **ICW1** (`enable = 0x11`) σε master και slave: ξεκινά τη διαδικασία αρχικοποίησης.
3. **ICW2** (`offset_1`/`offset_2`): ορίζει τη **νέα βάση vector** — δηλαδή σε ποιο IDT vector θα αντιστοιχεί το IRQ0 του master και το IRQ0 του slave αντίστοιχα. Αυτό είναι το ίδιο το "remap": προεπιλεγμένα τα IRQs ξεκινούν στο vector 8, συγκρουόμενα με τα CPU exceptions (0–31)· το remap τα μετατοπίζει, συνήθως σε 32 (master) και 40 (slave).
4. **ICW3**: δηλώνει τη σχέση cascade (master↔slave) μέσω των `master_bit`/`slave_bit`.
5. **ICW4** (`x86_mode`): θέτει λειτουργία 8086.
6. **Επαναφορά των αρχικών μασκών** που είχαν αποθηκευτεί στο βήμα 1.

## `send_eoi(vector)` — `[[gnu::regparm(1)]]`

```cpp
if(vector > 7) terminal::outb(slave_command, eoi_command);
terminal::outb(master_command, eoi_command);
```

Στέλνει την εντολή End-Of-Interrupt. Το `vector` εδώ είναι ο **τοπικός** αριθμός IRQ (0–15, όχι το IDT vector — η μετατροπή `vector - irq_base` γίνεται πριν την κλήση, στο `kernel_exceptions.cpp`). Αν το IRQ προήλθε από τον slave (>7), πρέπει να σταλεί EOI **και στους δύο** ελεγκτές — πρώτα στον slave, μετά στον master — αφού ο master δεν γνωρίζει τίποτα για την εσωτερική κατάσταση του slave παρά μόνο ότι έλαβε ένα cascade interrupt.

## `mask_all_except_timer_and_keyboard()`

```cpp
terminal::outb(master_data, 0xFC);
terminal::outb(slave_data, 0xFF);
```

Γράφει απευθείας τις μάσκες διακοπών: `0xFC` = `0b11111100` αφήνει ενεργά μόνο τα bits 0 και 1 (IRQ0 = timer, IRQ1 = keyboard), αποκρύπτοντας όλα τα υπόλοιπα IRQs του master. Ο slave αποκρύπτεται πλήρως (`0xFF`), αφού ο πυρήνας δεν έχει ακόμη κανέναν handler για συσκευές πίσω από αυτόν.

## Σχεδιαστικές παρατηρήσεις

- Αυτό το αρχείο είναι καθαρά κώδικας ρύθμισης hardware (configuration-time code), όχι hot path — δεν υπάρχουν βελτιστοποιήσεις όπως `regparm` πέρα από το `send_eoi`, το οποίο **καλείται σε κάθε interrupt** (άρα βρίσκεται στο hot path και δικαιολογημένα φέρει `[[gnu::regparm(1)]]`).
- Η αποθήκευση/επαναφορά των αρχικών μασκών στο `pic_remap` είναι μια άμυνα καλής πρακτικής: ακόμη κι αν το BIOS είχε ήδη αποκρύψει κάποια IRQs για δικούς του λόγους, το remap δεν τα "ξεκρύβει" ακούσια.
