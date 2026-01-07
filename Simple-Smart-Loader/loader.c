#include "loader.h"
#include <signal.h>

Elf32_Ehdr *ehdr=NULL;
Elf32_Phdr *phdr=NULL;
int fd;
char* file=NULL;
int fault_count = 0;
int alloc_count = 0;
int alloc_seg_count=0;
int k=0;

Elf32_Phdr* alloc_segments[200];
unsigned long frag_size1=0;

#define PAGE_SIZE 4096
#define MAX_PAGES 256
void* mapped_pages[MAX_PAGES];
int mapped_page_count = 0;



void allot_memory(int sig, siginfo_t *si, void *ucontext){
 unsigned long fault_addr = (unsigned long)si->si_addr;
  fault_count++;


  int faulty_seg=-1;
  Elf32_Phdr* target_seg = NULL;
  for(int i =0; i < ehdr->e_phnum; i++){
      if (fault_addr >= phdr[i].p_vaddr && fault_addr< (phdr[i].p_vaddr+phdr[i].p_memsz)){
        faulty_seg=i;
        target_seg=&(phdr[i]);
      }
  }
  if (target_seg == NULL) {
    printf("Segmentation fault at address %lx not in PT_LOAD segment\n", fault_addr);
    exit(1);
  }
  int flag=0;
  for(int i=0;i<alloc_seg_count;i++){
    if(target_seg->p_vaddr==alloc_segments[i]->p_vaddr){
      flag=1;
      break;
    }
  }
  if (!flag){
    
    alloc_segments[alloc_seg_count]=target_seg;
    alloc_seg_count++;
  }
  if(target_seg==NULL){
    perror("Not in ptload");
    exit(1);
  }

 unsigned long page_start = fault_addr - fault_addr%4096;

  void* page = mmap((void*)page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,-1, 0);
  if (page == MAP_FAILED) {
    perror("mmap failed in handler");
    exit(1);
}


  if (page == MAP_FAILED) {
    perror("mmap failed in handler");
    exit(1);
  }

  alloc_count++;
  mapped_pages[mapped_page_count++] = (void*)page_start;

 unsigned long page_offset= page_start - target_seg->p_vaddr;
 unsigned long segment_offset = target_seg->p_offset;

 unsigned long page_offset_elf = segment_offset+page_offset;



  unsigned long used_bytes = 0;
  if (page_offset < target_seg->p_filesz) {
      unsigned long size = target_seg->p_filesz - page_offset;
      if (size > PAGE_SIZE) size = PAGE_SIZE;
      used_bytes = size;
      memcpy((void*)page_start, file + target_seg->p_offset + page_offset, size);
  } else if (page_offset < target_seg->p_memsz) {

      unsigned long size = target_seg->p_memsz - page_offset;
      if (size > PAGE_SIZE) size = PAGE_SIZE;
      used_bytes = size;
      memset((void*)page_start, 0, size);  
  }
  frag_size1 += (PAGE_SIZE - used_bytes);

  int perms = 0;
    if (target_seg->p_flags & PF_R) perms |= PROT_READ;
    if (target_seg->p_flags & PF_W) perms |= PROT_WRITE;
    if (target_seg->p_flags & PF_X) perms |= PROT_EXEC;

    if (mprotect((void*)page_start, PAGE_SIZE, perms) == -1) {
        perror("mprotect failed");
        exit(1);
    }







}
/*
 * release memory and other cleanups
 */
void loader_cleanup() {

  for (int i = 0; i < mapped_page_count; i++) {
    if (munmap(mapped_pages[i], PAGE_SIZE) == -1) {
      perror("munmap cleanup");
    }
  }

  free(file);

}
/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd =open(exe[1], O_RDONLY);
  if (fd<0){
    printf("ERROR Opening File\n");
    return;
  }
  //Loading the file into memory.
  int size=lseek(fd, 0, SEEK_END);
  if (size == -1) {
    perror("lseek failed to get file size");
    close(fd);
    return;
  }
  lseek(fd,0,SEEK_SET);
  file = malloc(size);

  if (file==NULL){
    printf("ERROR ALLOCATING SPACE FOR FILE IN HEAP\n");
    close(fd);
    return;

  }
  if (read(fd, file, size) !=size) {
    printf("Error reading file\n");
    free(file);
    close(fd);
    return;
  }

  close(fd);




  ehdr = (Elf32_Ehdr*)file;

  if(ehdr->e_ident[0]!=0x7f || ehdr->e_ident[1]!='E' || ehdr->e_ident[2]!='L'||ehdr->e_ident[3]!='F'){ //check if its a valid ELF file
    printf("ERROR: Not a valid ELF file\n");
    free(file);
    return;
  }
  if(size<sizeof(Elf32_Ehdr)){ //check if file size is less than ELF header size
    printf("ERROR: File not a ELF_32 bit file\n");
    free(file);
    return;
  }


 unsigned long ph_off=ehdr->e_phoff;
 unsigned long n=ehdr->e_phnum;

  phdr=(Elf32_Phdr*)(file+ph_off);

  // 1. Load entire binary content into the memory from the ELF file.
 unsigned long entry=ehdr->e_entry;
 unsigned long entry_segment=-1;
//checking which PT_LOAD segment contains the entry point.
  for (int i=0;i< ehdr->e_phnum;i++) {

    if (phdr[i].p_type==PT_LOAD) {

      if (entry >= phdr[i].p_vaddr && entry< (phdr[i].p_vaddr+phdr[i].p_memsz)) {
        entry_segment=i;
        break;
      }
    }
  }
  // 2. Iterate through the PHDR table and find the section of PT_LOAD
  //    type that contains the address of the entrypoint method in fib.c


  int c=0;
  //Counting the PT_LOAD segments.
  for (int i=0; i< ehdr->e_phnum;i++) {

    if (phdr[i].p_type==PT_LOAD) {

      c++;
    }
  }


  k=0;



  int (*_start)() = (int(*)()) ehdr->e_entry;
  int result = _start();
  printf("Result: %d\n", result);

}

int main(int argc, char** argv)
{
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = allot_memory;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("sigaction failed");
    exit(1);
  }

  if(argc !=2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  load_and_run_elf(argv);
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution

  // 3. invoke the cleanup routine inside the loader

  unsigned long frag_size=4096*alloc_count;
  for (int i=0; i <alloc_seg_count; i++){
    frag_size-=alloc_segments[i]->p_memsz;
    printf("%u\n", alloc_segments[i]->p_vaddr);
  }
  loader_cleanup();
  printf("Fault count: %d\n", fault_count);
  printf("Allocations: %d\n", alloc_count);


 
  printf("Fragmentation size : %lu\n", frag_size1);


  return 0;
}
