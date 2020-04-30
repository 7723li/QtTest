#include "AVTCamera.h"

AVTCamera::AVTCamera() :
m_vimba_system(VimbaSystem::GetInstance())
{	

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

	m_vimba_system.GetCameras(m_avaliable_camera_list);
	if (m_avaliable_camera_list.empty())
	{
		return NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();

	if (VmbErrorSuccess != m_using_camera->Open(VmbAccessModeFull))
	{
		return OpenStatus::OpenFailed;
	}

	m_frame_cache.clear();		// ���֮ǰ�Ļ���
	m_frame_cache.resize(15);	// ��󻺳�����

	FeaturePtr feature;
	VmbInt64_t width = 0, height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", feature))
	{
		feature->GetValue(width);
	}
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", feature))
	{
		feature->GetValue(height);
	}
	for (auto frame = m_frame_cache.begin(); frame != m_frame_cache.end(); ++frame)
	{
		frame->reset(new Frame(width * height));
		(*frame)->RegisterObserver(IFrameObserverPtr(m_using_camera));
		m_using_camera->AnnounceFrame(*frame);
	}

	m_using_camera->StartCapture();
	return OpenStatus::OpenSucceed;
}

int AVTCamera::closeCamera()
{
	m_using_camera->Close();
	m_vimba_system.Shutdown();

	return CloseStatus::CloseSucceed;
}

void AVTCamera::CopyCurrentFrame(cv::Mat*)
{
	Fr
	m_using_camera->QueueFrame(m_frame_cache);
}

//int AVTCamera::test()
//{
//	if (m_avaliable_camera_list.empty())
//		return;
//
//	m_using_camera = m_avaliable_camera_list.front();
//
//	if (VmbErrorSuccess != m_using_camera->Open(VmbAccessModeFull))
//		return;
//	
//
//	
//
//	// �����������
//	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionMode", feature))
//	{
//		if (VmbErrorSuccess == feature->SetValue("Continuous"))
//		{
//			if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionStart",feature))
//			{
//				if (VmbErrorSuccess == feature->RunCommand())
//				{
//
//				}
//			}
//		}
//	}
//}
