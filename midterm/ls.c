#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "ls.h"

#define NOTPRINT 1

int A_flag, a_flag, c_flag, d_flag, F_flag, f_flag, h_flag, i_flag;
int k_flag, l_flag, n_flag, q_flag, R_flag, r_flag, S_flag, s_flag;
int t_flag, u_flag, w_flag, one_flag, C_flag, x_flag; 
int to_terminal_flag; 
int dirprint_total;
int regular_file_total;
int totalprint_flag;
int readoption;
int dirprint;
int termwidth = 80;
float blktimes;

static char *pro_name;
static void (*display)(void);
static int (*compare)(const FTSENT *a, const FTSENT *b);

int main(int argc, char *argv[]){
	int    		   c;
	char 		   dot[] = ".", *dotav[] = { dot, (char *)NULL };
	struct winsize win;
	FTS 		   *ftsp;
    FTSENT         *p, *tmp;

	pro_name = *argv;
	r_flag     = 1;
	C_flag     = 1;
	display    = orderdisplay;
	compare    = cmpname;
	readoption = FTS_PHYSICAL;
	blktimes   = 1;

	/* 
	** determine output to terminal or file
	** meantime if output is not a terminal 
	** set w, 1 flag to 1
	*/
	if (isatty(STDOUT_FILENO)) {
		if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &win) == 0 && win.ws_col > 0){
			termwidth = win.ws_col;
		}
		to_terminal_flag = q_flag = 1;
	} else{
		to_terminal_flag = 0;
		w_flag = one_flag = 1;
	}

	while((c = getopt(argc, argv, "1ACFRSacdfhiklnqrstuwx")) != -1)
		switch(c){
			case 'A':
				A_flag = 1;
				break;
			case 'a':
				a_flag = 1;
				readoption = FTS_PHYSICAL|FTS_SEEDOT;
				break;
			case 'c':
				c_flag = 1;
				u_flag = 0;
				compare = cmptimestch;
				break;
			case 'd':
				d_flag = 1;
				R_flag = 0;
				break;
			case 'F':
				F_flag = 1;
				break;
			case 'f':
				f_flag = 1;
				readoption = FTS_PHYSICAL|FTS_SEEDOT;
				break;
			case 'h':
				h_flag = 1;
				k_flag = 0;
				break;
			case 'i':
				i_flag = 1;
				break;
			case 'k':
				k_flag = 1;
				h_flag = 0;
				break;
			case 'l':
				l_flag = 1;
				one_flag = 0;
				n_flag = 0;
				C_flag = 0;
				x_flag = 0;
				display = longdisplay;
				break;
			case 'n':
				n_flag = 1;
				l_flag = 0;
				one_flag = 0;
				C_flag = 0;
				x_flag = 0;
				display = longdisplay;
				break;
			case 'q':
				q_flag = 1;
				w_flag = 0;
				break;
			case 'R':
				R_flag = 1;
				d_flag = 0;
				break;
			case 'r':
				r_flag = -1;
				break;
			case 'S':
				compare = cmpsize;
				break;
			case 's':
				s_flag = 1;
				break;
			case 't':
				compare = cmptimemdf;
				break;
			case 'u':
				u_flag = 1;
				c_flag = 0;
				compare = cmptimelacc;
				break;
			case 'w':
				w_flag = 1;
				q_flag = 0;
				break;
			case '1':
				n_flag = 1;
				l_flag = 0;
				one_flag = 0;
				C_flag = 0;
				x_flag = 0;
				display = onedisplay;
				break;
			case 'C':
				n_flag = 0;
				l_flag = 0;
				one_flag = 0;
				C_flag = 1;
				x_flag = 0;
				display = orderdisplay;
				break;
			case 'x':
				n_flag = 0;
				l_flag = 0;
				one_flag = 0;
				C_flag = 0;
				x_flag = 1;
				display = orderdisplay;
				break;
			default:
           	fprintf(stderr, "Usage: %s [−AaCcdFfhiklnqRrSstuwx1] [file ...]\n",
           	 pro_name);
           	exit(EXIT_FAILURE);
		}

	if(a_flag|f_flag)
		A_flag = 0;
	if(k_flag&&s_flag){
		if(getenv("BLOCKSIZE")!=NULL){
			blktimes = (float)atoi(getenv("BLOCKSIZE")) / (float)1024;
		}else
			blktimes = 0.5;
		setenv("BLOCKSIZE","1024",1);
	}
	if((argc==2 && argv[1][0]=='-')||argc==1){
		if((ftsp = fts_open(dotav, readoption,cmporder)) == (FTS *)NULL){
       		fprintf(stderr, "%s: fts_open error:%s\n",
       			pro_name,strerror(errno));
       		exit(EXIT_FAILURE);
   		}
   		argc -= 2;
	}else{
		if(argc>2 && argv[1][0]=='-'){
			argv+=2;
			argc -= 2;
		}else{
			argv++;
			argc--;
		}
		if((ftsp = fts_open(argv, readoption,cmporder)) == (FTS *)NULL){
       		fprintf(stderr, "%s: fts_open error:%s\n",
       			pro_name,strerror(errno));
       		exit(EXIT_FAILURE);
   		}
	} 

	do_print(NULL, fts_children(ftsp, 0));
    if(d_flag){
    	fts_close(ftsp);
    	return EXIT_SUCCESS;
    }

    while((p = fts_read(ftsp))!=NULL)
    	switch(p->fts_info){
    		case FTS_DC:
    			fprintf(stderr, "%s:A directory causes a cycle\n",p->fts_name);
    			exit(EXIT_FAILURE);
    			break;
    		case FTS_ERR:
    		case FTS_DNR:
    			errno = p->fts_errno; 
    			fprintf(stderr, "%s error: %s\n",p->fts_name,strerror(errno));
    			break;
    		case FTS_D:
    			if(dirprint){
    				printf("\n%s:\n",p->fts_path);
    			}else if(argc>1||R_flag){
    				printf("%s:\n", p->fts_path);
                	dirprint = 1;
    			}
    			tmp = fts_children(ftsp, 0);
    			totalprint_flag = 1;
				do_print(p, tmp);
				totalprint_flag = 0;
				if(R_flag == 0 && tmp)
					fts_set(ftsp, p, FTS_SKIP);
				break;
			default:
				break;
    	}

	fts_close(ftsp);
	return EXIT_SUCCESS;
}

void do_print(FTSENT *fp, FTSENT *list){
	register FTSENT *p;
	register struct stat *q;
	struct    passwd *pw;
	struct    group *gr;
	int       needstat = i_flag|s_flag|l_flag|n_flag|C_flag|x_flag;
	size_t    mx_fname  = 0;
	ino_t     mx_iod    = 0;
	blksize_t mx_blks   = 0;
	int       tt_blks   = 0;
	nlink_t   mx_links  = 0;
	int       mx_uidlen = 0;
	int       mx_gidlen = 0;
	off_t     mx_size   = 0;
	int       total     = 0;

	if(list == NULL)
		return;

    for(p = list; p; p = p->fts_link){
		if(p->fts_info == FTS_ERR || p->fts_info == FTS_NS 
			|| p->fts_info == FTS_NSOK){
            errno = p->fts_errno;
        	fprintf(stderr, "%s error: %s\n",p->fts_name,strerror(errno));
            p->fts_number = NOTPRINT;
            continue;
        }
        if(fp==NULL)
        	totalprint_flag = 0;
        if((fp == NULL) && (d_flag == 0)){
        	if(p->fts_info == FTS_D){
        		dirprint_total++;
     			p->fts_number = NOTPRINT;
     			continue;
        	}else{
        		regular_file_total++;
        	}
        }
        if((fp != NULL)&& (!(f_flag || a_flag || A_flag)) 
        	&& p->fts_name[0]=='.' &&(!d_flag)){
        	p->fts_number = NOTPRINT;
        	continue;
        }

        if(q_flag)
        	qfroceprint(p->fts_name, p->fts_name);

        if(needstat){
        	q = p->fts_statp;
        	mx_iod  = (mx_iod<q->st_ino)?q->st_ino:mx_iod;
        	mx_blks = (mx_blks<q->st_blocks)?q->st_blocks:mx_blks;
        	tt_blks += q->st_blocks;
        	mx_links= (mx_links<q->st_nlink)?q->st_nlink:mx_links;
        	mx_size = (mx_size<q->st_size)?q->st_size:mx_size;
        	if(l_flag){
        		pw = getpwuid(q->st_uid);
        		gr = getgrgid(q->st_gid);
        		if(pw!=NULL && gr!=NULL){
        			mx_uidlen = (mx_uidlen<strlen(pw->pw_name))
        				?strlen(pw->pw_name):mx_uidlen;
        			mx_gidlen = (mx_gidlen<strlen(gr->gr_name))
        				?strlen(gr->gr_name):mx_gidlen;
        		}else{
        			mx_uidlen = (mx_uidlen<digitslength(q->st_uid))
        				?digitslength(q->st_uid):mx_uidlen;
        			mx_gidlen = (mx_gidlen<digitslength(q->st_gid))
        				?digitslength(q->st_uid):mx_gidlen;
        		}
        	}
        	if(n_flag){
        		mx_uidlen = (mx_uidlen<digitslength(q->st_uid))
        			?digitslength(q->st_uid):mx_uidlen;
        		mx_gidlen = (mx_gidlen<digitslength(q->st_gid))
        			?digitslength(q->st_uid):mx_gidlen;
        	}
        }
		mx_fname = (mx_fname<p->fts_namelen)?p->fts_namelen:mx_fname;
		total++;
    }
    b.bf_total    = total;
    b.bf_fnamelen = mx_fname;
    if(needstat){
    	b.bf_inodwidth = digitslength(mx_iod);
    	b.bf_blklen    = digitslength((int)((float)mx_blks*blktimes));
    	b.bf_totalblks = (int)((float)tt_blks*blktimes);
    	b.bf_linklen   = digitslength(mx_links);
    	b.bf_uidlen    = mx_uidlen;
    	b.bf_gidlen    = mx_gidlen;
    	b.bf_sizelen   = digitslength(mx_size);
    }

    b.bf_list = list;
    display();

}

void onedisplay(void){
	register FTSENT *p;
	if(b.bf_total == 0)
		return;
	if(s_flag && to_terminal_flag && totalprint_flag){
		printf("total %d\n", b.bf_totalblks);
	}
	for(p = b.bf_list; p; p = p->fts_link){
		if(p->fts_number == NOTPRINT){
			continue;
		}
		if(i_flag){
			printf("%*d ",b.bf_inodwidth,(int)p->fts_statp->st_ino);
		}
		if(s_flag){
			blkprint(p->fts_statp);
		}
		printf("%s", p->fts_name);
		if(F_flag)
			signprint(p->fts_statp->st_mode);
		printf("\n");	
	}

}

void orderdisplay(void){
	register FTSENT *p;
	int total_entry_len   = b.bf_fnamelen+2;
	int col_per_row       = 0;
	int total_row         = 0;
	int write_space       = 0;
	int count             = 0;
	int count1            = 0;
	if(b.bf_total == 0)
		return;

	total_entry_len += F_flag            ? 1:0;
	total_entry_len += h_flag&&s_flag    ? 5:0;
	total_entry_len += (!h_flag)&&s_flag ? b.bf_blklen+1:0;
	total_entry_len += i_flag            ? b.bf_inodwidth+1:0;

	col_per_row = (int)(termwidth/(total_entry_len+1));
	if(col_per_row == 0){
		col_per_row = 1;
	}else{
		count1 = col_per_row > b.bf_total ? b.bf_total:col_per_row;
		write_space = (int)((termwidth/count1) - total_entry_len);
		if(write_space>10) write_space = 10;
		
	}
	if(col_per_row == 1){
		onedisplay();
		return;
	}
	total_row = (b.bf_total/col_per_row)+1;

	if(s_flag && to_terminal_flag && totalprint_flag){
		printf("total %d\n", b.bf_totalblks);
	}
	if(x_flag || total_row==1){
		for(p = b.bf_list; p; p = p->fts_link){
			if(p->fts_number == NOTPRINT){
				continue;
			}
			count++;
			filedisplay(p);
			if(count==col_per_row){
				count = 0;
				printf("\n");
			}else{
				for(count1 = 0;count1<write_space;count1++)
					printf(" ");
			}
		}
		if(count)
			printf("\n");
	}else{
		int i=0,j = 0;
		int total = b.bf_total;
		int lineend = col_per_row;
		FTSENT **array;
		p = b.bf_list;
		if((array = (FTSENT **)malloc(sizeof(FTSENT *) * b.bf_total)) == NULL){
            fprintf(stderr, "%s error:%s\n",
       			pro_name,strerror(errno));
			exit(EXIT_FAILURE);
        }
        for(p = b.bf_list; p; p = p->fts_link) 
        	if(p->fts_number != NOTPRINT) 
            	array[i++] = p;
		for(i=0,count=0,p = b.bf_list;i<total;i++){
			count++;
			j = total_row*((i)%col_per_row) + (i)/col_per_row;
			if(j >= b.bf_total){
				total++;
				if(count >1)
					filedisplay(NULL);
				count = 0;
			}else{
				filedisplay(array[j]);
				if(count==lineend){
					count = 0;
					printf("\n");
				}else{
					for(count1 = 0;count1<write_space;count1++)
						printf(" ");
				}
			}
		}
		if(count)
			printf("\n");
	}	
}

// simple output, not column
void filedisplay(FTSENT *p){
	struct stat *q;
	int count1 = 0;
	if(p==NULL){
		printf("\n");
		return;
	}
	q = p->fts_statp;
	if(i_flag){
		printf("%*d ",b.bf_inodwidth,(int)q->st_ino);
	}
	if(s_flag){
		blkprint(q);
	}
	printf("%s", p->fts_name);
	if(F_flag)
		signprint(q->st_mode);
	for(count1 = 0;count1<b.bf_fnamelen-p->fts_namelen;count1++)
		printf(" ");
	printf("  ");
}

// if argu contains l or n, force to long display
void longdisplay(void){
	register FTSENT *p;
	register struct stat *q;
	struct passwd *pw;
	struct group *gr;
	char   buf_mode[10];
	if(b.bf_total == 0)
		return;
	if(totalprint_flag){
		printf("total %d\n", b.bf_totalblks);
	}
	for(p = b.bf_list; p; p = p->fts_link){
		if(p->fts_number == NOTPRINT){
			continue;
		}
		q = p->fts_statp;
		if(i_flag){
			printf("%*d ",b.bf_inodwidth,(int)q->st_ino);
		}
		if(s_flag){
			blkprint(q);
		}
		modeprint(q->st_mode,buf_mode);
		printf("%s  ",buf_mode);
		printf("%*d ", b.bf_linklen,(int)q->st_nlink);

		if(l_flag){
			pw = getpwuid(q->st_uid);
        	gr = getgrgid(q->st_gid);
        	if(pw!=NULL && gr!=NULL){
        		printf("%*s  %*s  ",b.bf_uidlen,pw->pw_name
        		,b.bf_gidlen,gr->gr_name);
        	}else{
        		printf("%*d  %*d  ",b.bf_uidlen,(int)q->st_uid
        		,b.bf_gidlen,(int)q->st_gid);
        	}
		}else{
			printf("%*d  %*d  ",b.bf_uidlen,(int)q->st_uid
        		,b.bf_gidlen,(int)q->st_gid);
		}

		if(h_flag){
			readblesizeprint(q->st_size);
		}else{
			printf("%*ld ",b.bf_sizelen,(long int)q->st_size);
		}

		if(c_flag){
			timeprint(&q->st_ctime);
		}else if(u_flag){
			timeprint(&q->st_atime);
		}else{
			timeprint(&q->st_mtime);
		}

		printf("%s", p->fts_name);
		if(F_flag)
			signprint(q->st_mode);
		if(S_ISLNK(q->st_mode)){
			linkprint(p);
		}
		printf("\n");
	}
}

/***********************Helper***************************/

int cmporder(const FTSENT **a, const FTSENT **b){
    FTSENT *a_p, *b_p;
    a_p = *(FTSENT **)a;
    b_p = *(FTSENT **)b;

    if((a_p->fts_info == FTS_ERR)  || (b_p->fts_info == FTS_ERR))
        return 0; 
    if( (a_p->fts_info == FTS_NS)  || (a_p->fts_info == FTS_NSOK) 
    	||(b_p->fts_info == FTS_NS)|| (b_p->fts_info == FTS_NSOK))
        return cmpname(a_p, b_p);
    if(a_p->fts_info == b_p->fts_info)
        compare(a_p, b_p);
    if(a_p->fts_level == 0){       
        if(a_p->fts_info == FTS_D){
            if(b_p->fts_info != FTS_D)
                return 1;
        }
        else if(b_p->fts_info == FTS_D)
            return -1;
    }
    return compare(a_p, b_p);
}

int cmpname(const FTSENT *a, const FTSENT *b){
    return r_flag * strcmp(a->fts_name, b->fts_name);
}
int cmptimestch(const FTSENT *a, const FTSENT *b){
	return r_flag * (b->fts_statp->st_ctime - a->fts_statp->st_ctime);
}
int cmpsize(const FTSENT *a, const FTSENT *b){
	return r_flag * (b->fts_statp->st_size - a->fts_statp->st_size);
}
int cmptimemdf(const FTSENT *a, const FTSENT *b){
	return r_flag * (b->fts_statp->st_mtime - a->fts_statp->st_mtime);
}
int cmptimelacc(const FTSENT *a, const FTSENT *b){
	return r_flag * (b->fts_statp->st_atime - a->fts_statp->st_atime);
}

int digitslength(long int d){
	char buf[31];
	snprintf(buf, sizeof(buf), "%ld", d);
	return (strlen(buf));
}

/******************** output part ************************/
void modeprint(mode_t mode, char *buf){
    strcpy(buf, "----------");
    if(S_ISDIR(mode))
        buf[0] = 'd';
    if(S_ISCHR(mode))
        buf[0] = 'c';
    if(S_ISBLK(mode))
        buf[0] = 'b';
    if(S_ISLNK(mode))
        buf[0] = 'l';
    if(S_ISSOCK(mode))
        buf[0] = 's';
    if(S_ISFIFO(mode))
        buf[0] = 'p';
    if(mode & S_IRUSR)
        buf[1] = 'r';
    if(mode & S_IWUSR)
        buf[2] = 'w';
    if(mode & S_IXUSR)
        buf[3] = 'x';
    if(mode & S_ISUID)
        buf[3] = 's';
    if(mode & S_IRGRP)
        buf[4] = 'r';
    if(mode & S_IWGRP)
        buf[5] = 'w';
    if(mode & S_IXGRP)
        buf[6] = 'x';
    if(mode & S_ISGID)
        buf[6] = 'x';
    if(mode & S_IROTH)
        buf[7] = 'r';
    if(mode & S_IWOTH)
        buf[8] = 'w';
    if(mode & S_IXOTH)
        buf[9] = 'x';
    if(mode & S_ISVTX)
        buf[9] = 't';
}

void signprint(mode_t mode){
    switch(mode & S_IFMT){
    case S_IFDIR:
        putchar('/');
        break;
    case S_IFLNK:
        putchar('@');
        break;
    case S_IFSOCK:
        putchar('=');
        break;
    case S_IFIFO:
        putchar('|');
        break;
    default:
        if(mode & (S_IXUSR | S_IXGRP | S_IXOTH)){
            putchar('*');
        }else{
            putchar(' ');
        }
        break;
    }
}

void qfroceprint(char *a, char *b){
    char *p, *q;

    for(p=a, q=b; *p; p++, q++){
        if(isprint(*p))
            *q = *p;
        else
            *q = '?';
    }
} 

void blkprint(struct stat *q){ 
    if(h_flag&&(!l_flag)){
        readblesizeprint(q->st_size);
    }else{
        printf("%*d ",b.bf_blklen,
           (int)((float)q->st_blocks*blktimes));
    }  
}

void readblesizeprint(off_t size){
    float size_tmp;
    if(size>=0 && size<=999){
        printf(" %3dB ",(int)size);
    }else if(size>999 && size<(off_t)(999*BYTE+512)){
        size_tmp = ((float)size / (float)BYTE);
        if(size_tmp < 10){
            printf(" %1.1fK ",(size_tmp));
        }else{
            printf(" %3dK ",(int)(size_tmp));
        }
    }else if(size>=(off_t)(999*BYTE+512) && size<(long int)(999*K+512)){
        size_tmp = ((float)size / (float)K);
        if(size_tmp < 10){
            printf(" %1.1fM ",(size_tmp));
        }else{
            printf(" %3dM ",(int)(size_tmp));
        }
    }else if(size>=(long int)(999*K+512) && size<=(long int)(999*M+512)){
        size_tmp = ((float)size / (float)M);
        if(size_tmp < 10){
            printf(" %1.1fG ",(size_tmp));
        }else{
            printf(" %3dG ",(int)(size_tmp));
        }
    }else{
        printf("     ");
    }
}

void timeprint(time_t *rawtime){
    struct tm * timeinfo = localtime(rawtime);
    static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if(timeinfo){
        printf("%.3s%3d %.2d:%.2d ",
        mon_name[timeinfo->tm_mon],
        timeinfo->tm_mday,
        timeinfo->tm_hour,
        timeinfo->tm_min);
    }else
        printf("             ");
}

void linkprint(FTSENT *p){
    int lnklen;
    char name[256], path[256];

    if(p->fts_level == 0)
        snprintf(name, sizeof(name), "%s", p->fts_name);
    else 
        snprintf(name, sizeof(name), "%s/%s", p->fts_parent->fts_accpath, p->fts_name);    
    if((lnklen = readlink(name, path, sizeof(path) - 1)) == -1){
        fprintf(stderr, "\n%s error: %s\n", name, strerror(errno));
        return;
    }
    path[lnklen] = '\0';
    printf(" -> %s", path);
}
