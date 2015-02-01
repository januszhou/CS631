/* Author: JIANLONG ZHOU */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define MAXPATHLEN 500

int 
main(int argc,char** argv){

void *file_buf_src,*file_buf_dest;
int fd_src,fd_dest;
size_t file_from_len;
struct stat file_from_stat;

if(argc != 3){
	(void)fprintf(stderr,
		"Usage: %s [file1] [file2]\n\
		or: %s [file1] [dir]\n\
		or: %s [file1] [dir/file2]\n\
		or: %s [file1] [dir/subdir/subdir.../file2]\n",
		    argv[0],argv[0],argv[0], argv[0]);
	exit(EXIT_FAILURE);
}

/* try to open the source file, print error if any error happened */
// only give read permission
if((fd_src = open(argv[1],O_RDONLY)) == -1){
	(void)fprintf(stderr,"Open %s Error: %s\n",argv[1],strerror(errno));
	exit(EXIT_FAILURE);
}
if (fstat (fd_src, &file_from_stat) < 0){ 
	(void)fprintf(stderr,"Open %s Error: %s\n",argv[1],strerror(errno));
	//close fd_src
	close(fd_src);

	exit(EXIT_FAILURE);
}
file_from_len = file_from_stat.st_size;

/*create the target file*/
if((fd_dest = open(argv[2],O_RDWR|O_CREAT,S_IRUSR|S_IWUSR)) == -1){

	/*create dest file and check if it's directory*/
	if(errno == EISDIR){
		char to_path[MAXPATHLEN + 1]="";
		char *p_path;
		char *slash = "/";
		if(strlen(argv[2])+strlen(argv[1])<MAXPATHLEN){
			p_path = strncpy(to_path, argv[2], strlen(argv[2]));
			if(p_path[strlen(to_path)-1] != '/')
				p_path = strncat(to_path, slash, (int)strlen(slash));
    		p_path = strncat(to_path, argv[1], strlen(argv[1]));

			/*if directory: create file with same name as source file*/
			if((fd_dest = open(p_path,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR)) == -1){
				(void)fprintf(stderr,"Open %s Error: %s\n",argv[2],strerror(errno));
				//close fd_src
				close(fd_src);

				exit(EXIT_FAILURE);
			}
		}else{
			(void)fprintf(stderr,"%s: name/path too long",argv[2]);
			//close fd_src
			close(fd_src);

			exit(EXIT_FAILURE);
		}
	}else{
		(void)fprintf(stderr,"Open %s Error: %s\n",argv[2],strerror(errno));
		//close fd_src
		close(fd_src);

		exit(EXIT_FAILURE);
	}
}

/*Set the new file length same as source file*/
if(ftruncate(fd_dest,file_from_len)<0){
		(void)fprintf(stderr,"Open %s Error: %s\n",argv[2],strerror(errno));
		//close both
		close(fd_src);
		close(fd_dest);
		
		exit(EXIT_FAILURE);
}

/*Map source file into memory & close descriptor*/
file_buf_src=mmap(NULL,file_from_len,PROT_READ,MAP_SHARED,fd_src,SEEK_SET);
close(fd_src);

/*if map into memory failed, print error*/
if((int *)file_buf_src == MAP_FAILED){
	(void)fprintf(stderr,"map %s Error: %s\n",argv[1],strerror(errno));
	exit(EXIT_FAILURE);
}

/*Map the new file to the same place as source file & close the file discriptor*/
file_buf_dest =mmap(NULL,file_from_len,PROT_WRITE,MAP_SHARED,fd_dest,SEEK_SET);
close(fd_dest);
if((int *)file_buf_dest == MAP_FAILED){
	(void)fprintf(stderr,"map %s Error: %s\n",argv[2],strerror(errno));
	// if failed, remove a mapping
	munmap(file_buf_src,file_from_len);
	exit(EXIT_FAILURE);
}

/*Copy the memory into the new file map place*/
if(memcpy(file_buf_dest,file_buf_src,file_from_len)==NULL){
	(void)fprintf(stderr,"map %s Error: %s\n",argv[2],strerror(errno));
	munmap(file_buf_src,file_from_len);
	munmap(file_buf_dest,file_from_len);
	exit(EXIT_FAILURE);
}

/*copy success, nothing display, remove mapping*/
munmap(file_buf_src,file_from_len);
munmap(file_buf_dest,file_from_len);

exit(EXIT_SUCCESS);
}