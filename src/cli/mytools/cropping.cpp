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
  Mat scrolled_image;
  int x = 0;

void boundingbox(int action, int x, int y, int flags, void *userdata)
{

  if( action == EVENT_LBUTTONDOWN )
  {
    top_corner = Point(x,y);
    cout<<top_corner<<endl;
  }
  // Action to be taken when left mouse button is released
  else if( action == EVENT_LBUTTONUP)
  {
    
        bottom_corner = Point(x,y);
        if (bottom_corner != top_corner){
            // Calculate center of rectangle
            //Point middle = Point ((bottom_corner.x - top_corner.x)/2 , (bottom_corner.y - top_corner.y)/2) ;
            // Draw the boundingbox
            rectangle(scrolled_image, top_corner, bottom_corner, Scalar(0,0,0), 0, CV_AA );
            imshow("Window", source);
            //cout<<middle<<endl;
            crop = scrolled_image(Range(top_corner.y,bottom_corner.y),Range(top_corner.x,bottom_corner.x));
            imshow("cropped region",crop);
        }
  }

  
}

int main(int argc,char** argv)
{
    
    if(argc != 2){
    cerr<<"need path as argument\n";
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
  int image_width = source.cols;
  int image_height = source.rows;
  int height_start = 800;
  if(image_height < height_start){
  height_start = image_height - 1;
  }
  int scroll_height = 0;
  createTrackbar("scroll","Window",&scroll_height,(source.rows - height_start));
  int k=0;
  
  string filename = "";
  // loop until escape character is pressed
  while(k!='q')
  { 
    scrolled_image = source(Rect(0,scroll_height,image_width,height_start));
    imshow("Window", scrolled_image);
    /*
    putText(scrolled_image,"click on the upper right corner, and drag\n Press ESC to exit, s to save crop image n for start back with original picture " ,Point(10,30), FONT_HERSHEY_SIMPLEX, 0.7,Scalar(255,255,255), 2 );
    */
    k= waitKey(1) & 0xFF;
    //press to save
    if(k == 's')
    {   
        filename = "./newImage" + to_string(x) + ".png";
        imwrite(filename,crop);
        x++;
    }
    //press n to restart cropping
    if(k == 'n')
    {
        original.copyTo(source);
    }  
  }
  return 0;
}
