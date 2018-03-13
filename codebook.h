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

#define CHANNELS 2

typedef struct ce {
    uchar learnHigh[CHANNELS];
    uchar learnLow[CHANNELS];
    uchar max[CHANNELS];
    uchar min[CHANNELS];
    int t_last_update;
    int stale;
} code_element;

typedef struct codeBook {
	code_element **cb;
	int numEntries;
	int t;
} code_book;


#define MAXPIXELS 500000
class CodeBook
{
private:
	int numPixels;
	unsigned int cbBounds[CHANNELS];
    int minMod[CHANNELS];
    int maxMod[CHANNELS];

	uchar background_diff( uchar* p, int pixelCount );
	int clear_stale_entries( int pixelCount );

public:
	code_book cbs[MAXPIXELS];
	CodeBook( CvSize sz );
	int update_codebook( uchar* p, int pixelCount );
    void update_codebooks( IplImage* hsv );
    void update_codebooks_mask( IplImage* hsv, IplImage* mask );
    void background_diffs( IplImage* hsv, IplImage* dst );
	void clear_all_stale_entries();
	int check_light_level( IplImage* );
	void write_to_disk();
	void read_from_disk();
	virtual ~CodeBook();
};