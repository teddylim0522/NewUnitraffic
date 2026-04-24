#include "DarknetData.h"
#include <cuda.h>
#include <cuda_runtime.h>

DarknetData::DarknetData()
{
}


DarknetData::~DarknetData()
{
}

void DarknetData::convertimgtoBM(Mat & input_img, Size imageSize, HDC hdc, int offsetX, int offsetY)
{
	Mat cvImgTmp;
	resize(input_img, cvImgTmp, imageSize, 0, 0, INTER_AREA);
	int stride = ((((imageSize.width * 24) + 31)  &  ~31) >> 3);
	uchar* pcDibBits = (uchar*)malloc(imageSize.height * stride);
	if (pcDibBits != NULL)
	{
		// Copy the raw pixel data over to our dibBits buffer.
		// NOTE: Can setup cvImgTmp to add the padding to skip this.
		for (int row = 0; row < cvImgTmp.rows; ++row)
		{
			// Get pointers to the beginning of the row on both buffers
			uchar* pcSrcPixel = cvImgTmp.ptr<uchar>(row);
			uchar* pcDstPixel = pcDibBits + (row * stride);

			// We can just use memcpy
			memcpy(pcDstPixel, pcSrcPixel, stride);
		}

		// Initialize the BITMAPINFO structure
		BITMAPINFO bitInfo;
		bitInfo.bmiHeader.biBitCount = 24;
		bitInfo.bmiHeader.biWidth = cvImgTmp.cols;
		bitInfo.bmiHeader.biHeight = -cvImgTmp.rows;
		bitInfo.bmiHeader.biPlanes = 1;
		bitInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitInfo.bmiHeader.biCompression = BI_RGB;
		bitInfo.bmiHeader.biClrImportant = 0;
		bitInfo.bmiHeader.biClrUsed = 0;
		bitInfo.bmiHeader.biSizeImage = 0;      //winSize.height * winSize.width * * 3;
		bitInfo.bmiHeader.biXPelsPerMeter = 0;
		bitInfo.bmiHeader.biYPelsPerMeter = 0;

		// Add header and OPENCV image's data to the HDC
		StretchDIBits(hdc, offsetX, offsetY, cvImgTmp.cols, cvImgTmp.rows, 0, 0, cvImgTmp.cols,
			cvImgTmp.rows, pcDibBits, &bitInfo, DIB_RGB_COLORS, SRCCOPY);

		free(pcDibBits);
	}
	if (!cvImgTmp.empty()) cvImgTmp.release();
}

void DarknetData::mosaic_draw(Mat& frame, cv::Rect _rect, int mb)
{
	int w_point = 0, h_point = 0, y_start = 0, x_start = 0;
	int r = 0, g = 0, b = 0;
	int w_p, h_p;
	int nCnt;
	for (int i = 0; i < ceil(_rect.height) / mb; i++)
	{
		for (int j = 0; j < ceil(_rect.width) / mb; j++)
		{
			nCnt = 0;
			r = 0; g = 0; b = 0;

			x_start = _rect.x + j * mb;
			y_start = _rect.y + i * mb;

			for (int mb_y = y_start; mb_y < y_start + mb; mb_y++)
			{
				for (int mb_x = x_start; mb_x < x_start + mb; mb_x++)
				{
					//cv::Scalar color;
					w_p = mb_x;
					h_p = mb_y;

					if (mb_x >= frame.cols)
					{
						w_p = frame.cols - 1;
					}
					if (mb_y >= frame.rows)
					{
						h_p = frame.rows - 1;
					}

					//color = cv::Get2D(&frame, h_p, w_p);
					cv::Vec3b &color = frame.at<cv::Vec3b>(h_p, w_p);
					b += color.val[0];
					g += color.val[1];
					r += color.val[2];
					nCnt++;
				}
			}
			b /= nCnt;
			g /= nCnt;
			r /= nCnt;

			//rectangle(frame, Rect(x_start, y_start, mb, mb), Scalar(b, g, r, 0), CV_FILLED, 1, 0);
			rectangle(frame, Rect(x_start, y_start, mb, mb), Scalar(b, g, r, 0), cv::FILLED, 1, 0);
		}
	}
}


bool DarknetData::set_class_id(st_det_mot_param *param)
{
	TCHAR currentPath[MAX_PATH];
	DWORD dwLength = GetModuleFileName(nullptr, currentPath, MAX_PATH);
	if (0 != dwLength)
	{
		TCHAR* path = currentPath + dwLength;
		while (--path != currentPath && *path != L'\\');
		if (*path == L'\\')
		{
			*path = L'\0';
		}
	}

	TCHAR file[MAX_PATH];

#if SPEED_KOREA_EXPRESSWAY_MODE == 1 // for speed test (elementary school videos)
	sprintf(file, "%s/class_group_elementary.ini", currentPath);
#else
	sprintf(file, "%s/class_group.ini", currentPath);
#endif

	FILE *fp = fopen(file, "r");
	if (fp == NULL)
	{
		printf("there is invalid class_group.ini file!!!\n");
		return false;
	}
	if (!get_param_int_array(fp, "CLASS_GROUP_ID", param->class_group_id, param->nLclasses))
	{
		fclose(fp);
		return false;
	}
#ifndef XYCOOR_MODE
    // these lines make error while free memory
	if (!get_param_int_array(fp, "VIOLATION_HEAD_CLASS", param->vio_head_class, param->n_vio_head_class))
	{
		fclose(fp);
		return false;
	}
	// 2018.09.27 hai
	if (!get_param_int(fp, "VIOLATION_UMBRELLA_CLASS", param->vio_umbrella_class))
	{
		fclose(fp);
		return false;
	}
	// 2018.09.27 hai
	if (!get_param_int(fp, "VIOLATION_MOBILE_CLASS", param->vio_mobile_class))
	{
		fclose(fp);
		return false;
	}
#endif
	if (!get_param_int(fp, "REAR_BIKER_ID", param->rear_biker_id))
	{
		fclose(fp);
		return false;
	}
	if (!get_param_int(fp, "BIKER_GROUP_ID", param->biker_group_id))
	{
		fclose(fp);
		return false;
	}
	if (!get_param_int(fp, "HEAD_GROUP_ID", param->head_group_id))
	{
		fclose(fp);
		return false;
	}
	// 2018.09.27 hai
	if (!get_param_int(fp, "UMBRELLA_GROUP_ID", param->umbrella_group_id))
	{
		fclose(fp);
		return false;
	}
	// 2018.09.27 hai
	if (!get_param_int(fp, "MOBILE_GROUP_ID", param->mobile_group_id))
	{
		fclose(fp);
		return false;
	}

	fclose(fp);
	return true;
}



bool DarknetData::get_param_string(FILE *fp, char *q_string, char *a_string)
{
	char tbuf[300];
	char tmp[100];
	fscanf(fp, "%s", tbuf);
	if (strcmp(tbuf, q_string))
	{
		printf("Invalid %s\n", tbuf);
		fclose(fp);
		return false;
	}
	fscanf(fp, "%s", tmp);
	fscanf(fp, " %[^\n]", a_string);
	printf("%s%s%s\n", tbuf, tmp, a_string);
	return true;
}

bool DarknetData::get_param_string_title(FILE *fp, char *q_string, char *a_string)
{
	char tmp[100];

	fscanf(fp, "%s", tmp);

	return true;
}

bool DarknetData::get_param_int(FILE *fp, char *q_string, int &value)
{
	char tbuf[300];
	char tmp[100];
	fscanf(fp, "%s", tbuf);
	if (strcmp(tbuf, q_string))
	{
		printf("Invalid %s\n", tbuf);
		fclose(fp);
		return false;
	}
	fscanf(fp, "%s", tmp);
	fscanf(fp, "%d", &value);
	printf("%s%s%d\n", tbuf, tmp, value);
	return true;
}

bool DarknetData::get_param_float(FILE *fp, char *q_string, float &value)
{
	char tbuf[300];
	char tmp[100];
	fscanf(fp, "%s", tbuf);
	if (strcmp(tbuf, q_string))
	{
		printf("Invalid %s\n", tbuf);
		fclose(fp);
		return false;
	}
	fscanf(fp, "%s", tmp);
	fscanf(fp, "%f", &value);
	printf("%s%s%f\n", tbuf, tmp, value);
	return true;
}

bool DarknetData::get_param_int_array(FILE *fp, char *q_string, int *value, int n)
{
	char tbuf[300];
	char tmp[100];
	fscanf(fp, "%s", tbuf);
	if (strcmp(tbuf, q_string))
	{
		printf("Invalid %s\n", tbuf);
		fclose(fp);
		return false;
	}
	fscanf(fp, "%s", tmp);
	printf("%s%s", tbuf, tmp);
	for (int i = 0; i < n; i++)
	{
		fscanf(fp, "%d", &value[i]);
		printf(" %d", value[i]);
	}
	printf("\n");
	return true;
}


void DarknetData::catch_biker_violation(Mat& frame, Mat& res_frame, st_det_mot_param *param, st_biker_info &biker_info, st_violation_info &vio_status, int line, CRegion* pRegion)
{

	make_biker_info_structure(param, biker_info);
	make_vio_status_structure(frame, res_frame, param, biker_info, vio_status, line, pRegion);

#if 0		
	printf("vio_status.n_violation = %d\n", vio_status.n_violation);
	for (int v = 0; v < n_violation; v++)
	{
		printf("%d)type = (%d %d)\n", v, vio_status.violation_box_info[v].max_rider_violation, vio_status.violation_box_info[v].helmet_violation);
		printf("box = (%d %d %d %d)\n", v, vio_status.violation_box_info[v].bouding_box.x, vio_status.violation_box_info[v].bouding_box.y,
			vio_status.violation_box_info[v].bouding_box.width, vio_status.violation_box_info[v].bouding_box.height);

	}
#endif

}



void DarknetData::make_biker_info_structure(st_det_mot_param *param, st_biker_info &biker_info)
{
	int n_track = param->mot_info->n_track_num;
	int n_detect = param->mot_info->n_detect_num;
	int n_biker_box = 0;
	st_mot_box_info *mot_info = param->mot_info->mot_box_info;
	st_det_box_info *det_info = param->mot_info->det_box_info;
	int group_id = param->biker_group_id;
	int sub_group_id = param->head_group_id;
	float iou_thresh = param->head_nms;

	int umbrella_group_id = param->umbrella_group_id; // 2018.09.27 hai
	int mobile_group_id = param->mobile_group_id; // 2018.09.27 hai

	//std::cout << "umbrella_group_id: " << umbrella_group_id << "; mobile_group_id: " << mobile_group_id << std::endl;

	for (int t = 0; t < n_track; t++)
	{
		mot_info[t].n_sub_box = 0;

		if (mot_info[t].class_group_id == group_id) //biker, cyclist¿¡ ÇØ´çµÇ´Â class
		{
			biker_info.biker_box_info[n_biker_box].class_id = mot_info[t].class_id;
			biker_info.biker_box_info[n_biker_box].track_id = mot_info[t].track_id;
			biker_info.biker_box_info[n_biker_box].bouding_box = mot_info[t].bouding_box;

			param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].n_box_idx++;
			param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].n_box_idx = param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].n_box_idx % MAX_FRAME_NUM;
			int sub_idx = param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].n_box_idx;

			param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].bouding_box[sub_idx] = mot_info[t].bouding_box;

			int n_head_box = 0;
			float mean_score = 0; //2018.10.12 hai

			param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].nIsMobile = 0;
			param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].nIsUmbrella = 0;

			for (int d = 0; d < n_detect; d++)
			{
				if (det_info[d].class_group_id == sub_group_id) //head¿¡ ÇØ´çµÇ´Â class
				{

					cv::Rect roi_box; //biker ¿µ¿ªÁß¿¡¼­ headºÎºÐ¿¡ ÇØ´çµÇ´Â °ü½É ¿µ¿ª
					roi_box.x = mot_info[t].bouding_box.x + mot_info[t].bouding_box.width / 6;
					roi_box.y = mot_info[t].bouding_box.y;
					roi_box.width = mot_info[t].bouding_box.width * 2 / 3;

#if ENGINE_MODE == 0
					if (mot_info[t].class_id == cls_truckbike_front || mot_info[t].class_id == cls_truckbike_rear || mot_info[t].class_id == cls_truckbike_side)
					{
						roi_box.height = mot_info[t].bouding_box.height / 3;
					}
					else
#endif
					{
						roi_box.height = mot_info[t].bouding_box.height / 4;
					}

					float iou = rect_iou(roi_box, det_info[d].bouding_box, 1);
					if (iou > iou_thresh)// && det_info[d].score >= HEAD_SCORE_THRESH)
					{
						mot_info[t].sub_box_info.bouding_box = biker_info.biker_box_info[n_biker_box].head_box_info[n_head_box].bouding_box = det_info[d].bouding_box;
						mot_info[t].sub_box_info.class_id = biker_info.biker_box_info[n_biker_box].head_box_info[n_head_box].class_id = det_info[d].class_id;

						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].head_box_list[sub_idx].head_box_info[n_head_box].class_id = det_info[d].class_id;
						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].head_box_list[sub_idx].head_box_info[n_head_box].bouding_box = det_info[d].bouding_box;

						mean_score += det_info[d].score; //2018.10.12 hai
						n_head_box++;
					}
				}

				// 2018.09.27 hai - umbrella
				if (det_info[d].class_group_id == umbrella_group_id) //head¿¡ ÇØ´çµÇ´Â class
				{
					//std::cout << "-------------------------------------------det_info[d].class_group_id == umbrella_group_id" << std::endl;

					cv::Rect roi_box; //biker ¿µ¿ªÁß¿¡¼­ headºÎºÐ¿¡ ÇØ´çµÇ´Â °ü½É ¿µ¿ª
					roi_box.x = mot_info[t].bouding_box.x;
					roi_box.y = mot_info[t].bouding_box.y;
					roi_box.width = mot_info[t].bouding_box.width;
					roi_box.height = mot_info[t].bouding_box.height;
					float iou = rect_iou(roi_box, det_info[d].bouding_box, 1);
					if (iou > iou_thresh)
					{
						/*mot_info[t].umbrella_box_info.bouding_box = biker_info.biker_box_info[n_biker_box].umbrella_box_info.bouding_box = det_info[d].bouding_box;
						mot_info[t].umbrella_box_info.class_id = biker_info.biker_box_info[n_biker_box].umbrella_box_info.class_id = det_info[d].class_id;*/

						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].umbrella_box_info.class_id = det_info[d].class_id;
						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].umbrella_box_info.bouding_box = det_info[d].bouding_box;

						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].nIsUmbrella = 1;
					}
				}

				// 2018.09.27 hai - mobile
				if (det_info[d].class_group_id == mobile_group_id) //head¿¡ ÇØ´çµÇ´Â class
				{
					//std::cout << "-------------------------------------------det_info[d].class_group_id == mobile_group_id" << std::endl;
					cv::Rect roi_box; //biker ¿µ¿ªÁß¿¡¼­ headºÎºÐ¿¡ ÇØ´çµÇ´Â °ü½É ¿µ¿ª
					roi_box.x = mot_info[t].bouding_box.x;
					roi_box.y = mot_info[t].bouding_box.y;
					roi_box.width = mot_info[t].bouding_box.width;
					roi_box.height = mot_info[t].bouding_box.height / 2;
					float iou = rect_iou(roi_box, det_info[d].bouding_box, 1);
					if (iou > iou_thresh)
					{
						/*mot_info[t].mobile_box_info.bouding_box = biker_info.biker_box_info[n_biker_box].mobile_box_info.bouding_box = det_info[d].bouding_box;
						mot_info[t].mobile_box_info.class_id = biker_info.biker_box_info[n_biker_box].mobile_box_info.class_id = det_info[d].class_id;*/

						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].mobile_box_info.class_id = det_info[d].class_id;
						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].mobile_box_info.bouding_box = det_info[d].bouding_box;

						param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].nIsMobile = 1;
					}
				}

			}
			mot_info[t].n_sub_box = biker_info.biker_box_info[n_biker_box].n_box = n_head_box;

			//20190406
			if (n_head_box > 0)
			{
				param->sub_box_list[mot_info[t].track_id].mean_score[sub_idx] = mean_score / n_head_box;
				//param->sub_box_list[biker_info.biker_box_info[n_biker_box].track_id].mean_score[sub_idx] = mean_score / n_head_box; //2018.10.12 hai
			}
			//20190406
			n_biker_box++;
		}
	}
	biker_info.n_biker_box = n_biker_box;

}



//// 2018.09.03

void DarknetData::make_vio_status_structure(Mat& frame, Mat& res_frame, st_det_mot_param *param, st_biker_info &biker_info, st_violation_info &vio_status, int line, CRegion* pRegion)
{
	int max_rider = param->max_rider;
	const int n_violation_class = param->n_vio_head_class;
	int *violation_class = param->vio_head_class;
	int n_violation = 0;
	int n_violation_confirm = param->n_vio_confirm;

	// 2018.09.27 hai
	int umbrella_violation_class = param->vio_umbrella_class;
	int mobile_violation_class = param->vio_mobile_class;

	bool bIsTruckBike = false;


	int nCheckBikeViolation = param->nCheckBikeViolation;
	if (!getMode(param, OPERATION_MODE_BIKE))
	{
		nCheckBikeViolation = 0;
	}

	//std::cout << "umbrella_violation_class: " << umbrella_violation_class << "; mobile_violation_class: " << mobile_violation_class << std::endl;

	for (int t = 0; t < biker_info.n_biker_box; t++)
	{
		//		printf("%d)n_head_box = %d\n", t, biker_info.biker_box_info[t].n_box);

		//2018.10.24 Young
		int nPtBottomX = biker_info.biker_box_info[t].bouding_box.x + (biker_info.biker_box_info[t].bouding_box.width / 2);
		int nPtBottomY = biker_info.biker_box_info[t].bouding_box.y + (biker_info.biker_box_info[t].bouding_box.height);
		int nBoxWidth = biker_info.biker_box_info[t].bouding_box.width; // 2018.11.09 hai

		if (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, nBoxWidth) == NO_VIOLATION_AREA)
		{
			continue;
		}

#if ENGINE_MODE == 0
		if ((biker_info.biker_box_info[t].class_id == cls_truckbike_front) || (biker_info.biker_box_info[t].class_id == cls_truckbike_rear))
			bIsTruckBike = true;
		else
#endif
			bIsTruckBike = false;

		bool b_violation = false;
		vio_status.violation_box_info[n_violation].max_rider_violation = 0;
		vio_status.violation_box_info[n_violation].helmet_violation = 0;

		// 2018.09.27 hai
		vio_status.violation_box_info[n_violation].umbrella_violation = 0;
		vio_status.violation_box_info[n_violation].mobile_violation = 0;

		sprintf(vio_status.violation_box_info[n_violation].vio_type, "N");

		int nTrackID = biker_info.biker_box_info[t].track_id; //2018.10.12 hai
		int sub_idx = param->sub_box_list[nTrackID].n_box_idx;
		param->sub_box_list[nTrackID].n_box_list[sub_idx] = biker_info.biker_box_info[t].n_box;

		// 2018.10.26 hai
		//if (param->sub_box_list[nTrackID].vio_image[sub_idx])
		//	cvReleaseImage(&param->sub_box_list[nTrackID].vio_image[sub_idx]);

		if (param->show_det_on)
		{
			//param->sub_box_list[nTrackID].vio_image[sub_idx] = cvCreateImage(cvSize(frame.cols, frame.rows), frame->depth, frame->nChannels);
			//cvCopy(frame, param->sub_box_list[nTrackID].vio_image[sub_idx]);
			param->sub_box_list[nTrackID].vio_image[sub_idx].clone();
		}


		//2018.10.12 hai [making histogram of number head boxes]
		int nPad = 20;
		for (int m = 0; m < MAX_SUB_BOX; m++)
		{
			if (((param->sub_box_list[nTrackID].bouding_box[sub_idx].x > nPad)
				&& (param->sub_box_list[nTrackID].bouding_box[sub_idx].x + param->sub_box_list[nTrackID].bouding_box[sub_idx].width < frame.cols - nPad)
				&& (param->sub_box_list[nTrackID].bouding_box[sub_idx].y + param->sub_box_list[nTrackID].bouding_box[sub_idx].height < frame.rows - nPad))
				|| ((param->sub_box_list[nTrackID].bouding_box[sub_idx].x > nPad)
					&& (param->sub_box_list[nTrackID].bouding_box[sub_idx].x + param->sub_box_list[nTrackID].bouding_box[sub_idx].width < frame.cols - nPad)
					&& (param->sub_box_list[nTrackID].bouding_box[sub_idx].height >= param->sub_box_list[nTrackID].bouding_box[sub_idx].width)))
			{
				if (biker_info.biker_box_info[t].n_box == m)
					param->sub_box_list[nTrackID].n_rider_histogram[m]++;
			}
		}



		if ((!line_touch(biker_info.biker_box_info[t].bouding_box, line))
#if ENGINE_MODE == 0
			|| ((biker_info.biker_box_info[t].class_id != cls_bike_rear) && (biker_info.biker_box_info[t].class_id != cls_bike_front)
				&& (biker_info.biker_box_info[t].class_id != cls_truckbike_front) && (biker_info.biker_box_info[t].class_id != cls_truckbike_rear)
				//&& (biker_info.biker_box_info[t].class_id != cls_truckbike_side) && (biker_info.biker_box_info[t].class_id != cls_truckbike_side)
				)
#else
				|| (is_bike(biker_info.biker_box_info[t].class_id) == false)
#endif
				)
			//	if ((!line_touch(biker_info.biker_box_info[t].bouding_box, line)) || (biker_info.biker_box_info[t].class_id != 4))
		{
			//hai 2018.12.12 
			//if (biker_info.TO_violation[nTrackID]++ >= n_violation_confirm)
			//{
			//	biker_info.rider_violation_confirm[nTrackID] = 0;		////=== hai edited ===////
			//	biker_info.helmet_violation_confirm[nTrackID] = 0;		////=== hai edited ===////
			//	biker_info.TO_violation[nTrackID] = 0;

			//	biker_info.umbrella_violation_confirm[nTrackID] = 0;		// 2018.09.27 hai
			//	biker_info.mobile_violation_confirm[nTrackID] = 0;		// 2018.09.27 hai

			//	biker_info.violation_confirm[nTrackID] = 0;				// 2018.10.22 hai
			//}
			//std::cout << "no touch: trackid: " << nTrackID << "no touch: count: " << biker_info.TO_violation[nTrackID] << std::endl;
			continue;
		}
		else if ((line_touch(biker_info.biker_box_info[t].bouding_box, line) && biker_info.violation_confirm[nTrackID] == 1))
		{
			biker_info.n_touched_count[nTrackID] = 0;
			continue;
		}

		//biker_info.n_touched_count[nTrackID]++; //200309 hai

		//biker_info.n_touched_count[nTrackID] = biker_info.n_touched_count[nTrackID] % 100;


		int track_id = nTrackID;
		//		printf("%d = %d\n", track_id, param->mot_info->n_violated[track_id]);

		std::cout << "touched: trackid: " << track_id << "confirm: " << biker_info.rider_violation_confirm[track_id] << ";" << biker_info.helmet_violation_confirm[track_id] << std::endl;

		//2018.10.23 hai
		/*int n_min_rider = find_min_rider(res_frame, track_id, param->sub_box_list);
		int n_min_idx = param->sub_box_list[track_id].n_min_rider_idx;*/
		int n_min_rider = find_num_rider_histogram(frame, track_id, param->sub_box_list);
		int n_min_idx = param->sub_box_list[track_id].n_his_rider_index;


		std::cout << "============= n_min_rider_idx: " << n_min_idx << "=========== n_min_rider: " << n_min_rider << " ===== current n_box: " << biker_info.biker_box_info[t].n_box << std::endl;

		if (nCheckBikeViolation && n_min_rider >= max_rider && biker_info.rider_violation_confirm[track_id] < n_violation_confirm && !bIsTruckBike)
		{
			biker_info.rider_violation_confirm[track_id]++;

			b_violation = true;
			vio_status.violation_box_info[n_violation].max_rider_violation = 1;
			sprintf(vio_status.violation_box_info[n_violation].vio_type, "O");

			vio_status.violation_box_info[n_violation].class_id = biker_info.biker_box_info[t].class_id;
			vio_status.violation_box_info[n_violation].bouding_box = biker_info.biker_box_info[t].bouding_box;

			vio_status.violation_box_info[n_violation].track_id = biker_info.biker_box_info[t].track_id; // 2018.10.23

		}

		vio_status.violation_box_info[n_violation].n_person = param->sub_box_list[track_id].n_box_list[n_min_idx];
		vio_status.violation_box_info[n_violation].n_person_helmet = vio_status.violation_box_info[n_violation].n_person;

		bool b_helmet_violation = false;

		std::cout << "rider_violation_confirm " << biker_info.rider_violation_confirm[track_id] << std::endl;
		std::cout << "helmet_violation_confirm " << biker_info.helmet_violation_confirm[track_id] << std::endl;

		for (int n = 0; n < param->sub_box_list[track_id].n_box_list[n_min_idx]; n++)
		{
			//			printf("%d)head_box_class = %d\n", n, biker_info.biker_box_info[t].head_box_info[n].class_id);
			for (int v = 0; v < n_violation_class; v++)
			{
				if (nCheckBikeViolation && param->sub_box_list[track_id].head_box_list[n_min_idx].head_box_info[n].class_id == violation_class[v] && biker_info.helmet_violation_confirm[track_id] < n_violation_confirm) ////=== hai edited ===////
				{
					////=== hai edited ===////
					b_helmet_violation = true;
					b_violation = true;
					vio_status.violation_box_info[n_violation].helmet_violation = 1;
					sprintf(vio_status.violation_box_info[n_violation].vio_type, "H");

					vio_status.violation_box_info[n_violation].n_person_helmet--;
					vio_status.violation_box_info[n_violation].class_id = biker_info.biker_box_info[t].class_id;
					vio_status.violation_box_info[n_violation].bouding_box = biker_info.biker_box_info[t].bouding_box;

					vio_status.violation_box_info[n_violation].track_id = biker_info.biker_box_info[t].track_id; // 2018.10.23
				}
			}
		}

		if (b_helmet_violation == true)
		{
			biker_info.helmet_violation_confirm[track_id] ++;
		}

		// 2018.09.27 hai - umbrella
		bool b_umbrella_violation = false;

		//std::cout << "umbrella_violation_confirm " << biker_info.umbrella_violation_confirm[track_id] << std::endl;
		//std::cout << "biker_info.biker_box_info[track_id].umbrella_box_info.class_id: " << param->sub_box_list[track_id].umbrella_box_info.class_id << std::endl;

		//if(biker_info.biker_box_info[track_id].umbrella_box_info.class_id == umbrella_violation_class && biker_info.umbrella_violation_confirm[track_id] < n_violation_confirm)
		if (nCheckBikeViolation && param->sub_box_list[track_id].umbrella_box_info.class_id == umbrella_violation_class && biker_info.umbrella_violation_confirm[track_id] < n_violation_confirm
			&& param->sub_box_list[track_id].nIsUmbrella == 1 && !bIsTruckBike)
		{
			biker_info.umbrella_violation_confirm[track_id]++;

			b_umbrella_violation = true;
			b_violation = true;
			vio_status.violation_box_info[n_violation].umbrella_violation = 1;
			sprintf(vio_status.violation_box_info[n_violation].vio_type, "U");

			vio_status.violation_box_info[n_violation].class_id = biker_info.biker_box_info[t].class_id;
			vio_status.violation_box_info[n_violation].bouding_box = biker_info.biker_box_info[t].bouding_box;
			vio_status.violation_box_info[n_violation].Um_bouding_box = param->sub_box_list[track_id].umbrella_box_info.bouding_box; //2018.10.12 hai

			vio_status.violation_box_info[n_violation].track_id = biker_info.biker_box_info[t].track_id; // 2018.10.23

		}
		//param->sub_box_list[track_id].nIsUmbrella = 0;

		// 2018.09.27 hai - mobile
		bool b_mobile_violation = false;

		//std::cout << "mobile_violation_confirm " << biker_info.mobile_violation_confirm[track_id] << std::endl;
		//std::cout << "biker_info.biker_box_info[track_id].mobile_box_info.class_id: " << param->sub_box_list[track_id].mobile_box_info.class_id << std::endl;

		//if (biker_info.biker_box_info[track_id].mobile_box_info.class_id == mobile_violation_class && biker_info.mobile_violation_confirm[track_id] < n_violation_confirm)
		if (nCheckBikeViolation && param->sub_box_list[track_id].mobile_box_info.class_id == mobile_violation_class && biker_info.mobile_violation_confirm[track_id] < n_violation_confirm
			&& param->sub_box_list[track_id].nIsMobile == 1 && !bIsTruckBike)
		{
			biker_info.mobile_violation_confirm[track_id]++;

			b_mobile_violation = true;
			b_violation = true;
			vio_status.violation_box_info[n_violation].mobile_violation = 1;
			sprintf(vio_status.violation_box_info[n_violation].vio_type, "M");

			vio_status.violation_box_info[n_violation].class_id = biker_info.biker_box_info[t].class_id;
			vio_status.violation_box_info[n_violation].bouding_box = biker_info.biker_box_info[t].bouding_box;
			vio_status.violation_box_info[n_violation].Mo_bouding_box = param->sub_box_list[track_id].mobile_box_info.bouding_box;

			vio_status.violation_box_info[n_violation].track_id = biker_info.biker_box_info[t].track_id; // 2018.10.23

		}
		//param->sub_box_list[track_id].nIsMobile = 0;

		if (biker_info.rider_violation_confirm[nTrackID] >= n_violation_confirm || biker_info.helmet_violation_confirm[nTrackID] >= n_violation_confirm
			|| biker_info.umbrella_violation_confirm[nTrackID] >= n_violation_confirm || biker_info.mobile_violation_confirm[nTrackID] >= n_violation_confirm)
		{
			biker_info.violation_confirm[nTrackID] = 1;
		}
		else
		{
			biker_info.violation_confirm[nTrackID] = 0;
		}

		if (b_violation == true)
		{
			param->mot_info->n_violated[track_id]++;

			biker_info.TO_violation[track_id] = 0;

			std::cout << n_violation_confirm << "; " << param->mot_info->n_violated[track_id] << std::endl;

			//if (param->mot_info->n_violated[track_id] >= n_violation_confirm) //200309 hai
			if (param->mot_info->n_violated[track_id] >= n_violation_confirm)// && biker_info.violation_confirm[nTrackID] == 1) //200309 hai
			{

				param->mot_info->n_violated[track_id] = 0;
				n_violation++;
			}
			//		if (b_violation == true) n_violation++;
		}

	}

	vio_status.n_violation = n_violation;

}




float DarknetData::rect_iou(cv::Rect a, cv::Rect b, int type)
{
	//overlap is invalid in the beginning
	float o = -1;

	float aPointX1 = (float)a.x;
	float aPointY1 = (float)a.y;
	float aPointX2 = (float)(a.x + a.width - 1);
	float aPointY2 = (float)(a.y + a.height - 1);

	float bPointX1 = (float)b.x;
	float bPointY1 = (float)b.y;
	float bPointX2 = (float)(b.x + b.width - 1);
	float bPointY2 = (float)(b.y + b.height - 1);

	//get overlapping area
	float x1 = MAX(aPointX1, bPointX1);
	float y1 = MAX(aPointY1, bPointY1);
	float x2 = MIN(aPointX2, bPointX2);
	float y2 = MIN(aPointY2, bPointY2);

	//compute width and height of overlapping area
	float w = x2 - x1;
	float h = y2 - y1;

	//set invalid entries to 0 overlap
	if (w <= 0 || h <= 0)
		return 0;

	//get overlapping areas
	float inter = w * h;
	float a_area = (aPointX2 - aPointX1) * (aPointY2 - aPointY1);
	float b_area = (bPointX2 - bPointX1) * (bPointY2 - bPointY1);

	//intersection over union overlap depending on users choice
	if (type == 0)
	{
		o = inter / (a_area + b_area - inter);
	}
	else
	{
		o = (a_area > b_area) ? inter / b_area : inter / a_area;
	}

	//overlap
	return o;
}



bool DarknetData::line_touch(cv::Rect box, int line)
{
	int y1 = box.y;
	int y2 = box.y + box.height - 1;
	//if ((line > y1) && (line < y2))	//kyk
	//if ((line > y2) && (line < y2 + 100))
	if ((line > y2) && (line < y2 + 150)) //for TensorRT
	{
		return true;
	}
	else
	{
		return false;
	}
}


void DarknetData::free_det_mot_param(st_det_mot_param *param)
{
	if (param)
	{
		//free(param->boxes);
		//free_ptrs((void **)param->probs, param->l.w*param->l.h*param->l.n);
		
		if (param->mot_info)
		{
			free(param->mot_info->object_flow_number);
			//free(param->mot_info->object_flow_number_Daewoo); //20190423 hai Daewoo
			free(param->mot_info->object_flow_total); //20190214 hai Status Log
			free(param->mot_info->object_flow_number_forTopline);//yk 2018.12.03

			free(param->mot_info->object_flow_number_out);
			free(param->mot_info->object_flow_total_out);

			free(param->mot_info);

			param->mot_info = NULL;
		}

		free(param->class_group_id);
		free(param->vio_head_class);

		//if (param->names)
		//	free_ptrs((void**)param->names, param->net.layers[param->net.n - 1].classes);
		//free_network(param->net);	//20190715 young
		param = NULL;
	}
}



//int DarknetData::make_det_mot_param(st_det_mot_param *param, int nCamIndex)
//{
//	//write_log("make_det_mot_param start", nCamIndex);
//	char *datacfg = param->data_full_name;
//	char *cfgfile = param->cfg_full_name;
//	char *weightfile = param->weight_full_name;
//	param->nms = BOX_IOU_THRESH;
//
//	//tensorRT
//	param->config_TRT.gpu_id = param->gpu_id;
//	param->config_TRT.file_model_cfg = param->cfg_full_name;
//	param->config_TRT.file_model_weights = param->weight_full_name;
//	param->config_TRT.calibration_image_list_file_txt = "/data/calibration_images.txt";
//	param->config_TRT.inference_precison = Precision(param->nPrecison); //TENSOR_INT8;
//	param->config_TRT.detect_thresh = param->thresh;
//	param->config_TRT.nms_thresh = param->nms;
//	param->config_TRT.net_type = ModelType(param->nNetType);
//	param->detector_TRT.init(param->config_TRT);
//	
//	param->mot_info = (st_mot_info*)calloc(1, sizeof(st_mot_info));
//	param->mot_info->object_flow_number = (int*)calloc(param->nLclasses, sizeof(int));
//	param->mot_info->object_flow_total = (int*)calloc(param->nLclasses, sizeof(int)); 
//	param->mot_info->object_flow_number_forTopline = (int*)calloc(param->nLclasses, sizeof(int));//yk 2018.12.03
//	param->class_group_id = (int*)calloc(param->nLclasses, sizeof(int));
//	param->vio_head_class = (int*)calloc(param->n_vio_head_class, sizeof(int));
//
//	param->mot_info->object_flow_number_out = (int*)calloc(param->nLclasses, sizeof(int)); //20191211 skkang stats as direction
//	param->mot_info->object_flow_total_out = (int*)calloc(param->nLclasses, sizeof(int)); //20191211 skkang stats as direction
//
//	for (int c = 0; c < MAX_TRACK; c++) {
//		param->track_color[c] = cv::Scalar(rand() % 256, rand() % 256, rand() % 256, rand() % 256);
//	}
//
//	//20190222 hai Class color
//	for (int c = 0; c <= cls_total; c++)
//	{
//		if (SHOW_AS_SIMPLE)
//		{
//			if (c == 7 || c == 8 || c == 9)
//				param->class_color[c] = cv::Scalar(0, 128, 32, 0);
//			else
//				param->class_color[c] = cv::Scalar(88, 181, 53, 0);//102, 0, 0, 0);// 초록
//		}
//		else
//			param->class_color[c] = cv::Scalar(rand() % 256, rand() % 256, rand() % 256, rand() % 256);
//
//	}
//
//	//20190222 hai Violation Color B-G-R
//	if (SHOW_AS_SIMPLE)
//	{
//		param->vio_color[BIKE_VIOLATION_CLR] = cv::Scalar(36, 32, 237, 0);
//		param->vio_color[SIGNAL_VIOLATION_CLR] = cv::Scalar(36, 32, 237, 0);
//
//		param->vio_color[OVER_SPEED_CLR] = cv::Scalar(36, 32, 237, 0);
//		param->vio_color[LOW_SPEED_CLR] = cv::Scalar(36, 32, 237, 0);
//		param->vio_color[LANE_VIOLATION_CLR] = cv::Scalar(36, 32, 237, 0);
//		param->vio_color[ILLEGAL_PARKING_CLR] = cv::Scalar(32, 127, 237, 0);
//
//		param->vio_color[ACCIDENT_CLR] = cv::Scalar(36, 32, 237, 0);
//		param->vio_color[REVERSE_VIOLATION_CLR] = cv::Scalar(36, 32, 237, 0);
//
//		param->vio_color[TRACKING_CLR] = cv::Scalar(238, 238, 0, 0);
//	}
//	else
//	{
//		param->vio_color[BIKE_VIOLATION_CLR] = cv::Scalar(255, 0, 0, 0);
//		param->vio_color[SIGNAL_VIOLATION_CLR] = cv::Scalar(0, 255, 0, 0);
//
//		param->vio_color[OVER_SPEED_CLR] = cv::Scalar(128, 0, 255, 0);
//		param->vio_color[LOW_SPEED_CLR] = cv::Scalar(0, 128, 255, 0);
//		param->vio_color[LANE_VIOLATION_CLR] = cv::Scalar(255, 0, 128, 255);
//		param->vio_color[ILLEGAL_PARKING_CLR] = cv::Scalar(255, 128, 0, 0);
//
//		param->vio_color[ACCIDENT_CLR] = cv::Scalar(255, 255, 128, 0);
//		param->vio_color[REVERSE_VIOLATION_CLR] = cv::Scalar(0, 0, 128, 0);
//
//		param->vio_color[TRACKING_CLR] = cv::Scalar(238, 238, 0, 0);
//	}
//
//	//param->nms = NMS_IOU;
//	//param->nms = BOX_IOU_THRESH;
//	//	param->thresh = DETECT_THRESH;
//
//	//	param->fp_count = fopen("inin_count.log", "w");
//	//	sprintf(param->fp_count_end_name, "inin_count.end");
//
//	return 1;
//}


//2018.10.12 hai
int DarknetData::find_num_rider_histogram(Mat& frame, int nTrackID, st_sub_box_info *sub_box_list)
{
	sub_box_list[nTrackID].n_his_rider = -1;
	sub_box_list[nTrackID].n_his_rider_index = -1; //19113

	int max_hisvalue = sub_box_list[nTrackID].n_rider_histogram[1];
	//if (max_hisvalue > 0) sub_box_list[nTrackID].n_his_rider = 1;

	for (int m = 2; m < MAX_SUB_BOX; m++)
	{
		if (sub_box_list[nTrackID].n_rider_histogram[m] > max_hisvalue)
		{
			max_hisvalue = sub_box_list[nTrackID].n_rider_histogram[m];
			sub_box_list[nTrackID].n_his_rider = m;
		}
	}

	float max_score = 0;

	if (sub_box_list[nTrackID].n_his_rider > 0)
	{
		for (int i = 0; i < MAX_FRAME_NUM; i++)
		{
			if (sub_box_list[nTrackID].n_box_list[i] == sub_box_list[nTrackID].n_his_rider && sub_box_list[nTrackID].mean_score[i] > max_score)
			{
				sub_box_list[nTrackID].n_his_rider_index = i;
				max_score = sub_box_list[nTrackID].mean_score[i];
			}
		}
	}

	if (sub_box_list[nTrackID].n_his_rider == -1 || sub_box_list[nTrackID].n_his_rider_index == -1) //19113
	{
		sub_box_list[nTrackID].n_his_rider = sub_box_list[nTrackID].n_box_list[sub_box_list[nTrackID].n_box_idx];
		sub_box_list[nTrackID].n_his_rider_index = sub_box_list[nTrackID].n_box_idx;
	}

	return sub_box_list[nTrackID].n_his_rider;
}



void DarknetData::biker_violation_draw(Mat& image, st_violation_info &vio_status)
{
	//	printf("vio_status.n_violation = %d\n", vio_status.n_violation);
	for (int v = 0; v < vio_status.n_violation; v++)
	{
		//		printf("%d)type = (%d %d)\n", v, vio_status.violation_box_info[v].max_rider_violation, vio_status.violation_box_info[v].helmet_violation);
		//		printf("box = (%d %d %d %d)\n", v, vio_status.violation_box_info[v].bouding_box.x, vio_status.violation_box_info[v].bouding_box.y,
		//			vio_status.violation_box_info[v].bouding_box.width, vio_status.violation_box_info[v].bouding_box.height);

		//rectangle(image, vio_status.violation_box_info[v].bouding_box, cv::Scalar(0, 0, 255), 4, 1, 0);
		//convert_2D_to_3D_Box(image, vio_status.violation_box_info[v].bouding_box, cv::Scalar(0, 0, 255));
	}

}



void DarknetData::biker_violation_save_log(FILE *fp, st_biker_info &biker_info, int nVioIdx, st_violation_info &vio_status, int ch_id, char *time, int nMaxRider)
{
	char textTmp[CHAR_LENGTH] = "";
	char chid_time_Tmp[CHAR_LENGTH] = "";
	char saveTmp[CHAR_LENGTH] = "";
	sprintf(chid_time_Tmp, "%02d,%s", ch_id, time);

	int i = nVioIdx;

	//for (int i = 0; i < vio_status.n_violation; i++)
	{
		int class_id = vio_status.violation_box_info[i].class_id;
		char *vio_type = vio_status.violation_box_info[i].vio_type;
		int n_person = vio_status.violation_box_info[i].n_person;
		int n_person_helmet = vio_status.violation_box_info[i].n_person_helmet;

		int nTrackID = vio_status.violation_box_info[i].track_id;

		int n_person_over = n_person - nMaxRider + 1;
		if (n_person_over < 0) n_person_over = 0;

		if (n_person_helmet >= 0 && n_person >= 1)
		{
			if (vio_status.violation_box_info[i].max_rider_violation && n_person_over >= 1) //19113
			{
				sprintf(textTmp, "%s%s,%02d,%s,%d,%d,%d,%d\n", textTmp, chid_time_Tmp, class_id, "O", n_person, n_person_helmet, n_person_over, nTrackID);
			}

			if (vio_status.violation_box_info[i].helmet_violation)
			{
				sprintf(textTmp, "%s%s,%02d,%s,%d,%d,%d,%d\n", textTmp, chid_time_Tmp, class_id, "H", n_person, n_person_helmet, n_person_over, nTrackID);
			}
		}

		if (vio_status.violation_box_info[i].umbrella_violation)
		{
			if (n_person == 0)
			{
				n_person = 1;
				n_person_helmet = 1;
			}
			sprintf(textTmp, "%s%s,%02d,%s,%d,%d,%d,%d\n", textTmp, chid_time_Tmp, class_id, "U", n_person, n_person_helmet, n_person_over, nTrackID);
		}
		if (vio_status.violation_box_info[i].mobile_violation)
		{
			if (n_person == 0)
			{
				n_person = 1;
				n_person_helmet = 1;
			}
			sprintf(textTmp, "%s%s,%02d,%s,%d,%d,%d,%d\n", textTmp, chid_time_Tmp, class_id, "M", n_person, n_person_helmet, n_person_over, nTrackID);
		}
	}
	if (fp)
	{
		fprintf(fp, "%s\n", textTmp);
	}

}


void DarknetData::violation_box_draw_save_image(st_det_mot_param *param, Mat& ori_image, Mat& res_image, int nVioIdx, st_violation_info &vio_status)
{
	//	printf("vio_status.n_violation = %d\n", vio_status.n_violation);
	//float x_scale = (float)ori_image.cols / res_image.cols;
	//float y_scale = (float)ori_image.rows / res_image.rows;
	cv::Rect box;
	int v = nVioIdx;

	// 2018.10.26 hai
	int nTrackID = vio_status.violation_box_info[v].track_id;
	int nHisIdx = param->sub_box_list[nTrackID].n_his_rider_index;

	std::cout << "n_head_box: " << param->sub_box_list[nTrackID].n_box_list[nHisIdx] << std::endl;

	if (!param->sub_box_list[nTrackID].vio_image[nHisIdx].empty())
	{
		for (int n = 0; n < param->sub_box_list[nTrackID].n_box_list[nHisIdx]; n++)
		{
			cv::Rect RectHead = param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].bouding_box;
			/*RectHead.x = (int)(param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].bouding_box.x * x_scale);
			RectHead.y = (int)(param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].bouding_box.y * y_scale);
			RectHead.width = (int)(param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].bouding_box.width * x_scale);
			RectHead.height = (int)(param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].bouding_box.height * y_scale);*/

			rectangle(param->sub_box_list[nTrackID].vio_image[nHisIdx], RectHead, cv::Scalar(0, 0, 255, 0), 2, 1, 0);

			char textTmp[CHAR_LENGTH];
			sprintf(textTmp, "[%d]", param->sub_box_list[nTrackID].head_box_list[nHisIdx].head_box_info[n].class_id);
			//CvFont font;
			//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.5, 0.5, 0, 0, 1);
			//putText(param->sub_box_list[nTrackID].vio_image[nHisIdx], textTmp, cv::Point((int)RectHead.x, (int)RectHead.y), cv::CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.5, cv::Scalar(0, 0, 255, 0), 0, 1);
			putText(param->sub_box_list[nTrackID].vio_image[nHisIdx], textTmp, cv::Point((int)RectHead.x, (int)RectHead.y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, cv::Scalar(0, 0, 255, 0), 0, 1);
		}
	}

	//for (int v = 0; v < vio_status.n_violation; v++)
	{
		box = vio_status.violation_box_info[v].bouding_box;
		/*box.x = (int)(vio_status.violation_box_info[v].bouding_box.x * x_scale);
		box.y = (int)(vio_status.violation_box_info[v].bouding_box.y * y_scale);
		box.width = (int)(vio_status.violation_box_info[v].bouding_box.width * x_scale);
		box.height = (int)(vio_status.violation_box_info[v].bouding_box.height * y_scale);*/

		//rectangle(ori_image, box, cv::Scalar(0, 0, 255), 1, 1, 0);
		draw_objectbox_forLPR(ori_image, box, cv::Scalar(0, 0, 255), false);
		//convert_2D_to_3D_Box(ori_image, box, cv::Scalar(0, 0, 255));

		Rect RectUMHead;
		if (vio_status.violation_box_info[v].umbrella_violation == 1)
		{
			RectUMHead = vio_status.violation_box_info[v].Um_bouding_box;
			/*RectUMHead.x = (int)(vio_status.violation_box_info[v].Um_bouding_box.x * x_scale);
			RectUMHead.y = (int)(vio_status.violation_box_info[v].Um_bouding_box.y * y_scale);
			RectUMHead.width = (int)(vio_status.violation_box_info[v].Um_bouding_box.width * x_scale);
			RectUMHead.height = (int)(vio_status.violation_box_info[v].Um_bouding_box.height * y_scale);*/

			//std::cout << "============= rectHead.x: " << RectUMHead.x << "============= rectHead.y: " << RectUMHead.y << std::endl;

			rectangle(ori_image, RectUMHead, Scalar(0, 0, 255), 2, 1, 0);
		}
		if (vio_status.violation_box_info[v].mobile_violation == 1)
		{
			RectUMHead = vio_status.violation_box_info[v].Mo_bouding_box;
			/*RectUMHead.x = (int)(vio_status.violation_box_info[v].Mo_bouding_box.x * x_scale);
			RectUMHead.y = (int)(vio_status.violation_box_info[v].Mo_bouding_box.y * y_scale);
			RectUMHead.width = (int)(vio_status.violation_box_info[v].Mo_bouding_box.width * x_scale);
			RectUMHead.height = (int)(vio_status.violation_box_info[v].Mo_bouding_box.height * y_scale);*/

			//std::cout << "============= rectHead.x: " << RectUMHead.x << "============= rectHead.y: " << RectUMHead.y << std::endl;

			rectangle(ori_image, RectUMHead, Scalar(0, 0, 255), 2, 1, 0);
		}
	}

}

//20191221 object box for lpr
void DarknetData::draw_objectbox_forLPR(Mat& SaveImage, cv::Rect rectArea, cv::Scalar VioColor, bool bEdit)
{
	cv::Rect rectArea_tmp = rectArea;

	if (bEdit)
	{
		int nPad = LPR_Y_PAD;

		if (rectArea_tmp.y + rectArea_tmp.height + nPad <= SaveImage.rows - 1)
		{
			rectArea_tmp.height = rectArea_tmp.height + nPad;
		}
	}

	cv::Point pTopL, pTopLRight, pTopLBot, pBotR, pBotRTop, pBotRLeft;
	cv::Point pTopR, pTopRLeft, pTopRBot, pBotL, pBotLTop, pBotLRight;

	pTopL.x = rectArea_tmp.x;
	pTopL.y = rectArea_tmp.y;

	pTopLRight.x = pTopL.x + rectArea_tmp.width / 8;
	pTopLRight.y = pTopL.y;

	pTopLBot.x = pTopL.x;
	pTopLBot.y = pTopL.y + rectArea_tmp.height / 8;

	pBotR.x = rectArea_tmp.x + rectArea_tmp.width - 1;
	pBotR.y = rectArea_tmp.y + rectArea_tmp.height - 1;

	pBotRTop.x = pBotR.x;
	pBotRTop.y = pBotR.y - rectArea_tmp.height / 8;

	pBotRLeft.x = pBotR.x - rectArea_tmp.width / 8;
	pBotRLeft.y = pBotR.y;

	////

	pTopR.x = rectArea_tmp.x + rectArea_tmp.width - 1;
	pTopR.y = rectArea_tmp.y;

	pTopRLeft.x = pTopR.x - rectArea_tmp.width / 8;
	pTopRLeft.y = pTopR.y;

	pTopRBot.x = pTopR.x;
	pTopRBot.y = pTopR.y + rectArea_tmp.height / 8;

	pBotL.x = rectArea_tmp.x;
	pBotL.y = rectArea_tmp.y + rectArea_tmp.height - 1;

	pBotLTop.x = pBotL.x;
	pBotLTop.y = pBotL.y - rectArea_tmp.height / 8;

	pBotLRight.x = pBotL.x + rectArea_tmp.width / 8;
	pBotLRight.y = pBotL.y;


	line(SaveImage, pTopL, pTopLRight, VioColor, 2, 1, 0);
	line(SaveImage, pTopL, pTopLBot, VioColor, 2, 1, 0);

	line(SaveImage, pTopR, pTopRLeft, VioColor, 2, 1, 0);
	line(SaveImage, pTopR, pTopRBot, VioColor, 2, 1, 0);

	line(SaveImage, pBotL, pBotLRight, VioColor, 2, 1, 0);
	line(SaveImage, pBotL, pBotLTop, VioColor, 2, 1, 0);

	line(SaveImage, pBotR, pBotRLeft, VioColor, 2, 1, 0);
	line(SaveImage, pBotR, pBotRTop, VioColor, 2, 1, 0);
}

void DarknetData::release_lane_violation(st_det_mot_param *param, int nTrackID)
{
	/*param->mot_LaneSave[nTrackID].nLaneViolation = 0;
	param->mot_LaneSave[nTrackID].nLaneVioCnt = 0;

	cvReleaseImage(&param->mot_LaneSave[nTrackID].pIplImage);
	param->mot_LaneSave[nTrackID].pIplImage = NULL;

	cvReleaseImage(&param->mot_LaneSave[nTrackID].pLPImage);
	param->mot_LaneSave[nTrackID].pLPImage = NULL;

	param->mot_LaneSave[nTrackID].nLPR = 0;

	param->mot_LaneSave[nTrackID].nTouchBot = 0;
	param->mot_LaneSave[nTrackID].nTouchTop = 0;

	param->mot_LaneSave[nTrackID].nStartStatus = -1;
	param->mot_LaneSave[nTrackID].nEndStatus = -1;*/


	if (!param->mot_LaneSave[nTrackID].pIplImage.empty())
	{
		param->mot_LaneSave[nTrackID].pIplImage.release();
	}

	if (!param->mot_LaneSave[nTrackID].pLPImage.empty())
	{
		param->mot_LaneSave[nTrackID].pLPImage.release();
	}

	memset(&param->mot_LaneSave[nTrackID], 0, sizeof(st_lane_DataSave));

	param->mot_LaneSave[nTrackID].nStartStatus = -1;
	param->mot_LaneSave[nTrackID].nEndStatus = -1;
	param->mot_LaneSave[nTrackID].nLaneViolation = 0;
	param->mot_LaneSave[nTrackID].nLPR = 0;
	param->mot_LaneSave[nTrackID].nLaneVioCnt = 0;
	param->mot_LaneSave[nTrackID].pIplImage = NULL;
	param->mot_LaneSave[nTrackID].pLPImage = NULL;
	param->mot_LaneSave[nTrackID].nTouchBot = 0;
	param->mot_LaneSave[nTrackID].nTouchTop = 0;

	param->mot_LaneSave[nTrackID].nFirstClass = -1;

	param->mot_DataSave[nTrackID].nVioDisplay[LANE_VIOLATION_CLR] = 0; //2019022
}


void DarknetData::release_reverse_violation(st_det_mot_param *param, int nTrackID)
{
#if ITS_MODE
	param->mot_ReverseSave[nTrackID].bStoreImage = 0;
	param->mot_ReverseSave[nTrackID].nReverseResetCnt = 0;
#endif

	if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
	{
		param->mot_ReverseSave[nTrackID].pIplImage.release();
	}

	if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
	{
		param->mot_ReverseSave[nTrackID].pLPImage.release();
	}

	memset(&param->mot_ReverseSave[nTrackID], 0, sizeof(st_reverse_DataSave));
	param->mot_ReverseSave[nTrackID].nReverseViolation = 0;
	param->mot_ReverseSave[nTrackID].nLPR = 0;
	param->mot_ReverseSave[nTrackID].nReverseVioCnt = 0;
	param->mot_ReverseSave[nTrackID].pIplImage = NULL;
	param->mot_ReverseSave[nTrackID].pLPImage = NULL;
	param->mot_ReverseSave[nTrackID].nTouchBot = 0;
	param->mot_ReverseSave[nTrackID].nTouchTop = 0;

	param->mot_ReverseSave[nTrackID].nFirstClass = -1;

	param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 0; //2019022
}


void DarknetData::release_stop_violation(st_det_mot_param *param, int nTrackID)
{
#if ITS_MODE
	param->mot_DataSave[nTrackID].bStoreImage = 0;
	param->mot_DataSave[nTrackID].nStopResetCnt = 0;
	param->mot_DataSave[nTrackID].nStopVioCnt = 0;
	param->mot_DataSave[nTrackID].nStopZeroCnt = 0;
	param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR] = 0; 
	param->mot_DataSave[nTrackID].stopTime = 0;

#endif
	param->mot_DataSave[nTrackID].bCheckAccident = 0;
	param->mot_DataSave[nTrackID].bSaveAccident = 0;
	param->mot_DataSave[nTrackID].parkingTime = 0;
	if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
	{
		param->mot_DataSave[nTrackID].pIplImageParkingStart.release();
	}
	param->mot_DataSave[nTrackID].nVioDisplay[ACCIDENT_CLR] = 0; //2019022
}



bool DarknetData::line_touch_forcounting(cv::Rect box, int line)
{
	int y1 = box.y;
	int y2 = box.y + box.height - 1;
	//if ((line > y1) && (line < y2))
	if ((line > (y2 - 50)) && (line < (y2 + 50))) // 2018.12.13
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool DarknetData::Radar_line_touch_Distance(int nPtY, int line)
{
	if (nPtY > line - 50 && nPtY < line + 50)
	{
		return true;
	}
	else 
	{
		return false;
	}
}

bool DarknetData::is_bike(int nClass)
{
#if ENGINE_MODE == 0
	if ((nClass >= cls_bike_front && nClass <= cls_bike_side) || (nClass >= cls_truckbike_front && nClass <= cls_truckbike_side))
#elif ENGINE_MODE == 1
	if (nClass >= cls_bike)
#endif
	{
		return true;
	}

	return false;
}

bool DarknetData::is_car(int nClass)
{
	if (nClass >= cls_car && nClass <= cls_truck)
	{
		return true;
	}

	return false;
}

bool DarknetData::is_under_consideration_vehicle(int nClass)
{
	if (is_bike(nClass) || is_car(nClass))
	{
		return true;
	}

	return false;
}



char* DarknetData::getStringClassID(int nClassID)
{
#if ENGINE_MODE == 1
	switch (nClassID)
	{
	case cls_car:
		return "car";
		break;
	case cls_ban:
		return "suv"; 
		break;
	case cls_bus:
		return "bus";
		break;
	case cls_truck:
		return "Truck";
		break;
	case cls_bike:
		return "bike";
		break;
	case cls_human:
		return "human";
		break;
	case cls_bicycle:
		return "bicycle";
		break;
	case cls_kickboard:
		return "kickboard";
		break;

	default:
		break;

	}
#else
	switch (nClassID)
	{

#if KAI_POC_MODE
	case ITEM_CLASS_ID:
		return "item";
		break;
	case HAND_CLASS_ID:
		return "hand";
		break;
	case PERSON_CLASS_ID:
		return "human";
		break;
#else
	case cls_car:
		if (COUNT_MODE_COUNTRY == 0) return "car";//"Car";//20191015 for recording with img_count
		else if (COUNT_MODE_COUNTRY == 1) return "Sedan"; //20190213 hai Car->Sedan
		else if (COUNT_MODE_COUNTRY == 3) return "A";
		else if (COUNT_MODE_COUNTRY == 4) return "A";
		break;
	case cls_ban:
		if (COUNT_MODE_COUNTRY == 3) return "B";
		if (COUNT_MODE_COUNTRY == 4) return "B";
		return "Suv"; //20190214
		break;
	case cls_bus:
		if (COUNT_MODE_COUNTRY == 3) return "B";
		if (COUNT_MODE_COUNTRY == 4) return "B";
		return "Bus";
		break;
	case cls_truck:
		if (COUNT_MODE_COUNTRY == 3) return "Human";
		if (COUNT_MODE_COUNTRY == 4) return "Human";
		return "Truck";
		break;
#if ITS_MODE == 1
	case PEDESTRIAN_CLASS_ID:
		return "human";
		break;
#else
	case cls_bike_front:
	case cls_bike_rear:
	case cls_bike_side:
		return "Bike";
		break;
	case cls_human:
		return "Human";
		break;
	case cls_bicycle:
		return "Bicycle";
		break;
	case cls_truckbike_front:
	case cls_truckbike_rear:
	case cls_truckbike_side:
		if (COUNT_MODE_COUNTRY == 0) return "TruckBike";
		else if (COUNT_MODE_COUNTRY == 1) return "Bajaj"; //20190214 hai TruckBike->Bajaj
														  //else if (COUNT_MODE_COUNTRY == 1) return "Bagac"; //20190214 hai TruckBike->Bajaj
		break;
#endif
#endif
	default:
		break;

	}
#endif
	return "";
}


void DarknetData::object_flow_save(FILE *fp, st_mot_info *mot_info, int num, int ch_id, char *time)
{
	char textTmp[CHAR_LENGTH] = "";
	char chid_time_Tmp[CHAR_LENGTH] = "";
	char saveTmp[CHAR_LENGTH] = "";
	sprintf(chid_time_Tmp, "%02d,%s", ch_id, time);
#ifndef XYCOOR_MODE
	//this line makes errors in free_det_mot_param()
	num = cls_umbrella;	//20180814 Do not use classId 15, 16, 17 for statistics.
#endif

	for (int c_id = 0; c_id < num - BACKGROUND_CLASS; c_id++)
	{

		//printf("(%02d : %d) ", c_id, mot_info->object_flow_number[c_id]);		                     		
		//sprintf(textTmp, "%s,%02d,%d", textTmp, c_id, mot_info->object_flow_number[c_id]);//20190214 hai Status Log 

		//20190214 hai Status Log
		mot_info->object_flow_total[c_id] += mot_info->object_flow_number[c_id];
		mot_info->object_flow_total_out[c_id] += mot_info->object_flow_number_out[c_id];

		//20180719 Request Yun, edited kyk, count for 1 second 
		mot_info->object_flow_number[c_id] = 0;
		mot_info->object_flow_number_out[c_id] = 0;
		//mot_info->object_flow_number_forTopline[c_id] = 0;//yk 2018.12.03
	}
	if (fp)
	{
		//fprintf(fp, "%s%s\n", chid_time_Tmp, textTmp); //20190214 hai Status Log
	}
}


//20190222 hai Violation Color
void DarknetData::violation_text_display(Mat& image, st_det_mot_param *param, Rect bounding_box, int nTrackID)
{
	char textTmp[CHAR_LENGTH] = "";
	//sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "");
	sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "");
	int nViolation = 0;
	for (int c_id = 0; c_id < PEDESTRIAN_VIO_CLR + 1; c_id++) //20190714
	{
		if (c_id == BIKE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[BIKE_VIOLATION_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sBikeVio", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sBIKEVIO ", textTmp);
			nViolation++;
		}
		else if (c_id == SIGNAL_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[SIGNAL_VIOLATION_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sSignal", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sSIGNAL ", textTmp);
			nViolation++;
		}
		else if (c_id == OVER_SPEED_CLR && param->mot_DataSave[nTrackID].nVioDisplay[OVER_SPEED_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sSpeed", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sSPEED ", textTmp);
			nViolation++;
		}
		else if (c_id == LOW_SPEED_CLR && param->mot_DataSave[nTrackID].nVioDisplay[LOW_SPEED_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sSpeed", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sSPEED ", textTmp);
			nViolation++;
		}
		else if (c_id == LANE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[LANE_VIOLATION_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sLane", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sLANE ", textTmp);
			nViolation++;
		}
#if ITS_MODE == 1
		else if (c_id == ILLEGAL_PARKING_CLR && param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sS", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sS ", textTmp);
			nViolation++;
		}
		//20190523 hai reverse violation
		else if (c_id == REVERSE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sR", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sR ", textTmp);
			nViolation++;
		}
		else if (c_id == PEDESTRIAN_VIO_CLR && param->mot_DataSave[nTrackID].nVioDisplay[PEDESTRIAN_VIO_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sP", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sP ", textTmp);
			nViolation++;
		}
#else
		else if (c_id == ILLEGAL_PARKING_CLR && param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sParking", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sPARKING ", textTmp);
			nViolation++;
		}
		//20190523 hai reverse violation
		else if (c_id == REVERSE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR])
		{
			sprintf(param->mot_DataSave[nTrackID].textViolationInfo, "%sReverse", param->mot_DataSave[nTrackID].textViolationInfo);
			sprintf(textTmp, "%sREVERSE ", textTmp);
			nViolation++;
		}
#endif

		//if (c_id == OVER_SPEED_CLR && param->mot_DataSave[nTrackID].nVioDisplay[OVER_SPEED_CLR])
		//{
		//	sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "%sSpeed", param->mot_info->mot_box_info[ntrack].textViolationInfo);
		//	sprintf(textTmp, "%sSPEED ", textTmp);
		//	nViolation++;
		//}
		//else if (c_id == LOW_SPEED_CLR && param->mot_DataSave[nTrackID].nVioDisplay[LOW_SPEED_CLR])
		//{
		//	sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "%sSpeed", param->mot_info->mot_box_info[ntrack].textViolationInfo);
		//	sprintf(textTmp, "%sSPEED ", textTmp);
		//	nViolation++;
		//}
		//else if (c_id == LANE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[LANE_VIOLATION_CLR])
		//{
		//	sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "%sLane", param->mot_info->mot_box_info[ntrack].textViolationInfo);
		//	sprintf(textTmp, "%sLANE ", textTmp);
		//	nViolation++;
		//}
		//else if (c_id == ILLEGAL_PARKING_CLR && param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR])
		//{
		//	sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "%sParking", param->mot_info->mot_box_info[ntrack].textViolationInfo);
		//	sprintf(textTmp, "%sPARKING ", textTmp);
		//	nViolation++;
		//}
		////20190523 hai reverse violation
		//else if (c_id == REVERSE_VIOLATION_CLR && param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR])
		//{
		//	sprintf(param->mot_info->mot_box_info[ntrack].textViolationInfo, "%sReverse", param->mot_info->mot_box_info[ntrack].textViolationInfo);
		//	sprintf(textTmp, "%sREVERSE ", textTmp);
		//	nViolation++;
		//}

		if (SHOW_AS_SIMPLE)
		{

		}
		else
		{
			int x = bounding_box.x;
			int y = bounding_box.y;

			if (nViolation == 1)
			{
				if (param->mot_DataSave[nTrackID].nVioDisplay[OVER_SPEED_CLR]) 
					putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, param->vio_color[OVER_SPEED_CLR], 2, 1);
				if (param->mot_DataSave[nTrackID].nVioDisplay[LOW_SPEED_CLR]) 
					putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, param->vio_color[LOW_SPEED_CLR], 2, 1);
				if (param->mot_DataSave[nTrackID].nVioDisplay[LANE_VIOLATION_CLR]) 
					putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, param->vio_color[LANE_VIOLATION_CLR], 2, 1);
				if (param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR]) 
					putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, param->vio_color[ILLEGAL_PARKING_CLR], 2, 1);
				//20190523 hai reverse violation
				if (param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR]) 
					putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1);
			}
			else
				putText(image, textTmp, cv::Point(x, y), cv::FONT_HERSHEY_SIMPLEX | cv::FONT_ITALIC, 0.5, cv::Scalar(0, 255, 255, 0), 2, 1);
		}
	}
}


//void DarknetData::run_det(Mat frame, st_det_mot_param *param)
//{
//	vector<Result> res_detect;
//	cv::Mat frame_process = cv::cvarrToMat(frame);
//
//	cudaSetDevice(param->gpu_id);
//
//	param->detector_TRT.detect(frame_process, res_detect);
//
//	int n_box_cnt = 0;
//	int *group_class = param->class_group_id;
//
//	for (int i = 0; i < res_detect.size(); i++)
//	{
//		int g_c1 = group_class[res_detect[i].id];
//
//		param->mot_info->det_box_info[n_box_cnt].bouding_box.x = res_detect[i].rect.x;
//		param->mot_info->det_box_info[n_box_cnt].bouding_box.y = res_detect[i].rect.y;
//		param->mot_info->det_box_info[n_box_cnt].bouding_box.width = res_detect[i].rect.width;
//		param->mot_info->det_box_info[n_box_cnt].bouding_box.height = res_detect[i].rect.height;
//		param->mot_info->det_box_info[n_box_cnt].class_group_id = g_c1;
//		param->mot_info->det_box_info[n_box_cnt].class_id = res_detect[i].id;
//		param->mot_info->det_box_info[n_box_cnt].score = res_detect[i].prob;
//
//		n_box_cnt++;
//	}
//	param->mot_info->n_detect_num = n_box_cnt;
//}
//
//
//void DarknetData::run_mot(int nCameraIndex, Mat frame, st_det_mot_param *param, st_biker_info *biker_info, float fXratio, float fYratio)//hai 2018.12.12
//{
//	//param->mot_info->n_track_num = tracking_function(frame, param->l.w*param->l.h*param->l.n, param->boxes, param->probs, param->l.classes, param->mot_info, param->sub_box_list);
//	param->mot_info->n_track_num = tracking_function(nCameraIndex, frame, param->mot_info, param->sub_box_list, biker_info, param->mot_DataSave, param, fXratio, fYratio); //hai 2019.01.14
//}
//
//
//void DarknetData::run_det_mot(int nCameraIndex, Mat frame, Mat res_frame, st_det_mot_param *param, char *strTitle, st_biker_info *biker_info, float fXratio, float fYratio)//hai 2018.12.12
//{
//	float detection_time = 0, tracking_time = 0, total_time = 0;;
//	clock_t detection_time_c, tracking_time_c, total_time_c;
//	total_time_c = clock();
//
//	//hai 2018.11.30
//	Mat disp_image = cvCreateImage(cvGetSize(frame), frame->depth, frame->nChannels);
//	cvCopy(frame, disp_image, 0);
//	//cvCvtColor(frame, frame, CV_RGB2BGR);
//
//
//	//image im = make_image(frame.cols, frame.rows, 3);
//	//ipl_into_image(frame, im);
//
//	//cvCopy(disp_image, frame, 0);	//20190406 copy back
//
//	//==================================
//
//	//float *X = im.data;
//
//	//detection_time_c = clock();
//	run_det(frame, param);
//
//	//detection_time = sec(clock() - detection_time_c);
//
//	if (param->show_det_on)
//	{
//		for (int d = 0; d < param->mot_info->n_detect_num; d++)
//		{
//			if (param->mot_info->det_box_info[d].class_id == cls_helmet || param->mot_info->det_box_info[d].class_id == cls_hat || param->mot_info->det_box_info[d].class_id == cls_head)
//				rectangle(disp_image, param->mot_info->det_box_info[d].bouding_box, param->track_color[param->mot_info->det_box_info[d].class_id], 2, 1, 0);
//			//cvShowImage("[Test]", disp_image);
//		}
//	}
//
//	//tracking_time_c = clock();
//	run_mot(nCameraIndex, res_frame, param, biker_info, fXratio, fYratio);
//	//tracking_time = sec(clock() - tracking_time_c);
//
//	// 2018.10... hai
//	for (int d = 0; d < param->mot_info->n_detect_num; d++)
//	{
//		cv::Rect rtResBox;
//		rtResBox.x = (int)((float)param->mot_info->det_box_info[d].bouding_box.x / fXratio);
//		rtResBox.width = (int)((float)param->mot_info->det_box_info[d].bouding_box.width / fXratio);
//		rtResBox.y = (int)((float)param->mot_info->det_box_info[d].bouding_box.y / fYratio);
//		rtResBox.height = (int)((float)param->mot_info->det_box_info[d].bouding_box.height / fYratio);
//
//		if (rtResBox.width < LIMITED_WIDTH && rtResBox.height < LIMITED_HEIGHT)//indonesia demo
//		{
//			if (COUNT_MODE_COUNTRY == 2)
//			{
//				if (param->mot_info->det_box_info[d].class_id == 7 || param->mot_info->det_box_info[d].class_id == 8 || param->mot_info->det_box_info[d].class_id == 9 || param->mot_info->det_box_info[d].class_id == 10 || param->mot_info->det_box_info[d].class_id == 11)
//				{
//					//rectangle(res_frame, rtResBox, param->track_color[param->mot_info->det_box_info[d].class_id], 2, 1, 0);
//					//rectangle(res_frame, rtResBox, param->class_color[param->mot_info->det_box_info[d].class_id], 1, 1, 0); //20190222 hai Class color  //20190327
//				}
//			}
//			else
//			{
//				if (COUNT_MODE_COUNTRY == 3 || COUNT_MODE_COUNTRY == 4)
//				{
//					if (param->mot_info->det_box_info[d].class_id == 2)
//						param->mot_info->det_box_info[d].class_id = 1;
//
//					if (param->mot_info->det_box_info[d].class_id == cls_helmet || param->mot_info->det_box_info[d].class_id == cls_hat || param->mot_info->det_box_info[d].class_id == cls_head)
//					{
//						mosaic_draw(res_frame, rtResBox, 8);
//					}
//
//					if (rtResBox.height >= (frame.rows / 5) && COUNT_MODE_COUNTRY == 3) //20190429
//						rectangle(res_frame, rtResBox, param->class_color[cls_truck], 1, 1, 0);
//					else
//						rectangle(res_frame, rtResBox, param->class_color[param->mot_info->det_box_info[d].class_id], 1, 1, 0);
//				}
//				else if (param->mot_info->det_box_info[d].class_id == cls_helmet || param->mot_info->det_box_info[d].class_id == cls_hat || param->mot_info->det_box_info[d].class_id == cls_head)//|| param->mot_info->det_box_info[d].class_id == 10 || param->mot_info->det_box_info[d].class_id == 11)
//				{
//					rectangle(res_frame, rtResBox, param->class_color[param->mot_info->det_box_info[d].class_id], 1, 1, 0); //20190222 hai Class color  //20190327
//				}
//			}
//		}
//
//		if (param->show_det_on)
//		{
//			if (param->mot_info->det_box_info[d].class_id == 4 || param->mot_info->det_box_info[d].class_id == 5 || param->mot_info->det_box_info[d].class_id == 6)
//			{
//				rectangle(disp_image, param->mot_info->det_box_info[d].bouding_box, cv::Scalar(0, 255, 0, 0), 2, 1, 0);
//				char textTmp[CHAR_LENGTH];
//				sprintf(textTmp, "[MT]%.2f", param->mot_info->det_box_info[d].score);
//				CvFont font;
//				cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.8, 0.8, 0, 0, 1);
//				putText(disp_image, textTmp, cv::Point((int)param->mot_info->det_box_info[d].bouding_box.x, (int)param->mot_info->det_box_info[d].bouding_box.y), &font, cv::Scalar(0, 255, 0, 0));
//
//				cvShowImage("[MT]", disp_image);
//			}
//
//			if (param->mot_info->det_box_info[d].class_id == 15)
//			{
//				rectangle(disp_image, param->mot_info->det_box_info[d].bouding_box, cv::Scalar(0, 0, 255, 0), 2, 1, 0);
//				char textTmp[CHAR_LENGTH];
//				sprintf(textTmp, "[UMBRELLA]%.2f", param->mot_info->det_box_info[d].score);
//				CvFont font;
//				cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.8, 0.8, 0, 0, 1);
//				putText(disp_image, textTmp, cv::Point((int)param->mot_info->det_box_info[d].bouding_box.x, (int)param->mot_info->det_box_info[d].bouding_box.y), &font, cv::Scalar(0, 0, 255, 0));
//
//				cvShowImage("[UMBRELLA]", disp_image);
//			}
//			else if (param->mot_info->det_box_info[d].class_id == 16)
//			{
//				rectangle(disp_image, param->mot_info->det_box_info[d].bouding_box, cv::Scalar(0, 0, 255, 0), 2, 1, 0);
//				char textTmp[CHAR_LENGTH];
//				sprintf(textTmp, "[MOBILE]%.2f", param->mot_info->det_box_info[d].score);
//				CvFont font;
//				cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.8, 0.8, 0, 0, 1);
//				putText(disp_image, textTmp, cv::Point((int)param->mot_info->det_box_info[d].bouding_box.x, (int)param->mot_info->det_box_info[d].bouding_box.y), &font, cv::Scalar(0, 0, 255, 0));
//
//				cvShowImage("[MOBILE]", disp_image);
//			}
//		}
//	}
//
//	if (disp_image)
//	{
//		cvReleaseImage(&disp_image);
//		disp_image = NULL;
//	}
//	//cvReleaseImage(&detect_image);//hai 2018.11.30 ------> making non-detection region
//
//	//free_image(im);
//
//	//waitKey(1);
//	//total_time = sec(clock() - total_time_c);
//	//printf("det: %.4f / mot: %.4f / total: %.4f\n", detection_time, tracking_time, total_time);
//}