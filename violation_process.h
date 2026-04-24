#pragma once
#include <afxtempl.h>
#include "DarknetData.h"

#if KITS_FEATURES_ENABLED == 1
	#define KITS_REVERSE_CONFIRMCNT		SPEED_STATS_PERIOD
	#define KITS_REVERSE_RESETCNT		5

	#define KITS_BOTH_ENTRY				0
	#define KITS_BOTH_EXIT				1
	#define KITS_LEFTENTRY_RIGHTEXIT	2
	#define KITS_LEFTEXIT_RIGHTENTRY	3

	#define KITS_STOP_CONFIRMCNT		SPEED_STATS_PERIOD
	#define KITS_STOP_ZEROCNT			5
	#define KITS_STOP_RESETCNT			5

	#define KITS_PEDESTRIAN_MAXCNT		3

	#define	KITS_EVENT_RELEASE_PERIOD	15
	#define KITS_REVERSE_MIN_SPEED		5
	#define KITS_STOP_MAX_SPEED			10
#endif

#if VDS_MODE == 1
	#define VDS_REVERSE_CONFIRMCNT		8
	#define VDS_REVERSE_RESETCNT		5

	#define VDS_BOTH_ENTRY				0
	#define VDS_BOTH_EXIT				1
	#define VDS_LEFTENTRY_RIGHTEXIT		2
	#define VDS_LEFTEXIT_RIGHTENTRY		3

	#define	VDS_EVENT_RELEASE_PERIOD	15
	#define VDS_REVERSE_MIN_SPEED		5
	#define VDS_FIX_BASED_WIDTH			1
#endif

#define FIX_HEIGHT						0.15f

using namespace cv;
using namespace std;

class CViolationProcess
{
public:
	CViolationProcess();
	~CViolationProcess();

	HANDLE hSaveImageDll;
	HMODULE hMSaveImageDll;

	int nTotalCount;

	void SavePicture_FreeDll();
	void SavePicture_LoadDll();
	void SavePicture_Start(int size, int nCamIndex);
	void SavePicture_Save(HANDLE hHandle, char* path, cv::Mat* image, int* saveParam, int isMat);
	void SavePicture_Stop();

public:
	int m_nSaveDebugImages;
	int m_nSpeedPeriod;
	float m_fLoopAngle;
	float m_fLoopLength;
	bool m_bHorizontalCheck;
	float		m_fWratio, m_fHratio;

#ifdef XYCOOR_MODE
	void XYCOOR_save_speed_log(char* strDate, char* strTime, int nTrackID, float fdeltaX, float fdeltaY, float fSpeed, int nX, int nY, float fDistanceX, float fDistanceY);

	CString XYCOOR_calculate_speed(DarknetData &darknet, CRegion* pRegion, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, int ntrack, float fHratio, float fWratio, unsigned long long timestamp);

#endif


#if ITS_MODE == 1
	void KITS_reverse_violation_analysis(int nMode, DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, float fHratio, float fWratio);

	void KITS_stop_violation_analysis(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, float fHratio, float fWratio);

	void KITS_pedestrian_detection(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, float fHratio, float fWratio);

	void KITS_save_log(char* strDate, char* strTime, char* eventType, int nDistance, int nEventCount, int nTrackID);

#elif VDS_MODE == 1
	CString VDS_application(DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, int nCheckReverseViolation, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox,
		CArray<int, int&> &nVioCntArray, int ntrack, float fHratio, float fWratio, unsigned long long timestamp, int nCamIndex);

	CString VDS_save_log(DarknetData &darknet, CRegion* pRegion, Mat& frame, st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex);
	void VDS_save_log_checkinfo(int nFixType, st_det_mot_param *param, int nTrackID, int nClassID, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex);
	int VDS_fix_invalid_case(st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX);
	float VDS_calculate_carlength(st_det_mot_param *param, int nTrackID, float fET_distanceY, float fEB_distanceY);			//ET: end-top, EB: end-bottom (for end time)
	void VDS_calculate_speed_occupancy(st_det_mot_param *param, CRegion* pRegion, int nTrackID, float fEB_distanceY, float fEB_distanceX, bool bUpdateOcpTime = false);
	void VDS_calculate_vehicleGap(CRegion* pRegion, st_det_mot_param *param, int ntrack, float fEB_distanceY, bool bUpdateSpeed = false);
	void VDS_reverse_violation_analysis(int nProcessStep, float fEB_distanceY, DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox,
		int ntrack, float fHratio, float fWratio);
	float VDS_get_fixed_speed(float fSpeed, float fdisspeed, float fdisspeed_1);

#if SPEED_KOREA_EXPRESSWAY_MODE == 1
	CString KEW_application(DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox,
		CArray<int, int&> &nVioCntArray, int ntrack, float fHratio, float fWratio, unsigned long long timestamp, int nCamIndex);

	void get_speed_list(st_det_mot_param *param, int nTrackID, float* fSpeedList, float* fTimeList = nullptr, bool bCenter = false);
	float get_decided_speed(float* fSpeedList, bool bCenter = false);
	float get_decided_plate_speed(st_det_mot_param *param, int nTrackID, int nClassID);
	CString XYCOOR_calculate_speed_simple(cv::Mat & res_image, DarknetData &darknet, CCamPosition* pCamPosition, CRegion* pRegion, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, int ntrack, float fHratio, float fWratio, unsigned long long timestamp);
	void SSM_calculate_speed(st_det_mot_param *param, int nTrackID, float fEB_distanceY, float fEB_distanceX);
	CString SSM_save_log(DarknetData &darknet, CRegion* pRegion, Mat& frame, st_det_mot_param *param, int ntrack, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex, char* img_name);
	void SSM_save_log_checkinfo(st_det_mot_param *param, int nTrackID, int nClassID, float fET_distanceY, float fEB_distanceY, float fET_distanceX, float fEB_distanceX, int nCamIndex, bool bFix = false);
	void save_all_info_forLPDR(DarknetData &darknet, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int ntrack, int nCamIndex, char* img_name);
#endif

#else

	void reverse_violation_both_LeftEntry(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, char * strTime, float fHratio, float fWratio);
	void reverse_violation_both_LeftExit(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, char * strTime, float fHratio, float fWratio);
	void reverse_violation_entry(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, char * strTime, float fHratio, float fWratio);
	void reverse_violation_exit(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, char * strTime, float fHratio, float fWratio);

	void accident_detection(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, char * strTime, clock_t currentClock, float fHratio, float fWratio);

#endif

#if KITS_FEATURES_ENABLED == 1 && ITS_MODE == 0
	// [Daeju] KITS pedestrian/stop/(reverse) handlers exposed inside VDS build
	void KITS_reverse_violation_analysis(int nMode, DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, int nEndY, float fHratio, float fWratio);

	void KITS_stop_violation_analysis(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, float fHratio, float fWratio);

	void KITS_pedestrian_detection(DarknetData &darknet, CRegion* pRegion, Mat& frame, Mat& res_frame, Mat& clone_frame, st_det_mot_param *param, sIOUbox& box_iou, int nTrackCount, cv::Rect rtResBox, CArray<int, int&> &nVioCntArray,
		int ntrack, float fHratio, float fWratio);

	void KITS_save_log(char* strDate, char* strTime, char* eventType, int nDistance, int nEventCount, int nTrackID);
#endif
};

