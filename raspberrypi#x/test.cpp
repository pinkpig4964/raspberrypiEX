#include <opencv2/opencv.hpp>

using namespace cv;

int main( int argc, char** argv ){
  Mat image;
  String file_name = "./pic.jpg";
  image = imread( file_name, CV_WINDOW_AUTOSIZE );

  namedWindow( "IMG", WINDOW_AUTOSIZE );
  imshow( "IMG", image );
  waitKey(0);
  return 0;
}


