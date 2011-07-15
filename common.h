// standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <setjmp.h>
#define GETMAX(_a_, _b_)	( ( (_a_) > (_b_) ) ? (_a_) : (_b_) )
#define GETMIN(a, b)		( ( (a) < (b) ) ? (a) : (b) )
#define NULLFREE(_a_)		if((_a_)) { free((_a_)); (_a_) = 0; }

// bdb
#include <db.h>
#include "rb/red_black_tree.h"
#define DATABASE 	"access.db"
#define MAXRES		24
#define MAXINDEX	4
#define GROWBY		16
typedef char		cmpString[64];
typedef char		context_t[16];

// opencv
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <math.h>

#define CHANNELS	3
#define MAXFILE		20
#define OUTSIZE		20
#define BINCOUNT	32
#define BINMAX		BINCOUNT * CHANNELS
typedef struct {
	cmpString name;
	float d;
}distanceEl;
typedef struct { unsigned char l,u,v; }ee;
typedef distanceEl resSet[MAXRES * MAXINDEX];

// server
#include <time.h>
#include <langinfo.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

#define SMALL		128 
#define MEDIUM		4096
#define LARGE		16384
#define TRIES		5
#define READCLIENT	0x02
#define WRITECLIENT	0x08
#define HELP "+ Add\n? Search\ns Stats\nq Quit\n"
typedef char output[MEDIUM];

#define ISA(_s_, _l_) 		( ( !strcmp(*argv, _s_) ) || ( !strcmp(*argv, _l_) ) )

enum {
	FALSE, 
	TRUE, 
};

typedef struct {
	float data[CHANNELS][BINCOUNT];
	int   indexes[MAXINDEX];
} analSet;

typedef struct {
	analSet	set;
	cmpString *name;
} analTuple;

typedef struct {
	context_t context;
	char*name;
} ctx;

typedef	int compSet[CHANNELS][BINCOUNT];

void start_server();
void db_connect();
void db_close();
void format(resSet*res, output*out, int len);
void uncompress(cmpString in, char*out);

int search(context_t context, char*fname, resSet*set, int*len);
int insert(context_t context, char*fname);
int analyze(char*fname, analSet*set);
int match(analSet*ref, analTuple*comp, resSet*res, int len);
