#pragma once

//#include "darknet.h"
#include "common.h"
#include "argv.h"
#include <time.h>
#include <iostream>
#include <winsock2.h>
#include <algorithm>//// 2018.09.03
//#include "license.h" // eyw 2018.09.07
#include <fstream> // 2018.12.13 Osan CCTV
#include <string> //eyw 209.01.07
#include <vector>
#include <ctime>
#include <process.h>


// opencv
//#include <opencv2/opencv.hpp>
//#include "opencv/cv.h"
//#include "opencv/highgui.h"
using namespace cv;
using namespace std;

#include "param_set.h"       //190322 about parameter functions
#include "CCamPosition.h"    //190325 about camera position 
#include "CRegion.h"         //190401 about regions like line and roi(parking area) 

//20190508 tensorflow Test
//#include "tf_classifier_hai.h"

#define BOX_W					184
#define BOX_H					72
#define BOX_Y					204

#define SEDAN_X					227
#define SUV_X					417
#define TRUCK_X					609
#define BUS_X					799
#define MOTO_X					989
#define WHEL_X					1179

typedef struct __DataInfo {
	int mJigNum;
	int mData;
	char sData[100];
}DINFO, *PINFO;


class DarknetData
{
public:
	DarknetData();
	~DarknetData();
	//void mosaic_draw(IplImage* frame, CvRect _rect);
	void convertimgtoBM(Mat& input_img, Size imageSize, HDC hdc, int offsetX, int offsetY);
public:
	
	bool set_class_id(st_det_mot_param *param);
	bool get_param_string(FILE *fp, char *q_string, char *a_string);
	bool get_param_string_title(FILE *fp, char *q_string, char *a_string);
	bool get_param_int(FILE *fp, char *q_string, int &value);
	bool get_param_float(FILE *fp, char *q_string, float &value);
	bool get_param_int_array(FILE *fp, char *q_string, int *value, int n);

	void mosaic_draw(Mat& frame, cv::Rect _rect, int mb);

	//bool alive_message_send();
	//void run_det(Mat frame, st_det_mot_param *param);

	//void run_mot(int nCameraIndex, Mat frame, st_det_mot_param *param, st_biker_info *biker_info, float fXratio, float fYratio);

	void free_det_mot_param(st_det_mot_param *param);

	//int make_det_mot_param(st_det_mot_param *param, int nCamIndex);

	//void run_det_mot(int nCameraIndex, Mat frame, Mat res_frame, st_det_mot_param *param, char *strTitle, st_biker_info *biker_info, float fXratio, float fYratio);//hai 2018.12.12

	void catch_biker_violation(Mat& frame, Mat& res_frame, st_det_mot_param *param, st_biker_info &biker_info, st_violation_info &vio_status, int line, CRegion* pRegion);

	void make_biker_info_structure(st_det_mot_param *param, st_biker_info &biker_info);

	void make_vio_status_structure(Mat& frame, Mat& res_frame, st_det_mot_param *param, st_biker_info &biker_info, st_violation_info &vio_status, int line, CRegion* pRegion);

	float rect_iou(cv::Rect a, cv::Rect b, int type); //2018.10.12 hai

	bool line_touch(cv::Rect box, int line);

	int find_num_rider_histogram(Mat& frame, int nTrackID, st_sub_box_info *sub_box_list); //2018.10.12 hai

	void biker_violation_draw(Mat& image, st_violation_info &vio_statu);

	void biker_violation_save_log(FILE *fp, st_biker_info &biker_info, int nVioIdx, st_violation_info &vio_status, int ch_id, char *time, int nMaxRider);

	void violation_box_draw_save_image(st_det_mot_param *param, Mat& ori_image, Mat& res_image, int nVioIdx, st_violation_info &vio_status); // 2018.10.26

	char* getStringClassID(int nClassID);

	bool line_touch_forcounting(cv::Rect box, int line);

	void release_lane_violation(st_det_mot_param *param, int nTrackID);

	void release_reverse_violation(st_det_mot_param *param, int nTrackID); //20190523 hai reverse violation

	void release_stop_violation(st_det_mot_param *param, int nTrackID); //stop violation

	void object_flow_save(FILE *fp, st_mot_info *mot_info, int num, int ch_id, char *time);

	void violation_text_display(Mat& image, st_det_mot_param *param, Rect bounding_box, int nTrackID);

	void draw_objectbox_forLPR(Mat& SaveImage, cv::Rect rectArea, cv::Scalar VioColor, bool bEdit);

	bool is_bike(int nClass);

	bool is_car(int nClass);

	bool is_under_consideration_vehicle(int nClass);

	//Radar
	bool Radar_line_touch_Distance(int nPtY, int line);

};

//================ From darknet.cpp ======================
//20190627



//=========================================================