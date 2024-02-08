/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "exec_parser.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

static so_exec_t *exec;
char *file_source;
int z=0;
struct sigaction def_handler;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	// /* TODO - actual loader implementation */

	int ok=0;	//verifica daca semnalul se incadreaza in segmente
	int nr_segments=exec->segments_no;	//numar segmente
	int lenght_page = getpagesize();	// lungime pagina
	char* pagefault = (char*)info->si_addr; //adresa fault pe care o cautam
	int d_file = open(file_source, O_RDONLY);

	for(int i=0;i<nr_segments;i++){
		int lenght_segment = exec->segments[i].mem_size ; //lungime segment [i]
		char* addr_segment = (char*)exec->segments[i].vaddr; //adresa de inceput a segmentului
		int lenght_file = exec->segments[i].file_size ; //lungime file in segment
		int *data=(int*)exec->segments[i].data;
		int offset_segment = exec->segments[i].offset ; //offset segment
		int permission_segment = exec->segments[i].perm;//permisiuni segment

		if((addr_segment) <= (pagefault) && (pagefault) < (addr_segment + lenght_segment)){
			ok=1;
			char *j;
			int c;

			for(j=addr_segment, c=0; (j+lenght_page) <= (pagefault) ;j=j+lenght_page,c++);

			if(data[c] == 1){
				def_handler.sa_sigaction(signum,info,context);
				return;
			}

			if(lenght_file != lenght_segment){
				if(addr_segment + lenght_file <= j && j < addr_segment + lenght_segment){	//
					mmap(j,lenght_page,permission_segment,MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
					data[c]=1;
					return;
				}

				if(j < (addr_segment + lenght_file) && (addr_segment + lenght_file) < j+lenght_page){	//Ma aflu in pagina unde sa gaseste sfarsitul fisierului
					mmap(j,lenght_page,permission_segment,MAP_FIXED | MAP_PRIVATE ,d_file,offset_segment+c*lenght_page);
					memset(j+lenght_file % lenght_page,0,(c+1)*lenght_page-lenght_file);
					data[c]=1;
					return ;
				}
				mmap(j,lenght_page,permission_segment,MAP_FIXED | MAP_PRIVATE ,d_file,offset_segment+c*lenght_page);
				data[c]=1;
				
			}else{
			mmap(j,lenght_page,permission_segment,MAP_FIXED | MAP_PRIVATE ,d_file,offset_segment+c*lenght_page);
				data[c]=1;
			}

		}

	}

	if(ok == 0)
		def_handler.sa_sigaction(signum,info,context);
}
int so_init_loader(void) 
{
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;
// ######################################################
	file_source = path;
	for(int i=0;i<exec->segments_no;i++){
		exec->segments[i].data = (int*)calloc( exec->segments[i].mem_size/getpagesize()+1,sizeof(int));
		if(exec->segments[i].data == NULL){
			printf("Nu s-a reusit alocarea *data\n");
			return -1;
		}
	}
// ########################################################
	so_start_exec(exec, argv);

	return -1;
}
