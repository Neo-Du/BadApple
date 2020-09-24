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



#define OUTPUT_HEIGHT 640
#define OUTPUT_WIDTH 480

char video_path[] = "../../Video/BadApple.mp4";
char test_pic_path[] = "tset.jpg";
char test_pic2_path[] = "tset2.jpg";


VideoCapture srcVideo = NULL;
ofstream outFile("out.bin", ios::out | ios::binary);
Mat testPic2 = imread("tset.jpg");
Mat testPic;

void Bin_output(char* path)
{
	ofstream bin_out("out3.bin", ios::binary);
	int b, g, r, total;
	total = 0;
	Mat srcImage, outImage;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);
	uint8_t d1, d2;
	uint16_t n;
	srcImage = imread(path);
	imshow("srcImage", srcImage);
	resize(srcImage, outImage, dsize);
	imshow("outImage", outImage);

	for (int i = 0; i < outImage.rows; i++)
	{
		for (int j = 0; j < outImage.cols; j++)
		{
			b = outImage.at<Vec3b>(i, j)[0] >> 3;
			g = outImage.at<Vec3b>(i, j)[1] >> 2;
			r = outImage.at<Vec3b>(i, j)[2] >> 3;
			n = b << 11 | g << 5 | r;

			d1 = (uint8_t)n;
			d2 = n >> 8;
			bin_out.write((char*)&d1, sizeof(d1));
			bin_out.write((char*)&d2, sizeof(d2));
		}
	}
	bin_out.close();
}
void Bin_output_2pic(char* path, char* path2)
{
	ofstream bin_out("out4_2pic.bin", ios::binary);
	int b, g, r, total;
	total = 0;
	Mat srcImage, srcImage2, outImage, outImage2;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);
	uint8_t d1, d2;
	uint16_t n;
	srcImage = imread(path);
	srcImage2 = imread(path2);

	resize(srcImage, outImage, dsize);
	resize(srcImage2, outImage2, dsize);

	imshow("srcImage", srcImage);
	imshow("srcImage2", srcImage2);
	imshow("outImage", outImage);
	imshow("outImage", outImage2);


	for (int i = 0; i < outImage.rows; i++)
	{
		for (int j = 0; j < outImage.cols; j++)
		{
			b = outImage.at<Vec3b>(i, j)[0] >> 3;
			g = outImage.at<Vec3b>(i, j)[1] >> 2;
			r = outImage.at<Vec3b>(i, j)[2] >> 3;
			n = b << 11 | g << 5 | r;

			d1 = (uint8_t)n;
			d2 = n >> 8;
			bin_out.write((char*)&d2, sizeof(d2));
			bin_out.write((char*)&d1, sizeof(d1));
		}
	}

	for (int i = 0; i < outImage2.rows; i++)
	{
		for (int j = 0; j < outImage2.cols; j++)
		{
			b = outImage2.at<Vec3b>(i, j)[0] >> 3;
			g = outImage2.at<Vec3b>(i, j)[1] >> 2;
			r = outImage2.at<Vec3b>(i, j)[2] >> 3;
			n = b << 11 | g << 5 | r;

			d1 = (uint8_t)n;
			d2 = n >> 8;
			bin_out.write((char*)&d2, sizeof(d2));
			bin_out.write((char*)&d1, sizeof(d1));
		}
	}
	bin_out.close();
}
void pic_to_array(char* path)
{
	int b, g, r, total, n;
	total = 0;
	Mat srcImage, outImage;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);
	ofstream p2a("out.txt");

	srcImage = imread(path);
	imshow("srcImage", srcImage);
	resize(srcImage, outImage, dsize);
	imshow("outImage", outImage);


	p2a << "const uint16_t pic_array[" << OUTPUT_HEIGHT * OUTPUT_WIDTH << "]= {";
	for (int i = 0; i < outImage.rows; i++)
	{
		for (int j = 0; j < outImage.cols; j++)
		{
			b = outImage.at<Vec3b>(i, j)[0] >> 3;
			g = outImage.at<Vec3b>(i, j)[1] >> 2;
			r = outImage.at<Vec3b>(i, j)[2] >> 3;

			if (total % 20 == 0)
			{
				p2a << endl;
			}
			n = b << 11 | g << 5 | r;
			p2a << "0x" << hex << n << ',';
			total++;
		}
	}
	p2a << endl << "};" << endl;
	p2a.close();
}
void play_video(char* path)
{
	Mat SrcFrame, OutFrame;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);

	namedWindow("SourceVideo", WINDOW_AUTOSIZE);
	namedWindow("OutVideo", WINDOW_AUTOSIZE);
	srcVideo = VideoCapture(path);
	if (!srcVideo.isOpened())
	{
		cout << "Error, can not open file" << endl;
	}
	cout << "Source video Width=" << srcVideo.get(CAP_PROP_FRAME_WIDTH)
		<< "  Height=" << srcVideo.get(CAP_PROP_FRAME_HEIGHT)
		<< " FPS : " << srcVideo.get(CAP_PROP_FPS)
		<< endl;

	int delay = 1000 / srcVideo.get(CAP_PROP_FPS);


	for (;;)
	{
		srcVideo >> SrcFrame;
		if (SrcFrame.empty())
		{
			break;
		}
		imshow("SourceVideo", SrcFrame);
		resize(SrcFrame, OutFrame, dsize);
		imshow("OutVideo", OutFrame);
		waitKey(delay);
	}
}
void test_bin_file()
{
	uint8_t d1, d2;
	uint16_t n;
	ofstream test_bin_file("test_bin_file.bin", ios::binary);
	n = 0x001F;
	d1 = (uint8_t)n;
	d2 = n >> 8;
	test_bin_file.write((char*)&d2, sizeof(d2));
	test_bin_file.write((char*)&d1, sizeof(d1));
	n = 0xf800;
	d1 = (uint8_t)n;
	d2 = n >> 8;
	test_bin_file.write((char*)&d2, sizeof(d2));
	test_bin_file.write((char*)&d1, sizeof(d1));
	n = 0x07E0;
	d1 = (uint8_t)n;
	d2 = n >> 8;
	test_bin_file.write((char*)&d2, sizeof(d2));
	test_bin_file.write((char*)&d1, sizeof(d1));
	n = 0xF81F;
	d1 = (uint8_t)n;
	d2 = n >> 8;
	test_bin_file.write((char*)&d2, sizeof(d2));
	test_bin_file.write((char*)&d1, sizeof(d1));
	test_bin_file.close();
}
void Bin_100_frames(char* path)
{
	Mat SrcFrame, OutFrame;
	Size dsize = Size(OUTPUT_HEIGHT, OUTPUT_WIDTH);
	ofstream bin100_out("badapple_15_fps.bin", ios::binary);

	namedWindow("SourceVideo", WINDOW_AUTOSIZE);
	namedWindow("OutVideo", WINDOW_AUTOSIZE);
	srcVideo = VideoCapture(path);
	if (!srcVideo.isOpened())
	{
		cout << "Error, can not open file" << endl;
	}
	cout << "Source video Width=" << srcVideo.get(CAP_PROP_FRAME_WIDTH)
		<< "  Height=" << srcVideo.get(CAP_PROP_FRAME_HEIGHT)
		<< " FPS : " << srcVideo.get(CAP_PROP_FPS)
		<< endl;

	int delay = 1000 / srcVideo.get(CAP_PROP_FPS);
	int frames = 0;
	int b, g, r;
	uint8_t d1, d2;
	uint16_t n;

	for (;;)
	{
		srcVideo >> SrcFrame;
		if (frames % 2 != 0 && frames > 35)
		{			
			if (SrcFrame.empty())
			{
				break;
			}
			imshow("SourceVideo", SrcFrame);
			resize(SrcFrame, OutFrame, dsize);
			imshow("OutVideo", OutFrame);
			waitKey(delay);

			for (int i = 0; i < OutFrame.rows; i++)
			{
				for (int j = 0; j < OutFrame.cols; j++)
				{
					b = OutFrame.at<Vec3b>(i, j)[0] >> 3;
					g = OutFrame.at<Vec3b>(i, j)[1] >> 2;
					r = OutFrame.at<Vec3b>(i, j)[2] >> 3;
					/*n = b << 11 | g << 5 | r;*/
					n = r << 11 | g << 5 | b;
					d1 = (uint8_t)n;
					d2 = n >> 8;
					bin100_out.write((char*)&d1, sizeof(d1));
					bin100_out.write((char*)&d2, sizeof(d2));
				}
				cout << frames << endl;
			}
		}
		frames++;
	}
	bin100_out.close();
}
int main()
{
	//play_video(video_path);
	//pic_to_array(test_pic_path);
	//Bin_output(test_pic2_path);
	//Bin_output_2pic(test_pic_path,test_pic2_path);
	//test_bin_file();

	Bin_100_frames(video_path);

	cout << "done" << endl;
	while (1);
	waitKey(0);
	return 0;
}