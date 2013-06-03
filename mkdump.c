#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elf.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

static const struct option options[]= {
	{"cpu_note_paddr",1,0,'c'},
	{"vmcore_note_paddr",1,0,'v'},
	{0,0,0,0},
};

static const char short_options[] = "c:v:";

void main(int argc, char * argv[])
{
	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;	
	Elf32_Nhdr *nhdr_cpu_ptr;
	Elf32_Nhdr *nhdr_vmcoreinfo_ptr;

	void * note_cpu_section=NULL;
	void * note_vmcoreinfo_section=NULL;
	struct stat stat_memory_file;
	int fd_vmcore=0;
	int fd_new=0;
	int fd_memory=0;
	int opt;
	unsigned long cpu_note_paddr =0;
	unsigned long vmcore_note_paddr=0;
	char * memory_path=NULL;
	char * stop_string;
	unsigned char buffer[116];
	
	while((opt=getopt_long(argc,argv,short_options,options,0))!=-1){
		switch(opt){
		case 'c':
			printf("c %s\n",optarg);
			cpu_note_paddr = strtoul(optarg,&stop_string,16);
			break;
		case 'v':
			printf("v %s\n",optarg);
			vmcore_note_paddr = strtoul(optarg,&stop_string,16);	
			break;
		}
	
	}
	memory_path = argv[optind];
	if(cpu_note_paddr==0||vmcore_note_paddr==0||memory_path==NULL)
	{
		printf("usage: %s -c 0Xxxx -v 0Xxxx memory-image\n",argv[0]);
		return;
	};	
	fd_vmcore=open("./ElfHead", O_RDONLY);
	fd_new=open("./newvmcore", O_RDWR|O_CREAT, 00777);
	fd_memory=open(memory_path, O_RDONLY|O_SYNC);
	
	
	if(stat(memory_path, &stat_memory_file)!=0){
		printf("Could stat memory core file: %s\n",strerror(errno));
	}
	
	if((fd_vmcore < 0 )||(fd_new < 0 )||(fd_memory < 0)){
		printf("Fuck, could open vmcore %d %d %d\n",fd_vmcore,fd_new,fd_memory);
		return;
	}
	
	read(fd_vmcore, buffer, 116);
	
	ehdr =(Elf32_Ehdr *)buffer;
	ehdr->e_phnum = 2;
	ehdr++;

	lseek(fd_memory,cpu_note_paddr,SEEK_SET);//cpu_notes
	note_cpu_section = malloc(1024);
	memset(note_cpu_section,0,1024);
	read(fd_memory,note_cpu_section,1024);
	
	nhdr_cpu_ptr = note_cpu_section;
	unsigned long sz_cpu = 0;
	sz_cpu = sizeof(Elf32_Nhdr)+
		((nhdr_cpu_ptr->n_namesz+3)&~3)+
		((nhdr_cpu_ptr->n_descsz+3)&~3);
	printf("cpu note name size: %x\n",nhdr_cpu_ptr->n_namesz);

	lseek(fd_memory,vmcore_note_paddr,SEEK_SET);//vmcoreinfo
	note_vmcoreinfo_section = malloc(1024);
	memset(note_vmcoreinfo_section,0,1024);
	read(fd_memory,note_vmcoreinfo_section,1024);

	nhdr_vmcoreinfo_ptr = note_vmcoreinfo_section;
	unsigned long sz_vmcoreinfo = 0;
	sz_vmcoreinfo = sizeof(Elf32_Nhdr)+
			 ((nhdr_vmcoreinfo_ptr->n_namesz+3)&~3) + 
			 ((nhdr_vmcoreinfo_ptr->n_descsz+3)&~3);
	printf("vm note name size: %x\n",nhdr_vmcoreinfo_ptr->n_namesz);
	
	
	phdr =(Elf32_Phdr *)ehdr;
	/*
 	* phdr[0] is for PT_NOTE type
 	* phdr[1] is for PT_LOAD type 	
 	* */
	
	phdr->p_filesz = phdr->p_memsz = sz_cpu + sz_vmcoreinfo;
	phdr->p_offset = sizeof(Elf32_Ehdr)+
				2 * sizeof(Elf32_Phdr);

	phdr->p_type = PT_NOTE;
	
	phdr++;
	phdr->p_filesz = phdr->p_memsz = stat_memory_file.st_size;
	phdr->p_offset = sizeof(Elf32_Ehdr)+
				2 * sizeof(Elf32_Phdr)+
				sz_cpu+
				sz_vmcoreinfo;

	phdr->p_type = PT_LOAD;
	
	//write PT_NOTE program segment to new vmcore
	write(fd_new, buffer, 116);	
	write(fd_new, note_cpu_section, sz_cpu);	
	write(fd_new, note_vmcoreinfo_section, sz_vmcoreinfo);	
	
	printf("fd_memory size :%lx\n",stat_memory_file.st_size);
	unsigned char tmp[4096];
	lseek(fd_memory, 0, SEEK_SET);
	ssize_t ret;
	while((ret = read(fd_memory, tmp, 4096)) > 0)
	{
		write(fd_new,tmp,ret);
	}	
#if 0
	printf("%d\n",nhdr.n_namesz);
	printf("%d\n",nhdr.n_descsz);
	printf("%d\n",nhdr.n_type);
#endif	
	free(note_cpu_section);
	free(note_vmcoreinfo_section);

	close(fd_memory);
	close(fd_new);	
	close(fd_vmcore);

}
