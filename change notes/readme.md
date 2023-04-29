Here documents changes made downstream that need to be introduced back upstream when editing upstream code.

For notes on changes made in task templates, see `templates.md`.

## srcv4 -> srcv4_cleanup

`main_01alt.c` -> `mine_single_block`: `i<=2147483647` changed to `<` to prevent an infinite loop (due to integer overflow).

`main_02Q2.c` -> main loop, in the access mutex: removed a `free` and immediate re-`malloc`

`main_03.c`, `main_04.c` -> main, prepare semaphores: removed a line that begins with that short block comment

`main_02.c`, `main_03.c`, `main_04.c` -> end: spelling fix (successfullt -> -ly)

## srcv4_cleanup -> part01alt

`main_01alt.c`: "found a result" printf removed the word "process" in the string.

## srcv4_cleanup -> part02

`main_02.c`: at the end, `shm_unlink` unlinks the incorrect shared memory name (should have a slash). (That part is yanked for the student anyway...)

Also, `simply` -> `simplify` (comment where I explained the dummy genesis block)

Also need to `free` two pointers.