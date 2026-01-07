// 1. An uninitialized global array (tests the .bss segment)
// This will create a segment where p_filesz is 0 but p_memsz is large.
// Your loader must correctly map a zero-filled page for this.
int my_bss_array[1024]; // 1024 * 4 bytes = 4096 bytes

// 2. An initialized global variable (tests the .data segment)
// This will create a segment where p_filesz > 0.
// Your loader must correctly copy this data from the file.
int my_data_var = 12345;

int fib(int n) {
  if (n < 2) return n;
  else return fib(n-1) + fib(n-2);
}

int _start() {
    // I changed 40 to 10. fib(40) takes minutes to run
    // and you'll think your loader is frozen. fib(10) is fast.
	int val = fib(10); 

    // 3. This write will cause a page fault in the .bss segment.
    // Your loader's "page_offset >= p_filesz" logic will be tested.
	my_bss_array[0] = 5; 

    // 4. This read will cause a page fault in the .data segment.
    // Your loader's "memcpy" logic will be tested.
	val = val + my_data_var;

    // 5. This read tests the .bss page again (to make sure it's mapped).
	val = val + my_bss_array[0];

	return val;
}
