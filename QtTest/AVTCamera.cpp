#include "AVTCamera.h"

void CameraObserver::CameraListChanged(CameraPtr pCamera, UpdateTriggerType reason)
{
	if (UpdateTriggerPluggedIn == reason ||
		UpdateTriggerPluggedOut == reason)
	{
		emit obsr_cameralist_changed(reason);
	}
}

FrameObserver::FrameObserver(CameraPtr camera):
	IFrameObserver(camera)
{
	int reg_res = qRegisterMetaType<FramePtr>("FramePtr");	// ע��ΪQtԪ���� �����ź�Ҫ�õ�
}

void FrameObserver::FrameReceived(const FramePtr newframe)
{
	bool is_inqueue = true;
	VmbFrameStatusType eReceiveStatus;

	if (VmbErrorSuccess == newframe->GetReceiveStatus(eReceiveStatus))
	{
		if (SP_ISNULL(newframe))
			return;

		emit obsr_get_new_frame(newframe);
		is_inqueue = false;
	}

	if (is_inqueue)
	{
		m_pCamera->QueueFrame(newframe);
	}
}

// GetFeatureByName���� ���п���feature(����)
/*
AcquisitionFrameCount
AcquisitionFrameRate
AcquisitionFrameRateEnable
AcquisitionFrameRateMode
AcquisitionMode
AcquisitionStart
AcquisitionStatus
AcquisitionStatusSelector
AcquisitionStop
AutoModeRegionHeight
AutoModeRegionOffsetX
AutoModeRegionOffsetY
AutoModeRegionSelector
AutoModeRegionWidth
BlackLevel
BlackLevelSelector
ColorTransformationEnable
ContrastBrightLimit
ContrastConfigurationMode
ContrastDarkLimit
ContrastEnable
ContrastShape
CorrectionDataSize
CorrectionEntryType
CorrectionMode
CorrectionSelector
CorrectionSet
CorrectionSetDefault
DeviceFamilyName
DeviceFirmwareID
DeviceFirmwareIDSelector
DeviceFirmwareVersion
DeviceFirmwareVersionSelector
DeviceGenCPVersionMajor
DeviceGenCPVersionMinor
DeviceIndicatorLuminance
DeviceIndicatorMode
DeviceLinkSpeed
DeviceLinkThroughputLimit
DeviceLinkThroughputLimitMode
DeviceManufacturerInfo
DeviceModelName
DeviceReset
DeviceSFNCVersionMajor
DeviceSFNCVersionMinor
DeviceSFNCVersionSubMinor
DeviceScanType
DeviceSerialNumber
DeviceTemperature
DeviceTemperatureSelector
DeviceUserID
DeviceVendorName
DeviceVersion
ExposureAuto
ExposureAutoMax
ExposureAutoMin
ExposureMode
ExposureTime
FileAccessBuffer
FileAccessLength
FileAccessOffset
FileOpenMode
FileOperationExecute
FileOperationResult
FileOperationSelector
FileOperationStatus
FileProcessStatus
FileSelector
FileSize
FileStatus
Gain
GainAuto
GainAutoMax
GainAutoMin
GainSelector
Gamma
Height
HeightMax
IntensityAutoPrecedenceMode
IntensityControllerAlgorithm
IntensityControllerOutliersBright
IntensityControllerOutliersDark
IntensityControllerRate
IntensityControllerRegion
IntensityControllerSelector
IntensityControllerTarget
IntensityControllerTolerance
LineInverter
LineMode
LineSelector
LineSource
LineStatus
LineStatusAll
MaxDriverBuffersCount
OffsetX
OffsetY
PayloadSize
PixelFormat
PixelSize
ReverseX
ReverseY
SensorHeight
SensorWidth
StreamAnnounceBufferMinimum
StreamAnnouncedBufferCount
StreamBufferHandlingMode
StreamID
StreamIsGrabbing
StreamType
TestPendingAck
TriggerActivation
TriggerMode
TriggerSelector
TriggerSoftware
TriggerSource
UserSetDefault
UserSetLoad
UserSetSave
UserSetSelector
Width
WidthMax
*/

AVTCamera::AVTCamera() :
m_vimba_system(VimbaSystem::GetInstance())
{
	SP_SET(m_CameraObserver, new CameraObserver);
	connect(SP_DYN_CAST(m_CameraObserver, CameraObserver).get(), &CameraObserver::obsr_cameralist_changed, this, &AVTCamera::slot_obsr_camera_list_changed);

	SP_SET(m_FrameObserver, new FrameObserver(m_using_camera));
	connect(SP_DYN_CAST(m_FrameObserver, FrameObserver).get(), &FrameObserver::obsr_get_new_frame, this, &AVTCamera::slot_obsr_get_new_frame);

	m_is_connected = false;
	m_fps = 0;
	m_frame_obsr_cnt = 0;
	m_save_frame_obsr_cnt = 0;

	m_cnt_fps_timer = new QTimer;
	connect(m_cnt_fps_timer, &QTimer::timeout, this, &AVTCamera::slot_cnt_fps);
}

AVTCamera::~AVTCamera()
{

}

int AVTCamera::openCamera()
{
	/*	AVT���������ͨ������ ���AlliedVision�ṩ��Cpp����Դ�� ûɶ��˵��	*/

	if (VmbErrorSuccess != m_vimba_system.Startup())	// ����Vimbaϵͳ
	{
		return camerabase::NoCameras;
	}
	
	if (VmbErrorSuccess != m_vimba_system.RegisterCameraListObserver(m_CameraObserver))	// ע������β�۲���
	{
		return camerabase::NoCameras;
	}	

	m_vimba_system.GetCameras(m_avaliable_camera_list);	// ��ȡ����б�
	if (m_avaliable_camera_list.empty())
	{
		return camerabase::NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();	// Ĭ��ʹ�õ�һ��
	if (VmbErrorSuccess != m_using_camera->Open(VmbAccessModeFull) || SP_ISNULL(m_using_camera))
	{
		return camerabase::OpenFailed;
	}

	// lzx 20200506 command_feature(����ִ��GVSPAdjustPacketSize����)���µ���δ��� �Ӱ���ֱ�ӳ������� ������������û���������
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

	FeaturePtr format_feature;						// ͼ������
	VmbInt64_t frame_width = 0, frame_height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", format_feature))
	{
		format_feature->GetValue(frame_width);		// ͼ����
	}
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", format_feature))
	{
		format_feature->GetValue(frame_height);		// ͼ��߶�
	}

	if (frame_width <= 0 || frame_height <= 0)		// ȷ����������ͼ��ߴ�
		return camerabase::OpenFailed;

	m_frame_width = (int)frame_width;
	m_frame_height = (int)frame_height;
	m_frame_area = m_frame_width * m_frame_height;

	m_frame_pixelformat = -1;
	if (VmbErrorSuccess == SP_ACCESS(m_using_camera)->GetFeatureByName("PixelFormat", format_feature))
	{
		format_feature->GetValue(m_frame_pixelformat);	// VmbPixelFormatMono8 8bit rgb ��ͨ�� "VmbCommonTypes.h"
	}

	m_fps = 0;							// ����fps
	m_frame_obsr_cnt = 0;				// ���ûص��������ô���
	m_save_frame_obsr_cnt = 0;			// ���ñ���� ��һ��Ļص��������ô���

	// ��3��֡�Ŀռ���Ϊ����Ļ�������С ����ʼ�����ɼ� �۲��߿�ʼ����
	if (VmbErrorSuccess != m_using_camera->StartContinuousImageAcquisition(3, m_FrameObserver))
	{
		return OpenStatus::OpenFailed;
	}

	m_fps = 0;										// ����fps
	m_frame_obsr_cnt = 0;							// ���ûص���������
	m_cnt_fps_timer->start(1000);					// ��ʼ����fps
	m_is_connected = true;							// ����Ϊ������
	return OpenStatus::OpenSuccess;
}

bool AVTCamera::get_one_frame(cv::Mat* frame)
{
	if (m_framedata_queue.empty())
		return false;

	m_mutex.lock();
	uchar* data = m_framedata_queue.front();
 	m_framedata_queue.pop();
 	m_mutex.unlock();

	if (nullptr == data)
		return false;

	// ��ȫ����Ϊ ȷ��Ŀ�������������ȡ��֡��Сһ�� ��ÿ���ȷ����Ҫ����������һ��!!!
	if (frame->cols != m_frame_width || frame->rows != m_frame_height)
	{
		if (!frame->empty())
			frame->release();

		cv::Mat to_copy(m_frame_height, m_frame_width, CV_8UC1);
		to_copy.copyTo(*frame);
	}
	
	memcpy(frame->data, data, m_frame_area);	// frame���ڴ��ɵ��������й���
	return true;
}

int AVTCamera::closeCamera()
{
	if (VmbErrorSuccess != m_vimba_system.Shutdown())	// �ر�ϵͳ(ԭ����ֻ��Ҫ��һ���Ϳ���)
	{
		return camerabase::CloseFailed;
	}

	if (m_cnt_fps_timer->isActive())					// ֹͣ����fps
	{
		m_cnt_fps_timer->stop();
	}

	if (!SP_ISNULL(m_using_camera))						// ֹͣ�ɼ� ����ջ�����
	{
		m_using_camera->StopContinuousImageAcquisition();
		m_using_camera->FlushQueue();
		m_using_camera->RevokeAllFrames();		
	}

	if (!m_framedata_queue.empty())						// ������ݶ���
	{
		std::queue<uchar*> to_clear_cache;
		m_framedata_queue.swap(to_clear_cache);
	}

	if (!SP_ISNULL(m_using_camera) &&
		VmbErrorSuccess != m_using_camera->Close())		// �ر�����ͷ
	{
		return camerabase::CloseFailed;
	}

	m_is_connected = false;

	return camerabase::CloseSuccess;
}

void AVTCamera::slot_obsr_camera_list_changed(int reason)
{
	if (UpdateTriggerPluggedIn == reason)
	{
		emit camera_plugin();
	}
	else if (UpdateTriggerPluggedOut == reason)
	{
		emit camera_plugout();
	}
}

void AVTCamera::slot_obsr_get_new_frame(FramePtr frame)
{
	if (SP_ISNULL(frame))						// ��ȫ��ʩ �����������
		return;

	++m_frame_obsr_cnt;							// ���ӻص��������ü���
	if (m_frame_obsr_cnt == INT_MAX)			// ��ֹ��ֵ���
	{
		m_frame_obsr_cnt = 1;
		m_save_frame_obsr_cnt = 0;
	}

	VmbUchar_t* new_frame_buffer = nullptr;
	frame->GetImage(new_frame_buffer);			// ������ͨ�ø�ʽ

	m_mutex.lock();
	m_framedata_queue.push(new_frame_buffer);	// �洢�����еȴ�ȡ��
	m_mutex.unlock();

	m_using_camera->QueueFrame(frame);			// ����AVT����Ļ���ռ� �����������״̬
}

void AVTCamera::slot_cnt_fps()
{
	// frame per second = ����һ���������ص��������ô��� = ����һ���ڵ��ü����ܺ� - ��һ�뱣�������ܺ�
	int cnt = m_frame_obsr_cnt;
	if (cnt != m_save_frame_obsr_cnt)
	{
		m_fps = cnt - m_save_frame_obsr_cnt;
		m_save_frame_obsr_cnt = cnt;
	}

	qDebug() << m_fps;
}
