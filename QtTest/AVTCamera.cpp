#include "AVTCamera.h"

enum    { NUM_FRAMES = 1000, };

AVTCamera::AVTCamera() :
m_vimba_system(VimbaSystem::GetInstance())
{
	SP_SET(m_CameraObserver, new CameraObserver);
	connect(SP_DYN_CAST(m_CameraObserver, CameraObserver).get(), &CameraObserver::AVT_CameraListChanged, this, &AVTCamera::slot_camera_list_changed);

	SP_SET(m_FrameObserver, new FrameObserver(m_using_camera));
	connect(SP_DYN_CAST(m_FrameObserver, FrameObserver).get(), &FrameObserver::obsr_get_new_frame, this, &AVTCamera::slot_get_new_frame);
}

AVTCamera::~AVTCamera()
{

}

int AVTCamera::openCamera()
{
	if (VmbErrorSuccess != m_vimba_system.Startup())
	{
		return OpenStatus::NoCameras;
	}
	
	if (VmbErrorSuccess != m_vimba_system.RegisterCameraListObserver(m_CameraObserver))
	{
		return OpenStatus::NoCameras;
	}	

	m_vimba_system.GetCameras(m_avaliable_camera_list);
	if (m_avaliable_camera_list.empty())
	{
		return OpenStatus::NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();
	if (VmbErrorSuccess != m_using_camera->Open(VmbAccessModeFull))
	{
		return OpenStatus::OpenFailed;
	}

	FeaturePtr command_feature;
	// configures GigE cameras to use the largest possible packets. <Vimba C++ manual 4.12.1>
	if (VmbErrorSuccess == SP_ACCESS(m_using_camera)->GetFeatureByName("GVSPAdjustPacketSize", command_feature))
	{
		if (VmbErrorSuccess == SP_ACCESS(command_feature)->RunCommand())
		{
			bool is_command_done = false;
			do
			{
				if (VmbErrorSuccess != SP_ACCESS(command_feature)->IsCommandDone(is_command_done))
				{
					break;
				}
			} while (is_command_done == false);
		}
	}

	FeaturePtr format_feature;
	m_frame_width = 0, m_frame_height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", format_feature))
	{
		format_feature->GetValue(m_frame_width);
	}
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", format_feature))
	{
		format_feature->GetValue(m_frame_height);
	}
	if (VmbErrorSuccess == SP_ACCESS(m_using_camera)->GetFeatureByName("PixelFormat", format_feature))
	{
		format_feature->SetValue(VmbPixelFormatRgb8);
		format_feature->SetValue(VmbPixelFormatMono8);
		format_feature->GetValue(m_frame_pixelformat);
	}

	if (VmbErrorSuccess != m_using_camera->StartContinuousImageAcquisition(NUM_FRAMES, m_FrameObserver))
	{
		return OpenStatus::OpenFailed;
	}

	return OpenStatus::OpenSucceed;
}

void AVTCamera::getOneFrame(cv::Mat* frame)
{
	m_frame_mutex.lock();
	if (m_frame_queue.empty())
		return;

	FramePtr ret_frame = m_frame_queue.front();
	m_frame_queue.pop_front();
	m_frame_mutex.unlock();

	if (SP_ISNULL(ret_frame))
		return;

	VmbUchar_t* frame_nuffer;
	if (VmbErrorSuccess != ret_frame->GetImage(frame_nuffer))
	{
		return;
	}

	cv::Mat ori_mat(static_cast<int>(m_frame_height), static_cast<int>(m_frame_width), CV_8U);
	memcpy(ori_mat.data, frame_nuffer, m_frame_height * m_frame_width);
	ori_mat.copyTo(*frame);

	//	m_using_camera->QueueFrame(m_frame_cache);
}

int AVTCamera::closeCamera()
{
	m_using_camera->Close();
	m_vimba_system.Shutdown();

	return CloseStatus::CloseSucceed;
}

void AVTCamera::slot_camera_list_changed(int reason)
{
	if (UpdateTriggerPluggedIn == reason)
	{
		qDebug("");
	}
	else if (UpdateTriggerPluggedOut == reason)
	{
		qDebug("");
	}
}

void AVTCamera::slot_get_new_frame()
{
	m_frame_mutex.lock();
	SP_DYN_CAST(m_FrameObserver, FrameObserver)->get_frame_queue(m_frame_queue);
	m_frame_mutex.unlock();
}
