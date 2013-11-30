// Calibrate from a list of images
//
// console application.
// Gary Bradski April'08
// 
/* *************** License:**************************
   Oct. 3, 2008
   Right to use this code in any way you want without warrenty, support or any guarentee of it working.

   BOOK: It would be nice if you cited it:
   Learning OpenCV: Computer Vision with the OpenCV Library
     by Gary Bradski and Adrian Kaehler
     Published by O'Reilly Media, October 3, 2008
 
   AVAILABLE AT: 
     http://www.amazon.com/Learning-OpenCV-Computer-Vision-Library/dp/0596516134
     Or: http://oreilly.com/catalog/9780596516130/
     ISBN-10: 0596516134 or: ISBN-13: 978-0596516130    

   OTHER OPENCV SITES:
   * The source code is on sourceforge at:
     http://sourceforge.net/projects/opencvlibrary/
   * The OpenCV wiki page (As of Oct 1, 2008 this is down for changing over servers, but should come back):
     http://opencvlibrary.sourceforge.net/
   * An active user group is at:
     http://tech.groups.yahoo.com/group/OpenCV/
   * The minutes of weekly OpenCV development meetings are at:
     http://pr.willowgarage.com/wiki/OpenCV
   ************************************************** */

#include <cv.h>
#include <highgui.h>
#include <stdio.h>
#include <stdlib.h>

int n_boards = 0; //Will be set by input list
const int board_dt = 10;
int board_w;
int board_h;


void help()
{
	printf("Calibration from disk. Call convention:\n\n"
		"  ch11_ex11_1_fromdisk board_w board_h image_list\n\n"
		"Where: board_{w,h} are the # of internal corners in the checkerboard\n"
		"       width (board_w) and height (board_h)\n"
		"       image_list is space separated list of path/filename of checkerboard\n"
		"       images\n\n"
		"Hit 'p' to pause/unpause, ESC to quit.  After calibration, press any other key to step through the images\n\n");
}

int main(int argc, char* argv[]) 
{
	
	CvCapture* capture;// = cvCreateCameraCapture( 0 );
	// assert( capture );
	
	if(argc != 4){
		help();
		return -1;
	}
	help();
	board_w = atoi(argv[1]);
	board_h = atoi(argv[2]);

	int board_n  = board_w * board_h;
	CvSize board_sz = cvSize( board_w, board_h );

	FILE *fptr = fopen(argv[3],"r");
	char names[2048];

	//COUNT THE NUMBER OF IMAGES:
	while(fscanf(fptr,"%s ",names)==1)
	{
		n_boards++;
	}
	rewind(fptr);
	
	cvNamedWindow( "Calibration" );
	//ALLOCATE STORAGE
	CvMat* image_points      = cvCreateMat(n_boards*board_n,2,CV_32FC1);
	CvMat* object_points     = cvCreateMat(n_boards*board_n,3,CV_32FC1);
	CvMat* point_counts      = cvCreateMat(n_boards,1,CV_32SC1);
	
	///  CvMat * image_points	= cvCreateMat(1, n_boards*board_n, CV_32FC2);
	///  CvMat * object_points = cvCreateMat(1, n_boards*board_n, CV_32FC3);
	///  CvMat * point_counts  = cvCreateMat(1, n_boards, CV_32SC1);
	
	CvMat* intrinsic_matrix  = cvCreateMat(3,3,CV_32FC1);
	CvMat* distortion_coeffs = cvCreateMat(4,1,CV_32FC1);
	
	
	IplImage* image			= 0;	// = cvQueryFrame( capture );
	IplImage* gray_image	= 0;	//for subpixel
	CvPoint2D32f* corners	= new CvPoint2D32f[ board_n ];
	int corner_count;
	int successes = 0;
	int step;
	
	int i, j;

	for( int frame=0; frame<n_boards; frame++ ) 
	{
		fscanf(fptr,"%s ",names);
		
		if(image)
		{
			cvReleaseImage(&image);
			image = 0;
		}
		image = cvLoadImage( names);

		if(gray_image == 0  && image)	//We'll need this for subpixel accurate stuff
		{
			gray_image = cvCreateImage(cvGetSize(image),8,1);
		}
		
		if(!image)
		{
			printf("null image\n");
		}
		
		int found = cvFindChessboardCorners(
			image,
			board_sz,
			corners,
			&corner_count, 
			CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS
			);
		
		//Get Subpixel accuracy on those corners
		cvCvtColor(image, gray_image, CV_BGR2GRAY);
		cvFindCornerSubPix(
			gray_image, 
			corners, 
			corner_count, 
			cvSize(11,11),
			cvSize(-1,-1), 
			cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 30, 0.1 )
			);

		//Draw it		
		cvDrawChessboardCorners(image, board_sz, corners, corner_count, found);
		cvShowImage( "Calibration", image );

		cvWaitKey(0);	// Wait for key to continue

		// If we got a good board, add it to our data
		//
		if( corner_count == board_n ) 
		{
			step = successes*board_n;
			//	printf("Found = %d for %s\n",found,names);
			for( i=step, j=0; j<board_n; ++i,++j ) 
			{
				// CV_MAT_ELEM(*image_points, CvPoint2D32f,0,i) = cvPoint2D32f(corners[j].x,corners[j].y);
				// CV_MAT_ELEM(*object_points,CvPoint3D32f,0,i) = cvPoint3D32f(j/board_w, j%board_w, 0.0f);
				CV_MAT_ELEM(*image_points, float,i,0) = corners[j].x;
				CV_MAT_ELEM(*image_points, float,i,1) = corners[j].y;
				CV_MAT_ELEM(*object_points,float,i,0) = j/board_w;
				CV_MAT_ELEM(*object_points,float,i,1) = j%board_w;
				CV_MAT_ELEM(*object_points,float,i,2) = 0.0f;
				
			}
			// CV_MAT_ELEM(*point_counts, int,0,successes) = board_n;
			CV_MAT_ELEM(*point_counts, int,successes,0) = board_n;		
			successes++;
		}
		
		//    if( successes == n_boards ) break;
		
		int c = cvWaitKey(15);
		if(c == 'p') 
		{
			c = 0;
			while(c != 'p' && c != 27)
			{
				c = cvWaitKey(250);
			}
		}
		if(c == 27)
			return 0;
	}
	printf("successes = %d, n_boards=%d\n",successes,n_boards);
	//ALLOCATE MATRICES ACCORDING TO HOW MANY IMAGES WE FOUND CHESSBOARDS ON
	///  CvMat* image_points2      = cvCreateMat(1,successes*board_n,CV_32FC2);
	///  CvMat* object_points2     = cvCreateMat(1,successes*board_n,CV_32FC3);
	///  CvMat* point_counts2      = cvCreateMat(1,successes,CV_32SC1);
	CvMat* object_points2     = cvCreateMat(successes*board_n,3,CV_32FC1);
	CvMat* image_points2      = cvCreateMat(successes*board_n,2,CV_32FC1);
	CvMat* point_counts2      = cvCreateMat(successes,1,CV_32SC1);

	//TRANSFER THE POINTS INTO THE CORRECT SIZE MATRICES
	
	for(i = 0; i<successes*board_n; ++i)
	{
		///      CV_MAT_ELEM(*image_points2, CvPoint2D32f,0,i)  = CV_MAT_ELEM(*image_points, CvPoint2D32f,0,i);
		///      CV_MAT_ELEM(*object_points2,CvPoint3D32f,0,i)  = CV_MAT_ELEM(*object_points,CvPoint3D32f,0,i);
		CV_MAT_ELEM(*image_points2, float,i,0) = CV_MAT_ELEM(*image_points, float,i,0);
		CV_MAT_ELEM(*image_points2, float,i,1) = CV_MAT_ELEM(*image_points, float,i,1);
		CV_MAT_ELEM(*object_points2,float,i,0) = CV_MAT_ELEM(*object_points,float,i,0) ;
		CV_MAT_ELEM(*object_points2,float,i,1) = CV_MAT_ELEM(*object_points,float,i,1) ;
		CV_MAT_ELEM(*object_points2,float,i,2) = CV_MAT_ELEM(*object_points,float,i,2) ;
		
	} 

	for(i=0; i<successes; ++i)
	{
		///		CV_MAT_ELEM(*point_counts2,int,0, i) = CV_MAT_ELEM(*point_counts, int,0,i);
		CV_MAT_ELEM(*point_counts2,int,i, 0) = CV_MAT_ELEM(*point_counts, int,i,0);
	}
	cvReleaseMat(&object_points);
	cvReleaseMat(&image_points);
	cvReleaseMat(&point_counts);
	
	// cvWaitKey();	//Now we have to reallocate the matrices
	// return 0;
	// At this point we have all of the chessboard corners we need.
	//
	
    // Initialize the intrinsic matrix such that the two focal
    // lengths have a ratio of 1.0
    //
    CV_MAT_ELEM( *intrinsic_matrix, float, 0, 0 ) = 1.0f;
    CV_MAT_ELEM( *intrinsic_matrix, float, 1, 1 ) = 1.0f;
	printf("cvCalibrateCamera2\n");

    cvCalibrateCamera2(
		object_points2,
		image_points2,
		point_counts2,
		cvGetSize( image ),
		intrinsic_matrix,
		distortion_coeffs,
		NULL,
		NULL,
		0		//CV_CALIB_FIX_ASPECT_RATIO
		);

	// Save our work
	cvSave("Intrinsics.xml",intrinsic_matrix);
	cvSave("Distortion.xml",distortion_coeffs);

	// Load test
	CvMat *intrinsic = (CvMat*)cvLoad("Intrinsics.xml");
	CvMat *distortion = (CvMat*)cvLoad("Distortion.xml");
	
	// Build the undistort map which we will use for all 
	// subsequent frames.
	//
	IplImage* mapx = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	IplImage* mapy = cvCreateImage( cvGetSize(image), IPL_DEPTH_32F, 1 );
	printf("cvInitUndistortMap\n");
	cvInitUndistortMap(
		intrinsic,
		distortion,
		mapx,
		mapy
		);

	//////////////////////////////////////////////////////////////////////////

	int point_count = board_n; //CV_MAT_ELEM( *point_counts2, int, 1, 0);
    printf("point_count=%d\n",point_count);
    CvMat* points1;
    CvMat* points2;
    CvMat* status;
    CvMat* fundamental_matrix;
	
    points1 = cvCreateMat(1,point_count,CV_32FC2);
    points2 = cvCreateMat(1,point_count,CV_32FC2);
    status = cvCreateMat(1,point_count,CV_8UC1);
	//CV_MAT_ELEM( *mat, float, 3, 2 );
		
	// create pointers to 2D points in points1, points2, initialize it etc...
	// Here points* & pt* actually points to the same data area
	//
	CvPoint2D32f* pt1 = (CvPoint2D32f*)points1->data.fl;
	CvPoint2D32f* pt2 = (CvPoint2D32f*)points2->data.fl;
	
	// OK imgpt1, imgpt2指向两副图像的角点数据, 实际上应该是来自两个摄像机同时采集的两幅图像
	// 计算这两个摄像机之间的基础矩阵
	//
	CvPoint2D32f* imgpt1 = (CvPoint2D32f*)image_points2->data.fl;
	CvPoint2D32f* imgpt2 = (CvPoint2D32f*)(image_points2->data.fl + point_count);	
	
	for( i =  0; i < point_count; i++ )
	{
		pt1[i] = imgpt1[i];
		pt2[i] = imgpt2[i];
	}
	
    fundamental_matrix = cvCreateMat(3,3,CV_32FC1);

	int fm_count = cvFindFundamentalMat( 
		points1,
		points2,
		fundamental_matrix,
		CV_FM_RANSAC,
		1.0,
		0.99,
		status 
		);
	
	printf("DONE computing fundamental matrix:\n\n");

	for(int r = 0; r<3; r++) 
	{
		for(int c = 0; c < 3; c++)
		{
			printf("%+10f  ",CV_MAT_ELEM( *fundamental_matrix, float,  r, c));
		}
		printf("\n");
	}   
	
	printf("\n");
	//////////////////////////////////////////////////////////////////////////

	// Just run the camera to the screen, now only showing the undistorted
	// image.
	//
	rewind(fptr);
	cvNamedWindow( "Undistort" );
	printf("\n\nPress any key to step through the images, ESC to quit\n\n");
	while(fscanf(fptr,"%s ",names)==1)
	{
		if(image){
			cvReleaseImage(&image);
			image = 0;
		}  
		image = cvLoadImage( names);
		IplImage *t = cvCloneImage(image);
		cvShowImage( "Calibration", image );
		cvRemap( t, image, mapx, mapy );
		cvReleaseImage(&t);
		//	  cvUndistort2(
		cvShowImage("Undistort", image);
		if((cvWaitKey()&0x7F) == 27) break;  
	}
	
	return 0;
} 
