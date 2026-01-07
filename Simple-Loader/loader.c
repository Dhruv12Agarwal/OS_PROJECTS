#include "loader.h"


Elf32_Ehdr *ehdr=NULL;
Elf32_Phdr *phdr=NULL;
int fd;
char* file=NULL;//Pointer to the file
void** map=NULL; //Array that stores the addresses of the segments mapped.
int k=0;
int* segment_sizes= NULL;




/*
 * release memory and other cleanups
 */
void loader_cleanup() {
    for (int i = 0; i < k; i++) {
        munmap(map[i], (size_t)segment_sizes[i]);} //Minimal AI used here to understand about mmap and munmap function.
    free(map);
    free(segment_sizes);

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


  int ph_off=ehdr->e_phoff;
  int n=ehdr->e_phnum;

  phdr=(Elf32_Phdr*)(file+ph_off);

  // 1. Load entire binary content into the memory from the ELF file.
  unsigned int entry=ehdr->e_entry;
  int entry_segment=-1;
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
  map =malloc(c * sizeof(void*));//
  if (map==NULL){
    printf("malloc failed for map");
    free(file);
    return;
  }
  segment_sizes =malloc(c * sizeof(int));
  k=0;
  int load_entry=-1;
  for (int i=0;i<n;i++) {
    if (phdr[i].p_type == PT_LOAD) {
      if (i==entry_segment){
        load_entry=k;
      }
//Loading segments into memory by mmap library.
      char* addr= mmap(NULL, phdr[i].p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
      if (addr==MAP_FAILED) {
          printf("mmap failed\n");
        free(file);
        free(map);
        free(segment_sizes);
        return;
      }
      for (int j=0;j<phdr[i].p_memsz;j++) {
        addr[j]= file[phdr[i].p_offset +j];
      }
      map[k]=addr;
      segment_sizes[k]= phdr[i].p_memsz;
      k++;
    }
  }

  // 3. Allocate memory of the size "p_memsz" using mmap function
  //    and then copy the segment content
 void* entry_point_addr=(char*)map[load_entry] +(ehdr->e_entry-phdr[entry_segment].p_vaddr);
  int (*_start)()=(int(*)())entry_point_addr;
  //minimal ai used here to understand function typecasting
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
  int result= _start();
  printf("User _start return value = %d\n",result);
}

int main(int argc, char** argv)
{
  if(argc !=2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();
  return 0;
}
