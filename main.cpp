#include "pyCaller.h"

int main(int argc, const char *argv[])
{
	const char *path = argc == 1 ? "image.jpg" : argv[1];

	pyCaller py;
	py.Init("detect");
	py.ActivateFunc("test_image");
	tfOutput output;
	for (int i = 0; i < 10; ++i)
	{
		output = py.CallFunction("test_image", path);
		output.PrintBoxes(0);
	}
	
	Mat im = imread(path);
	const float c = im.cols, r = im.rows;
	if (output.boxes)
		rectangle(im, cvPoint(c * output.at(0, 1), r * output.at(0, 0)), 
			cvPoint(c * output.at(0, 3), r * output.at(0, 2)), Scalar(255,0,0));
	imshow("object detection", im);
	waitKey(0);

	return 0;
}
