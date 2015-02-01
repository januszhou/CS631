#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <time.h>
#include <unistd.h>

#define BYTE (1024L)
#define K    (1024L*BYTE)
#define M    (1024L*K)

extern int A_flag; 
extern int a_flag; 
extern int c_flag; 
extern int d_flag; 
extern int F_flag; 
extern int f_flag; 
extern int h_flag; 
extern int i_flag; 
extern int k_flag; 
extern int l_flag; 
extern int n_flag; 
extern int q_flag; 
extern int R_flag; 
extern int r_flag; 
extern int S_flag; 
extern int s_flag;
extern int t_flag; 
extern int u_flag; 
extern int w_flag; 
extern int one_flag;
extern int C_flag;
extern int x_flag;
extern int to_terminal_flag;
extern int termwidth;
extern float blktimes;

struct Buf{
    int bf_inodwidth;
    int bf_fnamelen;
    int bf_blklen;
    int bf_totalblks;
    int bf_linklen;
    int bf_uidlen;
    int bf_gidlen;
    int bf_sizelen;
    int bf_total;
    FTSENT *bf_list;
}b;

int do_ls(int argc, char **argv);
int cmporder(const FTSENT **a, const FTSENT **b);
int cmpname(const FTSENT *a, const FTSENT *b);
int cmptimestch(const FTSENT *a, const FTSENT *b);
int cmpsize(const FTSENT *a, const FTSENT *b);
int cmptimemdf(const FTSENT *a, const FTSENT *b);
int cmptimelacc(const FTSENT *a, const FTSENT *b);
void do_print(FTSENT *fp, FTSENT *list);

void onedisplay(void);
void orderdisplay(void);
void longdisplay(void);
void filedisplay(FTSENT *p);
int  digitslength(long int d);

void modeprint(mode_t mode, char *buf);
void signprint(mode_t mode);
void qfroceprint(char *a, char *b);
void blkprint(struct stat *q);
void readblesizeprint(off_t size);
void timeprint(time_t *rawtime);
void linkprint(FTSENT *p);




