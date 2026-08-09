# `kernel_idt.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί τις δύο θεμελιώδεις λειτουργίες διαχείρισης του **Interrupt Descriptor Table (IDT)** σε επίπεδο υλικού x86: τη συγγραφή μιας εγγραφής (gate) στον πίνακα, και τη φόρτωση του πίνακα στον επεξεργαστή μέσω της εντολής `lidt`.

## Ενσωματώσεις (Includes)

- `kernel_idt.h`: δηλώνει τις δομές `idt_entry`/`idtr_descriptor` και το δημόσιο API.
- `<stddef.h>`: για `size_t`.

## Ο πίνακας IDT (ανώνυμος χώρος ονομάτων)

```cpp
constexpr size_t total_entries{256};
kernel::idt_entry idt_entry_table[total_entries];
```

Ο πραγματικός πίνακας IDT ζει ως στατικός πίνακας (static array) περιορισμένος σε αυτό το translation unit — κανένα άλλο αρχείο δεν έχει άμεση πρόσβαση σε αυτόν, μόνο μέσω των δύο δημόσιων συναρτήσεων. Το x86 protected mode υποστηρίζει ακριβώς 256 πιθανά vectors διακοπών, εξ ου και το σταθερό μέγεθος.

## `set_interrupt_gate(vector, handler_address, selector, type_attributes)`

Γεμίζει **μία** εγγραφή του IDT στη θέση `vector`. Η δομή `idt_entry` (x86 interrupt gate descriptor) χωρίζει τη διεύθυνση του handler σε δύο μισά 16-bit (`offset_low`, `offset_high`) εξαιτίας του ιστορικού σχεδιασμού segmentation του x86:

```cpp
entry->offset_low = static_cast<uint16_t>(handler_address & 0xFFFFu);
entry->selector = selector;
entry->zero = 0;
entry->type_attributes = type_attributes;
entry->offset_high = static_cast<uint16_t>((handler_address >> 16) & 0xFFFFu);
```

- **`offset_low`/`offset_high`**: τα κάτω και άνω 16 bits της 32-bit διεύθυνσης του handler.
- **`selector`**: ο επιλογέας segment στο GDT που πρέπει να χρησιμοποιηθεί κατά την είσοδο στον handler (συνήθως το kernel code segment, `0x08`).
- **`zero`**: δεσμευμένο byte, πρέπει να είναι μηδέν σύμφωνα με την αρχιτεκτονική.
- **`type_attributes`**: κωδικοποιεί τον τύπο gate (interrupt/trap), το DPL (privilege level) και το present bit.

Αυτή η συνάρτηση καλείται μία φορά ανά exception/IRQ κατά την αρχικοποίηση (από το `kernel_exceptions.cpp`), όχι σε hot path, οπότε δεν φέρει ειδικές σημειώσεις βελτιστοποίησης όπως `regparm`.

## `load_idt()`

```cpp
const idtr_descriptor descriptor
{
    static_cast<uint16_t>(sizeof(idt_entry_table) - 1),
    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(idt_entry_table))
};
asm volatile("lidt %0" : : "m"(descriptor) : "memory");
```

Χτίζει έναν **περιγραφέα IDTR** (limit + base address) και τον φορτώνει στον επεξεργαστή με την ειδική εντολή assembly `lidt`. Το `limit` είναι το μέγεθος του πίνακα **μείον ένα** (σύμβαση της αρχιτεκτονικής x86: το `limit` είναι η τελευταία έγκυρη byte offset, όχι το συνολικό μέγεθος). Το `"memory"` clobber στο inline assembly ενημερώνει τον compiler ότι αυτή η εντολή μπορεί να επηρεάσει τη μνήμη με τρόπο που δεν μπορεί να προβλεφθεί στατικά, αποτρέποντας επικίνδυνες αναδιατάξεις εντολών (instruction reordering) γύρω από αυτήν.

## Σχεδιαστικές παρατηρήσεις

- Ο διαχωρισμός σε δύο μικρές, ξεκάθαρες συναρτήσεις (μία για "γράψε μια εγγραφή", μία για "ενεργοποίησε τον πίνακα") ακολουθεί την αρχή του ενιαίου σκοπού (single responsibility): η `kernel_exceptions.cpp` καλεί το `set_interrupt_gate` πολλές φορές σε βρόχο, και το `load_idt` **μία μόνο φορά** στο τέλος, αφού όλες οι εγγραφές έχουν ήδη οριστεί.
- Ο πίνακας `idt_entry_table` **δεν** αρχικοποιείται ρητά πριν την κλήση των `set_interrupt_gate` — κάθε στοιχείο του παραμένει σε απροσδιόριστη κατάσταση μέχρι να γραφτεί ρητά. Αυτό είναι ασφαλές επειδή το `kernel_exceptions.cpp` καλύπτει **όλα** τα 256 vectors πριν καλέσει `load_idt()` (κάθε γνωστό exception/IRQ, και το `default_interrupt_handler` για όλα τα υπόλοιπα, μέσω του δικού του `g_interrupt_handlers_table` — προσοχή όμως: αυτός ο πίνακας-εγγραφών IDT στο υλικό χρειάζεται να έχει *κάποιο* stub σε κάθε θέση πριν φορτωθεί, ώστε ένα απρόσμενο interrupt να μην οδηγήσει σε ανάγνωση άκυρης εγγραφής IDT).
