#include "violation_process.h"

#define _DLL_UNSSAVEPICTURE_AS_EXPLICIT_
#include "../UNSSavePicture/UNSSavePicture.h"

CViolationProcess::CViolationProcess()
{
	nTotalCount = 0;
	m_nSaveDebugImages = 0;
	m_nSpeedPeriod = 8;
}


CViolationProcess::~CViolationProcess()
{
}



void CViolationProcess::SavePicture_FreeDll()
{
	if (hMSaveImageDll)
	{
		FreeUNSSAVEPICTUREDll(hMSaveImageDll);
		hMSaveImageDll = nullptr;
	}
}

void CViolationProcess::SavePicture_LoadDll()
{
	hMSaveImageDll = LoadUNSSAVEPICTUREDll();
}

void CViolationProcess::SavePicture_Start(int size, int nCamIndex)
{
	hSaveImageDll = UNSSAVEPICTURE_Start(size, nCamIndex);
}

void CViolationProcess::SavePicture_Save(HANDLE hHandle, char* path, cv::Mat* image, int* saveParam, int isMat)
{
	if (hSaveImageDll)
	{
		UNSSAVEPICTURE_Save(hHandle, path, image, saveParam, isMat);
	}
}

void CViolationProcess::SavePicture_Stop()
{
	if (hSaveImageDll)
	{
		UNSSAVEPICTURE_Stop(hSaveImageDll);
		hSaveImageDll = nullptr;
	}
}

#ifdef XYCOOR_MODE

void CViolationProcess::XYCOOR_save_speed_log(char* strDate, char* strTime, int nTrackID, float fdeltaX, float fdeltaY, float fSpeed, int nX, int nY, float fDistanceX, float fDistanceY)
{

	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	CString strLog;

	strLog.Format("[%s:%s]  TrackID: %03d  deltaXdis: %.2f  deltaYdis: %.2f  speed: %.2f, X: %d, Y: %d, disX: %.2f, disY: %.2f", strDate, strTime, nTrackID, fdeltaX, fdeltaY, fSpeed, nX, nY, fDistanceX, fDistanceY);

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
#if ITS_MODE == 1
	wsprintf(makePath, "%s\\kits_event", currentPath);
#elif VDS_MODE == 1
	wsprintf(makePath, "%s\\VDS_data\\logs", currentPath);
#endif
	CreateDirectory(makePath, nullptr);

	//CommLog
	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Speed_%04d%02d%02d.log", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}

}

CString CViolationProcess::XYCOOR_calculate_speed(DarknetData &darknet, CRegion* pRegion, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, int ntrack, float fHratio, float fWratio, unsigned long long timestamp)
{
	if (pRegion == nullptr) return _T("nothing");

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);

	int nPush = -1;
	CString strSpeed;

	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\image_tracking", currentPath);
	CreateDirectory(makePath, nullptr);
	wsprintf(makePath, "%s\\VDS_data\\image_tracking\\%d", currentPath, nTrackID);
	CreateDirectory(makePath, nullptr);

#if ENGINE_MODE == 0
	if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
#else
	if (darknet.is_car(nClassID))
#endif
	{
		if (box_iou.total > 0.5f)
		{
			// calculate distance
			float fDistanceY, fDistanceX;

			pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, fDistanceY, fDistanceX);

			if (param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() < param->n_speed_period)
			{
				param->mot_DataSave[nTrackID].nXYCOOR_distanceY.push_back(fDistanceY);
				param->mot_DataSave[nTrackID].nXYCOOR_distanceX.push_back(fDistanceX);

				//param->mot_DataSave[nTrackID].nXYCOOR_time.push_back(currentClock);
				param->mot_DataSave[nTrackID].nXYCOOR_time.push_back(timestamp);

				//strSpeed.Format(_T("timestamp: %llu"), timestamp);
				//store prev calculated speed
				param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.push_back(param->mot_DataSave[nTrackID].fXYCOOR_speed_display);
				param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.push_back(param->mot_DataSave[nTrackID].fXYCOOR_deltaTime);

			}

			//calculate current speed
			if (param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() >= param->n_speed_period)
			{
				float fDiffTime = (float)(param->mot_DataSave[nTrackID].nXYCOOR_time[param->n_speed_period - 1] - param->mot_DataSave[nTrackID].nXYCOOR_time[0]) / (float)CLOCKS_PER_SEC;

				//strSpeed.Format(_T("%s final: %llu begin: %llu"), strSpeed, param->mot_DataSave[nTrackID].nXYCOOR_time[SPEED_STATS_PERIOD - 1], param->mot_DataSave[nTrackID].nXYCOOR_time[0]);

				float fDeltaX = param->mot_DataSave[nTrackID].nXYCOOR_distanceX[param->n_speed_period - 1] - param->mot_DataSave[nTrackID].nXYCOOR_distanceX[0];
				float fDeltaY = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->n_speed_period - 1] - param->mot_DataSave[nTrackID].nXYCOOR_distanceY[0];

				if (m_bHorizontalCheck == false)
				{
					// disable horizontal distance calculation
					fDeltaX = 0.0f;
				}

				float fDiffDistance = 0.0f;
				if (fDeltaY < 0.0f)
					fDiffDistance = 0 - sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);
				else
					fDiffDistance = sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

				if (fDiffTime != 0.0f)
				{
					param->mot_DataSave[nTrackID].fXYCOOR_speed = fDiffDistance / fDiffTime * 3.6f;
					param->mot_DataSave[nTrackID].fXYCOOR_speed_display = abs(param->mot_DataSave[nTrackID].fXYCOOR_speed);
					param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = fDiffTime;

					//XYCOOR_save_speed_log(strDate, strTime, nTrackID, fDeltaX, fDeltaY, param->mot_DataSave[nTrackID].fXYCOOR_speed, nPtBottomX, nPtBottomY, fDistanceX, fDistanceY);
				}
				else
				{
					param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
					param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
					param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
				}

				////save image
				//sprintf(full_save_image_info, "%s/VDS_data/image_tracking/%d/%s_%s_%d_%d_%d",
				//	currentPath,
				//	nTrackID,
				//	strDate, strTime,
				//	nTrackID,
				//	abs((int)param->mot_DataSave[nTrackID].fXYCOOR_speed_display),
				//	(int)(fDiffTime * 1000.0f));

				//sprintf(full_save_image_path, "%s.jpg", full_save_image_info);
				//clone_frame = frame.clone();
				//darknet.draw_objectbox_forLPR(clone_frame, rtBoundingBox, Scalar(0, 0, 255), false);
				//SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
				////

				//erase the first element
				param->mot_DataSave[nTrackID].nXYCOOR_distanceY.erase(param->mot_DataSave[nTrackID].nXYCOOR_distanceY.begin());
				param->mot_DataSave[nTrackID].nXYCOOR_distanceX.erase(param->mot_DataSave[nTrackID].nXYCOOR_distanceX.begin());

				param->mot_DataSave[nTrackID].nXYCOOR_time.erase(param->mot_DataSave[nTrackID].nXYCOOR_time.begin());
				param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.erase(param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.begin());
				param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.erase(param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.begin());
			}
			else
			{
				param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
				param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
				param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
				//XYCOOR_save_speed_log(strDate, strTime, nTrackID, 0.0f, 0.0f, param->mot_DataSave[nTrackID].fXYCOOR_speed, nPtBottomX, nPtBottomY, fDistanceX, fDistanceY);
			}
		}
		else
		{
			param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
			param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
			param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
		}
	}
	else
	{
		param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
		param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
		param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
	}

	return strSpeed;
}

#endif


#if KITS_FEATURES_ENABLED == 1

void CViolationProcess::KITS_save_log(char* strDate, char* strTime, char* eventType, int nDistance, int nEventCount, int nTrackID)
{
	nTotalCount++;
	nTotalCount = nTotalCount % 100000;

	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	CString strLog;
	if (strcmp(eventType, "pedestrian") == 0)
		strLog.Format("%05d,%s,%s,%s,%03d,%05d,%03d_01", nTotalCount, strDate, strTime, eventType, nDistance, nEventCount, nTrackID);
	else
		strLog.Format("%05d,%s,%s,%s,%03d,%05d,%03d_02", nTotalCount, strDate, strTime, eventType, nDistance, nEventCount, nTrackID);

	char currentPath[MAX_PATH], makePath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\kits_event", currentPath);
	CreateDirectory(makePath, nullptr);

	//CommLog
	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d.csv", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}

}

//ITS 역주행

void CViolationProcess::KITS_reverse_violation_analysis(int nMode, DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, int nEndY, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptRoad[MAX_PTROAD];
	pRegion->CRegion_copy(4, ptRoad);
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);

	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());

	char full_save_image_path[CHAR_LENGTH];

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtRealBottomX = rtBoundingBox.x;

	cv::Rect fullRect = rtBoundingBox;

	if (fullRect.height <= 6 * fullRect.width && fullRect.width <= 3 * fullRect.height)
	{
		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			// process reverse
			if (box_iou.total > 0.5f)
			{
				switch (nMode)
				{
				case KITS_LEFTENTRY_RIGHTEXIT:	//LeftEntry
				{
					// left lane 						
					if (box_iou.left > 0.5f && box_iou.right < 0.5f)
					{
						if (param->mot_DataSave[nTrackID].fXYCOOR_speed > KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						else
							param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
					}
					// right lane
					else if (box_iou.right > 0.5f && box_iou.left < 0.5f)
					{
						if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						else
							param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
					}
				}
				break;
				case KITS_LEFTEXIT_RIGHTENTRY:	//LeftExit
				{
					// left lane 						
					if (box_iou.left > 0.5f && box_iou.right < 0.5f)
					{
						if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						else
							param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
					}
					// right lane
					else if (box_iou.right > 0.5f && box_iou.left < 0.5f)
					{
						if (param->mot_DataSave[nTrackID].fXYCOOR_speed > KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						else
							param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
					}
				}
				break;
				case KITS_BOTH_ENTRY:			//both entry
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed > KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
				break;
				case KITS_BOTH_EXIT:			//both exit
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * KITS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
				break;
				default:
					break;
				}

				// store the first proof image
				if (param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].bStoreImage == 0)
				{
					param->mot_ReverseSave[nTrackID].bStoreImage = 1;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}
					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}
			}

			// reset checked information
			if (param->mot_ReverseSave[nTrackID].nReverseResetCnt >= KITS_REVERSE_RESETCNT)
			{
				darknet.release_reverse_violation(param, nTrackID);
			}

			// violation confirmed
			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt >= KITS_REVERSE_CONFIRMCNT)
			{
				param->mot_ReverseSave[nTrackID].nReverseVioCnt = KITS_REVERSE_CONFIRMCNT;

				//save image and log
				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
				{
					param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					param->mot_ReverseSave[nTrackID].nReverseResetCnt = 0;

					//20200115 index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100000;
					int nEventCount = nVioCntArray[REVERSE_VIOLATION_CNT];

					//calculate distance
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;
					float fDistanceY, fDistanceX;
					pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, fDistanceY, fDistanceX);

					//save to log file
					KITS_save_log(strDate, strTime, "reverse", (int)fDistanceY, nEventCount, nTrackID);

#if ITS_DEBUG
					//1st image
					sprintf(full_save_image_path, "%s/kits_event/%05d_%s_%s_reverse_%03d_%05d_%03d_01.jpg", currentPath, nTotalCount, strDate, strTime, (int)round(fDistanceY), nEventCount, nTrackID);
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);
#endif
					//2nd image [used for POC]
					sprintf(full_save_image_path, "%s/kits_event/%05d_%s_%s_reverse_%03d_%05d_%03d_02.jpg", currentPath, nTotalCount, strDate, strTime, (int)round(fDistanceY), nEventCount, nTrackID);

					if (!clone_frame.empty())
					{
						clone_frame = frame.clone();
#if ITS_DEBUG
						cv::line(clone_frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
#if ITS_DEBUG
						cv::line(frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					//use to release
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
				}
			}
		}


		//20190130 hai for Overlay Display
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
		}


		// ===== [release violation info] =====
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);
			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= KITS_EVENT_RELEASE_PERIOD) //xx초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}
	}
}


//ITS 정지 차량

void CViolationProcess::KITS_stop_violation_analysis(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptRoad[MAX_PTROAD];
	pRegion->CRegion_copy(4, ptRoad);
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);


	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	char full_save_image_path[CHAR_LENGTH], textTmp[CHAR_LENGTH];


	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtRealBottomX = rtBoundingBox.x;

	cv::Rect fullRect = rtBoundingBox;

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);


	if (fullRect.height <= 6 * fullRect.width && fullRect.width <= 3 * fullRect.height)
	{
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			// start to process stop event
			if (box_iou.total > 0.5f)
			{
				if (param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f && param->mot_DataSave[nTrackID].fXYCOOR_speed_display < KITS_STOP_MAX_SPEED)
				{
					param->mot_DataSave[nTrackID].nStopVioCnt++;

					//check zero speed
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed_display == 0.0f)
						param->mot_DataSave[nTrackID].nStopZeroCnt++;
				}
				else
					param->mot_DataSave[nTrackID].nStopResetCnt++;
			}

			// store the first proof image
			if (box_iou.total > 0.5f && param->mot_DataSave[nTrackID].fXYCOOR_speed_display == 0.0f && param->mot_DataSave[nTrackID].bStoreImage == 0)
			{
				param->mot_DataSave[nTrackID].bStoreImage = 1;

				if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
				{
					param->mot_DataSave[nTrackID].pIplImageParkingStart.release();
				}
				param->mot_DataSave[nTrackID].pIplImageParkingStart = frame.clone();
				darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].pIplImageParkingStart, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], false);
			}

			// reset checked information
			if (param->mot_DataSave[nTrackID].nStopResetCnt >= KITS_STOP_RESETCNT)
			{
				darknet.release_stop_violation(param, nTrackID);
			}

			// violation confirmed
			if (param->mot_DataSave[nTrackID].nStopVioCnt >= KITS_STOP_CONFIRMCNT && param->mot_DataSave[nTrackID].nStopZeroCnt < KITS_STOP_ZEROCNT)
			{
				int nStopVioCnt_tmp = param->mot_DataSave[nTrackID].nStopVioCnt;
				int nStopZeroCnt_tmp = param->mot_DataSave[nTrackID].nStopZeroCnt;

				darknet.release_stop_violation(param, nTrackID);
				if (nStopZeroCnt_tmp > 0)
				{
					param->mot_DataSave[nTrackID].nStopZeroCnt = nStopZeroCnt_tmp;
					param->mot_DataSave[nTrackID].nStopVioCnt = KITS_STOP_CONFIRMCNT - KITS_STOP_ZEROCNT;
				}
			}
			else if (param->mot_DataSave[nTrackID].nStopVioCnt >= KITS_STOP_CONFIRMCNT && param->mot_DataSave[nTrackID].nStopZeroCnt >= KITS_STOP_ZEROCNT)
			{
				param->mot_DataSave[nTrackID].nStopVioCnt = KITS_STOP_CONFIRMCNT;
				param->mot_DataSave[nTrackID].nStopZeroCnt = KITS_STOP_ZEROCNT;

				//save image and log
				if (param->mot_DataSave[nTrackID].bSaveAccident == 0)
				{
					param->mot_DataSave[nTrackID].bSaveAccident = 1;
					param->mot_DataSave[nTrackID].nStopResetCnt = 0;

					//20200115 index counting for violation
					nVioCntArray[ILLEGALPARKING_CNT]++;
					nVioCntArray[ILLEGALPARKING_CNT] = nVioCntArray[ILLEGALPARKING_CNT] % 100000;
					int nEventCount = nVioCntArray[ILLEGALPARKING_CNT];

					//calculate distance
					int millisecond = (int)((float)(currentClock - 0) / CLOCKS_PER_SEC * 100.0f) % 100;
					float fDistanceY, fDistanceX;
					pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, fDistanceY, fDistanceX);

					//save to log file
					KITS_save_log(strDate, strTime, "stop", (int)fDistanceY, nEventCount, nTrackID);

#if ITS_DEBUG
					//1st image
					sprintf(full_save_image_path, "%s/kits_event/%05d_%s_%s_stop_%03d_%05d_%03d_01.jpg", currentPath, nEventCount, strDate, strTime, (int)round(fDistanceY), nEventCount, nTrackID);
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_DataSave[nTrackID].pIplImageParkingStart, 0, 1);
#endif
					//2nd image [used for POC]
					bool bUseClone = false;
					sprintf(full_save_image_path, "%s/kits_event/%05d_%s_%s_stop_%03d_%05d_%03d_02.jpg", currentPath, nEventCount, strDate, strTime, (int)round(fDistanceY), nEventCount, nTrackID);

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
#if ITS_DEBUG
						cv::line(clone_frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
#if ITS_DEBUG
						cv::line(frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
					{
						param->mot_DataSave[nTrackID].pIplImageParkingStart.release();
					}


					//use to release
					param->mot_DataSave[nTrackID].stopTime = currentClock;
				}
			}

		}


		//20190130 hai for Overlay Display
		if (param->mot_DataSave[nTrackID].bSaveAccident == 1)
		{
			param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
		}


		// ===== [release violation info] =====
		if (param->mot_DataSave[nTrackID].bSaveAccident == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_DataSave[nTrackID].stopTime) / CLOCKS_PER_SEC);
			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= KITS_EVENT_RELEASE_PERIOD) //xx초 초과
			{
				darknet.release_stop_violation(param, nTrackID);
			}
		}
	}
}


//ITS 보행자

void CViolationProcess::KITS_pedestrian_detection(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptRoad[MAX_PTROAD];
	pRegion->CRegion_copy(4, ptRoad);
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);

	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());

	char full_save_image_path[CHAR_LENGTH];

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtRealBottomX = rtBoundingBox.x;

	cv::Rect fullRect = rtBoundingBox;

	// ===== [car/van/truck Violation] =====
	if (nClassID == PEDESTRIAN_CLASS_ID)
	{
		//if ()
		{
			// check pedestrian appearance
			if (box_iou.total > 0.5f)
			{
				param->mot_DataSave[nTrackID].nPedestVioCnt++;
			}
			else
			{
				param->mot_DataSave[nTrackID].nPedestResetCnt++;
			}

			// reset checked information
			if (param->mot_DataSave[nTrackID].nPedestResetCnt >= KITS_PEDESTRIAN_MAXCNT)
			{
				param->mot_DataSave[nTrackID].nPedestVioCnt = 0;
				param->mot_DataSave[nTrackID].nPedestResetCnt = 0;
				param->mot_DataSave[nTrackID].nPedestViolation = 0;
				param->mot_DataSave[nTrackID].appearTime = 0;
				param->mot_DataSave[nTrackID].nVioDisplay[PEDESTRIAN_VIO_CLR] = 0;
			}

			// violation confirmed
			if (param->mot_DataSave[nTrackID].nPedestVioCnt >= KITS_PEDESTRIAN_MAXCNT)
			{
				param->mot_DataSave[nTrackID].nPedestVioCnt = KITS_PEDESTRIAN_MAXCNT;

				//save image and log
				if (param->mot_DataSave[nTrackID].nPedestViolation == 0)
				{
					param->mot_DataSave[nTrackID].nPedestViolation = 1;
					param->mot_DataSave[nTrackID].nPedestResetCnt = 0;

					//20200115 index counting for violation
					nVioCntArray[PEDESTRIAN_CNT]++;
					nVioCntArray[PEDESTRIAN_CNT] = nVioCntArray[PEDESTRIAN_CNT] % 100000;
					int nEventCount = nVioCntArray[PEDESTRIAN_CNT];

					//calculate distance
					float fDistanceY, fDistanceX;
					pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, fDistanceY, fDistanceX);

					//save to log file
					KITS_save_log(strDate, strTime, "pedestrian", (int)fDistanceY, nEventCount, nTrackID);

					//image [used for POC]
					sprintf(full_save_image_path, "%s/kits_event/%05d_%s_%s_pedestrian_%03d_%05d_%03d_01.jpg", currentPath, nTotalCount, strDate, strTime, (int)round(fDistanceY), nEventCount, nTrackID);

					if (!clone_frame.empty())
					{
						clone_frame = frame.clone();
#if ITS_DEBUG
						cv::line(clone_frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(clone_frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[PEDESTRIAN_VIO_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
#if ITS_DEBUG
						cv::line(frame, cv::Point(ptLine[10].x, ptLine[10].y * fHratio), cv::Point(ptLine[10].x + 200, ptLine[10].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptLine[3].x, ptLine[3].y * fHratio), cv::Point(ptLine[3].x + 200, ptLine[3].y * fHratio), cv::Scalar(255, 255, 0, 0));
						cv::line(frame, cv::Point(ptRoad[15].x, ptRoad[15].y * fHratio), cv::Point(ptRoad[15].x + 200, ptRoad[15].y * fHratio), cv::Scalar(255, 255, 0, 0));
#endif
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[PEDESTRIAN_VIO_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//use to release
					param->mot_DataSave[nTrackID].appearTime = currentClock;
				}
			}
		}
	}


	//20190130 hai for Overlay Display
	if (param->mot_DataSave[nTrackID].nPedestViolation == 1)
	{
		param->mot_DataSave[nTrackID].nVioDisplay[PEDESTRIAN_VIO_CLR] = 1; //2019022
		//violation_text_display(res_frame, param, ntrack, nTrackID);
		darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
	}


	// ===== [release violation info] =====
	if (param->mot_DataSave[nTrackID].nPedestViolation == 1)
	{
		int seconds = (int)((float)(currentClock - param->mot_DataSave[nTrackID].appearTime) / CLOCKS_PER_SEC);
		//std::cout << "TimeOut ===============: " << seconds << std::endl;

		if (seconds >= KITS_EVENT_RELEASE_PERIOD) //xx초 초과
		{
			param->mot_DataSave[nTrackID].nPedestVioCnt = 0;
			param->mot_DataSave[nTrackID].nPedestResetCnt = 0;
			param->mot_DataSave[nTrackID].nPedestViolation = 0;
			param->mot_DataSave[nTrackID].appearTime = 0;
			param->mot_DataSave[nTrackID].nVioDisplay[PEDESTRIAN_VIO_CLR] = 0;
		}
	}
}

#endif // KITS_FEATURES_ENABLED

#if VDS_MODE == 1

CString CViolationProcess::VDS_application(DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, int nCheckReverseViolation, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox,
	CArray<int, int&> &nVioCntArray, int ntrack, float fHratio, float fWratio, unsigned long long timestamp, int nCamIndex)
{
	CString strLog;

	if (pRegion == nullptr)
	{
		strLog.Format("Region is NULL");
		return strLog;
	}
	cv::Point ptRoad[MAX_PTROAD];
	pRegion->CRegion_copy(4, ptRoad);
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);

	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());

	char full_save_image_path[CHAR_LENGTH];

	clock_t currentClock = clock();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + (float)rtBoundingBox.height) / fHratio);
	int nPtTopY = (int)round((float)rtBoundingBox.y / fHratio);
	int nPtCenterX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtLeftX = (int)round((float)rtBoundingBox.x / fWratio);
	int nPtRightX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width) / fWratio);

	int nPtProcessX = 0;
	if (nPtCenterX < pRegion->m_ptVanish[0].x)
		nPtProcessX = nPtLeftX;
	else
		nPtProcessX = nPtRightX;


	// default: get center x value to process

#if LINE_DETERMINE == 0
	int nPtBottomX = nPtCenterX;
	int nPtTopX = nPtCenterX;
#else
	int nPtBottomX = nPtProcessX;
	int nPtTopX = pRegion->CRegion_get_XTop(nPtBottomX, nPtBottomY, nPtTopY);
#endif


	//int nPtLeftXCheck = (int)round((float)rtBoundingBox.x / fWratio);
	//int nPtRightXCheck = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width) / fWratio);

	//// get left side of bbox to process
	//if (nPtLeftXCheck < pRegion->m_ptVanish[0].x && nPtRightXCheck <= pRegion->m_ptVanish[0].x)
	//{
	//	nPtTopX = nPtBottomX = (int)round(((float)rtBoundingBox.x) / fWratio);
	//}
	//// get right side of bbox to process
	//else if (nPtLeftXCheck >= pRegion->m_ptVanish[0].x && nPtRightXCheck > pRegion->m_ptVanish[0].x)
	//{
	//	
	//	nPtTopX = nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width) / fWratio);
	//}	

	int nProcessStep = 0; //[general - not started, not ended]

	// determine start time and start distance
	if (param->mot_DataSave[nTrackID].bVDS_started == TRUE && !param->mot_DataSave[nTrackID].bVDS_started_setdone/* && param->mot_DataSave[nTrackID].nVDS_startTime == -1*/)
	{
		param->mot_DataSave[nTrackID].bVDS_started_setdone = TRUE;
		//param->mot_DataSave[nTrackID].nVDS_startTime = currentClock;

		// calculate distance at start time
		// bottom
		pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, param->mot_DataSave[nTrackID].fVDS_SB_distanceY, param->mot_DataSave[nTrackID].fVDS_SB_distanceX);

		// top
		pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtTopY, nPtTopX, param->mot_DataSave[nTrackID].fVDS_ST_distanceY, param->mot_DataSave[nTrackID].fVDS_ST_distanceX);

		// store start image
		if (m_nSaveDebugImages)
		{
			if (!param->mot_DataSave[nTrackID].iVDS_startImg.empty())
			{
				param->mot_DataSave[nTrackID].iVDS_startImg.release();
			}
			param->mot_DataSave[nTrackID].iVDS_startImg = frame.clone();
			darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].iVDS_startImg, rtBoundingBox, param->vio_color[TRACKING_CLR], false);
		}
		nProcessStep = 1; //[LOOP-START]
	}

	// distance at end time
	float fET_distanceY = 0.0f, fET_distanceX = 0.0f;	//top
	float fEB_distanceY = 0.0f, fEB_distanceX = 0.0f;	//bottom

	// determine end time, end distance, speed, length, occupancy time

	if (param->mot_DataSave[nTrackID].bVDS_started == TRUE && param->mot_DataSave[nTrackID].bVDS_ended == TRUE && !param->mot_DataSave[nTrackID].bVDS_ended_setdone/*&& param->mot_DataSave[nTrackID].nVDS_endTime == -1*/)
	{
		param->mot_DataSave[nTrackID].bVDS_ended_setdone = TRUE;
		//param->mot_DataSave[nTrackID].nVDS_endTime = currentClock;

		// bottom
		pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtBottomY, nPtBottomX, fEB_distanceY, fEB_distanceX);

		// top
		pRegion->CRegion_get_XYdistance(param->nDisplayH, nPtTopY, nPtTopX, fET_distanceY, fET_distanceX);

		// calculate car length
		param->mot_DataSave[nTrackID].fVDS_carLength = VDS_calculate_carlength(param, nTrackID, fET_distanceY, fEB_distanceY);

		// calculate speed and occupancy time
		VDS_calculate_speed_occupancy(param, pRegion, nTrackID, fEB_distanceY, fEB_distanceX, false);

		// determine direction [left - 상행, right: 하행]
		param->mot_DataSave[nTrackID].nVDS_direction = pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, 0);

		// determine vehicle gap
		VDS_calculate_vehicleGap(pRegion, param, ntrack, fEB_distanceY, false);

		// check car info
		VDS_save_log_checkinfo(0, param, nTrackID, nClassID, fET_distanceY, fEB_distanceY, fET_distanceX, fEB_distanceX, nCamIndex);

		nProcessStep = 2; //[LOOP-END]
	}

	// check reverse violation
	if (nCheckReverseViolation)
	{
		VDS_reverse_violation_analysis(nProcessStep, fEB_distanceY, darknet, pRegion, frame, res_frame, clone_frame, param, rtResBox, ntrack, fHratio, fWratio);
	}

	m_fLoopAngle = pRegion->fLoopAngle;

	// save to log and push for transfering
	if (nProcessStep == 2)
	{
		if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
			pRegion->CRegion_get_Angle_Width(nPtLeftX, nPtCenterX, nPtRightX, nPtBottomY, param->mot_DataSave[nTrackID].fVDS_cosAngle, param->mot_DataSave[nTrackID].fVDS_carTotalWidth);
		strLog = VDS_save_log(darknet, pRegion, frame, param, ntrack, fET_distanceY, fEB_distanceY, fET_distanceX, fEB_distanceX, nCamIndex);

		if (strLog.Find("reject") < 0) // not case of rejecting
			param->mot_DataSave[nTrackID].nVDS_pushStatus = PUSH_PREPARED; // already to push
	}


	return strLog;
}

void CViolationProcess::VDS_save_log_checkinfo(int nFixType, st_det_mot_param *param, int nTrackID, int nClassID, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH], strImgDate[CHAR_LENGTH], strImgTime[CHAR_LENGTH];

	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d:%02d:%02d.%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	wsprintf(strImgDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strImgTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - param->mot_DataSave[nTrackID].nVDS_startTime) / (float)CLOCKS_PER_SEC;

	float disspeed = 0.0f, disspeed_1 = 0.0f;
	float distime = 0.0f, distime_1 = 0.0f;

	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 0)
	{
		disspeed = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.back();
		distime = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.back();
	}
	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 1)
	{
		disspeed_1 = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - 2];
		distime_1 = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime[param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.size() - 2];
	}

	CString strLog;
	// info: day, time, lane number, direction, car length, speed, classID, occupancy time, reverse (Y/N), vehicleGap
	strLog.Format("[%s-%s] [%03d] nTrackID %03d class %d lane %d direct %d speed %.2f disspeed %.2f disspeed_1 %.2f time %.3f distime %.3f distime_1 %.3f dis %.2f length: %04d height: %04d start [bot X %.2f Y %.2f top X %.2f Y %.2f] end [bot X %.2f Y %.2f top X %.2f Y %.2f]"/* delta [start %.2f end %.2f]"*/,
		//strLog.Format("[%s:%s] [%02d] nTrackID %03d class %d lane %d direct %d speed %.2f length: %04d height: %04d start [bot X %.2f Y %.2f top X %.2f Y %.2f] end [bot X %.2f Y %.2f top X %.2f Y %.2f]"/* delta [start %.2f end %.2f]"*/,
		strDate, strTime,
		nFixType,
		nTrackID,
		nClassID,
		param->mot_DataSave[nTrackID].nVDS_laneNum,
		param->mot_DataSave[nTrackID].nVDS_direction,
		param->mot_DataSave[nTrackID].fVDS_carSpeed,
		disspeed,
		disspeed_1,
		fDiffTime,
		distime,
		distime_1,
		abs(fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY),
		(int)round(param->mot_DataSave[nTrackID].fVDS_carLength * 100),
		(int)round(param->mot_DataSave[nTrackID].fVDS_carHeight * 100),
		param->mot_DataSave[nTrackID].fVDS_SB_distanceX,
		param->mot_DataSave[nTrackID].fVDS_SB_distanceY,
		param->mot_DataSave[nTrackID].fVDS_ST_distanceX,
		param->mot_DataSave[nTrackID].fVDS_ST_distanceY,
		fEB_distanceX,
		fEB_distanceY,
		fET_distanceX,
		fET_distanceY/*,
		(param->mot_DataSave[nTrackID].fVDS_ST_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY),
		(fET_distanceY - fEB_distanceY)*/);


	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\logs", currentPath);
	CreateDirectory(makePath, nullptr);

	//CommLog
	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d_checkInfo_%02d.log", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, nCamIndex);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}
}

float CViolationProcess::VDS_get_fixed_speed(float carSpeed, float disspeed, float disspeed_1)
{
	//determine good speed value
	float fNewSpeed = 0.0f;
	int nSelected = 0;

	if (disspeed == 1000.0f && disspeed_1 == 1000.0f)
	{
		fNewSpeed = carSpeed;
		return fNewSpeed;
	}
	else if (disspeed_1 == 1000.0f)
	{
		fNewSpeed = min(carSpeed, disspeed);
		nSelected = 0;
	}
	else if (disspeed == 1000.0f)
	{
		fNewSpeed = min(carSpeed, disspeed_1);
		nSelected = 1;
	}
	else
	{
		float fSpeed0 = abs(carSpeed - disspeed);
		float fSpeed1 = abs(carSpeed - disspeed_1);
		float fDis01 = abs(disspeed - disspeed_1);

		float fMinabs = min(fSpeed0, min(fSpeed1, fDis01));


		if (fMinabs == fSpeed0)
		{
			fNewSpeed = min(carSpeed, disspeed);
			nSelected = 0;
		}
		else if (fMinabs == fSpeed1)
		{
			fNewSpeed = min(carSpeed, disspeed_1);
			nSelected = 1;
		}
		else if (fMinabs == fDis01)
		{
			fNewSpeed = min(disspeed, disspeed_1);
			nSelected = 2;
		}
	}


	if (fNewSpeed < 1.0f) // broken frame, noisy
	{
		switch (nSelected)
		{
		case 0:
			if (max(carSpeed, disspeed) < 1.0f && disspeed_1 >= 1.0f && disspeed_1 < 250.0f)
			{
				fNewSpeed = disspeed_1;
			}
			else if (max(carSpeed, disspeed) >= 1.0f && max(carSpeed, disspeed) < 250.0f)
			{
				fNewSpeed = max(carSpeed, disspeed);
			}
			break;
		case 1:
			if (max(carSpeed, disspeed_1) < 1.0f && disspeed >= 1.0f && disspeed < 250.0f)
			{
				fNewSpeed = disspeed;
			}
			else if (max(carSpeed, disspeed_1) >= 1.0f && max(carSpeed, disspeed_1) < 250.0f)
			{
				fNewSpeed = max(carSpeed, disspeed_1);
			}
			break;
		case 2:
			if (max(disspeed, disspeed_1) < 1.0f && carSpeed >= 1.0f && carSpeed < 250.0f)
			{
				fNewSpeed = carSpeed;
			}
			else if (max(disspeed, disspeed_1) >= 1.0f && max(disspeed, disspeed_1) < 250.0f)
			{
				fNewSpeed = max(disspeed, disspeed_1);
			}
			break;
		default:
			fNewSpeed = carSpeed;
			break;
		}
	}

	return fNewSpeed;
}

CString CViolationProcess::VDS_save_log(DarknetData &darknet, CRegion* pRegion, Mat& frame, st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH], strImgDate[CHAR_LENGTH], strImgTime[CHAR_LENGTH];

	wsprintf(strDate, "%04d-%02d-%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d:%02d:%02d.%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	wsprintf(strImgDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strImgTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\logs", currentPath);
	CreateDirectory(makePath, nullptr);

	// [LOG]
	CString strLog;

	// fix invalid cases
	int nFixType = VDS_fix_invalid_case(param, ntrack, fET_distanceY, fEB_distanceY, fET_distanceX, fEB_distanceX);

	// fix final speed
	float disspeed = 0.0f, disspeed_1 = 0.0f;
	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 0)
		disspeed = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.back();
	else
		disspeed = 1000.0f;

	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 1)
		disspeed_1 = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - 2];
	else
		disspeed_1 = 1000.0f;

	if (disspeed <= 0.0f)
		disspeed = 1000.0f;

	if (disspeed_1 <= 0.0f)
		disspeed_1 = 1000.0f;

	float fNewSpeed = VDS_get_fixed_speed(param->mot_DataSave[nTrackID].fVDS_carSpeed, disspeed, disspeed_1);

	//fNewSpeed = min(param->mot_DataSave[nTrackID].fVDS_carSpeed, min(disspeed, disspeed_1));

	// update speed with good value
	param->mot_DataSave[nTrackID].fVDS_carSpeed = fNewSpeed;

	//update occupancy time and gap
	// occupancy time
	VDS_calculate_speed_occupancy(param, pRegion, nTrackID, fEB_distanceY, fEB_distanceX, true);

	// vehicle gap
	VDS_calculate_vehicleGap(pRegion, param, ntrack, fEB_distanceY, true);

	// [SAVE IMAGE]
	if (m_nSaveDebugImages)
	{
		float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - param->mot_DataSave[nTrackID].nVDS_startTime);
		//if (fDiffTime <= 150)
		{
			cv::Rect fullRect = param->mot_info->mot_box_info[ntrack].bouding_box;

			// start image
			sprintf(full_save_image_info, "%s/VDS_data/images/car_%s_%s_%d_%d_%04d_%04d_%04d_%03d_%04d_%d_%04d_%03d",
				currentPath,
				strImgDate, strImgTime,
				param->mot_DataSave[nTrackID].nVDS_laneNum,
				param->mot_DataSave[nTrackID].nVDS_direction,
				(int)round(param->mot_DataSave[nTrackID].fVDS_carLength * 100),
				(int)round(param->mot_DataSave[nTrackID].fVDS_carHeight * 100),
				(int)param->mot_DataSave[nTrackID].fVDS_carSpeed,
				nClassID,
				(int)round(param->mot_DataSave[nTrackID].fVDS_ocpTime * 1000),
				param->mot_ReverseSave[nTrackID].nReverseViolation,
				param->mot_DataSave[nTrackID].nVDS_vehicleGap,
				nTrackID);

			sprintf(full_save_image_path, "%s.jpg", full_save_image_info);
			if (!param->mot_DataSave[nTrackID].iVDS_startImg.empty())
			{
				SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_DataSave[nTrackID].iVDS_startImg, 0, 1);
			}

			// end image
			sprintf(full_save_image_path, "%s_02.jpg", full_save_image_info);
			darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[TRACKING_CLR], false);
			SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
		}
	}

	// save car info after fixing
	VDS_save_log_checkinfo(nFixType, param, nTrackID, nClassID, fET_distanceY, fEB_distanceY, fET_distanceX, fEB_distanceX, nCamIndex);

	// reject car with invalid information
	if (param->mot_DataSave[nTrackID].nVDS_laneNum < 0
		|| param->mot_DataSave[nTrackID].nVDS_direction == 0
		|| param->mot_DataSave[nTrackID].fVDS_carLength < 0.0f
		|| param->mot_DataSave[nTrackID].fVDS_carSpeed < 0.0f
		|| param->mot_DataSave[nTrackID].fVDS_ocpTime < 0.0f
		|| param->mot_DataSave[nTrackID].nVDS_vehicleGap < 0)
	{
		strLog.Format("reject car with invalid information, %d %d %.2f %.2f %.2f %d",
			param->mot_DataSave[nTrackID].nVDS_laneNum, param->mot_DataSave[nTrackID].nVDS_direction, param->mot_DataSave[nTrackID].fVDS_carLength,
			param->mot_DataSave[nTrackID].fVDS_carSpeed, param->mot_DataSave[nTrackID].fVDS_ocpTime, param->mot_DataSave[nTrackID].nVDS_vehicleGap);
		return strLog; //[Rejected]
	}

	/*int lanenum = param->mot_DataSave[nTrackID].nVDS_direction ==1? param->mot_DataSave[nTrackID].nVDS_laneNum : param->mot_DataSave[nTrackID].nVDS_laneNum + param->l_box_max;
	param->mot_DataSave[nTrackID].nVDS_laneNum = lanenum;*/

	// info: day, time, lane number, direction, car length, speed, classID, occupancy time, reverse (Y/N), vehicleGap
	strLog.Format("%s,%s,%d,%d,%04d,%.2f,%d,%04d,%d,%04d,%03d",
		strDate, strTime,
		param->mot_DataSave[nTrackID].nVDS_laneNum,
		param->mot_DataSave[nTrackID].nVDS_direction,
		(int)round(param->mot_DataSave[nTrackID].fVDS_carLength * 100.0f),
		param->mot_DataSave[nTrackID].fVDS_carSpeed,
		nClassID,
		(int)round(param->mot_DataSave[nTrackID].fVDS_ocpTime * 1000.0f),
		param->mot_ReverseSave[nTrackID].nReverseViolation,
		param->mot_DataSave[nTrackID].nVDS_vehicleGap,
		nTrackID);

	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d_%02d.csv", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, nCamIndex + 1);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}

	return strLog;
}

int CViolationProcess::VDS_fix_invalid_case(st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX)
{
	int nFixType = 0;

#if ENGINE_MODE == 0
	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	// invalid car length
	// condition1 (1):	[left] start distance > end distance
	//					[right] end distance > start distance
	// condition2 (2):  car length < min(start distance, end distance)

	float fStartDistance = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
	float fEndDistance = fET_distanceY - fEB_distanceY;
	float tana = param->fCamHeight / (param->fLengthLow + param->mot_DataSave[nTrackID].fVDS_ST_distanceY);
	float tanb = param->fCamHeight / (param->fLengthLow + fET_distanceY);

	// |=======================|
	// |== BASIC REQUIREMENT ==|
	// |=======================|

	bool bCond1 = TRUE, bCond2 = TRUE;

	if ((param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE && fEndDistance > 1.05f * fStartDistance)
		|| (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE && fStartDistance > 1.05f * fEndDistance))
	{
		bCond1 = FALSE;
	}

	if (param->mot_DataSave[nTrackID].fVDS_carLength > 0.95f * min(fStartDistance, fEndDistance))
	{
		bCond2 = FALSE;
	}

	if (bCond1 == FALSE || bCond2 == FALSE)
	{
		param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
		if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
			param->mot_DataSave[nTrackID].fVDS_carHeight = tana * (param->mot_DataSave[nTrackID].fVDS_ST_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
		else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
			param->mot_DataSave[nTrackID].fVDS_carHeight = tanb * (fET_distanceY - fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
		nFixType = 1;
	}

	// |=======================|
	// |====== RIGHT SIDE =====|
	// |=======================|

	if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
	{
		// SUV - SEDAN - small-TRUCK 
		if ((param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 0.8f)
			|| (param->mot_DataSave[nTrackID].fVDS_carLength > 5.0f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 0.7f)
			|| (nClassID == cls_truck && param->mot_DataSave[nTrackID].fVDS_carHeight < 1.4f))
		{
			if (tana != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 0.7f;
				if (nClassID == cls_ban)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 0.8f;
				else if (nClassID == cls_truck)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 1.4f;

				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 21;
			}
			else
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
				nFixType = 22;
			}
		}

		// check again
		if (nClassID == cls_car || nClassID == cls_ban)
		{
			if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.3f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 0.8f && param->mot_DataSave[nTrackID].fVDS_carHeight > 0.7f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 0.9f * param->mot_DataSave[nTrackID].fVDS_carHeight;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 31;
			}
			else if (nClassID == cls_ban && param->mot_DataSave[nTrackID].fVDS_carLength >= 4.5f && param->mot_DataSave[nTrackID].fVDS_carLength < 4.9f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 0.8f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.5f * param->mot_DataSave[nTrackID].fVDS_carHeight;
				nFixType = 32;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength > 5.8f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 0.8f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.0f;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 33;
			}
		}


		// TRUCK : from now all have height >= 1.4f
		if (nClassID == cls_truck)
		{
			if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight < 2.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.5f * param->mot_DataSave[nTrackID].fVDS_carHeight;
				param->mot_DataSave[nTrackID].fVDS_carHeight = tana * (param->mot_DataSave[nTrackID].fVDS_ST_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
				nFixType = 41;
			}
			else if ((param->mot_DataSave[nTrackID].fVDS_carLength > 6.0f && param->mot_DataSave[nTrackID].fVDS_carHeight < 2.0f)
				|| (param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 2.0f))
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 2.0f;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 42;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength < 6.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 2.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 43;
			}

			if (param->mot_DataSave[nTrackID].fVDS_carHeight > 4.1f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 4.1f;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 44;
			}
		}

		// TRUCK only
		if (nClassID == cls_truck && param->mot_DataSave[nTrackID].fVDS_carHeight >= 2.0f)
		{
			bool bChanged = false;

			if (param->mot_DataSave[nTrackID].fVDS_carLength > 7.0f && param->mot_DataSave[nTrackID].fVDS_carLength <= 8.5f && param->mot_DataSave[nTrackID].fVDS_carHeight < 2.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
				bChanged = true;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength > 8.5f && param->mot_DataSave[nTrackID].fVDS_carLength <= 9.5f && param->mot_DataSave[nTrackID].fVDS_carHeight < 3.2f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 3.2f;
				bChanged = true;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength > 9.5f && param->mot_DataSave[nTrackID].fVDS_carHeight < 3.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
				bChanged = true;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 3.2f;
				bChanged = true;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength > 10.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 3.9f;
				bChanged = true;
			}


			if (bChanged && tana != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = 45;
			}
			else if (bChanged && tana == 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
				nFixType = 46;
			}
		}

		// for Truck with big load but small length => cannot see the head of truck
		if (nClassID == cls_truck)
		{
			if (param->mot_DataSave[nTrackID].fVDS_carLength < 7.0f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 3.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 1.5f;
				nFixType = 47;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength < 5.0f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 2.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.8f;
				nFixType = 48;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength < 5.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 2.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.4f;
				nFixType = 49;
			}
		}
	}

	// |=======================|
	// |====== LEFT SIDE ======|
	// |=======================|

	if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
	{
		// SEDAN - SUV
		if (nClassID == cls_car || nClassID == cls_ban)
		{
			bool bChanged = false;

			if (nClassID == cls_car && param->mot_DataSave[nTrackID].fVDS_carLength >= 5.2f && param->mot_DataSave[nTrackID].fVDS_carHeight < 1.1f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.1f;
				bChanged = true;
			}
			else if (nClassID == cls_ban && param->mot_DataSave[nTrackID].fVDS_carLength >= 5.2f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 1.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.65f;
				bChanged = true;
			}
			else if (nClassID == cls_car && param->mot_DataSave[nTrackID].fVDS_carLength <= 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 1.1f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.1f;
				bChanged = true;
			}
			else if (nClassID == cls_ban && param->mot_DataSave[nTrackID].fVDS_carLength <= 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 1.6f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.6f;
				bChanged = true;
			}

			if (bChanged && tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
				nFixType = 51;
			}
			else if (bChanged && tanb == 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
				nFixType = 52;
			}

			// check again
			bChanged = false;
			if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.3f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 1.1f && param->mot_DataSave[nTrackID].fVDS_carHeight > 1.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 0.9f * param->mot_DataSave[nTrackID].fVDS_carHeight;
				bChanged = true;
			}
			else if (nClassID == cls_ban && param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 1.4f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.4f;
				bChanged = true;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength > 5.8f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 1.65f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.75f;
				bChanged = true;
			}

			if (bChanged && tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
				nFixType = 53;
			}

			// [cannot see rear of SUV / changed to SUV]
			if (param->mot_DataSave[nTrackID].fVDS_carHeight < 1.8f && param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.5f;
				nFixType = 54;
			}

		}

		// basic for TRUCK
		if (nClassID == cls_truck && (param->mot_DataSave[nTrackID].fVDS_carHeight < 1.0f
			|| (param->mot_DataSave[nTrackID].fVDS_carLength < 4.0f && param->mot_DataSave[nTrackID].fVDS_carHeight < 2.5f)))
		{
			if (tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.0f;
				if (param->mot_DataSave[nTrackID].fVDS_carLength > 3.2f)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 1.3f;

				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
				nFixType = 61;
			}
		}

		if (nClassID == cls_truck && param->mot_DataSave[nTrackID].fVDS_carHeight > 4.5f)
		{
			if (tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 4.5;

				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
				nFixType = 62;
			}
		}

		// calculate base width for Truck
		float fWidth = 0.0f, fLength = 0.0f;
		if (nClassID == cls_truck && cos(m_fLoopAngle) != 0)
		{
			fWidth = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - param->mot_DataSave[nTrackID].fVDS_carLength * param->mot_DataSave[nTrackID].fVDS_cosAngle) / cos(m_fLoopAngle);

			if (param->mot_DataSave[nTrackID].fVDS_cosAngle != 0.0f)
			{
				if (fWidth < 1.3f)
				{
					fWidth = 1.3f;
					param->mot_DataSave[nTrackID].fVDS_carLength = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - fWidth * cos(m_fLoopAngle)) / param->mot_DataSave[nTrackID].fVDS_cosAngle;
					nFixType = 63;
				}

				if (fWidth > 2.6f)
				{
					fWidth = 2.6f;
					param->mot_DataSave[nTrackID].fVDS_carLength = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - fWidth * cos(m_fLoopAngle)) / param->mot_DataSave[nTrackID].fVDS_cosAngle;
					nFixType = 64;
				}

				if (fWidth < 1.7f && param->mot_DataSave[nTrackID].fVDS_carLength > 5.5f)
				{
					fWidth = 1.7f;
					param->mot_DataSave[nTrackID].fVDS_carLength = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - fWidth * cos(m_fLoopAngle)) / param->mot_DataSave[nTrackID].fVDS_cosAngle;
					nFixType = 65;
				}

				if (fWidth < 2.1f && param->mot_DataSave[nTrackID].fVDS_carLength > 6.5f)
				{
					fWidth = 2.1f;
					param->mot_DataSave[nTrackID].fVDS_carLength = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - fWidth * cos(m_fLoopAngle)) / param->mot_DataSave[nTrackID].fVDS_cosAngle;
					nFixType = 66;
				}

				if (fWidth < 2.4f && param->mot_DataSave[nTrackID].fVDS_carLength > 8.5f)
				{
					fWidth = 2.4f;
					param->mot_DataSave[nTrackID].fVDS_carLength = (param->mot_DataSave[nTrackID].fVDS_carTotalWidth - fWidth * cos(m_fLoopAngle)) / param->mot_DataSave[nTrackID].fVDS_cosAngle;
					nFixType = 67;
				}

				param->mot_DataSave[nTrackID].fVDS_carHeight = tanb * (fET_distanceY - fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
			}
		}


		if (nClassID == cls_truck)
		{
			int nIntial = 0;

			if (nFixType >= 63 && nFixType <= 67)
				nIntial = 100;

			if ((param->mot_DataSave[nTrackID].fVDS_carLength < 4.6f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 2.5f)
				|| (param->mot_DataSave[nTrackID].fVDS_carLength < 7.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.2f))
			{
				if (tanb != 0.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 68;

					bool bCheck = true;
					// check again
					if (param->mot_DataSave[nTrackID].fVDS_carLength < 8.0f)
						bCheck = false;
					else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 8.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f)
						param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
					else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 10.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 12.5f)
						param->mot_DataSave[nTrackID].fVDS_carHeight = 3.0f;
					else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 12.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 14.0f)
						param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
					else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 14.0f)
						param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;

					if (bCheck)
					{
						param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
						nFixType = nIntial + 69;
					}
				}
			}

			//// check if VDS_FIX_BASED_WIDTH is not worked
			// TRUCK (NO load???)
			if (param->mot_DataSave[nTrackID].fVDS_carLength > 8.5f && param->mot_DataSave[nTrackID].fVDS_carHeight < 1.5f /*적재함 지상고*/)
			{

				if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 2.0f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 71;
				}
				else if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength >= 10.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 12.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 72;
				}
				else if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength >= 12.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 14.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 3.0f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 73;
				}
				else if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength >= 14.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 74;
				}
				else
				{
					param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
					nFixType = nIntial + 75;
				}

				// check again
				if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength > 12.0f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 3.5f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 76;
				}
				else if (tanb != 0.0f && param->mot_DataSave[nTrackID].fVDS_carLength >= 14.0f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 3.5f)
				{
					param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 77;
				}
			}

			// TRUCK (HAVE load???)
			if (param->mot_DataSave[nTrackID].fVDS_carLength > 8.5f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 1.5f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 2.5f)
			{
				bool bCheck = true;
				param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
				if (tanb != 0.0f)
				{
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 81;
				}

				if (param->mot_DataSave[nTrackID].fVDS_carLength < 8.5f)
					bCheck = false;
				else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 8.5f && param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 3.0f;
				else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 10.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 12.5f)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
				else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 12.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 14.0f)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;
				else if (param->mot_DataSave[nTrackID].fVDS_carLength >= 14.0f)
					param->mot_DataSave[nTrackID].fVDS_carHeight = 4.5f;

				if (tanb != 0.0f && bCheck)
				{
					param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
					nFixType = nIntial + 82;
				}
			}
		}
	}

	// |=======================|
	// |===== TOTAL CHECK =====|
	// |=======================|

	// SEDAN - SUV
	if (nClassID == cls_car || nClassID == cls_ban)
	{
		if (param->mot_DataSave[nTrackID].fVDS_carLength < 3.82f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = 3.90f;
			nFixType = 11;
		}
		else if (param->mot_DataSave[nTrackID].fVDS_carLength > 8.5f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = 0.7f * param->mot_DataSave[nTrackID].fVDS_carLength;
			nFixType = 12;
		}
	}

	int nIntial = 0;
	if (nFixType >= 63 && nFixType <= 67)
		nIntial = 100;
	else if (nIntial > 100)
		nIntial = 200;

	// stranger length (too short)
	if (param->mot_DataSave[nTrackID].fVDS_carLength < 3.0f || param->mot_DataSave[nTrackID].fVDS_carLength > 0.95f * min(fStartDistance, fEndDistance))
	{
		param->mot_DataSave[nTrackID].fVDS_carLength = 0.75f * min(fStartDistance, fEndDistance);
		if (param->mot_DataSave[nTrackID].fVDS_carLength < 3.0f)
			param->mot_DataSave[nTrackID].fVDS_carLength = 3.1f;

		nFixType = nIntial + 13;
	}

	// 국내 최장 길이 BUS
	if (nClassID == cls_bus &&
		(param->mot_DataSave[nTrackID].fVDS_carLength > 12.5f || param->mot_DataSave[nTrackID].fVDS_carHeight < 2.8f
			|| (param->mot_DataSave[nTrackID].fVDS_carLength > 10.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.8f)
			|| (param->mot_DataSave[nTrackID].fVDS_carLength <= 10.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.2f)))
	{
		if ((param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f && param->mot_DataSave[nTrackID].fVDS_carHeight > 3.2f)
			|| param->mot_DataSave[nTrackID].fVDS_carHeight < 2.8f)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 3.2f;
		else
			param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;

		if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE && tana != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
		}
		else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE && tanb != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
		}

		param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 1.0f;

		if (param->mot_DataSave[nTrackID].fVDS_carLength > 12.5f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = 12.3f;
		}

		nFixType = 14;
	}

	// TRUCK
	if (nClassID == cls_truck)
	{
		if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 2.3f)
		{
			param->mot_DataSave[nTrackID].fVDS_carHeight = 2.3f;
			if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE && tana != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
			}
			else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE && tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
			}
			nFixType = nIntial + 15;
		}

		if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.5f && param->mot_DataSave[nTrackID].fVDS_carHeight >= 2.3f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.5f;
			nFixType = nIntial + 16;
		}

		if (param->mot_DataSave[nTrackID].fVDS_carLength <= 3.5f && param->mot_DataSave[nTrackID].fVDS_carHeight > 1.4f)
		{
			if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE && tana != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 0.8f;
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
			}
			else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE && tanb != 0.0f)
			{
				param->mot_DataSave[nTrackID].fVDS_carHeight = 1.1f;
				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
			}
			nFixType = nIntial + 17;

			if (param->mot_DataSave[nTrackID].fVDS_carLength > 5.6f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = 5.3f;
				nFixType = nIntial + 18;
			}
			else if (param->mot_DataSave[nTrackID].fVDS_carLength < 4.2f)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_carLength + 0.5f;
				nFixType = nIntial + 19;
			}
		}
	}



	if (param->mot_DataSave[nTrackID].fVDS_carLength > 14.0f || param->mot_DataSave[nTrackID].fVDS_carHeight >= 4.5f)
	{
		if (param->mot_DataSave[nTrackID].fVDS_carLength <= 7.0f)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 3.0f;
		else if (param->mot_DataSave[nTrackID].fVDS_carLength > 7.0f && param->mot_DataSave[nTrackID].fVDS_carLength < 10.0f)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
		else if (param->mot_DataSave[nTrackID].fVDS_carLength > 14.0f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 3.5f)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;
		else
			param->mot_DataSave[nTrackID].fVDS_carHeight = 4.5f;


		if (tana != 0.0f && param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
			nFixType = nIntial + 91;
		}
		else if (tanb != 0.0f && param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
			nFixType = nIntial + 92;
		}


		//check again
		if (param->mot_DataSave[nTrackID].fVDS_carLength > 14.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carHeight = 4.0f;
			if (tana != 0.0f && param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
				nFixType = nIntial + 93;
			}
			else if (tanb != 0.0f && param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
				nFixType = nIntial + 94;
			}
			else
			{
				param->mot_DataSave[nTrackID].fVDS_carLength = 0.9f * min(fStartDistance, fEndDistance);
				nFixType = nIntial + 95;
			}
		}

	}

	// for all have to be considered car
	if (param->mot_DataSave[nTrackID].fVDS_carLength > 8.0f && param->mot_DataSave[nTrackID].fVDS_carHeight <= 1.0f)
	{
		if (nClassID == cls_bus)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 3.5f;
		else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE && nClassID == cls_truck)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
		else
			param->mot_DataSave[nTrackID].fVDS_carHeight = 1.5f;

		if (min(fStartDistance, fEndDistance) == fStartDistance && tana != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
		}
		else if (min(fStartDistance, fEndDistance) == fEndDistance && tanb != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
		}

		nFixType = nIntial + 96;
	}
	else if (param->mot_DataSave[nTrackID].fVDS_carLength > 14.0f && param->mot_DataSave[nTrackID].fVDS_carHeight < 1.5f)
	{
		if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
			param->mot_DataSave[nTrackID].fVDS_carHeight = 2.5f;
		else
			param->mot_DataSave[nTrackID].fVDS_carHeight = 2.0f;

		if (min(fStartDistance, fEndDistance) == fStartDistance && tana != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = param->mot_DataSave[nTrackID].fVDS_ST_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tana) - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;
		}
		else if (min(fStartDistance, fEndDistance) == fEndDistance && tanb != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carLength = fET_distanceY - (param->mot_DataSave[nTrackID].fVDS_carHeight / tanb) - fEB_distanceY;
		}

		nFixType = nIntial + 97;
	}

	// FINAL CALCULATION
	if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_RIGHT_SIDE)
	{
		//param->mot_DataSave[nTrackID].fVDS_carHeight = tanb * (fET_distanceY - fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
		param->mot_DataSave[nTrackID].fVDS_carHeight = tana * (param->mot_DataSave[nTrackID].fVDS_ST_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
	}
	else if (param->mot_DataSave[nTrackID].nVDS_direction == VDS_LEFT_SIDE)
	{
		param->mot_DataSave[nTrackID].fVDS_carHeight = tanb * (fET_distanceY - fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_carLength);
	}

#endif
	return nFixType;
}

float CViolationProcess::VDS_calculate_carlength(st_det_mot_param *param, int nTrackID, float fET_distanceY, float fEB_distanceY)
{
	float fST_distanceY = param->mot_DataSave[nTrackID].fVDS_ST_distanceY; //D1
	float fSB_distanceY = param->mot_DataSave[nTrackID].fVDS_SB_distanceY; //D01

	// test
	/*fST_distanceY = 31.04f;
	fSB_distanceY = 8.54f;

	fET_distanceY = 23.31f;
	fEB_distanceY = 3.5f;
	*/

	// determine alpha and beta
	float tana = param->fCamHeight / (param->fLengthLow + fST_distanceY);
	float tanb = param->fCamHeight / (param->fLengthLow + fET_distanceY);

	// car length
	float fCarLength = 0.0f;

	if (tana - tanb != 0.0f)
	{
		fCarLength = (tana * (fST_distanceY - fSB_distanceY) - tanb * (fET_distanceY - fEB_distanceY)) / (tana - tanb);

		param->mot_DataSave[nTrackID].fVDS_carHeight = tanb * (fET_distanceY - fEB_distanceY - fCarLength);
	}

	return fCarLength;
}


void CViolationProcess::VDS_calculate_speed_occupancy(st_det_mot_param *param, CRegion* pRegion, int nTrackID, float fEB_distanceY, float fEB_distanceX, bool bUpdateOcpTime)
{
	// Real Loop length
	float fLoopLength = m_fLoopLength;

	if (!bUpdateOcpTime)
	{
		// calculate time to car travel the loop distance
		float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - param->mot_DataSave[nTrackID].nVDS_startTime) / (float)CLOCKS_PER_SEC;

		float fDeltaX = fEB_distanceX - param->mot_DataSave[nTrackID].fVDS_SB_distanceX;
		float fDeltaY = fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;

		if (m_bHorizontalCheck == false)
		{
			// disable horizontal distance calculation
			fDeltaX = 0.0f;
		}

		float fDiffDistance = sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

		// calculate speed
		if (fDiffTime != 0.0f)
		{
			param->mot_DataSave[nTrackID].fVDS_carSpeed = fDiffDistance / fDiffTime * 3.6f; //km/h
		}

		// time for loop length
		float fDiffTime_Loop = 0.0f;
		if (param->mot_DataSave[nTrackID].fVDS_carSpeed != 0.0f)
		{
			//fDiffTime_Loop = pRegion->fLoopLength  * 3.6f / param->mot_DataSave[nTrackID].fVDS_carSpeed; //seconds
			fDiffTime_Loop = fLoopLength * 3.6f / param->mot_DataSave[nTrackID].fVDS_carSpeed; //seconds
		}

		// calculate additional time to car travel the car length
		float fTimeA = 0.0f;
		if (param->mot_DataSave[nTrackID].fVDS_carSpeed != 0.0f && fDiffDistance != 0.0f)
		{
			fTimeA = param->mot_DataSave[nTrackID].fVDS_carLength * fDiffTime / fDiffDistance; //seconds
		}

		// occupancy time
		//param->mot_DataSave[nTrackID].fVDS_ocpTime = fDiffTime + fTimeA;
		param->mot_DataSave[nTrackID].fVDS_ocpTime = fDiffTime_Loop + fTimeA; //seconds

	}
	else
	{
		if (param->mot_DataSave[nTrackID].fVDS_carSpeed != 0.0f)
		{
			//param->mot_DataSave[nTrackID].fVDS_ocpTime = (pRegion->fLoopLength + param->mot_DataSave[nTrackID].fVDS_carLength)  * 3.6f / param->mot_DataSave[nTrackID].fVDS_carSpeed;
			param->mot_DataSave[nTrackID].fVDS_ocpTime = (fLoopLength + param->mot_DataSave[nTrackID].fVDS_carLength)  * 3.6f / param->mot_DataSave[nTrackID].fVDS_carSpeed;
		}
		else
		{
			param->mot_DataSave[nTrackID].fVDS_ocpTime = 0.0f;
		}
	}
}

void CViolationProcess::VDS_calculate_vehicleGap(CRegion* pRegion, st_det_mot_param *param, int ntrack, float fEB_distanceY, bool bUpdateSpeed)
{
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	int nObjectLane = param->mot_DataSave[nTrackID].nVDS_laneNum;
	int nObjectDirect = param->mot_DataSave[nTrackID].nVDS_direction;

	if (nObjectLane < 0 || nObjectDirect <= 0)
		return;

	sRoadbox* check_roadbox = (nObjectDirect == VDS_LEFT_SIDE) ? pRegion->l_box : pRegion->r_box;
	int nIndex = (nObjectDirect == VDS_RIGHT_SIDE) ? (pRegion->l_box_max - nObjectLane) : (nObjectLane - 1);

	if (!bUpdateSpeed)
	{
		//clock_t currentClock = clock();

		if (check_roadbox[nIndex].gapData.hasGap)
		{
			float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - check_roadbox[nIndex].gapData.endTime) / (float)CLOCKS_PER_SEC;

			if (fDiffTime < 180) // less than 180 seconds - 3 minutes
			{
				float fCarDistance = check_roadbox[nIndex].gapData.fSpeed * fDiffTime / 3.6f;	//meters
				// use Gap
				//param->mot_DataSave[nTrackID].nVDS_vehicleGap = max(0, (int)(fCarDistance - check_roadbox[i].gapData.fLength));
				// use headway
				param->mot_DataSave[nTrackID].nVDS_vehicleGap = max(0, (int)round(fCarDistance * 100.0f));	//centimeters
			}
			else
			{
				param->mot_DataSave[nTrackID].nVDS_vehicleGap = 0;
			}
		}

		// update gap for next car of this lane
		check_roadbox[nIndex].gapData.hasGap = 1;
		check_roadbox[nIndex].gapData.endTime = param->mot_DataSave[nTrackID].nVDS_endTime;
		check_roadbox[nIndex].gapData.fLength = param->mot_DataSave[nTrackID].fVDS_carLength;
		check_roadbox[nIndex].gapData.fSpeed = param->mot_DataSave[nTrackID].fVDS_carSpeed;
	}
	else
	{
		check_roadbox[nIndex].gapData.fLength = param->mot_DataSave[nTrackID].fVDS_carLength;
		check_roadbox[nIndex].gapData.fSpeed = param->mot_DataSave[nTrackID].fVDS_carSpeed;
	}

}

void CViolationProcess::VDS_reverse_violation_analysis(int nProcessStep, float fEB_distanceY, DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox,
	int ntrack, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;

#if ENGINE_MODE == 0

	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	char full_save_image_path[CHAR_LENGTH], textTmp[CHAR_LENGTH];

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtRealBottomX = rtBoundingBox.x;

	int nDirection = pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, 0);

	cv::Rect fullRect = rtBoundingBox;

	if (fullRect.height <= 6 * fullRect.width && fullRect.width <= 3 * fullRect.height)
	{
		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			// process reverse
			switch (param->nReverseDirection)
			{
			case VDS_LEFTENTRY_RIGHTEXIT:	//LeftEntry
			{
				// left lanes 						
				if (nDirection == VDS_LEFT_SIDE)
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed > VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
				// right lanes
				else if (nDirection == VDS_RIGHT_SIDE)
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
			}
			break;
			case VDS_LEFTEXIT_RIGHTENTRY:	//LeftExit
			{
				// left lane 						
				if (nDirection == VDS_LEFT_SIDE)
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
				// right lane
				else if (nDirection == VDS_RIGHT_SIDE)
				{
					if (param->mot_DataSave[nTrackID].fXYCOOR_speed > VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					else
						param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
				}
			}
			break;
			case VDS_BOTH_ENTRY:			//both entry
			{
				if (param->mot_DataSave[nTrackID].fXYCOOR_speed > VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
					param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
				else
					param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
			}
			break;
			case VDS_BOTH_EXIT:				//both exit
			{
				if (param->mot_DataSave[nTrackID].fXYCOOR_speed < (-1) * VDS_REVERSE_MIN_SPEED && param->mot_DataSave[nTrackID].fXYCOOR_speed_display >= 0.0f)
					param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
				else
					param->mot_ReverseSave[nTrackID].nReverseResetCnt++;
			}
			break;
			default:
				break;
			}

			// store the first proof image 
			if (nProcessStep == 1) //in case of [LOOP-START] 
			{
				if (param->mot_ReverseSave[nTrackID].bStoreImage == 0)
				{
					param->mot_ReverseSave[nTrackID].bStoreImage = 1;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}
					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}
			}
			else //in general cases
			{
				if (param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].bStoreImage == 0)
				{
					param->mot_ReverseSave[nTrackID].bStoreImage = 1;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}
					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}
			}

			if (nProcessStep != 2) //in general cases
			{
				// reset checked information
				if (param->mot_ReverseSave[nTrackID].nReverseResetCnt >= VDS_REVERSE_RESETCNT)
				{
					darknet.release_reverse_violation(param, nTrackID);
				}
			}
			else //in case of [LOOP-END] - HAVE TO analysis reverse violation before transfer data
			{
				switch (param->nReverseDirection)
				{
				case VDS_LEFTENTRY_RIGHTEXIT:	//LeftEntry
				{
					// left lanes 						
					if (nDirection == VDS_LEFT_SIDE)
					{
						if (fEB_distanceY > param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
					}
					// right lanes
					else if (nDirection == VDS_RIGHT_SIDE)
					{
						if (fEB_distanceY < param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
					}
				}
				break;
				case VDS_LEFTEXIT_RIGHTENTRY:	//LeftExit
				{
					// left lane 						
					if (nDirection == VDS_LEFT_SIDE)
					{
						if (fEB_distanceY < param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
					}
					// right lane
					else if (nDirection == VDS_RIGHT_SIDE)
					{
						if (fEB_distanceY > param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
							param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
					}
				}
				break;
				case VDS_BOTH_ENTRY:			//both entry
				{
					if (fEB_distanceY > param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
				}
				break;
				case VDS_BOTH_EXIT:				//both exit
				{
					if (fEB_distanceY < param->mot_DataSave[nTrackID].fVDS_SB_distanceY)
						param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;
				}
				break;
				default:
					break;
				}
			}

			// violation confirmed
			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt >= VDS_REVERSE_CONFIRMCNT)
			{
				param->mot_ReverseSave[nTrackID].nReverseVioCnt = VDS_REVERSE_CONFIRMCNT;

				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
				{
					//save image and log
					param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					param->mot_ReverseSave[nTrackID].nReverseResetCnt = 0;

#if PROOF_REVERSE_IMAGE_DEBUG == 1
					//1st image
					sprintf(full_save_image_path, "%s/VDS_data/images/reverse_%05d_%s_%s_%03d_01.jpg", currentPath, nTotalCount, strDate, strTime, nTrackID);
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

					//2nd image 
					sprintf(full_save_image_path, "%s/VDS_data/images/reverse_%05d_%s_%s_%03d_02.jpg", currentPath, nTotalCount, strDate, strTime, nTrackID);

					if (!clone_frame.empty())
					{
						clone_frame = frame.clone();

						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{

						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}
#endif

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					//use to release
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
				}
			}

			//20190130 hai for Overlay Display
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
			{
				param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
				//violation_text_display(res_frame, param, ntrack, nTrackID);
				darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
			}


			// ===== [release violation info] =====
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
			{
				int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);
				//std::cout << "TimeOut ===============: " << seconds << std::endl;

				if (seconds >= VDS_EVENT_RELEASE_PERIOD) //xx초 초과
				{
					darknet.release_reverse_violation(param, nTrackID);
				}
			}
		}
	}
#endif
}


#if SPEED_KOREA_EXPRESSWAY_MODE == 1

#ifdef XYCOOR_MODE 
void CViolationProcess::get_speed_list(st_det_mot_param * param, int nTrackID, float* fSpeedList, float* fTimeList, bool bCenter)
{
	for (int i = 0; i < 3; i++)
	{
		if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > i)
		{
			fSpeedList[i] = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - i - 1];
			if (bCenter == true)
			{
				param->mot_DataSave[nTrackID].fDistance_Center_end[i] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - i - 1];
				if (param->mot_DataSave[nTrackID].track_rect.size() > i + param->n_speed_period)
					param->mot_DataSave[nTrackID].fDistance_Center_start[i] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - i - param->n_speed_period];
				fTimeList[i] = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime[param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.size() - i - 1];

				if (i >= 1)
				{
					if (param->mot_DataSave[nTrackID].track_rect.size() > i + param->n_speed_period)
					{
						param->mot_DataSave[nTrackID].image_pairs[i - 1].img[0] = param->mot_DataSave[nTrackID].track_image[param->mot_DataSave[nTrackID].track_image.size() - i - param->n_speed_period].clone();
						param->mot_DataSave[nTrackID].image_pairs[i - 1].img[1] = param->mot_DataSave[nTrackID].track_image[param->mot_DataSave[nTrackID].track_image.size() - i - 1].clone();

						param->mot_DataSave[nTrackID].image_pairs[i - 1].rect[0] = param->mot_DataSave[nTrackID].track_rect[param->mot_DataSave[nTrackID].track_rect.size() - i - param->n_speed_period];
						param->mot_DataSave[nTrackID].image_pairs[i - 1].rect[1] = param->mot_DataSave[nTrackID].track_rect[param->mot_DataSave[nTrackID].track_rect.size() - i - 1];

						param->mot_DataSave[nTrackID].image_pairs[i - 1].plate_rect[0] = param->mot_DataSave[nTrackID].track_plate_rect[param->mot_DataSave[nTrackID].track_plate_rect.size() - i - param->n_speed_period];
						param->mot_DataSave[nTrackID].image_pairs[i - 1].plate_rect[1] = param->mot_DataSave[nTrackID].track_plate_rect[param->mot_DataSave[nTrackID].track_plate_rect.size() - i - 1];

						param->mot_DataSave[nTrackID].fDis_plateStart[i - 1].fDis_plate_bot = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - i - param->n_speed_period].fDis_plate_bot;
						param->mot_DataSave[nTrackID].fDis_plateStart[i - 1].fDis_plate_top = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - i - param->n_speed_period].fDis_plate_top;

						param->mot_DataSave[nTrackID].fDis_plateEnd[i - 1].fDis_plate_bot = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - i - 1].fDis_plate_bot;
						param->mot_DataSave[nTrackID].fDis_plateEnd[i - 1].fDis_plate_top = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - i - 1].fDis_plate_top;
					}
				}
			}
		}
	}

	if (bCenter)
	{
		if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 3)
		{
			fSpeedList[3] = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - 4];
			param->mot_DataSave[nTrackID].fDistance_Center_end[3] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - 4];
			fTimeList[3] = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime[param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.size() - 4];

			if (param->mot_DataSave[nTrackID].track_rect.size() > 3 + param->n_speed_period)
			{
				param->mot_DataSave[nTrackID].fDistance_Center_start[3] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - 3 - param->n_speed_period];

				param->mot_DataSave[nTrackID].image_pairs[2].img[0] = param->mot_DataSave[nTrackID].track_image[param->mot_DataSave[nTrackID].track_image.size() - 3 - param->n_speed_period].clone();
				param->mot_DataSave[nTrackID].image_pairs[2].img[1] = param->mot_DataSave[nTrackID].track_image[param->mot_DataSave[nTrackID].track_image.size() - 4].clone();

				param->mot_DataSave[nTrackID].image_pairs[2].rect[0] = param->mot_DataSave[nTrackID].track_rect[param->mot_DataSave[nTrackID].track_rect.size() - 3 - param->n_speed_period];
				param->mot_DataSave[nTrackID].image_pairs[2].rect[1] = param->mot_DataSave[nTrackID].track_rect[param->mot_DataSave[nTrackID].track_rect.size() - 4];

				param->mot_DataSave[nTrackID].image_pairs[2].plate_rect[0] = param->mot_DataSave[nTrackID].track_plate_rect[param->mot_DataSave[nTrackID].track_plate_rect.size() - 3 - param->n_speed_period];
				param->mot_DataSave[nTrackID].image_pairs[2].plate_rect[1] = param->mot_DataSave[nTrackID].track_plate_rect[param->mot_DataSave[nTrackID].track_plate_rect.size() - 4];

				param->mot_DataSave[nTrackID].fDis_plateStart[2].fDis_plate_bot = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - 3 - param->n_speed_period].fDis_plate_bot;
				param->mot_DataSave[nTrackID].fDis_plateStart[2].fDis_plate_top = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - 3 - param->n_speed_period].fDis_plate_top;

				param->mot_DataSave[nTrackID].fDis_plateEnd[2].fDis_plate_bot = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - 4].fDis_plate_bot;
				param->mot_DataSave[nTrackID].fDis_plateEnd[2].fDis_plate_top = param->mot_DataSave[nTrackID].fXYCOOR_distancePlate[param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.size() - 4].fDis_plate_top;
			}

		}
		if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 4)
		{
			fSpeedList[4] = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - 5];
			param->mot_DataSave[nTrackID].fDistance_Center_end[4] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - 5];
			if (param->mot_DataSave[nTrackID].track_rect.size() > 3 + param->n_speed_period)
				param->mot_DataSave[nTrackID].fDistance_Center_start[4] = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() - 4 - param->n_speed_period];
			fTimeList[4] = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime[param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.size() - 5];
		}
	}
}

CString CViolationProcess::XYCOOR_calculate_speed_simple(cv::Mat & res_image, DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, int ntrack, float fHratio, float fWratio, unsigned long long timestamp)
{
	if (pRegion == nullptr) return _T("nothing");

	clock_t currentClock = clock();
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);
	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH];
	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtBottomX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);

	int nPush = -1;
	CString strSpeed;

	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\image_tracking", currentPath);
	CreateDirectory(makePath, nullptr);

	int nGapY = 20;

	if (box_iou.total > 0.5f)
	{
		// calculate distance
		float fDistanceY, fDistanceX;

		pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPtBottomX, nPtBottomY), fDistanceY, fDistanceX);

		if (nClassID != cls_bike)
			fDistanceY = fDistanceY * (param->fCamHeight - FIX_HEIGHT) / param->fCamHeight;

		float fDistance_plateTop = 0.0f, fDistance_plateBot = 0.0f, fDistance_plateTopX, fDistance_plateBotX;

		// plate distances
		if (param->mot_info->mot_box_info[ntrack].object_info.plate.id > 0)
		{
			cv::Rect plate_rect = param->mot_info->mot_box_info[ntrack].object_info.plate.rect;
			int nPlateTopY = (int)round((float)(plate_rect.y) / fHratio);
			int nPlateBotY = (int)round((float)(plate_rect.y + plate_rect.height) / fHratio);
			int nPlateX = (int)round((float)(plate_rect.x + (float)plate_rect.width / 2.0f) / fWratio);

			pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPlateX, nPlateTopY), fDistance_plateTop, fDistance_plateTopX);
			pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPlateX, nPlateBotY), fDistance_plateBot, fDistance_plateBotX);
		}

		if (param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() < param->n_speed_period * 2 && nPtBottomY < param->nDisplayH - nGapY)
		{
			/*
			if (param->mot_DataSave[nTrackID].track_image.size() > 0)
			{
				cv::Mat diff, out;
				cv::Mat mask(res_image.rows, res_image.cols, CV_8U, cv::Scalar(0));

				rectangle(mask, rtResBox, cv::Scalar(255), -1, LINE_8);
				//imshow("frame difference", mask);

				//cv::medianBlur(res_image, mean_image2, 3);
				absdiff(res_image, param->mot_DataSave[nTrackID].track_image[param->mot_DataSave[nTrackID].track_image.size() - 1], diff);
				cvtColor(diff, diff, COLOR_RGB2GRAY);
				threshold(diff, diff, 50, 255, THRESH_BINARY);
				cv::bitwise_and(diff, diff, out, mask);
				cvtColor(out, out, COLOR_GRAY2RGB);
				rectangle(out, rtResBox, cv::Scalar(255, 0, 0), 2, LINE_8);
				imshow("frame difference", out);
				cv::waitKey(1);
			}
			*/

			param->mot_DataSave[nTrackID].nXYCOOR_distanceY.push_back(fDistanceY);
			param->mot_DataSave[nTrackID].nXYCOOR_distanceX.push_back(fDistanceX);

			//param->mot_DataSave[nTrackID].nXYCOOR_time.push_back(currentClock);
			param->mot_DataSave[nTrackID].nXYCOOR_time.push_back(timestamp);
			param->mot_DataSave[nTrackID].track_image.push_back(res_image.clone());
			param->mot_DataSave[nTrackID].track_rect.push_back(rtResBox);

			//plate
			st_plate_dis plate_dis;
			if (fDistance_plateTop > 0.0f && fDistance_plateBot > 0.0f)
			{
				plate_dis.fDis_plate_top = fDistance_plateTop;
				plate_dis.fDis_plate_bot = fDistance_plateBot;
				param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.push_back(plate_dis);
				param->mot_DataSave[nTrackID].track_plate_rect.push_back(param->mot_info->mot_box_info[ntrack].object_info.plate.rect);
			}
			else
			{
				plate_dis.fDis_plate_top = 0.0f;
				plate_dis.fDis_plate_bot = 0.0f;
				param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.push_back(plate_dis);
				param->mot_DataSave[nTrackID].track_plate_rect.push_back(cv::Rect(0, 0, 0, 0));
			}

		}

		//calculate current speed
		if (param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size() >= param->n_speed_period)
		{
			int nListSize = param->mot_DataSave[nTrackID].nXYCOOR_distanceY.size();

			float fDiffTime = (float)(param->mot_DataSave[nTrackID].nXYCOOR_time[nListSize - 1] - param->mot_DataSave[nTrackID].nXYCOOR_time[nListSize - param->n_speed_period]) / (float)CLOCKS_PER_SEC;

			//float fDiffTime = float(SPEED_STATS_PERIOD - 1) / 15.0f;

			//strSpeed.Format(_T("%s final: %llu begin: %llu"), strSpeed, param->mot_DataSave[nTrackID].nXYCOOR_time[SPEED_STATS_PERIOD - 1], param->mot_DataSave[nTrackID].nXYCOOR_time[0]);

			float fDeltaX = param->mot_DataSave[nTrackID].nXYCOOR_distanceX[nListSize - 1] - param->mot_DataSave[nTrackID].nXYCOOR_distanceX[nListSize - param->n_speed_period];
			float fDeltaY = param->mot_DataSave[nTrackID].nXYCOOR_distanceY[nListSize - 1] - param->mot_DataSave[nTrackID].nXYCOOR_distanceY[nListSize - param->n_speed_period];

			if (m_bHorizontalCheck == false)
			{
				// disable horizontal distance calculation
				fDeltaX = 0.0f;
			}

			float fDiffDistance = 0.0f;
			if (fDeltaY < 0.0f)
				fDiffDistance = 0 - sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);
			else
				fDiffDistance = sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

			if (fDiffTime != 0.0f)
			{
				param->mot_DataSave[nTrackID].fXYCOOR_speed = fDiffDistance / fDiffTime * 3.6f;
				param->mot_DataSave[nTrackID].fXYCOOR_speed_display = abs(param->mot_DataSave[nTrackID].fXYCOOR_speed);
				param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = fDiffTime;

				//XYCOOR_save_speed_log(strDate, strTime, nTrackID, fDeltaX, fDeltaY, param->mot_DataSave[nTrackID].fXYCOOR_speed, nPtBottomX, nPtBottomY, fDistanceX, fDistanceY);
			}
			else
			{
				param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
				param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
				param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
			}

			if (m_nSaveDebugImages)
			{
				/*
				wsprintf(makePath, "%s\\VDS_data\\image_tracking\\%d", currentPath, nTrackID);
				CreateDirectory(makePath, nullptr);


				static int cnt = 0;
				//save image
				sprintf(full_save_image_info, "%s/VDS_data/image_tracking/%d/%s_%s_%d_%d_%d",
					currentPath,
					nTrackID,
					strDate, strTime,
					nTrackID,
					cnt++,
					abs((int)param->mot_DataSave[nTrackID].fXYCOOR_speed_display));

				sprintf(full_save_image_path, "%s.jpg", full_save_image_info);
				cv::Mat clone_frame = res_image.clone();
				darknet.draw_objectbox_forLPR(clone_frame, rtBoundingBox, Scalar(0, 0, 255), false);
				SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
				//
				*/
			}

			//erase the first element
			if (nListSize >= param->n_speed_period * 2)
			{
				param->mot_DataSave[nTrackID].nXYCOOR_distanceY.erase(param->mot_DataSave[nTrackID].nXYCOOR_distanceY.begin());
				param->mot_DataSave[nTrackID].nXYCOOR_distanceX.erase(param->mot_DataSave[nTrackID].nXYCOOR_distanceX.begin());

				param->mot_DataSave[nTrackID].nXYCOOR_time.erase(param->mot_DataSave[nTrackID].nXYCOOR_time.begin());
				param->mot_DataSave[nTrackID].track_image.erase(param->mot_DataSave[nTrackID].track_image.begin());
				param->mot_DataSave[nTrackID].track_rect.erase(param->mot_DataSave[nTrackID].track_rect.begin());

				param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.erase(param->mot_DataSave[nTrackID].fXYCOOR_distancePlate.begin());
				param->mot_DataSave[nTrackID].track_plate_rect.erase(param->mot_DataSave[nTrackID].track_plate_rect.begin());
			}
		}
		else
		{
			param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
			param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
			param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
			//XYCOOR_save_speed_log(strDate, strTime, nTrackID, 0.0f, 0.0f, param->mot_DataSave[nTrackID].fXYCOOR_speed, nPtBottomX, nPtBottomY, fDistanceX, fDistanceY);
		}
	}
	else
	{
		param->mot_DataSave[nTrackID].fXYCOOR_speed = -1.0f;
		param->mot_DataSave[nTrackID].fXYCOOR_speed_display = -1.0f;
		param->mot_DataSave[nTrackID].fXYCOOR_deltaTime = 0.0f;
	}

	//strSpeed.Format(_T("timestamp: %llu"), timestamp);
	//store prev calculated speed
	param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.push_back(param->mot_DataSave[nTrackID].fXYCOOR_speed_display);
	param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.push_back(param->mot_DataSave[nTrackID].fXYCOOR_deltaTime);

	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() >= param->n_speed_period * 2)
	{
		param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.erase(param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.begin());
		param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.erase(param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.begin());
	}

	return strSpeed;
}

#endif

CString CViolationProcess::KEW_application(DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox,
	CArray<int, int&> &nVioCntArray, int ntrack, float fHratio, float fWratio, unsigned long long timestamp, int nCamIndex)
{
	CString strLog;

	if (pRegion == nullptr)
	{
		strLog.Format("Region is NULL");
		return strLog;
	}
	cv::Point ptRoad[MAX_PTROAD];
	pRegion->CRegion_copy(4, ptRoad);
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);

	char currentPath[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());

	char full_save_image_path[CHAR_LENGTH];

	clock_t currentClock = clock();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	if (darknet.is_under_consideration_vehicle(nClassID) == false)
		return _T("");

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;
	int nPtBottomY = (int)round((float)(rtBoundingBox.y + (float)rtBoundingBox.height) / fHratio);
	int nPtTopY = (int)round((float)rtBoundingBox.y / fHratio);
	int nPtCenterX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width / 2.0f) / fWratio);
	int nPtLeftX = (int)round((float)rtBoundingBox.x / fWratio);
	int nPtRightX = (int)round(((float)rtBoundingBox.x + (float)rtBoundingBox.width) / fWratio);

	int nPtProcessX = 0;
	if (nPtCenterX < pRegion->m_ptVanish[0].x)
		nPtProcessX = nPtLeftX;
	else
		nPtProcessX = nPtRightX;


	// default: get center x value to process

#if LINE_DETERMINE == 0
	int nPtBottomX = nPtCenterX;
	int nPtTopX = nPtCenterX;
#else
	int nPtBottomX = nPtProcessX;
	int nPtTopX = pRegion->CRegion_get_XTop(nPtBottomX, nPtBottomY, nPtTopY);
#endif

	int nProcessStep = 0; //[general - not started, not ended]

	// [START LOOP]
	if (param->mot_DataSave[nTrackID].bVDS_started == TRUE && !param->mot_DataSave[nTrackID].bVDS_started_setdone/* && param->mot_DataSave[nTrackID].nVDS_startTime == -1*/)
	{
		param->mot_DataSave[nTrackID].bVDS_started_setdone = TRUE;
		//param->mot_DataSave[nTrackID].nVDS_startTime = currentClock;

		// calculate distance at start time
		// bottom
		nPtBottomX = nPtCenterX;
		pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPtBottomX, nPtBottomY), param->mot_DataSave[nTrackID].fVDS_SB_distanceY, param->mot_DataSave[nTrackID].fVDS_SB_distanceX);

		// top
		pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPtTopX, nPtTopY), param->mot_DataSave[nTrackID].fVDS_ST_distanceY, param->mot_DataSave[nTrackID].fVDS_ST_distanceX);

		//// store start image
		//if (m_nSaveDebugImages)
		//{
		//	if (!param->mot_DataSave[nTrackID].iVDS_startImg.empty())
		//	{
		//		param->mot_DataSave[nTrackID].iVDS_startImg.release();
		//	}
		//	param->mot_DataSave[nTrackID].iVDS_startImg = frame.clone();
		//	darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].iVDS_startImg, rtBoundingBox, param->vio_color[TRACKING_CLR], false);
		//}
		nProcessStep = 1; //[LOOP-START]
	}


	// [CENTER LOOP]

	if (param->mot_DataSave[nTrackID].bVDS_center == TRUE && param->mot_DataSave[nTrackID].nVDS_center_setdone == 0)
	{
		param->mot_DataSave[nTrackID].nVDS_center_setdone = 1;

		if (m_nSaveDebugImages)
		{
			if (!param->mot_DataSave[nTrackID].iVDS_startImg.empty())
			{
				param->mot_DataSave[nTrackID].iVDS_startImg.release();
			}
			param->mot_DataSave[nTrackID].iVDS_startImg = frame.clone();
			darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].iVDS_startImg, rtBoundingBox, param->vio_color[TRACKING_CLR], false);
		}

	}
	else if (param->mot_DataSave[nTrackID].nVDS_center_setdone >= 1/* && param->mot_DataSave[nTrackID].fSpeed_Center[0] < 0.0f*/)
	{
		param->mot_DataSave[nTrackID].nVDS_center_setdone++;
		if (param->mot_DataSave[nTrackID].nVDS_center_setdone == 3)
		{
			get_speed_list(param, nTrackID, param->mot_DataSave[nTrackID].fSpeed_Center, param->mot_DataSave[nTrackID].fTime_Center, true);

			char img_name[500];
			strLog = SSM_save_log(darknet, pRegion, frame, param, ntrack, 0.0f, 0.0f, 0.0f, 0.0f, nCamIndex, img_name);

			save_all_info_forLPDR(darknet, frame, res_frame, clone_frame, param, ntrack, nCamIndex, img_name);
		}
	}

	// [END LOOP]
	float fET_distanceY = 0.0f, fET_distanceX = 0.0f;	//top
	float fEB_distanceY = 0.0f, fEB_distanceX = 0.0f;	//bottom

	// determine end time, end distance, speed, length, occupancy time

	if (param->mot_DataSave[nTrackID].bVDS_started == TRUE && param->mot_DataSave[nTrackID].bVDS_ended == TRUE && !param->mot_DataSave[nTrackID].bVDS_ended_setdone/*&& param->mot_DataSave[nTrackID].nVDS_endTime == -1*/)
	{
		param->mot_DataSave[nTrackID].bVDS_ended_setdone = TRUE;
		//param->mot_DataSave[nTrackID].nVDS_endTime = currentClock;

		// bottom
		pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPtBottomX, nPtBottomY), fEB_distanceY, fEB_distanceX);

		// top
		pRegion->CRegion_get_distance_simple(pCamPosition, param->nDisplayH, cv::Point(nPtTopX, nPtTopY), fET_distanceY, fET_distanceX);

		//// end speed
		//get_speed_list(param, nTrackID, false);

		// direction
		param->mot_DataSave[nTrackID].nVDS_direction = pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, 0);

		// calculate speed and occupancy time
		SSM_calculate_speed(param, nTrackID, fEB_distanceY, fEB_distanceX);

		// check car info
		//SSM_save_log_checkinfo(param, nTrackID, nClassID, fET_distanceY, fEB_distanceY, fET_distanceX, fEB_distanceX, nCamIndex);

	}
	else if (param->mot_DataSave[nTrackID].bVDS_ended_setdone == true && param->mot_DataSave[nTrackID].nVDS_pushStatus == PUSH_STANDBY)
	{
		param->mot_DataSave[nTrackID].nVDS_pushStatus = PUSH_PREPARED; // already to push
	}

	return strLog;
}


void CViolationProcess::save_all_info_forLPDR(DarknetData &darknet, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int ntrack, int nCamIndex, char* img_name)
{
	char* vio_file_path = param->vio_result_path;

	char makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH], lpr_save_image_path[MAX_PATH];
	wsprintf(makePath, "%s/lpr/mot/", vio_file_path);
	CreateDirectory(makePath, nullptr);

	// show all info on LPR dialog
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	int nPadY = 100, nPadX = 100;
	//[full image]
	sprintf(full_save_image_path, "%s/%s.jpg", makePath, img_name);
	cv::Mat full_img = clone_frame.clone();
	darknet.draw_objectbox_forLPR(full_img, param->mot_info->mot_box_info[ntrack].bouding_box, param->vio_color[TRACKING_CLR], false);
	cv::Mat lpr_img;
	int nPlateType = param->mot_info->mot_box_info[ntrack].object_info.plate.id;
	
	cv::Rect plateRt = param->mot_info->mot_box_info[ntrack].object_info.plate.rect;
	plateRt.x = MAX(0, plateRt.x - 3);
	plateRt.y = MAX(0, plateRt.y - 3);
	int nMaxC = MIN(plateRt.x + plateRt.width + 6, frame.cols - 1);
	int nMaxR = MIN(plateRt.y + plateRt.height + 6, frame.rows - 1);
	plateRt.width = nMaxC - plateRt.x;
	plateRt.height = nMaxR - plateRt.y;

	if (param->mot_info->mot_box_info[ntrack].object_info.plate.id > 0)
	{
		lpr_img = full_img(plateRt);
	}
	else
	{
		lpr_img = cv::Mat(50, 150, CV_8UC3, Scalar(0, 0, 0));
		nPlateType = 100;
	}

	if (param->l_box_max == 2)
	{
		cv::Rect full_crop;
		full_crop.y = MAX(0, param->mot_info->mot_box_info[ntrack].bouding_box.y - nPadY);
		int nMaxRow = full_crop.y + param->mot_info->mot_box_info[ntrack].bouding_box.height + 2 * nPadY;
		if (nMaxRow < frame.rows)
			full_crop.height = nMaxRow - full_crop.y;
		else
			full_crop.height = frame.rows - full_crop.y - 1;

		int nIndex = param->l_box_max - param->mot_DataSave[nTrackID].nVDS_laneNum;
		if (nIndex == 1) //lane 1
		{
			full_crop.x = MAX(0, MIN(param->l_box[nIndex].point[0].x, param->l_box[nIndex].point[2].x) * m_fWratio - nPadX);
			full_crop.width = frame.cols - full_crop.x - 1;
		}
		else // lane 2
		{
			full_crop.width = MIN(frame.cols, MAX(param->l_box[nIndex].point[1].x, param->l_box[nIndex].point[3].x) * m_fWratio + nPadX);
			full_crop.x = 0;
		}

		full_img = full_img(full_crop);
	}
	else if (param->l_box_max == 1)
	{
		cv::Rect full_crop;
		full_crop.y = MAX(0, param->mot_info->mot_box_info[ntrack].bouding_box.y - nPadY);
		int nMaxRow = full_crop.y + param->mot_info->mot_box_info[ntrack].bouding_box.height + 2 * nPadY;
		if (nMaxRow < frame.rows)
			full_crop.height = nMaxRow - full_crop.y;
		else
			full_crop.height = frame.rows - full_crop.y - 1;

		full_crop.width = MIN(frame.cols, MAX(param->l_box[0].point[1].x, param->l_box[0].point[3].x) * m_fWratio + nPadX);
		full_crop.x = 0;

		full_img = full_img(full_crop);
	}

	SavePicture_Save(hSaveImageDll, full_save_image_path, &full_img, 0, 1);

	//[lpr image]
	wsprintf(makePath, "%s/lpr/input/", vio_file_path);
	CreateDirectory(makePath, nullptr);
	sprintf(lpr_save_image_path, "%s/%s_%d.jpg", makePath, img_name, nPlateType - cls_lpd_1);
	SavePicture_Save(hSaveImageDll, lpr_save_image_path, &lpr_img, 0, 1);


}


void CViolationProcess::SSM_calculate_speed(st_det_mot_param *param, int nTrackID, float fEB_distanceY, float fEB_distanceX)
{
	// calculate time to car travel the loop distance
	float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - param->mot_DataSave[nTrackID].nVDS_startTime) / (float)CLOCKS_PER_SEC;

	param->mot_DataSave[nTrackID].fVDS_ocpTime = fDiffTime;

	float fDeltaX = fEB_distanceX - param->mot_DataSave[nTrackID].fVDS_SB_distanceX;
	float fDeltaY = fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY;

	if (m_bHorizontalCheck == false)
	{
		// disable horizontal distance calculation
		fDeltaX = 0.0f;
	}

	float fDiffDistance = sqrt(fDeltaX * fDeltaX + fDeltaY * fDeltaY);

	// calculate speed
	if (fDiffTime != 0.0f)
	{
		param->mot_DataSave[nTrackID].fVDS_carSpeed = fDiffDistance / fDiffTime * 3.6f; //km/h
	}
}

float CViolationProcess::get_decided_plate_speed(st_det_mot_param *param, int nTrackID, int nClassID)
{
	// alpha1, alpha2: angles of plate (top.bot) at start position 
	// beta1, beta2: angles of plate (top.bot) at end position 
	// La1, La2: real distance based angles of top and bot of plate at start
	// Lb1, Lb2: real distance based angles of top and bot of plate at end
	// H1 : real height of plate's top
	// H2 : real height of plate's bot
	// La : real distance of vehicle at start
	// Lb : real distance of vehicle at end

	// tg(alpha1) = H1 / (La1 - La) = Hcam / La1
	// tg(alpha2) = H2 / (La2 - La) = Hcam / La2
	// tg(beta1) = H1 / (Lb1 - Lb) = Hcam / Lb1
	// tg(beta2) = H1 / (Lb2 - Lb) = Hcam / Lb2

	float H2 = 0.4;// , H_fix = 0.2f;
	if (nClassID == cls_truck)
		H2 = 0.45;

	for (int i = 0; i < 3; i++)
	{
		if (param->mot_DataSave[nTrackID].fTime_Center[i + 1] > 0.0f && param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot > 0.0f && param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot > 0.0f)
		{
			// La / Lb = La1 / Lb1 = La2 / Lb2
			// La = Lb * La2 / Lb2 = Lb * La1 / Lb1
			float La_top = 0.0f, La_bot = 0.0f, La = 0.0f;
			float Lb_top = 0.0f, Lb_bot = 0.0f, Lb = 0.0f;
			float fDiffDistance_fix_start = 0.0f, fDiffDistance_fix_end = 0.0f;

			// assume that Lb is known as fDistance_Center_end -> calculate La
			if (param->mot_DataSave[nTrackID].fDistance_Center_end[i + 1] > 0.0f)
			{
				//float H2 = 0.4f;
				Lb = param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot * (param->fCamHeight - H2) / param->fCamHeight;

				float Lb_fix = param->mot_DataSave[nTrackID].fDistance_Center_end[i + 1];// *(param->fCamHeight - H_fix) / param->fCamHeight;

				if (Lb > std::min(Lb_fix, param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot))
				{
					Lb = std::min(Lb_fix, param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot);
				}

				La_bot = Lb * param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot / param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot;

				if (param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_top > 0.0f && param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_top > 0.0f)
				{
					La_top = Lb * param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_top / param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_top;

					La = (La_top + La_bot) / 2.0f;
				}
				else
					La = La_bot;

				if (La > 0.0f && Lb > 0.0f && (La - Lb) != 0.0f)
					fDiffDistance_fix_end = abs(La - Lb);
			}

			// assume that La is known as fDistance_Center_start -> calculate Lb
			// Lb = La * Lb2 / La2 = La * Lb1 / La1
			La = 0.0f, Lb = 0.0f;
			if (param->mot_DataSave[nTrackID].fDistance_Center_start[i + 1] > 0.0f)
			{
				//float H2 = 0.4f;
				La = param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot * (param->fCamHeight - H2) / param->fCamHeight;

				float La_fix = param->mot_DataSave[nTrackID].fDistance_Center_start[i + 1];// *(param->fCamHeight - H_fix) / param->fCamHeight;

				if (La > std::min(param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot, La_fix))
				{
					La = std::min(param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot, La_fix);
				}

				Lb_bot = La * param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot / param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot;

				if (param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_top > 0.0f && param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_top > 0.0f)
				{
					Lb_top = La * param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_top / param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_top;

					Lb = (Lb_top + Lb_bot) / 2.0f;
				}
				else
					Lb = Lb_bot;

				if (La > 0.9f && Lb > 0.0f && (La - Lb) != 0.0f)
					fDiffDistance_fix_start = abs(La - Lb);
			}

			float fDiffDistance = abs(param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot - param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot);

			if (param->mot_DataSave[nTrackID].fTime_Center[i + 1] > 0.0f)
			{
				param->mot_DataSave[nTrackID].fSpeed_Plate[i] = fDiffDistance / param->mot_DataSave[nTrackID].fTime_Center[i + 1] * 3.6f;
				param->mot_DataSave[nTrackID].fSpeed_Plate_fix_end[i] = fDiffDistance_fix_end / param->mot_DataSave[nTrackID].fTime_Center[i + 1] * 3.6f;
				param->mot_DataSave[nTrackID].fSpeed_Plate_fix_start[i] = fDiffDistance_fix_start / param->mot_DataSave[nTrackID].fTime_Center[i + 1] * 3.6f;
			}
		}
	}

	return 0.0f;
}


float CViolationProcess::get_decided_speed(float* fSpeedList, bool bCenter)
{
	float fSpeedList_tmp[3];
	for (int i = 0; i < 3; i++)
	{
		fSpeedList_tmp[i] = bCenter == false ? fSpeedList[i] : fSpeedList[i + 1];
	}

	float fNewSpeed = 0.0f;

	int nSelected = 0;

	if (fSpeedList_tmp[1] <= 0.0f && fSpeedList_tmp[2] <= 0.0f)
	{
		fNewSpeed = fSpeedList_tmp[0];
		return fNewSpeed;
	}
	else if (fSpeedList_tmp[0] <= 0.0f && fSpeedList_tmp[2] <= 0.0f)
	{
		fNewSpeed = fSpeedList_tmp[1];
		return fNewSpeed;
	}
	else if (fSpeedList_tmp[0] <= 0.0f && fSpeedList_tmp[1] <= 0.0f)
	{
		fNewSpeed = fSpeedList_tmp[2];
		return fNewSpeed;
	}
	else
	{
		float fSpeed0 = abs(fSpeedList_tmp[0] - fSpeedList_tmp[1]);
		float fSpeed1 = abs(fSpeedList_tmp[0] - fSpeedList_tmp[2]);
		float fSpeed2 = abs(fSpeedList_tmp[1] - fSpeedList_tmp[2]);

		float fMinabs = min(fSpeed0, min(fSpeed1, fSpeed2));

		if (fMinabs == fSpeed0)
		{
			fNewSpeed = (fSpeedList_tmp[0] + fSpeedList_tmp[1]) / 2.0f;
			nSelected = 0;
		}
		else if (fMinabs == fSpeed1)
		{
			fNewSpeed = (fSpeedList_tmp[0] + fSpeedList_tmp[2]) / 2.0f;
			nSelected = 1;
		}
		else if (fMinabs == fSpeed2)
		{
			fNewSpeed = (fSpeedList_tmp[1] + fSpeedList_tmp[2]) / 2.0f;
			nSelected = 2;
		}
	}


	if (fNewSpeed < 1.0f) // broken frame, noisy
	{
		switch (nSelected)
		{
		case 0:
			if (max(fSpeedList_tmp[0], fSpeedList_tmp[1]) < 1.0f && fSpeedList_tmp[2] >= 1.0f && fSpeedList_tmp[2] < 250.0f)
			{
				fNewSpeed = fSpeedList_tmp[2];
			}
			break;
		case 1:
			if (max(fSpeedList_tmp[0], fSpeedList_tmp[2]) < 1.0f && fSpeedList_tmp[1] >= 1.0f && fSpeedList_tmp[1] < 250.0f)
			{
				fNewSpeed = fSpeedList_tmp[1];
			}
			break;
		case 2:
			if (max(fSpeedList_tmp[1], fSpeedList_tmp[2]) < 1.0f && fSpeedList_tmp[0] >= 1.0f && fSpeedList_tmp[0] < 250.0f)
			{
				fNewSpeed = fSpeedList_tmp[0];
			}
			break;
		default:
			fNewSpeed = fSpeedList_tmp[0];
			break;
		}
	}

	return fNewSpeed;
}

CString CViolationProcess::SSM_save_log(DarknetData &darknet, CRegion* pRegion, Mat& frame, st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex, char* img_name)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH], strImgDate[CHAR_LENGTH], strImgTime[CHAR_LENGTH];

	wsprintf(strDate, "%04d-%02d-%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d:%02d:%02d.%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	wsprintf(strImgDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strImgTime, "%02d%02d%02d_%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;

	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\logs", currentPath);
	CreateDirectory(makePath, nullptr);
	

	// [LOG]
	CString strLog;
	float fSpeed_Center = get_decided_speed(param->mot_DataSave[nTrackID].fSpeed_Center, true);

	get_decided_plate_speed(param, nTrackID, nClassID);
	float fSpeed_Plate = get_decided_speed(param->mot_DataSave[nTrackID].fSpeed_Plate);
	float fSpeed_Plate_fix_start = get_decided_speed(param->mot_DataSave[nTrackID].fSpeed_Plate_fix_start);
	float fSpeed_Plate_fix_end = get_decided_speed(param->mot_DataSave[nTrackID].fSpeed_Plate_fix_end);

	//float fSpeed_average = (fSpeed_Center + fSpeed_Start + fSpeed_End) / 3.0f;

	float min_SEspeed = 1000.0f;
	bool bSelectBBox = abs(fSpeed_Plate_fix_start - fSpeed_Plate_fix_end) >= 0.05f * fSpeed_Center ? true : false;
	if (fSpeed_Plate_fix_start > 0.0f && fSpeed_Plate_fix_end > 0.0f)
		min_SEspeed = (fSpeed_Plate_fix_start + fSpeed_Plate_fix_end) / 2.0f;
	else if (fSpeed_Plate_fix_start * fSpeed_Plate_fix_end < 0.0f)
		min_SEspeed = (fSpeed_Plate_fix_start > 0) ? fSpeed_Plate_fix_start : fSpeed_Plate_fix_end;

	float fSpeed_Final = ((fSpeed_Plate_fix_start > 0 || fSpeed_Plate_fix_end > 0) && bSelectBBox == false) ? min_SEspeed : fSpeed_Center;
	int nTrust = ((fSpeed_Plate_fix_start > 0 || fSpeed_Plate_fix_end > 0) && bSelectBBox == false) ? 0 : -1;

	fSpeed_Final = (fSpeed_Final == -1) ? 0.0f : fSpeed_Final;

	sprintf(img_name, "%s_%s_L%d_Id%03d_SP_%d",
		strImgDate, strImgTime,
		param->mot_DataSave[nTrackID].nVDS_laneNum,
		nTrackID,
		(int)round(fSpeed_Final * 100.0f));

	param->mot_DataSave[nTrackID].fSpeed_Final = fSpeed_Final;

	// [SAVE IMAGE]
	if (m_nSaveDebugImages)
	{
		for (int i = 0; i < 3; i++)
		{
			for (int j = 0; j < 2; j++)
			{
				sprintf(full_save_image_info, "%s/VDS_data/images/%s_%s_L%d_Id%03d_SP_%d_%d",
					currentPath,
					strImgDate, strImgTime,
					param->mot_DataSave[nTrackID].nVDS_laneNum,
					nTrackID,
					(int)round(fSpeed_Final * 100.0f),
					i * 2 + j + 1);

				sprintf(full_save_image_path, "%s.jpg", full_save_image_info);

				if (!param->mot_DataSave[nTrackID].image_pairs[i].img[j].empty())
				{
					darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].image_pairs[i].img[j], param->mot_DataSave[nTrackID].image_pairs[i].rect[j], param->vio_color[TRACKING_CLR], false);
					if (param->mot_DataSave[nTrackID].image_pairs[i].plate_rect[j].width > 0)
					{
						cv::Rect rtResBox;
						rtResBox.x = (int)((float)param->mot_DataSave[nTrackID].image_pairs[i].plate_rect[j].x / m_fWratio);
						rtResBox.width = (int)((float)param->mot_DataSave[nTrackID].image_pairs[i].plate_rect[j].width / m_fWratio);
						rtResBox.y = (int)((float)param->mot_DataSave[nTrackID].image_pairs[i].plate_rect[j].y / m_fHratio);
						rtResBox.height = (int)((float)param->mot_DataSave[nTrackID].image_pairs[i].plate_rect[j].height / m_fHratio);

						rectangle(param->mot_DataSave[nTrackID].image_pairs[i].img[j], rtResBox, param->class_color[param->mot_info->mot_box_info[nTrackID].object_info.plate.id], 1, 1, 0);
					}
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_DataSave[nTrackID].image_pairs[i].img[j], 0, 1);
				}
			}
		}

	}

	CString strDisplayLog;
	strDisplayLog.Format("\t%s\t %s\t speed\t %6.2f\t %s\t lane\t %d\t ID\t %03d\t",
		strDate, strTime,
		fSpeed_Final,
		nTrust == 0 ? " 0" : "-1",
		param->mot_DataSave[nTrackID].nVDS_laneNum,
		nTrackID);

	// info: day, time, lane number, direction, car length, speed, classID, occupancy time, reverse (Y/N), vehicleGap
	strLog.Format("\t%s\t %s\t speed\t %6.2f\t %s\t lane\t %d\t ID\t %03d\t bbox\t %6.2f\t plate\t %6.2f\t fix\t %6.2f\t %6.2f\t ",
		strDate, strTime,
		fSpeed_Final,
		nTrust == 0 ? " 0" : "-1",
		param->mot_DataSave[nTrackID].nVDS_laneNum,
		nTrackID,
		fSpeed_Center,
		fSpeed_Plate,
		fSpeed_Plate_fix_start,
		fSpeed_Plate_fix_end
		);

	for (int i = 0; i < 5; i++)
	{
		strLog.Format(_T("%s [%d] [%6.2f %6.2f v: %6.2f]\t"), strLog, i, param->mot_DataSave[nTrackID].fDistance_Center_end[i], param->mot_DataSave[nTrackID].fTime_Center[i], param->mot_DataSave[nTrackID].fSpeed_Center[i]);
	}

	for (int i = 0; i < 3; i++)
	{
		strLog.Format(_T("%s [p%d] [%6.2f %6.2f v: %6.2f %6.2f %6.2f]\t"), strLog, i, param->mot_DataSave[nTrackID].fDis_plateStart[i].fDis_plate_bot, param->mot_DataSave[nTrackID].fDis_plateEnd[i].fDis_plate_bot,
			param->mot_DataSave[nTrackID].fSpeed_Plate[i],
			param->mot_DataSave[nTrackID].fSpeed_Plate_fix_start[i],
			param->mot_DataSave[nTrackID].fSpeed_Plate_fix_end[i]);
	}

	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d_%02d.txt", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, nCamIndex + 1);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}

	return strDisplayLog;
}


void CViolationProcess::SSM_save_log_checkinfo(st_det_mot_param *param, int nTrackID, int nClassID, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex, bool bFix)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	char strDate[CHAR_LENGTH], strTime[CHAR_LENGTH], strImgDate[CHAR_LENGTH], strImgTime[CHAR_LENGTH];

	wsprintf(strDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strTime, "%02d:%02d:%02d.%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	wsprintf(strImgDate, "%04d%02d%02d", (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay);
	wsprintf(strImgTime, "%02d%02d%02d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond);

	float fDiffTime = (float)(param->mot_DataSave[nTrackID].nVDS_endTime - param->mot_DataSave[nTrackID].nVDS_startTime) / (float)CLOCKS_PER_SEC;

	/*float distime = 0.0f, distime_1 = 0.0f;

	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 0)
	{
		param->mot_DataSave[nTrackID].fSpeed_End[1] = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.back();
		distime = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.back();
	}
	if (param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() > 1)
	{
		param->mot_DataSave[nTrackID].fSpeed_End[2] = param->mot_DataSave[nTrackID].nXYCOOR_fSpeed[param->mot_DataSave[nTrackID].nXYCOOR_fSpeed.size() - 2];
		distime_1 = param->mot_DataSave[nTrackID].nXYCOOR_deltaTime[param->mot_DataSave[nTrackID].nXYCOOR_deltaTime.size() - 2];
	}*/

	CString strLog;

	if (bFix == false)
	{
		strLog.Format("[%s-%s] nTrackID %03d class %d lane %d direct %d speed %6.2f time %6.3f dis %6.2f start[%6.2f %6.2f] end[%6.2f %6.2f]",
			strDate, strTime,
			nTrackID,
			nClassID,
			param->mot_DataSave[nTrackID].nVDS_laneNum,
			param->mot_DataSave[nTrackID].nVDS_direction,
			param->mot_DataSave[nTrackID].fVDS_carSpeed,
			fDiffTime,
			abs(fEB_distanceY - param->mot_DataSave[nTrackID].fVDS_SB_distanceY),
			param->mot_DataSave[nTrackID].fVDS_SB_distanceX,
			param->mot_DataSave[nTrackID].fVDS_SB_distanceY,
			fEB_distanceX,
			fEB_distanceY);
	}
	else
	{
		strLog.Format("[%s-%s] nTrackID %03d class %d lane %d direct %d save [%6.2f %6.2f %6.2f]",
			strDate, strTime,
			nTrackID,
			nClassID,
			param->mot_DataSave[nTrackID].nVDS_laneNum,
			param->mot_DataSave[nTrackID].nVDS_direction,
			param->mot_DataSave[nTrackID].fSpeed_Center[2],
			param->mot_DataSave[nTrackID].fSpeed_Center[1],
			param->mot_DataSave[nTrackID].fSpeed_Center[0]/*,
			distime,
			distime_1*/);

		strLog.Format(_T("%s\n"), strLog);
	}

	char currentPath[MAX_PATH], makePath[MAX_PATH], full_save_image_info[MAX_PATH], full_save_image_path[MAX_PATH];
	sprintf(currentPath, "%s", getModulPath());
	wsprintf(makePath, "%s\\VDS_data\\logs", currentPath);
	CreateDirectory(makePath, nullptr);

	//CommLog
	char fileName[MAX_PATH];
	wsprintf(fileName, "%s\\Log_%04d%02d%02d_checkInfo_%02d.log", makePath, (int)tCurTime.wYear, (int)tCurTime.wMonth, (int)tCurTime.wDay, nCamIndex);

	try
	{
		//CAutoLock lock;
		FILE *log = nullptr;
		if (fopen_s(&log, fileName, "at") == 0)
		{
			fprintf_s(log, "%s\n", strLog.GetBuffer());
			fclose(log);
		}
	}
	catch (const std::exception& e)
	{
		CString str_tmp;
		str_tmp.Format(_T("error : %s"), e.what());
	}
}

#endif


#else

void CViolationProcess::reverse_violation_both_LeftEntry(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, int nEndY, char * strTime, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);
	cv::Point ptROI[4];
	pRegion->CRegion_copy(2, ptROI);

	char *vio_file_path = param->vio_result_path;
	char full_save_image_path[CHAR_LENGTH];

	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.7, 0.7, 0, 2, 1);

	clock_t currentClock = clock();

	cv::Point ptTop, ptBot;
	if (ptLine[0].y > ptLine[2].y)
	{
		ptTop = ptLine[2];
		ptBot = ptLine[0];
	}
	else
	{
		ptTop = ptLine[0];
		ptBot = ptLine[2];
	}

	//calculate_line_params();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	float fScore = param->mot_info->det_box_info[ntrack].score;
	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;

	int nPtBoxRightX = (int)((float)(rtBoundingBox.x + rtBoundingBox.width) / fWratio);
	int nPtBottomX = (int)((float)(rtBoundingBox.x + (rtBoundingBox.width / 2)) / fWratio);
	int nPtBottomY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);

	cv::Rect fullRect = rtBoundingBox;
	cv::Rect rectArea = fullRect;

	rectArea.y = rectArea.y + rectArea.height * 1 / 3;
	rectArea.height = rectArea.height * 2 / 3;

	int nReverseMaxCount = 10;

	if (fullRect.height <= 6 * fullRect.width)
	{
		// ===== [Taking LPR image if exists] =====
		if ((nClassID == cls_bike_rear || nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck || nClassID == cls_truckbike_rear
			|| (COUNT_MODE_COUNTRY == 1 && nClassID == cls_bike_front) || (COUNT_MODE_COUNTRY == 1 && nClassID == cls_truckbike_front))
			&& pRegion->CRegion_line_touch_save_LPImage(nPtBottomY, rtResBox.height, nEndY) && param->mot_ReverseSave[nTrackID].nLPR == 0) // 2018.11.12 hai
		{
			//std::cout << nTrackID << ": touch LPR line" << std::endl;
			/*CString str;
			str.Format(_T("%d: touch LPR line"), nTrackID);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			param->mot_ReverseSave[nTrackID].lprTime = currentClock;

			if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				param->mot_ReverseSave[nTrackID].pLPImage.release();

			param->mot_ReverseSave[nTrackID].pLPImage = frame.clone();
			//rectangle(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
			darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

			param->mot_ReverseSave[nTrackID].LPRect = fullRect;

			sprintf(param->mot_ReverseSave[nTrackID].save_LPimage_coordinate, "X%04d_Y%04d_W%04d_H%04d", rectArea.x, rectArea.y, rectArea.width, rectArea.height);

			param->mot_ReverseSave[nTrackID].nLPR = 1;
		}

		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}
				else if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}

				// === [from bottom to top] === 
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					int nPtLaneBotX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBot.y);
					int nPtLaneTopX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTop.y);

					int nPtLaneBotCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBotCen.y);
					int nPtLaneTopCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTopCen.y);

					if (param->mot_ReverseSave[nTrackID].PtTrackBot.x < nPtLaneBotX02 && param->mot_ReverseSave[nTrackID].PtTrackTop.x < nPtLaneTopX02
						&& (param->mot_ReverseSave[nTrackID].PtTrackBotCen.x < nPtLaneBotCenX02 || param->mot_ReverseSave[nTrackID].PtTrackTopCen.x < nPtLaneTopCenX02))
					{
						param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					}

				}
				//  === [from top to bottom] === 
				else if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					int nPtLaneBotX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBot.y);
					int nPtLaneTopX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTop.y);

					int nPtLaneBotCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBotCen.y);
					int nPtLaneTopCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTopCen.y);

					if (param->mot_ReverseSave[nTrackID].PtTrackTop.x > nPtLaneTopX02 && param->mot_ReverseSave[nTrackID].PtTrackBot.x > nPtLaneBotX02
						&& (param->mot_ReverseSave[nTrackID].PtTrackTopCen.x > nPtLaneTopCenX02 || param->mot_ReverseSave[nTrackID].PtTrackBotCen.x > nPtLaneBotCenX02))
					{
						param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					}
				}

				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];

					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10) && (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
							sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
								param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
							if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pLPImage.release();
					}

					//std::cout << "reverse violation" << std::endl;
					//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
				}
			}
		}

		// ===== [motobike and truck-bike Violation] =====

		if (//param->mot_info->mot_box_info[ntrack].n_sub_box > 0 &&
			(((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear) && pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == LEFT_VIOLATION_AREA
				&& (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == LEFT_CENTER_LANE_VIO || pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == LEFT_VIOLATION_AREA))
				|| ((nClassID == cls_bike_front || nClassID == cls_truckbike_front) && pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == RIGHT_VIOLATION_AREA
					&& (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == RIGHT_CENTER_LANE_VIO || pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == RIGHT_VIOLATION_AREA))))
		{
			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == 0)
			{
				param->mot_ReverseSave[nTrackID].startTime = currentClock;

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					param->mot_ReverseSave[nTrackID].pIplImage.release();

				param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
				//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
				darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

				param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
				param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;

				param->mot_ReverseSave[nTrackID].rtTrackStart = rtResBox; //2018.10.26 Young

				param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
				param->mot_ReverseSave[nTrackID].catchTime = currentClock;

				param->mot_ReverseSave[nTrackID].nFirstClass = nClassID;
			}
			else
			{
				if (nClassID == cls_bike_rear || nClassID == cls_truckbike_rear)
				{
					//Check the movement of the object when the rear bike.
					if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY > 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y > 0))
					{
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					}
				}
				else if (nClassID == cls_bike_front || nClassID == cls_truckbike_front)
				{
					//Check the movement of the object when the front bike.
					if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY < 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y < 0))
					{
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					}
				}
				else
				{

					param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
				}
			}

			//std::cout << nTrackID << ":reverse vio count: " << param->mot_ReverseSave[nTrackID].nReverseVioCnt << std::endl;
			/*CString str;
			str.Format(_T("%d: reverse vio count: %d, location(%d, %d)"), nTrackID, param->mot_ReverseSave[nTrackID].nReverseVioCnt, rtBoundingBox.x, rtBoundingBox.y);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == nReverseMaxCount && param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				param->mot_ReverseSave[nTrackID].nReverseViolation = 1;

				if (COUNT_MODE_COUNTRY == 1 ||
					(COUNT_MODE_COUNTRY == 0 && (param->mot_ReverseSave[nTrackID].nFirstClass == cls_bike_rear || param->mot_ReverseSave[nTrackID].nFirstClass == cls_truckbike_rear
						|| nClassID == cls_bike_rear || nClassID == cls_truckbike_rear)))
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];

					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);
					}

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (nClassID == cls_bike_rear && fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10)
						//	&& (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
							sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
								param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
							if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}


					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;
				}

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pIplImage.release();
				}

				if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pLPImage.release();
				}

				//std::cout << "reverse violation" << std::endl;
				//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
			}
		}

		//20190130 hai for Overlay Display
		//if (COUNT_MODE_COUNTRY == 1 && param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			//Mat  overlay_frame = cvCreateImage(cvSize(res_frame.cols, res_frame.rows), res_frame->depth, res_frame->nChannels);
			//cvCopy(res_frame, overlay_frame);
			//rectangle(overlay_frame, rtBoundingBox, param->vio_color[REVERSE_VIOLATION_CLR], -1); //20190222 hai Violation color
			//cvAddWeighted(overlay_frame, fOverlay_alpha, res_frame, 1 - fOverlay_alpha, 0, res_frame);

			param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
			////sprintf(textTmp, "LANE");
			////putText(res_frame, textTmp, cv::Point((int)rtBoundingBox.x, (int)rtBoundingBox.y), &font, param->vio_color[LANE_VIOLATION_CLR]); //20190222 hai Violation color

			//if (overlay_frame)
			//	cvReleaseImage(&overlay_frame);
		}

		int end_seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].endTime) / CLOCKS_PER_SEC);

		// ===== [release violation info] =====
		//20190714
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
		{
			if (param->mot_ReverseSave[nTrackID].endTime != 0 && end_seconds >= 2) //2초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}
		else if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);

			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= 5) //5초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}

		else if ((((nClassID == cls_bike_front || nClassID == cls_truckbike_front) && (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == LEFT_VIOLATION_AREA)) // 2018.11.09 hai
			|| ((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear) && (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == RIGHT_VIOLATION_AREA)) // 2018.11.09 hai
			|| ((nClassID == cls_bike_side || nClassID == cls_truckbike_side) && pRegion->CRegion_line_touch_forLaneVio_side(rtResBox.x, nPtBoxRightX, nPtBottomY) == 0))
			&& param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].nReverseVioCnt < nReverseMaxCount)
		{
			darknet.release_reverse_violation(param, nTrackID);
		}

		//else if (param->mot_ReverseSave[nTrackID].nLPR == 1)
		//{
		//	int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].lprTime) / CLOCKS_PER_SEC);

		//	//std::cout << "TimeOut for other cases ===============: " << seconds << std::endl;

		//	if (seconds >= 30) //30초 초과
		//	{
		//		release_reverse_violation(param, nTrackID);
		//	}
		//}

		param->mot_ReverseSave[nTrackID].endTime = currentClock; //// 2018.09.03
	}
}




void CViolationProcess::reverse_violation_both_LeftExit(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, int nEndY, char * strTime, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);
	cv::Point ptROI[4];
	pRegion->CRegion_copy(2, ptROI);

	char *vio_file_path = param->vio_result_path;
	char full_save_image_path[CHAR_LENGTH];

	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.7, 0.7, 0, 2, 1);

	clock_t currentClock = clock();

	cv::Point ptTop, ptBot;
	if (ptLine[0].y > ptLine[2].y)
	{
		ptTop = ptLine[2];
		ptBot = ptLine[0];
	}
	else
	{
		ptTop = ptLine[0];
		ptBot = ptLine[2];
	}

	//calculate_line_params();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	float fScore = param->mot_info->det_box_info[ntrack].score;
	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;

	int nPtBoxRightX = (int)((float)(rtBoundingBox.x + rtBoundingBox.width) / fWratio);
	int nPtBottomX = (int)((float)(rtBoundingBox.x + (rtBoundingBox.width / 2)) / fWratio);
	int nPtBottomY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);

	cv::Rect fullRect = rtBoundingBox;
	cv::Rect rectArea = fullRect;

	rectArea.y = rectArea.y + rectArea.height * 1 / 3;
	rectArea.height = rectArea.height * 2 / 3;

	int nReverseMaxCount = 10;

	if (fullRect.height <= 6 * fullRect.width)
	{
		// ===== [Taking LPR image if exists] =====
		if ((nClassID == cls_bike_rear || nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck || nClassID == cls_truckbike_rear
			|| (COUNT_MODE_COUNTRY == 1 && nClassID == cls_bike_front) || (COUNT_MODE_COUNTRY == 1 && nClassID == cls_truckbike_front))
			&& pRegion->CRegion_line_touch_save_LPImage(nPtBottomY, rtResBox.height, nEndY) && param->mot_ReverseSave[nTrackID].nLPR == 0) // 2018.11.12 hai
		{
			//std::cout << nTrackID << ": touch LPR line" << std::endl;
			/*CString str;
			str.Format(_T("%d: touch LPR line"), nTrackID);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			param->mot_ReverseSave[nTrackID].lprTime = currentClock;

			if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				param->mot_ReverseSave[nTrackID].pLPImage.release();

			param->mot_ReverseSave[nTrackID].pLPImage = frame.clone();
			//rectangle(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
			darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

			param->mot_ReverseSave[nTrackID].LPRect = fullRect;

			sprintf(param->mot_ReverseSave[nTrackID].save_LPimage_coordinate, "X%04d_Y%04d_W%04d_H%04d", rectArea.x, rectArea.y, rectArea.width, rectArea.height);

			param->mot_ReverseSave[nTrackID].nLPR = 1;
		}

		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}
				else if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}

				// === [from bottom to top] === 
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					int nPtLaneBotX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBot.y);
					int nPtLaneTopX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTop.y);

					int nPtLaneBotCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBotCen.y);
					int nPtLaneTopCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTopCen.y);

					if (param->mot_ReverseSave[nTrackID].PtTrackBot.x > nPtLaneBotX02 && param->mot_ReverseSave[nTrackID].PtTrackTop.x > nPtLaneTopX02
						&& (param->mot_ReverseSave[nTrackID].PtTrackBotCen.x > nPtLaneBotCenX02 || param->mot_ReverseSave[nTrackID].PtTrackTopCen.x > nPtLaneTopCenX02))
					{
						param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					}

				}
				//  === [from top to bottom] === 
				else if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					int nPtLaneBotX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBot.y);
					int nPtLaneTopX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTop.y);

					int nPtLaneBotCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackBotCen.y);
					int nPtLaneTopCenX02 = (int)pRegion->CRegion_get_Lane_Xvalue(2, param->mot_ReverseSave[nTrackID].PtTrackTopCen.y);

					if (param->mot_ReverseSave[nTrackID].PtTrackTop.x < nPtLaneTopX02 && param->mot_ReverseSave[nTrackID].PtTrackBot.x < nPtLaneBotX02
						&& (param->mot_ReverseSave[nTrackID].PtTrackTopCen.x < nPtLaneTopCenX02 || param->mot_ReverseSave[nTrackID].PtTrackBotCen.x < nPtLaneBotCenX02))
					{
						param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
					}
				}

				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];

					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10) && (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
							sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
								param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
							if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pLPImage.release();
					}

					//std::cout << "reverse violation" << std::endl;
					//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
				}
			}
		}

		// ===== [motobike and truck-bike Violation] =====

		if (//param->mot_info->mot_box_info[ntrack].n_sub_box > 0 &&
			(((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear) && pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == RIGHT_VIOLATION_AREA
				&& (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == RIGHT_CENTER_LANE_VIO || pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == RIGHT_VIOLATION_AREA))
				|| ((nClassID == cls_bike_front || nClassID == cls_truckbike_front) && pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == LEFT_VIOLATION_AREA
					&& (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == LEFT_CENTER_LANE_VIO || pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtCenterY, rtResBox.width) == LEFT_VIOLATION_AREA))))
		{
			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == 0)
			{
				param->mot_ReverseSave[nTrackID].startTime = currentClock;

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					param->mot_ReverseSave[nTrackID].pIplImage.release();

				param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
				//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
				darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

				param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
				param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;

				param->mot_ReverseSave[nTrackID].rtTrackStart = rtResBox; //2018.10.26 Young

				param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
				param->mot_ReverseSave[nTrackID].catchTime = currentClock;

				param->mot_ReverseSave[nTrackID].nFirstClass = nClassID;
			}
			else
			{
				if (nClassID == cls_bike_rear || nClassID == cls_truckbike_rear)
				{
					//Check the movement of the object when the rear bike.
					if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY > 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y > 0))
					{
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					}
				}
				else if (nClassID == cls_bike_front || nClassID == cls_truckbike_front)
				{
					//Check the movement of the object when the front bike.
					if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY < 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y < 0))
					{
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					}
				}
				else
				{

					param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
				}
			}

			//std::cout << nTrackID << ":reverse vio count: " << param->mot_ReverseSave[nTrackID].nReverseVioCnt << std::endl;
			/*CString str;
			str.Format(_T("%d: reverse vio count: %d, location(%d, %d)"), nTrackID, param->mot_ReverseSave[nTrackID].nReverseVioCnt, rtBoundingBox.x, rtBoundingBox.y);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == nReverseMaxCount && param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				param->mot_ReverseSave[nTrackID].nReverseViolation = 1;

				if (COUNT_MODE_COUNTRY == 1 ||
					(COUNT_MODE_COUNTRY == 0 && (param->mot_ReverseSave[nTrackID].nFirstClass == cls_bike_rear || param->mot_ReverseSave[nTrackID].nFirstClass == cls_truckbike_rear
						|| nClassID == cls_bike_rear || nClassID == cls_truckbike_rear)))
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];

					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);
					}

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (nClassID == cls_bike_rear && fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10)
						//	&& (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
						param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
						if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
						if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;
				}

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pIplImage.release();
				}

				if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pLPImage.release();
				}

				//std::cout << "reverse violation" << std::endl;
				//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
			}
		}

		//20190130 hai for Overlay Display
		//if (COUNT_MODE_COUNTRY == 1 && param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			//Mat  overlay_frame = cvCreateImage(cvSize(res_frame.cols, res_frame.rows), res_frame->depth, res_frame->nChannels);
			//cvCopy(res_frame, overlay_frame);
			//rectangle(overlay_frame, rtBoundingBox, param->vio_color[REVERSE_VIOLATION_CLR], -1); //20190222 hai Violation color
			//cvAddWeighted(overlay_frame, fOverlay_alpha, res_frame, 1 - fOverlay_alpha, 0, res_frame);

			param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
			////sprintf(textTmp, "LANE");
			////putText(res_frame, textTmp, cv::Point((int)rtBoundingBox.x, (int)rtBoundingBox.y), &font, param->vio_color[LANE_VIOLATION_CLR]); //20190222 hai Violation color

			//if (overlay_frame)
			//	cvReleaseImage(&overlay_frame);
		}

		int end_seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].endTime) / CLOCKS_PER_SEC);

		// ===== [release violation info] =====
		//20190714
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
		{
			if (param->mot_ReverseSave[nTrackID].endTime != 0 && end_seconds >= 2) //2초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}
		else if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);

			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= 5) //5초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}

		else if ((((nClassID == cls_bike_front || nClassID == cls_truckbike_front) && (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == RIGHT_VIOLATION_AREA)) // 2018.11.09 hai
			|| ((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear) && (pRegion->CRegion_Object_IsinArea(nPtBottomX, nPtBottomY, rtResBox.width) == LEFT_VIOLATION_AREA)) // 2018.11.09 hai
			|| ((nClassID == cls_bike_side || nClassID == cls_truckbike_side) && pRegion->CRegion_line_touch_forLaneVio_side(rtBoundingBox.x, nPtBoxRightX, nPtBottomY) == 0))
			&& param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].nReverseVioCnt < nReverseMaxCount)
		{
			darknet.release_reverse_violation(param, nTrackID);
		}

		//else if (param->mot_ReverseSave[nTrackID].nLPR == 1)
		//{
		//	int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].lprTime) / CLOCKS_PER_SEC);

		//	//std::cout << "TimeOut for other cases ===============: " << seconds << std::endl;

		//	if (seconds >= 30) //30초 초과
		//	{
		//		release_reverse_violation(param, nTrackID);
		//	}
		//}

		param->mot_ReverseSave[nTrackID].endTime = currentClock; //// 2018.09.03
	}
}


void CViolationProcess::reverse_violation_entry(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, int nEndY, char * strTime, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);
	cv::Point ptROI[4];
	pRegion->CRegion_copy(2, ptROI);

	char *vio_file_path = param->vio_result_path;
	char full_save_image_path[CHAR_LENGTH];

	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.7, 0.7, 0, 2, 1);

	clock_t currentClock = clock();

	cv::Point ptTop, ptBot;
	if (ptLine[0].y > ptLine[2].y)
	{
		ptTop = ptLine[2];
		ptBot = ptLine[0];
	}
	else
	{
		ptTop = ptLine[0];
		ptBot = ptLine[2];
	}

	//calculate_line_params();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	float fScore = param->mot_info->det_box_info[ntrack].score;
	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;

	int nPtBoxRightX = (int)((float)(rtBoundingBox.x + rtBoundingBox.width) / fWratio);
	int nPtBottomX = (int)((float)(rtBoundingBox.x + (rtBoundingBox.width / 2)) / fWratio);
	int nPtBottomY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);

	cv::Rect fullRect = rtBoundingBox;
	cv::Rect rectArea = fullRect;

	rectArea.y = rectArea.y + rectArea.height * 1 / 3;
	rectArea.height = rectArea.height * 2 / 3;

	int nReverseMaxCount = 10;

	if (fullRect.height <= 6 * fullRect.width)
	{
		// ===== [Taking LPR image if exists] =====
		if ((nClassID == cls_bike_rear || nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck || nClassID == cls_truckbike_rear
			|| (COUNT_MODE_COUNTRY == 1 && nClassID == cls_bike_front) || (COUNT_MODE_COUNTRY == 1 && nClassID == cls_truckbike_front))
			&& pRegion->CRegion_line_touch_save_LPImage(nPtBottomY, rtResBox.height, nEndY) && param->mot_ReverseSave[nTrackID].nLPR == 0) // 2018.11.12 hai
		{
			//std::cout << nTrackID << ": touch LPR line" << std::endl;
			/*CString str;
			str.Format(_T("%d: touch LPR line"), nTrackID);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			param->mot_ReverseSave[nTrackID].lprTime = currentClock;

			if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				param->mot_ReverseSave[nTrackID].pLPImage.release();

			param->mot_ReverseSave[nTrackID].pLPImage = frame.clone();
			//rectangle(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
			darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

			param->mot_ReverseSave[nTrackID].LPRect = fullRect;

			sprintf(param->mot_ReverseSave[nTrackID].save_LPimage_coordinate, "X%04d_Y%04d_W%04d_H%04d", rectArea.x, rectArea.y, rectArea.width, rectArea.height);

			param->mot_ReverseSave[nTrackID].nLPR = 1;
		}

		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				//Start from bot touching
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}

				// === [from bottom to top] === 
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
				}


				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];
					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10) && (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
							sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
								param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
							if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;
					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pLPImage.release();
					}

					//std::cout << "reverse violation" << std::endl;
					//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
				}
			}
		}

		// ===== [motobike and truck-bike Violation] =====

		if ((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear) && nPtBottomY > ptTop.y && nPtBottomY < ptBot.y)
		{
			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == 0)
			{
				param->mot_ReverseSave[nTrackID].startTime = currentClock;

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					param->mot_ReverseSave[nTrackID].pIplImage.release();

				param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
				//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
				darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

				param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
				param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;

				param->mot_ReverseSave[nTrackID].rtTrackStart = rtResBox; //2018.10.26 Young

				param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
				param->mot_ReverseSave[nTrackID].catchTime = currentClock;
			}
			else
			{
				if (nClassID == cls_bike_rear || nClassID == cls_truckbike_rear /* || nClassID == cls_bike_side || nClassID == cls_truckbike_side*/)
				{
					//Check the movement of the object when the rear bike.
					if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY > 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y > 0))
					{
						param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
						param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					}
				}
			}

			//std::cout << nTrackID << ":reverse vio count: " << param->mot_ReverseSave[nTrackID].nReverseVioCnt << std::endl;
			/*CString str;
			str.Format(_T("%d: reverse vio count: %d, location(%d, %d)"), nTrackID, param->mot_ReverseSave[nTrackID].nReverseVioCnt, rtBoundingBox.x, rtBoundingBox.y);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == nReverseMaxCount && param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				param->mot_ReverseSave[nTrackID].nReverseViolation = 1;

				int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

				//20200115 index counting for violation
				int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];
				//1st image
				//(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
				sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
				cv::Point ptReverse[2];
				ptReverse[0].x = (int)((float)ptTop.x * fWratio);
				ptReverse[0].y = (int)((float)ptTop.y * fHratio);
				ptReverse[1].x = (int)((float)ptBot.x * fWratio);
				ptReverse[1].y = (int)((float)ptBot.y * fHratio);

				line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
				SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

				//2nd image
				bool bUseClone = false;
				//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
				sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

				/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
				{
					fullRect.height = fullRect.height + 20;
				}*/

				if (!clone_frame.empty())
				{
					bUseClone = true;
					//20190327 Hai
					clone_frame = frame.clone();
					line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
					//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

					SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
				}
				else
				{
					//20190222 hai Violation color
					line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
					//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

					SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
				}

				//lpr image
				//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
				sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
				if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
				{
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);

					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
				}
				else
				{
					//if (nClassID == cls_bike_rear && fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10)
					//	&& (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
					{
						if (!bUseClone)
						{
							SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
						}
						else
						{
							SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}
					}
					/*else
					{
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
							param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
						if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
						if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}*/
				}

				//20200115 update index counting for violation
				nVioCntArray[REVERSE_VIOLATION_CNT]++;
				nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;

				if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pIplImage.release();
				}

				if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				{
					param->mot_ReverseSave[nTrackID].pLPImage.release();
				}

				//std::cout << "reverse violation" << std::endl;
				//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
			}
		}

		//20190130 hai for Overlay Display
		//if (COUNT_MODE_COUNTRY == 1 && param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			//Mat  overlay_frame = cvCreateImage(cvSize(res_frame.cols, res_frame.rows), res_frame->depth, res_frame->nChannels);
			//cvCopy(res_frame, overlay_frame);
			//rectangle(overlay_frame, rtBoundingBox, param->vio_color[REVERSE_VIOLATION_CLR], -1); //20190222 hai Violation color
			//cvAddWeighted(overlay_frame, fOverlay_alpha, res_frame, 1 - fOverlay_alpha, 0, res_frame);

			param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
			////sprintf(textTmp, "LANE");
			////putText(res_frame, textTmp, cv::Point((int)rtBoundingBox.x, (int)rtBoundingBox.y), &font, param->vio_color[LANE_VIOLATION_CLR]); //20190222 hai Violation color

			//if (overlay_frame)
			//	cvReleaseImage(&overlay_frame);
		}

		int end_seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].endTime) / CLOCKS_PER_SEC);

		// ===== [release violation info] =====
		//20190714
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
		{
			if (param->mot_ReverseSave[nTrackID].endTime != 0 && end_seconds >= 2) //2초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}
		else if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);

			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= 5) //5초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}

		else if (((nClassID == cls_bike_front || nClassID == cls_truckbike_front)
			|| ((nClassID == cls_bike_side || nClassID == cls_truckbike_side) && pRegion->CRegion_line_touch_forLaneVio_side(rtResBox.x, nPtBoxRightX, nPtBottomY) == 0))
			&& param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].nReverseVioCnt < nReverseMaxCount)
		{
			darknet.release_reverse_violation(param, nTrackID);
		}

		//else if (param->mot_ReverseSave[nTrackID].nLPR == 1)
		//{
		//	int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].lprTime) / CLOCKS_PER_SEC);

		//	//std::cout << "TimeOut for other cases ===============: " << seconds << std::endl;

		//	if (seconds >= 30) //30초 초과
		//	{
		//		release_reverse_violation(param, nTrackID);
		//	}
		//}

		param->mot_ReverseSave[nTrackID].endTime = currentClock; //// 2018.09.03
	}
}


void CViolationProcess::reverse_violation_exit(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, int nEndY, char * strTime, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);
	cv::Point ptROI[4];
	pRegion->CRegion_copy(2, ptROI);

	char *vio_file_path = param->vio_result_path;
	char full_save_image_path[CHAR_LENGTH];

	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.7, 0.7, 0, 2, 1);

	clock_t currentClock = clock();

	cv::Point ptTop, ptBot;
	if (ptLine[0].y > ptLine[2].y)
	{
		ptTop = ptLine[2];
		ptBot = ptLine[0];
	}
	else
	{
		ptTop = ptLine[0];
		ptBot = ptLine[2];
	}

	//calculate_line_params();

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	float fScore = param->mot_info->det_box_info[ntrack].score;
	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;

	int nPtBoxRightX = (int)((float)(rtBoundingBox.x + rtBoundingBox.width) / fWratio);
	int nPtBottomX = (int)((float)(rtBoundingBox.x + (rtBoundingBox.width / 2)) / fWratio);
	int nPtBottomY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);
	int nPtCenterY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height / 2) / fHratio);

	cv::Rect fullRect = rtBoundingBox;
	cv::Rect rectArea = fullRect;

	rectArea.y = rectArea.y + rectArea.height * 1 / 3;
	rectArea.height = rectArea.height * 2 / 3;

	int nReverseMaxCount = 10;

	if (fullRect.height <= 6 * fullRect.width)
	{
		// ===== [Taking LPR image if exists] =====
		if ((nClassID == cls_bike_rear || nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck || nClassID == cls_truckbike_rear
			|| (COUNT_MODE_COUNTRY == 1 && nClassID == cls_bike_front) || (COUNT_MODE_COUNTRY == 1 && nClassID == cls_truckbike_front))
			&& pRegion->CRegion_line_touch_save_LPImage(nPtBottomY, rtResBox.height, nEndY) && param->mot_ReverseSave[nTrackID].nLPR == 0) // 2018.11.12 hai
		{
			//std::cout << nTrackID << ": touch LPR line" << std::endl;
			/*CString str;
			str.Format(_T("%d: touch LPR line"), nTrackID);
			pPane->FillMainLogWindow(m_nCameraIndex, str);*/

			param->mot_ReverseSave[nTrackID].lprTime = currentClock;

			if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
				param->mot_ReverseSave[nTrackID].pLPImage.release();

			param->mot_ReverseSave[nTrackID].pLPImage = frame.clone();
			//rectangle(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
			darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pLPImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

			param->mot_ReverseSave[nTrackID].LPRect = fullRect;

			sprintf(param->mot_ReverseSave[nTrackID].save_LPimage_coordinate, "X%04d_Y%04d_W%04d_H%04d", rectArea.x, rectArea.y, rectArea.width, rectArea.height);

			param->mot_ReverseSave[nTrackID].nLPR = 1;
		}

		// ===== [car/van/truck Violation] =====
		if (nClassID == cls_car || nClassID == cls_ban || nClassID == cls_bus || nClassID == cls_truck)
		{
			if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
			{
				//Start from Top for touching 
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptTop.y, true) == 1 && param->mot_ReverseSave[nTrackID].nTouchTop == 0 && param->mot_ReverseSave[nTrackID].nTouchBot == 0) // 2018.11.12 hai
				{
					param->mot_ReverseSave[nTrackID].nTouchTop = 1;
					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTopCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);
				}


				//  === [from top to bottom] === 
				if (pRegion->CRegion_line_touch_forLaneVio_topbot(nPtBottomY, rtResBox.height, ptBot.y, false) == 1 && param->mot_ReverseSave[nTrackID].nTouchBot == 0 && param->mot_ReverseSave[nTrackID].nTouchTop == 1)
				{
					param->mot_ReverseSave[nTrackID].nTouchBot = 1;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;
					param->mot_ReverseSave[nTrackID].PtTrackBot.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBot.y = nPtBottomY;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackBotCen.y = nPtCenterY;

					param->mot_ReverseSave[nTrackID].nReverseViolation = 1;
				}

				if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
				{
					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];
					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
					SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10) && (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
							sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
								param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
							if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;
					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pLPImage.release();
					}

					//std::cout << "reverse violation" << std::endl;
					//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
				}
			}
		}

		// ===== [motobike and truck-bike Violation] =====

		if (COUNT_MODE_COUNTRY == 1) //only for Indonesia
		{
			if ((nClassID == cls_bike_front || nClassID == cls_truckbike_front) && nPtBottomY > ptTop.y && nPtBottomY < ptBot.y)
			{
				if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == 0)
				{
					param->mot_ReverseSave[nTrackID].startTime = currentClock;

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
						param->mot_ReverseSave[nTrackID].pIplImage.release();

					param->mot_ReverseSave[nTrackID].pIplImage = frame.clone();
					//rectangle(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
					darknet.draw_objectbox_forLPR(param->mot_ReverseSave[nTrackID].pIplImage, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], false);

					param->mot_ReverseSave[nTrackID].PtTrackTop.x = nPtBottomX;
					param->mot_ReverseSave[nTrackID].PtTrackTop.y = nPtBottomY;

					param->mot_ReverseSave[nTrackID].rtTrackStart = rtResBox; //2018.10.26 Young

					param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
					param->mot_ReverseSave[nTrackID].catchTime = currentClock;

				}
				else
				{
					if (nClassID == cls_bike_front || nClassID == cls_truckbike_front /*|| nClassID == cls_bike_side || nClassID == cls_truckbike_side*/)
					{
						//Check the movement of the object when the front bike.
						if ((param->mot_ReverseSave[nTrackID].PtTrackTop.y - nPtBottomY < 0) && (param->mot_ReverseSave[nTrackID].rtTrackStart.y - rtResBox.y < 0))
						{
							param->mot_ReverseSave[nTrackID].nReverseVioCnt++;
							param->mot_ReverseSave[nTrackID].catchTime = currentClock;
						}
					}
				}

				//std::cout << nTrackID << ":reverse vio count: " << param->mot_ReverseSave[nTrackID].nReverseVioCnt << std::endl;
				/*CString str;
				str.Format(_T("%d: reverse vio count: %d, location(%d, %d)"), nTrackID, param->mot_ReverseSave[nTrackID].nReverseVioCnt, rtBoundingBox.x, rtBoundingBox.y);
				pPane->FillMainLogWindow(m_nCameraIndex, str);*/

				if (param->mot_ReverseSave[nTrackID].nReverseVioCnt == nReverseMaxCount && param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
				{
					param->mot_ReverseSave[nTrackID].nReverseViolation = 1;

					int millisecond = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].startTime) / CLOCKS_PER_SEC * 100.0f) % 100;

					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[REVERSE_VIOLATION_CNT];
					//1st image
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_01.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);
					cv::Point ptReverse[2];
					ptReverse[0].x = (int)((float)ptTop.x * fWratio);
					ptReverse[0].y = (int)((float)ptTop.y * fHratio);
					ptReverse[1].x = (int)((float)ptBot.x * fWratio);
					ptReverse[1].y = (int)((float)ptBot.y * fHratio);

					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						line(param->mot_ReverseSave[nTrackID].pIplImage, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0); //2018.11.12
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pIplImage, 0, 1);
					}

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/reverse/%02d_%s_%02d_000_%s%02d_02.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						line(clone_frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//20190222 hai Violation color
						line(frame, ptReverse[0], ptReverse[1], cv::Scalar(0, 0, 255, 0), 2, 7, 0);
						//rectangle(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[REVERSE_VIOLATION_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty() && param->mot_ReverseSave[nTrackID].LPRect.x > 10 && param->mot_ReverseSave[nTrackID].LPRect.x + param->mot_ReverseSave[nTrackID].LPRect.width < frame.cols - 10)
					{
						//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_%s.jpg", vio_file_path, REVERSE_VIOLATION_CODE, param->camera_id, nClassID, strTime, n_millisecond, param->mot_ReverseSave[nTrackID].save_LPimage_coordinate);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_ReverseSave[nTrackID].pLPImage, 0, 1);
					}
					else
					{
						//if (nClassID == cls_bike_rear && fullRect.x > 10 && (fullRect.x + fullRect.width) < (frame.cols - 10)
						//	&& (nEndY >= nPtBottomY && (nEndY - abs(ptLine[0].y - ptLine[2].y) / 2) < nPtBottomY))
						{
							if (!bUseClone)
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
							}
							else
							{
								SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
							}
						}
						/*else
						{
						sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_000_%s%02d_02_X%04d_Y%04d_W%04d_H%04d_NoPL.jpg", vio_file_path, REVERSE_VIOLATION_CODE,
						param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
						if (hSaveImageDll && !bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
						if (hSaveImageDll && bUseClone) UNSSAVEPICTURE_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
						}*/
					}

					//20200115 update index counting for violation
					nVioCntArray[REVERSE_VIOLATION_CNT]++;
					nVioCntArray[REVERSE_VIOLATION_CNT] = nVioCntArray[REVERSE_VIOLATION_CNT] % 100;
					if (!param->mot_ReverseSave[nTrackID].pIplImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pIplImage.release();
					}

					if (!param->mot_ReverseSave[nTrackID].pLPImage.empty())
					{
						param->mot_ReverseSave[nTrackID].pLPImage.release();
					}

					//std::cout << "reverse violation" << std::endl;
					//pPane->FillMainLogWindow(m_nCameraIndex, _T("reverse violation"));
				}
			}
		}


		//20190130 hai for Overlay Display
		//if (COUNT_MODE_COUNTRY == 1 && param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			//	Mat  overlay_frame = cvCreateImage(cvSize(res_frame.cols, res_frame.rows), res_frame->depth, res_frame->nChannels);
			//	cvCopy(res_frame, overlay_frame);
			//	rectangle(overlay_frame, rtBoundingBox, param->vio_color[REVERSE_VIOLATION_CLR], -1); //20190222 hai Violation color
			//	cvAddWeighted(overlay_frame, fOverlay_alpha, res_frame, 1 - fOverlay_alpha, 0, res_frame);

			param->mot_DataSave[nTrackID].nVioDisplay[REVERSE_VIOLATION_CLR] = 1; //2019022
			//violation_text_display(res_frame, param, ntrack, nTrackID);
			darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
			////sprintf(textTmp, "LANE");
			////putText(res_frame, textTmp, cv::Point((int)rtBoundingBox.x, (int)rtBoundingBox.y), &font, param->vio_color[LANE_VIOLATION_CLR]); //20190222 hai Violation color

			//if (overlay_frame)
			//	cvReleaseImage(&overlay_frame);
		}

		int end_seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].endTime) / CLOCKS_PER_SEC);

		// ===== [release violation info] =====
		//20190714
		if (param->mot_ReverseSave[nTrackID].nReverseViolation == 0)
		{
			if (param->mot_ReverseSave[nTrackID].endTime != 0 && end_seconds >= 2) //2초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}
		else if (param->mot_ReverseSave[nTrackID].nReverseViolation == 1)
		{
			int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].catchTime) / CLOCKS_PER_SEC);

			//std::cout << "TimeOut ===============: " << seconds << std::endl;

			if (seconds >= 5) //5초 초과
			{
				darknet.release_reverse_violation(param, nTrackID);
			}
		}

		else if (((nClassID == cls_bike_rear || nClassID == cls_truckbike_rear)
			|| ((nClassID == cls_bike_side || nClassID == cls_truckbike_side) && pRegion->CRegion_line_touch_forLaneVio_side(rtResBox.x, nPtBoxRightX, nPtBottomY) == 0))
			&& param->mot_ReverseSave[nTrackID].nReverseVioCnt > 0 && param->mot_ReverseSave[nTrackID].nReverseVioCnt < nReverseMaxCount)
		{
			darknet.release_reverse_violation(param, nTrackID);
		}

		//else if (param->mot_ReverseSave[nTrackID].nLPR == 1)
		//{
		//	int seconds = (int)((float)(currentClock - param->mot_ReverseSave[nTrackID].lprTime) / CLOCKS_PER_SEC);

		//	//std::cout << "TimeOut for other cases ===============: " << seconds << std::endl;

		//	if (seconds >= 30) //30초 초과
		//	{
		//		release_reverse_violation(param, nTrackID);
		//	}
		//}

		param->mot_ReverseSave[nTrackID].endTime = currentClock; //// 2018.09.03
	}
}

////20190213 hai Accident Detection/Illegal Parking 
void CViolationProcess::accident_detection(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
	int ntrack, char * strTime, clock_t currentClock, float fHratio, float fWratio)
{
	if (pRegion == nullptr) return;
	cv::Point ptLine[MAX_PTLINE];
	pRegion->CRegion_copy(1, ptLine);
	cv::Point ptROI[4];
	pRegion->CRegion_copy(2, ptROI);

	//CvFont font;
	//cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX | CV_FONT_ITALIC, 0.7, 0.7, 0, 2, 1);

	int nClassID = param->mot_info->mot_box_info[ntrack].class_id;
	int nTrackID = param->mot_info->mot_box_info[ntrack].track_id;
	char *vio_file_path = param->vio_result_path;
	char full_save_image_path[CHAR_LENGTH];

	cv::Rect rtBoundingBox = param->mot_info->mot_box_info[ntrack].bouding_box;

	int nPtBottomX = (int)((float)(rtBoundingBox.x + (rtBoundingBox.width / 2)) / fWratio);
	int nPtBottomY = (int)((float)(rtBoundingBox.y + rtBoundingBox.height) / fHratio);

	cv::Rect rectArea;
	cv::Rect fullRect;

	//20180904 accident check area
	int nPtROIx02 = pRegion->CRegion_AccidentCheckArea(0, nPtBottomY);
	int nPtROIx13 = pRegion->CRegion_AccidentCheckArea(1, nPtBottomY);

	if (param->mot_DataSave[nTrackID].bCheckAccident == 0)
	{
		//20190215 hai Parking Indonesia
		//if (COUNT_MODE_COUNTRY == 1 && nPtBottomX > nPtROIx02 && nPtBottomX < nPtROIx13
		if ((COUNT_MODE_COUNTRY == 1 || COUNT_MODE_COUNTRY == 0) && nPtBottomX > nPtROIx02 && nPtBottomX < nPtROIx13
			&& (nPtBottomY - rtResBox.height / 3) < ptROI[2].y && (nPtBottomY - rtResBox.height / 3) > ptROI[0].y)
		{
			param->mot_DataSave[nTrackID].bCheckAccident = 1;
			param->mot_DataSave[nTrackID].parkingTime = currentClock;

			fullRect = param->mot_info->mot_box_info[ntrack].bouding_box;
			/*fullRect.x = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.x		* fXRate);
			fullRect.width = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.width	* fXRate);
			fullRect.y = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.y		* fYRate);
			fullRect.height = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.height	* fYRate);*/

			rectArea = fullRect;
			rectArea.y = rectArea.y + rectArea.height * 1 / 3;
			rectArea.height = rectArea.height * 2 / 3;

			if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
				param->mot_DataSave[nTrackID].pIplImageParkingStart.release();

			param->mot_DataSave[nTrackID].pIplImageParkingStart = frame.clone();
			//rectangle(param->mot_DataSave[nTrackID].pIplImageParkingStart, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], 2, 1, 0);
			darknet.draw_objectbox_forLPR(param->mot_DataSave[nTrackID].pIplImageParkingStart, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], false);
		}
		////20190215 hai Parking VietNam
		//else if (COUNT_MODE_COUNTRY == 0)
		//{
		//	if ((ptLine[0].y < ptLine[2].y && ptLine[1].x < nPtBottomX && ptLine[4].x > nPtBottomX && ptLine[1].y < nPtBottomY && ptLine[3].y > nPtBottomY)
		//		|| (ptLine[0].y >= ptLine[2].y && ptLine[3].x < nPtBottomX && ptLine[5].x > nPtBottomX && ptLine[3].y < nPtBottomY && ptLine[1].y > nPtBottomY))
		//	{
		//		param->mot_DataSave[nTrackID].bCheckAccident = 1;
		//	}
		//}
	}

	if (param->mot_DataSave[nTrackID].bCheckAccident)
	{
		//20190215 hai Parking Indonesia
		//if (COUNT_MODE_COUNTRY == 1)
		if (COUNT_MODE_COUNTRY == 1 || COUNT_MODE_COUNTRY == 0)
		{
			if (nPtBottomX > nPtROIx02 && nPtBottomX < nPtROIx13
				&& (nPtBottomY - rtResBox.height / 3) < ptROI[2].y && (nPtBottomY - rtResBox.height / 3) > ptROI[0].y)
			{
				int deltaTime = (int)((double)(currentClock - param->mot_DataSave[nTrackID].parkingTime) / CLOCKS_PER_SEC);
				if (deltaTime >= param->nIllegalTime /* SECONDS_PER_ONE_MINUTE*/ && param->mot_DataSave[nTrackID].bSaveAccident == 0) //20190218 hai Test 60->10
				{
					//20180904 accident confirm
					fullRect = param->mot_info->mot_box_info[ntrack].bouding_box;
					/*fullRect.x = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.x		* fXRate);
					fullRect.width = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.width	* fXRate);
					fullRect.y = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.y		* fYRate);
					fullRect.height = (int)((float)param->mot_info->mot_box_info[ntrack].bouding_box.height	* fYRate);*/

					rectArea = fullRect;
					rectArea.y = rectArea.y + rectArea.height * 1 / 3;
					rectArea.height = rectArea.height * 2 / 3;


					//1st image
					//int millisecond = (int)((float)(currentClock - clock()) / CLOCKS_PER_SEC * 100.0f) % 100;
					SYSTEMTIME  tCurTime;
					::GetLocalTime(&tCurTime);
					int millisecond = tCurTime.wMilliseconds / 10;
					//20200115 index counting for violation
					int n_millisecond = nVioCntArray[ILLEGALPARKING_CNT];

					//sprintf(full_save_image_path, "%s/stoparea/%02d_%s_%02d_%s%02d_01.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/stoparea/%02d_%s_%02d_%s%02d_01.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
					{
						SavePicture_Save(hSaveImageDll, full_save_image_path, &param->mot_DataSave[nTrackID].pIplImageParkingStart, 0, 1);
					}

					if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
					{
						param->mot_DataSave[nTrackID].pIplImageParkingStart.release();
					}

					//2nd image
					bool bUseClone = false;
					//sprintf(full_save_image_path, "%s/stoparea/%02d_%s_%02d_%s%02d_02.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, millisecond);
					sprintf(full_save_image_path, "%s/stoparea/%02d_%s_%02d_%s%02d_02.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, n_millisecond);

					/*if (fullRect.y + fullRect.height + 20 <= frame.rows - 1)
					{
						fullRect.height = fullRect.height + 20;
					}*/

					if (!clone_frame.empty())
					{
						bUseClone = true;
						//20190327 Hai
						clone_frame = frame.clone();
						//rectangle(clone_frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], 2, 1, 0);
						darknet.draw_objectbox_forLPR(clone_frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}
					else
					{
						//rectangle(frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], 2, 1, 0); //20190222 hai Violation color
						darknet.draw_objectbox_forLPR(frame, fullRect, param->vio_color[ILLEGAL_PARKING_CLR], true);

						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}

					//lpr image
					//sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_%s%02d_01_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					sprintf(full_save_image_path, "%s/lpr/input/%02d_%s_%02d_%s%02d_01_X%04d_Y%04d_W%04d_H%04d.jpg", vio_file_path, ILLEGALPARKING_CODE, param->camera_id, nClassID, strTime, n_millisecond, rectArea.x, rectArea.y, rectArea.width, rectArea.height);
					if (!bUseClone)
					{
						SavePicture_Save(hSaveImageDll, full_save_image_path, &frame, 0, 1);
					}
					else
					{
						SavePicture_Save(hSaveImageDll, full_save_image_path, &clone_frame, 0, 1);
					}

					param->mot_DataSave[nTrackID].bSaveAccident = 1;

					//20190716 Viewer Event
					CString strIllegalEvent;
					strIllegalEvent.Format(_T("%s\\%d %s Stop in the restricted are.txt"), param->viewer_event_path, RTSP_PORT_NUM, strTime);

					FILE *Illegalfile;
					errno_t e = _tfopen_s(&Illegalfile, strIllegalEvent, _T("wt,ccs=UTF-8"));
					CStdioFile f(Illegalfile);
					f.Close();

					//20200115 update index counting for violation
					nVioCntArray[ILLEGALPARKING_CNT]++;
					nVioCntArray[ILLEGALPARKING_CNT] = nVioCntArray[ILLEGALPARKING_CNT] % 100;
				}
				//Overlay
				else if (deltaTime >= param->nIllegalTime /* SECONDS_PER_ONE_MINUTE*/) //20190218 hai Test 60->10
				{
					//Mat  overlay_frame = cvCreateImage(cvSize(res_frame.cols, res_frame.rows), res_frame->depth, res_frame->nChannels);
					//cvCopy(res_frame, overlay_frame);
					////rectangle(overlay_frame, rtBoundingBox, cv::Scalar(0,0,255), -1);
					//rectangle(overlay_frame, rtBoundingBox, param->vio_color[ILLEGAL_PARKING_CLR], -1); //20190222 hai Violation color
					////rectangle(res_frame, rtBoundingBox, param->vio_color[ILLEGAL_PARKING_CLR], 2,1,0);
					//cvAddWeighted(overlay_frame, fOverlay_alpha, res_frame, 1 - fOverlay_alpha, 0, res_frame);

					param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR] = 1; //2019022

					//violation_text_display(res_frame, param, ntrack, nTrackID);
					darknet.violation_text_display(res_frame, param, rtResBox, nTrackID);
					////sprintf(textTmp, "PARKING");
					////putText(res_frame, textTmp, cv::Point((int)rtBoundingBox.x, (int)rtBoundingBox.y), &font, param->vio_color[ILLEGAL_PARKING_CLR]); //20190222 hai Violation color

					//if (overlay_frame)
					//	cvReleaseImage(&overlay_frame);

				}
			}
			else
			{
				param->mot_DataSave[nTrackID].bCheckAccident = 0;
				param->mot_DataSave[nTrackID].bSaveAccident = 0;
				param->mot_DataSave[nTrackID].parkingTime = currentClock;

				if (!param->mot_DataSave[nTrackID].pIplImageParkingStart.empty())
				{
					param->mot_DataSave[nTrackID].pIplImageParkingStart.release();
				}

				param->mot_DataSave[nTrackID].nVioDisplay[ILLEGAL_PARKING_CLR] = 0; //2019022
			}
		}
	}
}

#endif
