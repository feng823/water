// assist_water.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "water.h"

int main(int argc, char** argv)
{

	if (argc < 2) {
		//cout << "please input image name\n";
		return 0;
	}
	string image_name(argv[1]);
	Mat image = imread(image_name, IMREAD_COLOR);
	if (!image.data)
	{
		//cout << " No image data \n";
		return -1;
	}
	// 取出无后缀的文件名及文件所在文件夹路径
	string base_name, base_path;
	for (int i = image_name.size(); i >= 0; --i) {
		if (image_name[i] == 47 || image_name[i] == 92) {
			base_name.append(image_name.begin() + i + 1, image_name.end() - 4);
			base_path.append(image_name.begin(), image_name.begin() + i + 1);
			break;
		}
	}
	if (base_path == "") {
		base_name.append(image_name.begin(), image_name.end() - 4);
	}
	// 一些应该存在的重要文件所在路径
	string assist_image_name(base_path + "assist_" + base_name + string(image_name.end() - 4, image_name.end()));
	string assist_txt_name(base_path + "assist_" + base_name + ".txt");
	string template_image_name(base_path + "template.xml");
	string sub_name(base_path + "sub_" + base_name + ".bmp");
	string ref_name(base_path + "ref_" + base_name + ".bmp");
	string result_image(base_path + "result_" + base_name + ".jpg");
	string result_txt(base_path + "result_" + base_name + ".txt");
	map<string, string> main_ini;
	main_ini.insert(map<string, string>::value_type("assist_image", assist_image_name));
	main_ini.insert(map<string, string>::value_type("assist_txt", assist_txt_name));
	main_ini.insert(map<string, string>::value_type("ref", ref_name));
	main_ini.insert(map<string, string>::value_type("sub", sub_name));
	main_ini.insert(map<string, string>::value_type("template", template_image_name));
	main_ini.insert(map<string, string>::value_type("result_image", result_image));
	main_ini.insert(map<string, string>::value_type("result_txt", result_txt));
	for (int i = 2; i < argc; ++i) {
		string temp(argv[i]);
		string temp1, temp2;
		for (int j = 0; j < temp.size(); ++j) {
			if (temp[j] == 61) {
				temp1.append(temp.begin(), temp.begin() + j);
				temp2.append(temp.begin() + j + 1, temp.end());
			}
		}
		if (temp2 != "") {
			main_ini[temp1] = temp2;
		}
	}
	// 模板
	vector<Mat> template_image;
	bool flag;
	flag = input_template(main_ini["template"], template_image);
	if (!flag) {
		// 制作模板xml
		// 取出无后缀的文件名及文件所在文件夹路径
		string base_path;
		for (int i = main_ini["template"].size(); i >= 0; --i) {
			if (main_ini["template"][i] == 47 || main_ini["template"][i] == 92) {
				base_path.append(main_ini["template"].begin(), main_ini["template"].begin() + i + 1);
				break;
			}
		}
		vector<Mat> model;
		vector<string> files = getFiles(base_path, "template", ".bmp");
		for (auto &i : files) {
			Mat temp_im = imread(i, CV_LOAD_IMAGE_GRAYSCALE);
			model.push_back(temp_im.clone());
		}
		FileStorage fs(base_path+"template.xml", FileStorage::WRITE);
		fs << "model" << "[";
		for (auto &i : model) {
			fs << i;
		}
		fs << "]";
		fs.release();
		flag = input_template(main_ini["template"], template_image);
		if (!flag) {
			//cout << " template file error \n";
			return -2;
		}
	}
	// assit txt
	vector<assist_information> assist_files;
	flag = input_assist(image,main_ini, assist_files, template_image);
	if (!flag) {
		//cout << " assist file error \n";
		return -3;
	}
	compute_water_area(image, assist_files, main_ini["ref"]);
	save_file(image, assist_files, main_ini);
	return 1;
}

