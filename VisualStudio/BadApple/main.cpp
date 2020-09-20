#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>


using namespace cv;
using namespace std;


#define WINDOW1 "Input_Video"
#define WINDOW2 "Output_Video"
#define OUTPUT_HEIGHT 320
#define OUTPUT_WIDTH 240

char srcFile[] = "../../Video/BadApple.mp4";

VideoCapture srcVideo = NULL;
Mat SrcFrame,OutFrame;
ofstream outFile("out.bin",ios::out|ios::binary);
Mat testPic2 = imread("tset.jpg");
Mat testPic;
void Bin_output()
{
	ofstream file2("out2.bin", ios::binary);
	uint16_t a = 0xffff;
	uint16_t b = 0xffcc;
	uint16_t c = 0xabc0;
	uint8_t d1 = (uint8_t)c;
	uint8_t d2 = c>>8;

	file2.write((char*)&a, sizeof(a));
	file2.write((char*)&b, sizeof(b));
	file2.write((char*)&c, sizeof(c));

	file2.write((char*)&d2, sizeof(d2));
	file2.write((char*)&d1, sizeof(d1));

	file2.close();
}

void pic2RGB565()
{
	int b, g, r, total, n;
	total = 0;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);
	resize(testPic2, testPic, dsize);
	imshow(WINDOW2, testPic);
	//outFile << "const uint16_t pic_array [640*480]= {";
	for (int i = 0; i < testPic.rows; i++)
	{
		for (int j = 0; j < testPic.cols; j++)
		{
			b = testPic.at<Vec3b>(i, j)[0] >> 3;
			g = testPic.at<Vec3b>(i, j)[1] >> 2;
			r = testPic.at<Vec3b>(i, j)[2] >> 3;

			if (total % 20 == 0)
			{
				outFile << endl;
			}
			n = b << 11 | g << 5 | r;
			/*outFile << ios::binary << n ;*/
			outFile.write((char *)&n,1);
			
			total++;
		}
	}
	//outFile << endl << "};" << endl;
	outFile.close();
}

int main()
{
	//namedWindow(WINDOW1, WINDOW_AUTOSIZE);
	//namedWindow(WINDOW2, WINDOW_AUTOSIZE);
	//srcVideo = VideoCapture(srcFile);
	//if (!srcVideo.isOpened())
	//{
	//	cout << "Error, can not open file" << endl;
	//}
	//cout<< "Source video Width=" << srcVideo.get(CAP_PROP_FRAME_WIDTH)
	//	<< "  Height=" << srcVideo.get(CAP_PROP_FRAME_HEIGHT)
	//	<< " FPS : " << srcVideo.get(CAP_PROP_FPS)
	//	<< endl;
	//int delay = 1000 / srcVideo.get(CAP_PROP_FPS);
	//Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);

	//for (;;)
	//{
	//	srcVideo >> SrcFrame;
	//	if (SrcFrame.empty())
	//	{
	//		break;
	//	}
	//	imshow(WINDOW1, SrcFrame);
	//	resize(SrcFrame, OutFrame,dsize);
	//	imshow(WINDOW2, OutFrame);
	//	waitKey(delay);
	//}
	
	//pic2RGB565();

	Bin_output();
	cout << "done" << endl;
	while (1);
	waitKey(0);
	return 0;
}