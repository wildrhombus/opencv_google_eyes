========================================================================
    CONSOLE APPLICATION : google_two Project Overview
========================================================================
This is code to run an interactive sculpture that uses the OpenCV optical flow
algorithm to follow people and drive two servo motors.  You can see it in action
here:  https://www.youtube.com/watch?v=NrsMiT3muE8

It was built using Visual Studio on windows (original VS2008, but more recently VS2015)

To build with the OpenCV libraries, please reference these instructions:
http://www.learnopencv.com/install-opencv3-on-windows/

The motors used were Bioloid server motors that use the FTD2xx drivers.
To install the ftdi drivers and get the libaries please reference this site:
http://www.ftdichip.com/Support/Documents/TechnicalNotes/TN_153%20Instructions%20on%20Including%20the%20D2XX%20Driver%20in%20a%20VS%20Express%202013%20Project.pdf

I used an external wide angle webcam for the sculpture.  Any external camera that works with your computer will do,
but it's better to use a lower resolution camera to keep the processing requirements manageable.

Relevant files:
google_two.cpp
    This is the main application source file.

eyes.cpp
    The thread for motor control

googlecalibration.txt
  L and R settings to set a zero point for the motors


/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named google_two.pch and a precompiled types file named StdAfx.obj.


/////////////////////////////////////////////////////////////////////////////
