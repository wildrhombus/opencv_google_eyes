#pragma once

#include "stdio.h"
#include "windows.h"
#include "time.h"
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <string>

using namespace std;

#define MAX_CORNERS 2000
#define MAX_GROUPS 50
#define MAX_CONTOURS 10
#define CVCONTOUR_APPROX_LEVEL 2
#define CVCLOSE_ITR 1

//#define DEBUG 1

class ContourGroup 
{
private:
	IplImage* maskTemp;
public:
  CvSeq* contour;
  int centerx;
  int bbx;
  int bbx2;
  int bby;
  int bby2;
  CvPoint2D32f bpts[4];
  int ledge;
  int redge;
  int uedge;
  int dedge;
  int points_count;
  int lowmagcount;
 
  ContourGroup( CvSeq *seq, CvSize sz );
  virtual ~ContourGroup();
};

class PointGroup 
{
public:
  int count;
  int contour_group;
  double avg_angle;
  float avg_mag;
  float avgx;
  float avgy;
  float avgxm;
  float minx;
  float maxx;
  float direction;
  float trackx;
  
  PointGroup()
  {
    contour_group = NULL;
    avg_angle = -1.0;
    avg_mag = -1.0;
    avgx = -1.0;
    avgxm = -1.0;
    count = 0;
    minx = 641;
    maxx = 0;
  }

  bool operator< (const PointGroup& pgrp) const { return (count <= pgrp.count); }
  void operator++(int) { count++; } // Post-increment  
  virtual ~PointGroup() {}
};

class TrackGroup {
public:
    int lowmagcount;
	string group_id;

	TrackGroup() { lowmagcount = 0; group_id = ""; }
	virtual ~TrackGroup() {}
};

typedef map<string, PointGroup*> GroupMap;
typedef GroupMap::iterator GMIter;
typedef vector<ContourGroup*> ContourVector;
typedef ContourVector::iterator CVIter;
typedef list<TrackGroup*> PointVector;
typedef PointVector::iterator PVIter;

class ProcessCapture
{
private:
	IplImage* maskTemp;
	IplImage* eig_image;
	IplImage* tmp_image;
	IplImage* pyramid;
	IplImage* prev_pyramid;
	IplImage* swap_temp;
	IplImage* dst1;
	CvSize sz;
    CvPoint2D32f pointlist1[MAX_CORNERS];
    CvPoint2D32f pointlist2[MAX_CORNERS];
	float prev_direction;
    float avgx, prev_avgx;
    float avgxm, prev_avgxm;
	float edge, prev_edge;
	long corner_count;
	CvMemStorage* mem_storage; 
	int group_count;
	Track *eyes;

public:
	IplImage* cb_mask;
	IplImage* dst;
	IplImage* image;
    IplImage* hsv;
	IplImage* track;
	IplImage* grey;
	IplImage* prev_grey;
	GroupMap groups;
	CvPoint2D32f *points[2], *swap_points;
    PointVector pts;
	ContourVector contours;
	string dominant_group;
	float prev_trackx;
    int prev_num_contours;
	float direction;
	float trackx;
	int trackwidth;
	long lowmagcount;
	int endblob;
	int edge_completed;

	ProcessCapture( CvSize cap_size, Track *track_eyes );
    void init( IplImage* frame );
    int find_contours( IplImage* frame );
    int get_groups( IplImage* frame );
	void find_eye_position( IplImage* frame );
	void find_first();
	void get_points( IplImage *frame );
	virtual ~ProcessCapture();
};

