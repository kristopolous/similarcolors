/*
 * colors.c
 *
 * Finding similar colors (matching images)
 *
 * See https://github.com/kristopolous/similarcolors for the latest version
 */
#include "common.h"

extern context_t g_lastcontext;

int analyze(char*fname, analSet *calc){
  int ix = 0,
      iy = 0,
      iz = 0,
      ia,
      total = 0;

  compSet bins = {{0}};
  char *pStart;
  ee *temp;
  float tmp, 
        max[MAXINDEX] = {0};

//  printf("(ocv) Loading [%s]\n",fname);
  IplImage *image = cvLoadImage(fname, CV_LOAD_IMAGE_COLOR);
  IplImage *ycbcr = cvCreateImage(cvGetSize( image ), 8, 3);

//  printf ("(ocv) Colospace [%s]\n", fname);
  cvCvtColor( image, ycbcr, CV_BGR2YCrCb );

//  printf ("(ocv) Binning\n");
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
//  printf ("(ocv) Aggregating\n");
  for(ix = 0; ix < CHANNELS; ix++) {  
    for(iy = 0; iy < BINCOUNT; iy++) {
      tmp = calc->data[ix][iy] = ((float)bins[ix][iy]) / (float)total;

      // index work
      // slot 0 is the highest
      // MAXINDEX is the lowest
      for(iz = MAXINDEX - 1; iz >=0; iz--) {
        if(tmp < max[iz]) {
          break;
        }
      }
      // to small
      if(iz == MAXINDEX - 1) {
        continue;
      }
      // this makes things swappable
      iz++;
      // push down
      for(ia = MAXINDEX - 2 ; ia >= iz; ia--) {
        max[ia + 1] = max[ia];
        calc->indexes[ia + 1] = calc->indexes[ia];
      }
      // now there will be a duplicate at ia (iz)
      max[iz] = tmp;
      calc->indexes[iz] = ix * BINCOUNT + iy;
      /*
      printf("(ocv) agg: %d %d", iz, ix * BINCOUNT + iy);
      for(ia = 0; ia < MAXINDEX ; ia++) {
        printf(" %d:%f ", calc->indexes[ia], max[ia]);
      }
      printf("\n");
      */
    }
  }

  cvReleaseImage(&ycbcr);
  cvReleaseImage(&image);
  return 0;
}

int compare(const void*el1, const void*el2) {
  /*
  printf("(cmp) %f %f\n", 
      ((distanceEl*)el1)->d ,
      ((distanceEl*)el2)->d );

  */
  return (int)(((distanceEl*)el2)->d * (500.0 * 1000.0 * 1000.0) - 
      ((distanceEl*)el1)->d * (500.0 * 1000.0 * 1000.0));
}

int match(analSet*ref, analTuple*comp, resSet*res, int len){
  int ix = 0, 
      iy,
      iz;

  float distance;
  analSet *pOtherSet;

  for(iz = 0; iz < len; iz++) {
    distance = 0;

    pOtherSet = &comp[iz].set;
    //printf("(match) %x\n", comp[iz].set);
    for(ix = 0; ix < CHANNELS; ix++) {
      for(iy = 0; iy < BINCOUNT; iy++) {
        distance += sqrt(ref->data[ix][iy] * pOtherSet->data[ix][iy]);
      }
    }
    (*res)[iz].d = distance;
    printf("(match) [%s] %s %f %f\n", g_lastcontext, *(comp[iz].name), (*res)[iz].d, distance);
    strcpy((*res)[iz].name, *(comp[iz].name));
  }
  
  qsort((*res), len, sizeof(distanceEl), compare);

  return 1;
}

int main( int argc, char** argv ){
  db_connect();
  start_server(argc, argv);
  return 0;
}
