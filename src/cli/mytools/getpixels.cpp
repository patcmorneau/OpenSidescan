#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include <iostream>
#include <math.h>

using namespace cv;
using namespace std;

  // Points to store the center of the circle and a point on the circumference
  Point top_corner, bottom_corner;
  // Action to be taken when left mouse button is pressed
  // Source image
  Mat source;

void boundingbox(int action, int x, int y, int flags, void *userdata)
{

  if( action == EVENT_LBUTTONDOWN )
  {
    top_corner = Point(x,y);
    // Mark the center
    //circle(source, center, 1, Scalar(255,255,0), 2, CV_AA );
  }
  // Action to be taken when left mouse button is released
  else if( action == EVENT_LBUTTONUP)
  {
    bottom_corner = Point(x,y);
    // Calculate radius of the circle
    Point middle = Point ((bottom_corner.x - top_corner.x)/2 , (bottom_corner.y - top_corner.y)/2) ;
    // Draw the circle
    //circle(source, center, radius, Scalar(0,255,0), 2, CV_AA );
    //imshow("Window", source);
    cout<<middle<<endl;
  }
  
}

int main(int argc,char** argv)
{
    
    if(argc != 2){
    cerr<<"need path as argument";
    return 1;
    }
  string path = argv[1];
  cout<<path<<endl;
  source = imread(path,1);
  // Make a dummy image, will be useful to clear the drawing
  Mat dummy = source.clone();
  namedWindow("Window");
  // highgui function called when mouse events occur
  setMouseCallback("Window", boundingbox);
  int k=0;
  // loop until escape character is pressed
  while(k!=27)
  {
    imshow("Window", source );
    putText(source,"Choose center, and drag, Press ESC to exit and c to clear" ,Point(10,30), FONT_HERSHEY_SIMPLEX, 0.7,Scalar(255,255,255), 2 );
    k= waitKey(20) & 0xFF;
    if(k== 99)
            // Another way of cloning
            dummy.copyTo(source);
  }
  return 0;
}
