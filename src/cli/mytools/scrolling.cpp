#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <vector>
#include <iostream>
#include <math.h>
using namespace cv;
using namespace std;




int main(int argc,char** argv)
{
    /*
    if(argc != 2){
    cerr<<"need path as argument";
    return 1;
    }
  string path = argv[1];
  cout<<path<<endl;
  Mat source = imread(path,1);
    */
  Mat source = imread("/home/pat/projet_sidescan/image/image.jpg",1);
  namedWindow("Window");
  cout<<source.rows<<endl<<source.cols<<endl;
  int image_width = source.cols;
  int image_height = source.rows;
  int height_start = 800;
  int scroll_height = 0;
  createTrackbar("scroll","Window",&scroll_height,(source.rows - height_start));
  //resizeWindow("Window",image_width,height_start);
  int k=0;
  // loop until escape character is pressed
  while(k!=27)
  { 
    //Mat scrolled_image = source(rectangle())
    Mat scrolled_image = source(Rect(0,scroll_height,image_width,height_start));
    imshow("Window", scrolled_image);

    k= waitKey(20) & 0xFF;

  }
  return 0;
}
