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
  Mat crop;

void boundingbox(int action, int x, int y, int flags, void *userdata)
{

  if( action == EVENT_LBUTTONDOWN )
  {
    top_corner = Point(x,y);
  }
  // Action to be taken when left mouse button is released
  else if( action == EVENT_LBUTTONUP)
  {
    bottom_corner = Point(x,y);
    // Calculate center of rectangle
    Point middle = Point ((bottom_corner.x - top_corner.x)/2 , (bottom_corner.y - top_corner.y)/2) ;
    // Draw the boundingbox
    rectangle(source, top_corner, bottom_corner, Scalar(0,0,0), 0, CV_AA );
    imshow("Window", source);
    cout<<middle<<endl;
    crop = source(Range(top_corner.y,bottom_corner.y),Range(top_corner.x,bottom_corner.x));
    imshow("cropped region",crop);
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
  Mat original = source.clone();
  namedWindow("Window");
  // highgui function called when mouse events occur
  setMouseCallback("Window", boundingbox);
  int k=0;
  // loop until escape character is pressed
  while(k!=27)
  {
    imshow("Window", source );
    putText(source,"click on the upper right corner, and drag\n Press ESC to exit, s to save crop image n for start back with original picture " ,Point(10,30), FONT_HERSHEY_SIMPLEX, 0.7,Scalar(255,255,255), 2 );
    k= waitKey(20) & 0xFF;
    //press to save
    if(k == 's')
    {
        imwrite("./newFile.png",crop);
    }
    //press n to restart cropping
    if(k == 'n')
    {
        original.copyTo(source);
    }  
  }
  return 0;
}
