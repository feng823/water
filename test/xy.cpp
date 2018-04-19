﻿#include "stdafx.h"
#include "xy.h"

Mat Edge_Detect(Mat im, int aperture_size)
{
	Mat test_im;
	//
	Mat data, result;
	// 灰度图像
	if (im.channels() > 1)
		cvtColor(im, data, CV_BGR2GRAY);
	else
		data = im.clone();
	// BORDER_REPLICATE 表示当卷积点在图像的边界时，原始图像边缘的像素会被复制，并用复制的像素扩展原始图的尺寸  
	// 计算x方向的sobel方向导数，计算结果存在dx中  
	// 计算y方向的sobel方向导数，计算结果存在dy中  
	Mat dx, dy;
	Sobel(im, dx, CV_32F, 1, 0, aperture_size, 1, 0, cv::BORDER_REPLICATE);
	Sobel(im, dy, CV_32F, 0, 1, aperture_size, 1, 0, cv::BORDER_REPLICATE);
	// dy方向局部极大极小值 找出极小值，并进行处理
	vector<Point3f> points1 = localmax_point_score(dx, dx.cols / 3, dx.cols / 3 * 2, 3, 0);
	vector<Point3f> points2 = localmax_point_score(-dx, dx.cols / 3, dx.cols / 3 * 2, 3, 0);
	// 筛选points
	vector<vector<vector<Point3f>>> E_area1 = select_pointline(dx, dy, points1, true);
	vector<vector<vector<Point3f>>> E_area2 = select_pointline(dx, dy, points2, false);

	Mat temp[3];
	temp[0] = data.clone(); temp[1] = data.clone(); temp[2] = data.clone();
	merge(temp, 3, test_im);
	Scalar rgb[3];
	rgb[0] = Scalar(0, 255, 0);
	rgb[1] = Scalar(0, 0, 255);
	rgb[2] = Scalar(255, 0, 0);
	for (auto i : E_area1) {
		for (int j = 0; j<3; ++j)
			for (auto k : i[j]) {
				test_im.at<Vec3b>(k.y, k.x).val[0] = rgb[j].val[0];
				test_im.at<Vec3b>(k.y, k.x).val[1] = rgb[j].val[1];
				test_im.at<Vec3b>(k.y, k.x).val[2] = rgb[j].val[2];
			}
	}
	for (auto i : E_area2) {
		for (int j = 0; j<3; ++j)
			for (auto k : i[j]) {
				test_im.at<Vec3b>(k.y, k.x).val[0] = rgb[j].val[0];
				test_im.at<Vec3b>(k.y, k.x).val[1] = rgb[j].val[1];
				test_im.at<Vec3b>(k.y, k.x).val[2] = rgb[j].val[2];
			}
	}



	// 去除非中心区域
	return Mat();
}

vector<vector<vector<Point3f>>> select_pointline(Mat dx, Mat dy, vector<Point3f> points, bool flag)
{
	Mat point_scoreimage = flag ? dx.clone() : -dx.clone();
	Mat line_scoreimage = dy.clone();
	vector<vector<vector<Point3f>>> result;
	int dety = line_scoreimage.cols / 6;
	dety = dety < 3 ? 3 : dety;
	int detx = line_scoreimage.cols / 4;
	Mat flag_image = Mat::zeros(point_scoreimage.size(), CV_8UC1);
	for (auto &i : points) {
		if (i.y<3 || i.y>point_scoreimage.rows - 4)
			continue;
		vector<Point3f> temp_point, temp_line1, temp_line2;
		float score_t = i.z*0.5;
		Mat temp_flag_image = flag_image.clone();
		temp_point.push_back(i);
		temp_flag_image.at<uchar>(i.y, i.x) = 255;
		//temp_point = cluster_point(point_scoreimage, score_t, Point(i.x, i.y), temp_flag_image.clone(),4);
		//temp_point = cluster_point(temp_point, true, temp_flag_image);
		//if (temp_point.size() < 1)
		//	continue;
		Mat temp_data;
		Point index; double maxval;
		int x1, x2, y1, y2;
		x1 = flag ? i.x - detx : i.x + 1;
		x2 = flag ? i.x : i.x + 1 + detx;
		if (x1<0 || x2>line_scoreimage.cols)
			continue;
		y1 = i.y - dety >= 0 ? i.y - dety : 0;
		y2 = i.y + dety <= line_scoreimage.rows ? i.y + dety : line_scoreimage.rows;
		temp_data = -line_scoreimage(Range(y1, i.y), Range(x1, x2)).clone();
		minMaxLoc(temp_data, NULL, NULL, NULL, &index);
		float score_t1 = temp_data.at<float>(index.y, index.x)*0.5;
		temp_line1 = cluster_point(-line_scoreimage.clone(), score_t, Point(index.x + x1, index.y + y1), temp_flag_image.clone(), 8);
		temp_line1 = cluster_point(temp_line1, false, temp_flag_image);
		if (temp_line1.size() < 1)
			continue;
		temp_data = line_scoreimage(Range(i.y + 1, y2), Range(x1, x2)).clone();
		minMaxLoc(temp_data, NULL, NULL, NULL, &index);
		float score_t2 = temp_data.at<float>(index.y, index.x)*0.5;
		temp_line2 = cluster_point(line_scoreimage.clone(), score_t2, Point(index.x + x1, index.y + i.y + 1), temp_flag_image.clone(), 8);
		temp_line2 = cluster_point(temp_line2, false, temp_flag_image);
		if (temp_line2.size() < 1)
			continue;
		vector<vector<Point3f>> temp_result = new_point_line1_line2(dx, dy, temp_point, temp_line1, temp_line2, flag);
		if (temp_result.size() != 3)
			continue;
		if (temp_result[0].size()>temp_result[1].size() || temp_result[0].size()>temp_result[2].size())
			continue;
		int temp_value = 255;
		for (auto i : temp_result) {
			for (auto j : i)
				flag_image.at<uchar>(j.y, j.x) = temp_value;
			temp_value -= 100;
		}
		result.push_back(temp_result);
	}
	return result;
}

vector<Point3f> cluster_point(Mat score_image, float score_t, Point point, Mat flag, int number)
{
	if (score_t<50)
		return vector<Point3f>();
	vector<Point3f> result;
	int x = point.x;
	int y = point.y;
	if (x<0 || y<0 || x>score_image.cols - 1 || y>score_image.rows - 1 || flag.at<uchar>(y, x) != 0)
		return vector<Point3f>();
	flag.at<uchar>(y, x) = 1;
	Point3f temp_point;
	temp_point.x = (float)point.x;
	temp_point.y = (float)point.y;
	temp_point.z = score_image.at<float>(y, x);
	if (temp_point.z < score_t)
		return vector<Point3f>();
	// 四邻域
	result.push_back(temp_point);
	vector<Point3f> temp;
	temp = cluster_point(score_image, score_t, Point(point.x, point.y - 1), flag, number);
	result.insert(result.end(), temp.begin(), temp.end());
	temp = cluster_point(score_image, score_t, Point(point.x, point.y + 1), flag, number);
	result.insert(result.end(), temp.begin(), temp.end());
	temp = cluster_point(score_image, score_t, Point(point.x + 1, point.y), flag, number);
	result.insert(result.end(), temp.begin(), temp.end());
	temp = cluster_point(score_image, score_t, Point(point.x - 1, point.y), flag, number);
	if (number > 4) {
		result.insert(result.end(), temp.begin(), temp.end());
		temp = cluster_point(score_image, score_t, Point(point.x - 1, point.y - 1), flag, number);
		result.insert(result.end(), temp.begin(), temp.end());
		temp = cluster_point(score_image, score_t, Point(point.x + 1, point.y - 1), flag, number);
		result.insert(result.end(), temp.begin(), temp.end());
		temp = cluster_point(score_image, score_t, Point(point.x + 1, point.y + 1), flag, number);
		result.insert(result.end(), temp.begin(), temp.end());
		temp = cluster_point(score_image, score_t, Point(point.x - 1, point.y + 1), flag, number);
		result.insert(result.end(), temp.begin(), temp.end());
	}
	return result;
}
vector<Point3f> cluster_point(Mat score_image, float score_t, int number)
{
	vector<Point> points;
	for (int i = 0; i < score_image.total(); ++i) {
		if (*(score_image.ptr<float>(0)+i) > score_t) {
			points.push_back(Point(i / score_image.cols, i%score_image.cols));
		}
	}
	Mat flag = Mat::zeros(score_image.size(),CV_8U);
	vector<Point3f> result;
	for (auto i : points) {
		vector<Point3f> temp=cluster_point(score_image, score_t, i, flag, number);
		if (temp.size() > result.size())
			result = temp;
	}
	return result;
}
vector<Point3f> cluster_point(vector<Point3f> data, bool flag, Mat &flag_image)
{
	if (flag) {
		for (auto &i : data)
			std::swap(i.x, i.y);
	}
	stable_sort(data.begin(), data.end(),
		[](const Point3f&a, const Point3f&b) {return a.x < b.x; });
	for (int i = 0; i < data.size(); ++i) {
		if (data[i].x >= 1) {
			data.erase(data.begin(), data.begin() + i);
			break;
		}
	}
	vector<Point3f> result;
	for (auto i : data) {
		if (result.size() == 0)
			result.push_back(i);
		else if (result[result.size() - 1].x == i.x&&result[result.size() - 1].z < i.z) {
			result.pop_back();
			result.push_back(i);
		}
		else if (result[result.size() - 1].x != i.x) {
			result.push_back(i);
		}
	}
	if (flag) {
		for (auto &i : result)
			std::swap(i.x, i.y);
	}
	for (auto i : result)
		flag_image.at<uchar>(i.y, i.x) = flag ? 255 : 125;
	return result;
}



vector<vector<Point3f>> new_point_line1_line2(Mat dx, Mat dy, vector<Point3f> point, vector<Point3f> line1_point, vector<Point3f> line2_point, bool flag)
{
	// 竖直直线的x坐标
	int x = point[0].x, y = point[0].y;
	//int x = 0;
	//map<int,int> temp;
	//int max = 0;
	//for (auto i : point) {
	//	++temp[(int)i.x];
	//}
	//for (auto i : temp) {
	//	if (i.second > max) {
	//		x = i.first;
	//		max = i.second;
	//	}
	//	if (i.second == max && flag&&i.first < x)
	//		x = i.first;
	//	if (i.second == max && !flag&&i.first > x)
	//		x = i.first;

	//}
	// 直线拟合
	vector<Point2f> temp_f_point;
	Point2f point_first(9999, 0), point_end(-9999, 0);
	vector<Point> temp_point, temp_line1_point, temp_line2_point;
	Vec4f point_para, line1_para, line2_para;
	float mindx1 = 999, mindy1 = 999;
	for (auto i : line1_point) {
		if (i.x <= x && flag) {
			temp_line1_point.push_back(Point((int)i.x, (int)i.y));
			mindx1 = mindx1 < abs(i.x - x) ? mindx1 : abs(i.x - x);
			mindy1 = mindy1 < abs(i.y - y) ? mindy1 : abs(i.y - y);
		}
		if (i.x >= x && !flag) {
			temp_line1_point.push_back(Point((int)i.x, (int)i.y));
			mindx1 = mindx1 < abs(i.x - x) ? mindx1 : abs(i.x - x);
			mindy1 = mindy1 < abs(i.y - y) ? mindy1 : abs(i.y - y);
		}
	}
	if (temp_line1_point.size()<2 || mindx1>3)
		return vector<vector<Point3f>>();
	//
	float mindx2 = 999, mindy2 = 999;
	for (auto i : line2_point) {
		if (i.x <= x && flag) {
			temp_line2_point.push_back(Point((int)i.x, (int)i.y));
			mindx2 = mindx2 < abs(i.x - x) ? mindx2 : abs(i.x - x);
			mindy2 = mindy2 < abs(i.y - y) ? mindy2 : abs(i.y - y);

		}

		if (i.x >= x && !flag) {
			temp_line2_point.push_back(Point((int)i.x, (int)i.y));
			mindx2 = mindx2 < abs(i.x - x) ? mindx2 : abs(i.x - x);
			mindy2 = mindy2 < abs(i.y - y) ? mindy2 : abs(i.y - y);
		}
	}
	if (temp_line2_point.size()<2 || mindx2>3)
		return vector<vector<Point3f>>();
	if (abs(mindy2 - mindy1)>8)
		return vector<vector<Point3f>>();
	//
	fitLine(temp_line1_point, line1_para, CV_DIST_FAIR, 0, 1e-2, 1e-2);
	temp_f_point.push_back(Point2f((float)x,
		(x - line1_para[2]) * line1_para[1] / line1_para[0] + line1_para[3]));
	for (auto i : temp_line1_point) {
		float d = (i.x - line1_para[2])*line1_para[0] + (i.y - line1_para[3])*line1_para[1];
		float x = d * line1_para[0] + line1_para[2];
		float y = d * line1_para[1] + line1_para[3];
		temp_f_point.push_back(Point2f(x, y));
	}
	for (auto i : temp_f_point) {
		if (i.x < point_first.x)
			point_first = i;
		if (i.x > point_end.x)
			point_end = i;
	}
	temp_line1_point = get_line_point(point_first, point_end);
	fitLine(temp_line2_point, line2_para, CV_DIST_FAIR, 0, 1e-2, 1e-2);
	temp_f_point.clear();
	temp_f_point.push_back(Point2f((float)x,
		(x - line2_para[2]) * line2_para[1] / line2_para[0] + line2_para[3]));
	for (auto i : temp_line2_point) {
		float d = (i.x - line2_para[2])*line2_para[0] + (i.y - line2_para[3])*line2_para[1];
		float x = d * line2_para[0] + line2_para[2];
		float y = d * line2_para[1] + line2_para[3];
		temp_f_point.push_back(Point2f(x, y));
	}
	point_first.x = 9999;
	point_end.x = -9999;
	for (auto i : temp_f_point) {
		if (i.x < point_first.x)
			point_first = i;
		if (i.x > point_end.x)
			point_end = i;
	}
	temp_line2_point = get_line_point(point_first, point_end);
	point_first.y = 9999;
	point_end.y = -9999;
	if (flag) {
		point_first = *(temp_line1_point.end() - 1);
		point_end = *(temp_line2_point.end() - 1);
	}
	else {
		point_first = temp_line1_point[0];
		point_end = temp_line2_point[0];
	}
	temp_point = get_line_point(point_first, point_end);
	vector<vector<Point3f>> temp_result;
	point.clear();
	line1_point.clear();
	line2_point.clear();
	for (auto i : temp_point)
		point.push_back(Point3f((float)i.x, (float)i.y, dx.at<float>(i.y, i.x)));
	for (auto i : temp_line1_point)
		line1_point.push_back(Point3f((float)i.x, (float)i.y, dy.at<float>(i.y, i.x)));
	for (auto i : temp_line2_point)
		line2_point.push_back(Point3f((float)i.x, (float)i.y, dy.at<float>(i.y, i.x)));
	temp_result.push_back(point);
	temp_result.push_back(line1_point);
	temp_result.push_back(line2_point);
	return temp_result;
}