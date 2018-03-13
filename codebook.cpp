#include "stdafx.h"
#include "google_two.h"

CodeBook::CodeBook( CvSize sz )
{
  cbBounds[0] = 12;
  cbBounds[1] = 12;
  minMod[0] = 10;
  minMod[1] = 10;
  maxMod[0] = 8;
  maxMod[1] = 8;
  numPixels = sz.width * sz.height;
  if( numPixels > MAXPIXELS ) printf("error, maxpixels < %d\n", numPixels);
  for( int p = 0; p < numPixels; p++ )
  {
    cbs[p].numEntries = 0;
    cbs[p].t = 0;
  }
}

uchar CodeBook::background_diff( uchar* p, int pixelCount )
{
  int matchChannel;
  int i = 0;
  int min, max;
  uchar* ptr;
  code_element *cf;
  code_book *book = &cbs[pixelCount];
  int numEntries = book->numEntries;

  for( i = 0; i < numEntries; i++ )
  {
    matchChannel = 0;
    cf = book->cb[i];
    for(int n = 0; n < CHANNELS; n++)
    {
      ptr = p + n;
      min = cf->min[n] - minMod[n];
      max = cf->max[n] + maxMod[n];
      if((min <= *ptr) && (*ptr <= max)) matchChannel++;
      else break;
    }
    if( matchChannel == CHANNELS ) break;
  }
  if( i >= numEntries ) return(255);
	  
  return(0);
}

int CodeBook::clear_stale_entries( int pixelCount )
{
  code_book *book = &cbs[pixelCount];
  int numEntries = book->numEntries;
  int staleThresh = book->t>>1;
  int *keep = new int [numEntries];
  int keepCnt = 0;

  for( int i = 0; i < numEntries; i++ )
  {
    if( book->cb[i]->stale > staleThresh ) keep[i] = 0;
    else
    {
      keep[i] = 1;
      keepCnt += 1;
    }
  }
  book->t = 0;
  if( keepCnt > 0 )
  {
    code_element **foo = new code_element* [keepCnt];
    int k=0;
    for( int ii = 0; ii < numEntries; ii++ )
    {
      if( keep[ii] )
      {
        foo[k] = book->cb[ii];
        foo[k]->t_last_update = 0;
        k++;
      }
	  else delete book->cb[ii];
    }
    delete [] keep;
    delete [] book->cb;
    book->cb = foo;
  }
  else
  {
    for( int ii = 0; ii < numEntries; ii++ )
      delete book->cb[ii];
  
    delete [] keep;
    delete [] book->cb;
  }
  int numCleared = numEntries - keepCnt;
  book->numEntries = keepCnt;
  return( numCleared );
}

int CodeBook::update_codebook( uchar* p, int pixelCount )
{
  int high[CHANNELS], low[CHANNELS];
  int n;
  int i = 0;
  code_book* book = &cbs[pixelCount];
  int numEntries = book->numEntries;

  book->t++;
  for( n=0; n < CHANNELS; n++ )
  {
    high[n] = *(p+n)+*(cbBounds+n);
    if(high[n] > 255) high[n] = 255;

    low[n] = *(p+n)-*(cbBounds+n);
    if( low[n] < 0 ) low[n] = 0;
  }

  int matchChannel;
  code_element* element;
  for( i=0; i < numEntries; i++ )
  {
    element = book->cb[i];
    matchChannel = 0;
    for( n = 0; n < CHANNELS; n++ )
    {
      if(( element->learnLow[n] <= *(p+n)) &&
         (*(p+n) <= element->learnHigh[n])) matchChannel++;
    }

    if(matchChannel == CHANNELS)
    {
      element->t_last_update = book->t;
      for( n = 0; n < CHANNELS; n++ )
      {
        if( element->max[n] < *(p+n)) element->max[n] = *(p+n);
        else if( element->min[n] > *(p+n)) element->min[n] = *(p+n);
      }
      break;
    }
  }

  int s;
  for( s=0; s < numEntries; s++ )
  {
    element = book->cb[s];
    int negRun = book->t - element->t_last_update;
    if( element->stale < negRun ) element->stale = negRun;
  }

  if( i == numEntries )
  {
    code_element **foo = new code_element* [numEntries+1];
    for( int ii = 0; ii < numEntries; ii++ )
    {
      foo[ii] = book->cb[ii];
    }
    foo[numEntries] = new code_element;
    if( numEntries > 0 ) delete [] cbs[pixelCount].cb;
    cbs[pixelCount].cb = foo;

    element = book->cb[numEntries];
    for( n = 0; n < CHANNELS; n++)
    {
      element->learnHigh[n] = high[n];
      element->learnLow[n] = low[n];
      element->max[n] = *(p+n);
      element->min[n] = *(p+n);
    }
    element->t_last_update = book->t;
    element->stale = 0;
    book->numEntries += 1;
  }

  for( n=0; n < CHANNELS; n++ )
  {
	element = book->cb[i];
    if(element->learnHigh[n] < high[n]) element->learnHigh[n] += 1;
    if(element->learnLow[n] > low[n]) element->learnLow[n] -= 1;
  }

  return(i);
}

void CodeBook::update_codebooks( IplImage* hsv )
{
  int x, y;
  int pixelCount = 0;
  uchar* ptr;
  for( y = 0; y < hsv->height; y++ )
  {
    ptr = (uchar *)(hsv->imageData + y * hsv->widthStep);
    for( x = 0; x < hsv->width; x++ )
    {
      update_codebook( &ptr[3*x+1], pixelCount );
      pixelCount++;
    }
  }
}

void CodeBook::update_codebooks_mask( IplImage* hsv, IplImage* mask )
{
  int x, y;
  int pixelCount = 0;
  uchar *ptr, *mptr;
  for( y = 0; y < hsv->height; y++ )
  {
    ptr = (uchar *)(hsv->imageData + y * hsv->widthStep);
    mptr = (uchar *)(mask->imageData + y * mask->widthStep);
    for( x = 0; x < hsv->width; x++ )
    {
      if( mptr[x] == 255 ) update_codebook( &ptr[3*x+1], pixelCount );
      pixelCount++;
    }
  }
}

void CodeBook::background_diffs( IplImage* hsv, IplImage* dst )  
{
  int x, y;
  uchar *ptr, *dptr;
  int pixelCount = 0;

  for( y = 0; y < hsv->height; y++ )
  {
    ptr = (uchar *)(hsv->imageData + y * hsv->widthStep);
    dptr = (uchar *)(dst->imageData + y * dst->widthStep);
    for( x = 0; x < hsv->width; x++ )
    {
      dptr[x] = background_diff( &ptr[3*x+1], pixelCount );
      pixelCount++;
    }
  }
}

void CodeBook::clear_all_stale_entries()
{
  for( int p = 0; p < numPixels; p++ )
  {
    clear_stale_entries( p );
  }
}

int CodeBook::check_light_level( IplImage* hsv )
{
  int x, y;
  int pixelCount = 0;
  uchar* ptr;
  double sat_avg;
  sat_avg = 0;

  for( y = 0; y < hsv->height; y++ )
  {
    ptr = (uchar *)(hsv->imageData + y * hsv->widthStep);
    for( x = 0; x < hsv->width; x++ )
    {
	  sat_avg += ptr[3*x+1];
      pixelCount++;
    }
  }
  sat_avg /= pixelCount;
  return cvRound( sat_avg );
}

void CodeBook::write_to_disk()
{
  code_book* book;
  CvMemStorage* mem_storage; 

  mem_storage = cvCreateMemStorage( 0 ); 
  CvFileStorage* fs = cvOpenFileStorage( "codebooks.yaml", mem_storage, CV_STORAGE_WRITE );

  int p, i, j;
  for( p = 0; p < numPixels; p++ )
  {
    book = &cbs[p];
	cvStartWriteStruct( fs, 0, CV_NODE_SEQ );
	cvWriteInt( fs, 0, book->numEntries );
	cvWriteInt( fs, 0, book->t );
	for( i = 0; i < book->numEntries; i++ )
	{
	  cvStartWriteStruct( fs, 0, CV_NODE_SEQ );
	  for( j = 0; j < CHANNELS; j++ ) cvWriteInt( fs, 0, book->cb[i]->learnHigh[j] );	
	  for( j = 0; j < CHANNELS; j++ ) cvWriteInt( fs, 0, book->cb[i]->learnLow[j] );	
	  for( j = 0; j < CHANNELS; j++ ) cvWriteInt( fs, 0, book->cb[i]->max[j] );	
	  for( j = 0; j < CHANNELS; j++ ) cvWriteInt( fs, 0, book->cb[i]->min[j] );	
	  cvWriteInt( fs, 0, book->cb[i]->t_last_update );
	  cvWriteInt( fs, 0, book->cb[i]->stale );
	  cvEndWriteStruct( fs );
	}
	cvEndWriteStruct( fs );
  }
  cvReleaseFileStorage( &fs );
  cvClearMemStorage( mem_storage );
  cvReleaseMemStorage( &mem_storage );
 // printf("done\n");
}

void CodeBook::read_from_disk()
{
  code_book* book;
  code_element* cb;
  CvMemStorage* mem_storage; 
  CvSeq *seq, *s2;

  mem_storage = cvCreateMemStorage( 0 ); 
  CvFileStorage* fs = cvOpenFileStorage( "codebooks.yaml", mem_storage, CV_STORAGE_READ );

  int p, i;
  seq = cvGetRootFileNode( fs, 0 )->data.seq;
  for(p = 0; p < seq->total; p++ )
  {
    s2 = ((CvFileNode *)cvGetSeqElem( seq, p ))->data.seq;
    book = &cbs[p];
	book->numEntries = ((CvFileNode*)cvGetSeqElem( s2, 0 ))->data.i;
	book->t = ((CvFileNode*)cvGetSeqElem( s2, 1 ))->data.i;

//	if( book->numEntries != (s2->total - 2) ) printf( "error: numEntries is %d s2 total is %d\n", book->numEntries, s2->total );
    book->cb = new code_element* [book->numEntries];
	for( i = 0; i < book->numEntries; i++ )
	{
	  cb = new code_element;
	  CvSeq* s3 = ((CvFileNode *)cvGetSeqElem( s2, 2 + i ))->data.seq;
	  cb->learnHigh[0] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 0 ) );
	  cb->learnHigh[1] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 1 ) );
	  cb->learnLow[0] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 2 ) );
	  cb->learnLow[1] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 3 ) );
	  cb->max[0] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 4 ) );
	  cb->max[1] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 5 ) );
	  cb->min[0] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 6 ) );
	  cb->min[1] = (uchar)cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 7 ) );
	  cb->t_last_update = cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 8 ) );
	  cb->stale = cvReadInt( (CvFileNode*)cvGetSeqElem( s3, 9 ) );
	  book->cb[i] = cb;
	}
  }

  cvReleaseFileStorage( &fs );
  cvClearMemStorage( mem_storage );
  cvReleaseMemStorage( &mem_storage );
}

CodeBook::~CodeBook()
{
}

