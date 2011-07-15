#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <math.h>

IplImage* image= 0;
IplImage*luv= 0,
	*ycbcr;

#define CHANNELS	3
#define INFO	"color.info"
#define MAXFILE	80
#define OUTSIZE	64
#define BINCOUNT	32
typedef struct {
	unsigned char l,u,v;
}ee;
typedef struct {
	float bins[CHANNELS][BINCOUNT];
	char	filename[MAXFILE];
}block;

typedef struct {
	char filename[MAXFILE];
	float d;
}distanceEl;

int bins[CHANNELS][BINCOUNT];
void gen(int argc, char*argv[]){
	int iofile = open(INFO, O_WRONLY|O_CREAT|O_APPEND, 0666),
	    sz,
	    ix = 0,
	    iy = 0,
	    total = 0,
	    tmp = 0;

	char *pStart;
	block out;
	ee *temp;
	if(iofile == -1) {
		printf("Can't open %s for writing\n", INFO);
		exit(0);
	}
	memset(out.filename, 0, MAXFILE);
	while(--argc > 0) {
		memset(out.filename, 0, MAXFILE);
		memset(bins, 0, CHANNELS * BINCOUNT * sizeof(int));
		argv++;
		if((argc % 79) == 0) {
			printf("\033[9D%d  ", argc);
			fflush(0);
		}	
		image = cvLoadImage(*argv, CV_LOAD_IMAGE_COLOR);
		if(!image) {
			continue;
		}
		ycbcr = cvCreateImage(cvGetSize( image ), 8, 3);
		luv = cvCreateImage(cvGetSize( image ), 8, 3);
		cvCvtColor( image, ycbcr, CV_BGR2YCrCb );
		cvCvtColor( image, luv, CV_BGR2Luv );

		total = 0;
		for(ix = 0; ix < image->height; ix++) {
			pStart = ycbcr->imageData + ix * ycbcr->widthStep;
			for(iy = 0; iy < image->width; iy++) {
				temp = (ee*)(pStart + (iy * 3));
				bins[0][temp->l >> 3]++;
				bins[1][temp->u >> 3]++;
				bins[2][temp->v >> 3]++;

				total++;
			}
		}
		for(ix = 0; ix < CHANNELS; ix++) {	
			for(iy = 0; iy < BINCOUNT; iy++) {
				out.bins[ix][iy] = ((float)bins[ix][iy]) / (float)total;
			}
		}

		sz=strlen(*argv);
		strncpy((char*)out.filename, *argv, sz);
		tmp = write(iofile, &out, sizeof(block));	
		cvReleaseImage(&ycbcr);
		cvReleaseImage(&image);
	}
	close(iofile);
}
int compare(const void*el1, const void*el2) {
	return (int)(((distanceEl*)el2)->d * (500.0 * 1000.0 * 1000.0) - ((distanceEl*)el1)->d * (500.0 * 1000.0 * 1000.0));
}
void match(int argc, char*argv[]){
	struct stat st;
	int file,
	    ix = 0, 
	    iy,
	    iz,
	    count,
	    tmp;

	distanceEl *distanceList;
	float distance;
	block *pTemp = 0,
	      *pCompare = 0,
	      *pOther;

	file = open(INFO, O_RDONLY);
	if(file == 0) {
		exit(0);
	}
	stat(INFO, &st);
	pTemp = (block*)malloc(st.st_size);
	tmp = read(file, pTemp, st.st_size);
	count = st.st_size / sizeof(block);
	distanceList = (distanceEl*)malloc(count * sizeof(distanceEl));
	memset(distanceList, 0, sizeof(distanceEl) * count);

	// build up the reference set
	for(ix = 0; ix < count; ix++) {
		if(!memcmp(pTemp[ix].filename, argv[0], strlen(argv[0]))) {
			pCompare = &pTemp[ix];
			break;
		}
	}
	if(pCompare == 0) {
		//printf("[]");
		exit(0);
	}
	for(iz = 0; iz < count; iz++) {
		distance = 0;

		pOther = &pTemp[iz];
		for(ix = 0; ix < CHANNELS; ix++) {
			for(iy = 0; iy < BINCOUNT; iy++) {
				distance += sqrt(pCompare->bins[ix][iy] * pOther->bins[ix][iy]);
			}
		}
		distanceList[iz].d = distance;
		strcpy(distanceList[iz].filename, pOther->filename);
	}
	qsort(distanceList, count, sizeof(distanceEl), compare);
	if(count > OUTSIZE) {
		tmp = OUTSIZE;
	} else {
		tmp = count;
	}
	for(ix = 0; ix < tmp; ix++) {
		printf("<img src=%s>\n",distanceList[ix].filename);
	}

	/*
	tmp = count / 4 - (OUTSIZE / 2);
	for(ix = tmp; ix < tmp + OUTSIZE; ix++) {
		printf("%s ",distanceList[ix].filename);
	}
	tmp = count / 2 - (OUTSIZE / 2);
	for(ix = tmp; ix < tmp + OUTSIZE; ix++) {
		printf("%s ",distanceList[ix].filename);
	}
	*/

	free(pTemp);
}
int main( int argc, char** argv ){
	argv++;
	argc--;
	if((*argv && !strcmp(*argv, "-gen")) || argc < 1) {
		umask(0);
		gen(argc, argv);
		printf("\033[9D");
	} else {
		match(argc, argv);
	}
	return 0;
}
