// water_line.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"  
#include "opencv2/core/core.hpp"

using namespace std;
using namespace cv;
int main()
{
	Mat image = imread("4.png");  //����Լ�ͼ���·�� 
	imshow("��ʾͼ��", image);
	waitKey(0);
	
    return 0;
}

