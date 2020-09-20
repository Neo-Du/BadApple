#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

#define WINDOW1 "Input_Video"
#define WINDOW2 "Output_Video"
#define OUTPUT_HEIGHT 640
#define OUTPUT_WIDTH 480

char srcFile[] = "../../Video/BadApple.mp4";

VideoCapture srcVideo = NULL;
Mat SrcFrame,OutFrame;

int main()
{
	namedWindow(WINDOW1, WINDOW_AUTOSIZE);
	namedWindow(WINDOW2, WINDOW_AUTOSIZE);
	srcVideo = VideoCapture(srcFile);
	if (!srcVideo.isOpened())
	{
		cout << "Error, can not open file" << endl;
	}
	cout<< "Source video Width=" << srcVideo.get(CAP_PROP_FRAME_WIDTH)
		<< "  Height=" << srcVideo.get(CAP_PROP_FRAME_HEIGHT)
		<< " FPS : " << srcVideo.get(CAP_PROP_FPS)
		<< endl;
	int delay = 1000 / srcVideo.get(CAP_PROP_FPS);
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);

	for (;;)
	{
		srcVideo >> SrcFrame;
		if (SrcFrame.empty())
		{
			break;
		}
		imshow(WINDOW1, SrcFrame);
		resize(SrcFrame, OutFrame,dsize);
		imshow(WINDOW2, OutFrame);
		waitKey(delay);
	}
	cout << "done" << endl;
	waitKey(0);
	return 0;
}