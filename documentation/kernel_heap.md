# `kernel_heap.cpp` — Τεκμηρίωση

## Σκοπός αρχείου

Υλοποιεί έναν **kernel heap allocator** τύπου **next-fit**, με **συνένωση (coalescing) προς τα εμπρός και προς τα πίσω** κατά την αποδέσμευση, μέσω μιας διπλά συνδεδεμένης λίστας ελεύθερων/χρησιμοποιημένων μπλοκ (blocks). Προσφέρει τη γνωστή τριάδα `kmalloc` / `kfree` / `krealloc`, το αντίστοιχο του `malloc`/`free`/`realloc` της C βιβλιοθήκης, αλλά χτισμένο από την αρχή (freestanding, χωρίς εξάρτηση από libc).

## Ενσωματώσεις (Includes)

- `memory/heap/kernel_heap.h`: δηλώνει τη δομή `block_header` και το δημόσιο API.

## Δομή `block_header` (από το header, χρησιμοποιείται εδώ)

```cpp
struct alignas(8) block_header
{
    block_header* next;
    uint32_t flags;
    uint32_t size;
    block_header* prev;
    block_header* physical_prev;
};
```

Κάθε μπλοκ μνήμης (ελεύθερο ή δεσμευμένο) έχει μπροστά από τα δεδομένα του μία τέτοια κεφαλίδα (header). Σημειώστε τους δύο **διαφορετικούς** τύπους "γειτονίας":
- `next` / `prev`: θέση μέσα στη **συνδεδεμένη λίστα ελεύθερων μπλοκ** (free list) — δεν είναι απαραίτητα γειτονικά στη μνήμη.
- `physical_prev`: το μπλοκ που βρίσκεται **φυσικά ακριβώς πριν** από αυτό στη μνήμη (memory layout), ανεξάρτητα από το αν είναι στη free list. Αυτό είναι απαραίτητο για τη συνένωση προς τα πίσω (backward coalescing), αφού η free list μόνη της δεν αρκεί για να βρεθεί ο "φυσικός" γείτονας.

## Global state (ανώνυμος χώρος ονομάτων)

```cpp
kernel::memory::block_header* free_list_head{nullptr};
kernel::memory::block_header* heap_end{nullptr};
kernel::memory::block_header* searching_block{nullptr};
```

- `free_list_head`: κεφαλή της λίστας ελεύθερων μπλοκ.
- `heap_end`: η διεύθυνση αμέσως μετά το τέλος όλου του heap — χρησιμοποιείται για να ελεγχθεί αν ένα "επόμενο φυσικά" μπλοκ υπάρχει πράγματι μέσα στα όρια του heap.
- `searching_block`: δρομέας next-fit — η θέση από όπου ξεκινά η **επόμενη** αναζήτηση σε `kmalloc`, ανάλογη λογική με το `search_begin` του PMM.

## `heap_initialize(heap_start, heap_size)`

Αρχικοποιεί ολόκληρο τον heap ως **ένα** ενιαίο ελεύθερο μπλοκ που καλύπτει όλο τον διαθέσιμο χώρο (`heap_size - sizeof(block_header)` bytes χρησιμοποιήσιμα δεδομένα, αφού ο header "τρώει" χώρο). Θέτει `physical_prev = nullptr` (δεν υπάρχει τίποτα πριν) και υπολογίζει το `heap_end` προσθέτοντας το `heap_size` στην αρχική διεύθυνση.

## `kmalloc(requested_size)` — `[[gnu::regparm(1)]]`

### Βήμα 1: Αναζήτηση next-fit

```cpp
while(searching_block != nullptr)
{
    cached_size = searching_block->size;
    if((cached_size * (~(searching_block->flags) & 1)) >= requested_size) break;
    searching_block = searching_block->next;
}
```

Αυτό είναι ένα **branchless trick**: αντί για `if(!is_used && size >= requested_size)`, ο κώδικας πολλαπλασιάζει το μέγεθος με `(~flags) & 1`. Αν το μπλοκ είναι δεσμευμένο (`flags = 1`), το `(~1) & 1 = 0`, άρα το γινόμενο γίνεται `0` και η συνθήκη αποτυγχάνει αυτόματα χωρίς ξεχωριστό έλεγχο `if(used)`. Αν είναι ελεύθερο (`flags = 0`), το `(~0) & 1 = 1` και συγκρίνεται το πραγματικό μέγεθος. Αν δεν βρεθεί κατάλληλο μπλοκ, επιστρέφει `nullptr` (ενοποιημένη σύμβαση αποτυχίας, ίδια φιλοσοφία με το PMM).

### Βήμα 2: Πιθανό "σπάσιμο" (split) του μπλοκ

```cpp
constexpr uint32_t split_limit{sizeof(block_header) + 8};
const uint32_t remaining{cached_size - requested_size};
if(remaining >= split_limit) { ... }
```

Αν το βρεθέν μπλοκ είναι σημαντικά μεγαλύτερο απ' όσο ζητήθηκε (αρκετό περιθώριο για να χωρέσει ένα νέο `block_header` **και** τουλάχιστον 8 bytes χρήσιμου χώρου — το `split_limit`), δημιουργείται ένα δεύτερο μπλοκ (`remainder_block`) αμέσως μετά τα δεσμευμένα δεδομένα. Αυτό το νέο μπλοκ μπαίνει στη free list στη θέση του αρχικού και το `physical_prev` του γείτονά του (`remainder_block->next`) ενημερώνεται ώστε η φυσική αλυσίδα να παραμείνει συνεπής.

Αν το περιθώριο *δεν* είναι αρκετό, το μπλοκ δίνεται ολόκληρο (δεν αξίζει να δημιουργηθεί ένα μικροσκοπικό ελεύθερο θραύσμα — αυτό θα οδηγούσε σε εξωτερικό κατακερματισμό, external fragmentation, χωρίς πρακτικό όφελος).

### Βήμα 3: Σήμανση ως δεσμευμένο και ενημέρωση δρομέα αναζήτησης

```cpp
allocated->flags = 1;
const uintptr_t next_addr{reinterpret_cast<uintptr_t>(allocated->next)};
const bool is_null{next_addr == 0};
searching_block = reinterpret_cast<block_header*>(next_addr * !is_null + (reinterpret_cast<uintptr_t>(free_list_head) * is_null));
```

Ακόμη ένα **branchless** μοτίβο: αντί για `if(allocated->next) searching_block = allocated->next; else searching_block = free_list_head;`, ο κώδικας υπολογίζει και τις δύο πιθανές τιμές και επιλέγει τη σωστή πολλαπλασιάζοντας με boolean μάσκες (`is_null`/`!is_null`), αποφεύγοντας branch misprediction στο hot path.

Επιστρέφει `allocated + 1` — δηλαδή τον δείκτη **αμέσως μετά** τον header, που είναι η πραγματική διεύθυνση δεδομένων που βλέπει ο καλών (η κλασική τεχνική "header πριν τα δεδομένα").

## `kfree(ptr)` — `[[gnu::regparm(1)]]`

1. Αν `ptr == nullptr`, επιστρέφει αμέσως (ασφαλές no-op, όπως το standard `free`).
2. Ανακτά τον header με `reinterpret_cast<block_header*>(ptr) - 1` (αντίστροφο του `allocated + 1` στο `kmalloc`).
3. Καθαρίζει το flag (`flags = 0`).
4. **Forward coalescing**: υπολογίζει το φυσικά επόμενο μπλοκ (`ptr + size`). Αν αυτό βρίσκεται εντός των ορίων του heap (`next < heap_end`) και είναι ελεύθερο, το αφαιρεί από τη free list (επιδιορθώνοντας τους δεσμούς `prev`/`next`, συμπεριλαμβανομένων ειδικών περιπτώσεων όπου ήταν η κεφαλή της λίστας ή ο τρέχων `searching_block`) και **επεκτείνει** το μέγεθος του τρέχοντος μπλοκ ώστε να το απορροφήσει, μαζί με το μέγεθος του δικού του header (`sizeof(block_header) + next->size`).
5. Εισάγει το (πλέον ενδεχομένως μεγαλύτερο) μπλοκ στην **κεφαλή** της free list.
6. Θέτει `searching_block = allocated_memory`, ώστε το επόμενο `kmalloc` να ξεκινήσει την αναζήτηση από το μόλις ελευθερωμένο σημείο (next-fit heuristic — συχνά ένα μπλοκ που μόλις ελευθερώθηκε είναι καλός υποψήφιος για την επόμενη δέσμευση).

> Σημείωση: το backward coalescing (χρήση του `physical_prev`) αναφέρεται ρητά στην περιγραφή του αλγορίθμου next-fit με forward/backward coalescing στη μνήμη του project· σε αυτήν την έκδοση του `kfree` υλοποιείται ρητά μόνο η **forward** συνένωση μέσα στο σώμα της συνάρτησης. Το πεδίο `physical_prev` παραμένει διαθέσιμο στη δομή και ενημερώνεται σωστά σε κάθε `kmalloc`/split, έτοιμο να χρησιμοποιηθεί.

## `krealloc(ptr, new_size)` — `[[gnu::regparm(2)]]`

Ακολουθεί τη σημασιολογία (semantics) του standard `realloc`:
- Αν `ptr == nullptr`, ισοδυναμεί με `kmalloc(new_size)`.
- Αν `new_size == 0`, ισοδυναμεί με `kfree(ptr)` και επιστρέφει `nullptr`.
- Αν το υπάρχον μπλοκ έχει ήδη αρκετό χώρο (`size >= new_size`), επιστρέφει το **ίδιο** `ptr` χωρίς καμία αντιγραφή (fast path — αποφεύγει περιττή δουλειά).
- Αλλιώς, δεσμεύει νέο χώρο με `kmalloc`, αντιγράφει byte προς byte το μικρότερο από τα δύο μεγέθη (`new_size` ή το παλιό `size`, όποιο είναι μικρότερο — υπολογισμένο branchless μέσω πολλαπλασιασμού με `is_lower`/`!is_lower`), απελευθερώνει το παλιό μπλοκ με `kfree` και επιστρέφει τον νέο δείκτη.

## Σχεδιαστικές παρατηρήσεις

- Το next-fit σχήμα (σε αντίθεση με first-fit) διατηρεί έναν κινητό δρομέα αναζήτησης, μειώνοντας τον μέσο αριθμό συγκρίσεων όταν γίνονται πολλές διαδοχικές δεσμεύσεις.
- Η χρήση branchless αριθμητικής στα hot paths (`kmalloc`, μέρος του `krealloc`) είναι συνεπής με την αρχή του project ότι ο κώδικας πυρήνα σε κρίσιμα μονοπάτια πρέπει να αποφεύγει branches για να μειώνεται το κόστος branch misprediction.
- Το μέγεθος `sizeof(block_header) + 8` ως `split_limit` αποτρέπει τη δημιουργία εκφυλισμένων (πρακτικά άχρηστων) μικροσκοπικών ελεύθερων μπλοκ.
- Δεν υπάρχει `alignas`/padding ελέγχου εκτός του `alignas(8)` στη δομή `block_header` — αυτό εξασφαλίζει ότι κάθε επιστρεφόμενος δείκτης δεδομένων είναι τουλάχιστον 8-byte ευθυγραμμισμένος, αρκετό για τους περισσότερους τύπους δεδομένων σε 32-bit περιβάλλον.
