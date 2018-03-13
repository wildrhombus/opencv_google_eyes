#include "stdafx.h"
#include "google_two.h"

ContourGroup::ContourGroup(CvSeq *seq, CvSize sz )
{
  contour = seq;

  maskTemp = cvCreateImage( sz, 8, 1 );
  cvZero( maskTemp );

  cvDrawContours( maskTemp, contour, CVX_WHITE, CVX_WHITE, -1, CV_FILLED, 8 );
  CvMoments moments;
  cvMoments( maskTemp, &moments, 1);
  double M00=cvGetSpatialMoment(&moments,0,0);
  double M10=cvGetSpatialMoment(&moments,1,0);
  centerx = cvRound(M10/M00);

  CvRect bbox = cvBoundingRect(contour);
  bbx = bbox.x;
  bbx2 = bbox.x+bbox.width;
  bby = bbox.y;
  bby2 = bbox.y+bbox.height;

  CvMemStorage* area_mem = cvCreateMemStorage( 0 );
  CvPoint2D32f temp_bpts[4]; 
  CvBox2D box2d = cvMinAreaRect2( contour, area_mem );
  cvBoxPoints( box2d, temp_bpts );
  for( int i = 0; i < 4; i++ )
  {
    bpts[i].x = temp_bpts[4].x;
    bpts[i].y = temp_bpts[4].y;
  }
  cvClearMemStorage( area_mem );
  cvReleaseMemStorage( &area_mem );

  ledge = 0;
  redge = 0;
  uedge = 0;
  dedge = 0;

  CvPoint ipts[4], *irect = ipts;
  for( int j= 0; j < 4; j++ )
  {
    ipts[j] = cvPointFrom32f( bpts[j] );
    if( ipts[j].x <= 2 ) ledge = 1;
    if( ipts[j].x >= (sz.width - 2) ) redge = 1;
    if( ipts[j].y <= 2 ) uedge = 1;
    if( ipts[j].y >= (sz.height - 2) ) dedge = 1;
  }
  points_count = 0;
  lowmagcount = 0;
}

ContourGroup::~ContourGroup()
{
  cvReleaseImage( &maskTemp );
}


