#include "stdafx.h"
#include "google_two.h"

#define SHOWIMAGE 1
//#define DEBUG 1

ProcessCapture::ProcessCapture( CvSize cap_size, Track *track_eyes )
{
  sz = cap_size;
  trackwidth = sz.width;
  eyes = track_eyes;

  hsv = cvCreateImage( sz, 8, 3 );
  image= cvCreateImage( sz, 8, 3 );
  cb_mask = cvCreateImage( sz, 8, 1 );
  dst = cvCreateImage( sz, 8, 1 );
  dst1 = cvCreateImage( sz, 8, 1 );
  maskTemp = cvCreateImage( sz, 8, 1 );
  grey = cvCreateImage( sz, 8, 1 );
  prev_grey = cvCreateImage( sz, 8, 1 );
  eig_image = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
  tmp_image = cvCreateImage( sz, IPL_DEPTH_32F, 1 );
  cvZero( dst );
#ifdef DEBUG
  //printf("%d %d\n", sz.width, sz.height );
  //printf("clock ticks per sec %d\n", CLOCKS_PER_SEC );
#endif

  points[0] = pointlist1;
  points[1] = pointlist2; 
  corner_count = 0;

  CvSize pyr_sz = cvSize( sz.width+8, sz.height/3 );
  pyramid = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );
  prev_pyramid = cvCreateImage( pyr_sz, IPL_DEPTH_32F, 1 );

  trackx = 320;
  edge_completed = 1;
  mem_storage = NULL;
  lowmagcount = 0;
  endblob = 0;
}

void ProcessCapture::init( IplImage* frame )
{
  CV_SWAP( prev_grey, grey, swap_temp );
  CV_SWAP( points[0], points[1], swap_points );
  cvZero( prev_pyramid );
  cvZero( pyramid );
  cvZero( cb_mask );

  cvCvtColor( frame, hsv, CV_BGR2HSV );
  cvConvertImage( frame, grey, 0 );
 // cvSet( frame, cvScalarAll( 100 ) );
 
  prev_direction = direction;
  prev_avgx = avgx;
  prev_avgxm = avgxm;
  prev_edge = edge;
  prev_trackx = trackx;
  prev_num_contours = contours.size();
  group_count = 0;
	  
  while (!contours.empty()) {
    delete contours.back();  
    contours.pop_back();
  }
  contours.clear();
  if( mem_storage != NULL )
  {
    cvClearMemStorage( mem_storage );
	cvReleaseMemStorage( &mem_storage );
  }

  for(GMIter giter = groups.begin(); giter != groups.end(); ++giter)
  {
    PointGroup* group = (*giter).second;
	delete group;
  }
  groups.clear();
}

int ProcessCapture::find_contours( IplImage* frame )
{
  CvContourScanner scanner;
  CvSeq *found_contours;
  CvSeq *cc;
  CvSeq *c_new;
  cvMorphologyEx( dst, dst1, 0, 0, CV_MOP_OPEN, CVCLOSE_ITR );
  cvMorphologyEx( dst1, dst, 0, 0, CV_MOP_CLOSE, CVCLOSE_ITR );

  mem_storage = cvCreateMemStorage( 700000 ); 
      
  double len, q;
  scanner = cvStartFindContours( dst, mem_storage, sizeof( CvContour ), CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );
  while( (cc=cvFindNextContour( scanner )) != NULL )
  {
    len = cvContourPerimeter( cc );
    q = (dst->height + dst->width)/4;
    if( len <  q ) cvSubstituteContour( scanner, NULL );
    else
    {
      c_new = cvConvexHull2 ( cc, mem_storage, CV_CLOCKWISE, 1 );
      cvSubstituteContour( scanner, c_new );
    }
  }
  found_contours = cvEndFindContours( &scanner );
		
  int i;
  for(i = 0, cc = found_contours; cc != NULL; cc = cc->h_next,i++ )
  {
    ContourGroup* group = new ContourGroup( cc, sz );
    contours.push_back( group );
  }
   	 
  return contours.size();
}

int ProcessCapture::get_groups( IplImage* frame )
{
  int i, k;
  unsigned int ci;
  float x0, x1, y0, y1;
  double angle;
  float magnitude, xlength;
  int xdist;
  int angle_group, mag_group, pos_group;
  std::ostringstream group_id;
  string gid;
  PointGroup* group;
  int count;
  GroupMap::const_iterator giter;

  if( contours.size() <= 0 ) return 0;
  if( corner_count == 0 ) return 0;

  char status[MAX_CORNERS];
  float errors[MAX_CORNERS];
  cvCalcOpticalFlowPyrLK( prev_grey, grey, prev_pyramid, pyramid,
                          points[0], points[1], corner_count, cvSize(10,10),
                          3, status, errors, cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03),
                          0 );

  PVIter piter = pts.begin();
  lowmagcount = 0;
  for( i = k = 0; i < corner_count; i++ )
  {
#ifdef SHOWIMAGE
    cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 255, 255, 255, 0 ), -1, 8 );
#endif
	//   if( piter == pts.end() ) break;d
	if( !status[i] || (*piter)->lowmagcount >= 5 )
	{
      piter = pts.erase(piter);
	  cvCircle( cb_mask, cvPointFrom32f( points[1][i] ), 10, cvScalarAll( 255 ), -1, 8 );
	  continue;
    }
	if( contours.size() == 0 )
	{
	  cvCircle( cb_mask, cvPointFrom32f( points[1][i] ), 10, cvScalarAll( 255 ), -1, 8 );
	  piter = pts.erase(piter);
	  continue;
	}
 
    CvSeq* cc;
	pos_group = -1;
    for(ci = 0; ci < contours.size(); ci++)
	{
      cc = contours[ci]->contour;

#ifdef SHOWIMAGE
      cvDrawContours( frame, cc, CVX_WHITE, CVX_WHITE, 1, 1, 8 );
#endif
      double dist = cvPointPolygonTest( cc, points[1][i], 1 );
	  if( dist >= 0.0 )
	  {
        contours[ci]->points_count++;
		if( direction == 1 ) xdist = cvRound(contours[ci]->bbx2 - x1);
        else xdist = cvRound(x1 - contours[ci]->bbx);

        if( xdist < 200 ) pos_group = 0;
	    else if( xdist < 400 ) pos_group = 1;
	    else pos_group = 2;
        break;
      }
	}	
	if( pos_group == -1 )
	{
	  piter = pts.erase(piter);
	  cvCircle( cb_mask, cvPointFrom32f( points[1][i] ), 10, cvScalarAll( 255 ), -1, 8 );
	  continue;
    }

    x0 = points[0][i].x;
    x1 = points[1][i].x;
    y0 = points[0][i].y;
    y1 = points[1][i].y;

    magnitude = sqrt(pow( (x1 - x0), 2 ) + pow( (y1 - y0), 2 ) );
    angle = atan2((y1 - y0),(x1 - x0)) * 180 / PI;
    xlength = fabs( (float)(x1 - x0) );
	if( magnitude < 10.0 )
	{
	  lowmagcount++;
	  (*piter)->lowmagcount++;
	  contours[ci]->lowmagcount++;
	}

    angle_group = -1;
    if( angle >= -45.0 && angle < 45.0 ) angle_group = 0;
    else if( angle >= 45.0 && angle < 135.0 ) angle_group = 1;
    else if( angle >= 135.0 || angle < -135.0 ) angle_group = 2;
    else if( angle >= -135.0 && angle < -45.0 ) angle_group = 3;

    mag_group = -1;
    if( magnitude >= 0 && magnitude < 5 ) mag_group = 0;
    else if( magnitude >= 5 && magnitude < 100 ) mag_group = 1;
    else if( magnitude >= 100 && magnitude < 200 ) mag_group = 2;
    else if( magnitude >= 200 && magnitude < 500 ) mag_group = 3;
    else mag_group = 4;
		
	group_id.str("");
	group_id << ci << angle_group << mag_group << pos_group;

	gid = group_id.str();
	if( !groups[gid] )
	{
	  group = new PointGroup();
	  groups[group_id.str()] = group;
	}
    groups[group_id.str()]->count++; 
    
	group = groups[gid];
	count = group->count;
	group->contour_group = ci;
    group->avg_angle = (group->avg_angle*(count-1) + fabs( angle ))/count;
    group->avg_mag = (group->avg_mag*(count-1) + magnitude)/count;
    group->avgx = (group->avgx*(count-1) + x1)/count;
    group->avgxm = (group->avgxm*(count-1) + xlength)/count;
    group->avgy = (group->avgy*(count-1) + y1)/count;
    if( x1 > group->maxx ) group->maxx = x1;
    if( x1 < group->minx ) group->minx = x1;
    group_count++;
	(*piter)->group_id = gid;
    piter++;

    points[1][k] = points[1][i];
    points[0][k] = points[0][i];
	k++;
  }
  corner_count = k;
//  printf("lowmagcount %ld cornercount %ld\n", lowmagcount, corner_count);
  if( lowmagcount == corner_count )
  {
	for(ci = 0; ci < contours.size(); ci++)
	{
      if(contours[ci]->lowmagcount == contours[ci]->points_count )
		cvDrawContours( cb_mask, contours[ci]->contour, CVX_WHITE, CVX_WHITE, -1, CV_FILLED, 8 );
	}
    while (!contours.empty()) {
      delete contours.back();  
      contours.pop_back();
    }
    contours.clear();

    for(GMIter giter = groups.begin(); giter != groups.end(); ++giter)
    {
      PointGroup* group = (*giter).second;
      delete group;
    }
    groups.clear();
  }
  return groups.size();
}
	  
void ProcessCapture::find_eye_position( IplImage* frame )
{
#ifdef DEBUG
  printf(" fep " );
#endif
  if( prev_num_contours == 0 && contours.size() != 0 ) return find_first();
  if( groups.size() == 0 )
	  return;
  
  string groupfm = "";
  string groupfm2 = "";
  string groupmd = "";
  
  int maxcount = 0;
  int maxtmcount = 0;
  int maxtm2count = 0;
  int maxmdcount = 0;
  float trackMatch = 0.0;
  float trackMatch2 = 0.0;
  double mindiff = trackwidth*1.0;

  PointGroup *group;
  string highcount = "";
  string groupid;

  int gi = 0;
  double perc_count = 0.0;
  for( GMIter g = groups.begin(); g != groups.end(); ++g)
  {
    groupid = (*g).first;
    group = (*g).second;
	perc_count = (group->count*1.0/group_count*1.0)*100.0;
	if( perc_count < 5 ) continue;
	
	if( group->avg_angle <= 90.0 ) group->direction = 1.0;
    else group->direction = -1.0;
	group->avgxm *= group->direction;

    group->trackx = (prev_avgxm + prev_avgx + group->avgx)/2;
         
    if( maxcount < group->count )
	{
	  maxcount = group->count;
	  highcount = groupid;
    }
	trackMatch = (group->trackx - prev_trackx) * prev_direction;
    trackMatch2 = (group->avgx - prev_trackx) * prev_direction;
		
	if( (trackMatch >= -0.0) && (group->count > maxtmcount ) )
	{
	  maxtmcount = group->count;
	  groupfm = groupid;
	}

	if( (trackMatch2 >= -0.0) && (group->count > maxtmcount ) )
	{
	  maxtm2count = group->count;
	  groupfm2 = groupid;
	}

    if( (mindiff > fabs( prev_trackx - group->trackx )) && (group->count > maxmdcount) )
    {
      mindiff = fabs( prev_trackx - group->trackx);
	  maxmdcount = group->count;
      groupmd = groupid;
    }
	gi++;
  }
  if( gi != 0 )
  {
    dominant_group = "";
	if( maxtmcount > maxtm2count )
	{
      dominant_group = groupfm;
	  trackx = groups[groupfm]->trackx;
	}
	else if ( maxtm2count != 0 )
	{
      dominant_group = groupfm2;
      trackx = groups[groupfm2]->avgx;
	}
	else
	{
	  if( prev_edge )
      {
  	    dominant_group = groups.begin()->first;
		trackx = groups[dominant_group]->avgx;
      }
      else if( groups.begin()->second->direction != prev_direction )
      {
	    if( groupmd != "" )
		{
		  dominant_group = groupmd;
          trackx = groups[groupmd]->trackx;
		}
	    else
		{
		  dominant_group = highcount;
          trackx = groups[highcount]->trackx;
		}
      }
      else 
      {
	    dominant_group = groups.begin()->first;
		trackx = prev_trackx;
      }
	}
  }
  else 
  {
    dominant_group = groups.begin()->first;
	trackx = prev_trackx;
  }
	 
  direction = groups[dominant_group]->direction;
  avgx = groups[dominant_group]->avgx;
  avgxm = groups[dominant_group]->avgxm;

  int dc = groups[dominant_group]->contour_group;
  ContourGroup* contour = contours[dc];
  if( contour->ledge && direction == -1  && prev_direction == -1  && trackx < 60)
  {
    trackx = 0;
  }
  else if( contour->redge && direction == 1 && prev_direction == 1 && trackx > (trackwidth - 60) )
  {
    trackx = 640;
  }  
  if ( direction > 0)
    eyes->setMove( abs( trackx - prev_trackx ), RIGHT, trackx ); 
  else
    eyes->setMove( abs( trackx - prev_trackx ), LEFT, trackx );

  int i = 0;
  int found = 0;
  for( PVIter piter = pts.begin(); piter != pts.end(); ++piter )
  {
    if( (*piter)->group_id == "" )
	{
      cvCircle( cb_mask, cvPointFrom32f( points[1][i] ), 10, cvScalarAll( 255 ), -1, 8 );
#ifdef SHOWIMAGE
	  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 127, 127, 127, 0 ), -1, 8 );
      cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 127, 127, 127, 0 ), 1, 8 );
#endif
	}
#ifdef SHOWIMAGE
	else
	{
      if( (*piter)->group_id == dominant_group )
      {
	    cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 0, 0, 0, 0 ), -1, 8 );
		cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 0, 0, 0, 0 ), 1, 8 );
      }
	  else
	  {
	    if( (*piter)->group_id == groupfm )
		{
		  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 0, 0, 255, 0 ), -1, 8 );
		  cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 0, 0, 255, 0 ), 1, 8 );
		}
		else if( (*piter)->group_id == groupfm2 )
		{
		  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 0, 255, 0, 0 ), -1, 8 );
		  cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 0, 255, 0, 0 ), 1, 8 );
		}
		else if( (*piter)->group_id == groupmd )
		{
		  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 255, 0, 0, 0 ), -1, 8 );
		  cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 255, 0, 0, 0 ), 1, 8 );
		}
		else if( (*piter)->group_id == highcount )
		{
		  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 0, 255, 255, 0 ), -1, 8 );
		  cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 0, 255, 255, 0 ), 1, 8 );
		}
		else
		{
		  cvCircle( frame, cvPointFrom32f( points[1][i] ), 3, cvScalar( 255, 0, 255, 0 ), -1, 8 );
		  cvLine( frame, cvPointFrom32f( points[0][i] ), cvPointFrom32f( points[1][i] ), cvScalar( 255, 0, 255, 0 ), 1, 8 );
	    }
      }
	}
#endif
	i++;
  }
}

void ProcessCapture::find_first()
{
  if( contours.size() == 0 ) return;
  double centerx;
  centerx = 0.0;
  int bbx, bbx2, bby, bby2;
  bbx = bbx2 = bby = bby2 = 0;
  unsigned int i;      
  for(i = 0; i < contours.size(); i++ )
  {
    centerx += contours[i]->centerx;
    bbx += contours[i]->bbx;
    bbx2 += contours[i]->bbx2;
    bby += contours[i]->bby;
    bby2 += contours[i]->bby2;
  }
  centerx /= i;
  bbx /= i;
  bbx2 /= i;
  bby /= i;
  bby2 /= i;
#ifdef DEBUG
  printf( " Ff " );
#endif
  if( bbx < 70 && edge_completed ) 
  {          
#ifdef DEBUG
    printf( " L " );
#endif
    eyes->setAction( FARLEFT );

    avgx = 0;
    trackx = 0;
    direction = 1;
	edge_completed = FALSE;
  }
  else if( bbx2 > (trackwidth - 70) && edge_completed )
  {
#ifdef DEBUG
    printf( " R " );
#endif
	eyes->setAction( FARRIGHT );
    avgx = trackwidth*1.0;
    trackx = trackwidth*1.0;
    direction = -1.0;
	edge_completed = FALSE;
  }
  else if( ( bby < 70 || bby2 > 460 ) && edge_completed )
  {
#ifdef DEBUG
    printf( " Y " );
#endif
  	if ( direction == 1 ) eyes->setMove( 0, RIGHT, centerx ); 
    else eyes->setMove( 0, LEFT, centerx );
    avgx = centerx*1.0;
    trackx = centerx*1.0;
    direction = 1;
	edge_completed = FALSE;
  } 
  else if( !edge_completed || endblob ) 
  {
#ifdef DEBUG
    printf( " M " );
#endif
  	if ( direction == 1 ) eyes->setMove( 0, RIGHT, centerx ); 
    else eyes->setMove( 0, LEFT, centerx );
    avgx = centerx*1.0;
    trackx = centerx*1.0;
    direction = prev_direction;
    endblob = 0;
  }
  else
  {
#ifdef DEBUG
//	printf(" ignore contour\n");
#endif
	while (!contours.empty()) {
      delete contours.back();  
      contours.pop_back();
    }
    contours.clear();

 	for(GMIter giter = groups.begin(); giter != groups.end(); ++giter)
	{
      PointGroup* group = (*giter).second;
      delete group;
	}
    groups.clear();
  }
}

void ProcessCapture::get_points( IplImage *frame )
{
  if( contours.size() == 0 ) return;

  int corner_count2 = MAX_CORNERS;
  cvGoodFeaturesToTrack( grey, eig_image, tmp_image, points[0], &corner_count2,
                         0.05, 5.0, NULL, 3, 0, 0.04 );
  cvFindCornerSubPix( grey, points[0], corner_count2, cvSize( 10, 10 ), cvSize( -1, -1 ),
                      cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.03) );
  int corner_count3 = corner_count;
  int i, k;
  float x0, x1, y0, y1;
  double dx, dy;
  for( k = 0; k < corner_count2; k++ )
  {
    for( i = 0; i < corner_count; i++ )
    {
      x0 = points[0][k].x;
      x1 = points[1][i].x;
      y0 = points[0][k].y;
      y1 = points[1][i].y;

      dx = x0 - x1;
      dy = y0 - y1;

      if( dx*dx + dy*dy <= 9 )
        break;
    }
    if( i != corner_count ) continue;

	TrackGroup* pnt;
    if( corner_count3 < MAX_CORNERS)
    {
      pnt = new TrackGroup();
      pts.push_back( pnt );

      points[1][corner_count3] = points[0][k];
      corner_count3++;
    }
  }
  corner_count = corner_count3;
}

ProcessCapture::~ProcessCapture()
{
  cvReleaseImage( &maskTemp );
  cvReleaseImage( &eig_image );
  cvReleaseImage( &tmp_image );
  cvReleaseImage( &grey );
  cvReleaseImage( &prev_grey );
  cvReleaseImage( &pyramid );
  cvReleaseImage( &prev_pyramid );
  cvReleaseImage( &dst1 );
  cvReleaseImage( &dst );
  cvReleaseImage( &image );
  cvReleaseImage( &hsv );
  contours.clear();
  if( mem_storage != NULL )
  {
	cvClearMemStorage( mem_storage );
	cvReleaseMemStorage( &mem_storage );
  }

  for(GMIter giter = groups.begin(); giter != groups.end(); ++giter)
  {
    PointGroup* group = (*giter).second;
	delete group;
  }
  groups.clear();

  while( !pts.empty() )
  {
    delete pts.back();
    pts.pop_back();
  }
  pts.clear();
}
