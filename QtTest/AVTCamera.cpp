#include "AVTCamera.h"

void CameraObserver::CameraListChanged(CameraPtr pCamera, UpdateTriggerType reason)
{
	if (UpdateTriggerPluggedIn == reason ||
		UpdateTriggerPluggedOut == reason)
	{
		emit obsr_cameralist_changed(reason);
	}
}

FrameObserver::FrameObserver(CameraPtr camera) :
IFrameObserver(camera)
{
	int reg_res = qRegisterMetaType<FramePtr>("FramePtr");	// ע��ΪQtԪ���� �����ź�Ҫ�õ�
}

void FrameObserver::FrameReceived(const FramePtr newframe)
{
	bool is_inqueue = true;
	VmbFrameStatusType eReceiveStatus;

	if (SP_ISNULL(newframe))
		return;

	if (VmbErrorSuccess == newframe->GetReceiveStatus(eReceiveStatus))
	{
		if (VmbFrameStatusComplete == eReceiveStatus)
		{
			emit obsr_get_new_frame(newframe);
		}
		is_inqueue = false;
	}

	if (is_inqueue)
	{
		m_pCamera->QueueFrame(newframe);
	}
}

// ���п���feature(����) from GetFeatures
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
	m_frame_width = 0;
	m_frame_height = 0;
	m_frame_area = 0;
	m_is_connected = false;
	m_framerate = 0.0;

	SP_SET(m_CameraObserver, new CameraObserver);
	connect(SP_DYN_CAST(m_CameraObserver, CameraObserver).get(), &CameraObserver::obsr_cameralist_changed,
		this, &AVTCamera::slot_obsr_camera_list_changed);

	SP_SET(m_FrameObserver, new FrameObserver(m_using_camera));
	connect(SP_DYN_CAST(m_FrameObserver, FrameObserver).get(), &FrameObserver::obsr_get_new_frame,
		this, &AVTCamera::slot_obsr_get_new_frame);

	m_cnt_framerate_timer = new QTimer;
	connect(m_cnt_framerate_timer, &QTimer::timeout, this, &AVTCamera::slot_cnt_framerate);

	m_vimba_system.Startup();											// ����Vimbaϵͳ
	m_vimba_system.RegisterCameraListObserver(m_CameraObserver);		// ע������β�۲���
}

AVTCamera::~AVTCamera()
{

}

int AVTCamera::openCamera()
{
	/*	AVT������������� ��Դ���϶��� ������Щ����Ҳ��֪��ʲô��˼(!-_-) ����Ȥ���AlliedVision�ṩ���ֲ���Cpp����Դ�� */

	if (VmbErrorSuccess != m_vimba_system.GetCameras(m_avaliable_camera_list) ||	// ��ȡ����б�
		m_avaliable_camera_list.empty())
	{
		return camerabase::NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();		// Ĭ��ʹ�õ�һ��
	auto a = m_using_camera->Open(VmbAccessModeFull);
	if (SP_ISNULL(m_using_camera) || VmbErrorSuccess != a)
	{
		return camerabase::OpenFailed;
	}
	SP_DYN_CAST(m_FrameObserver, FrameObserver)->set_camera(m_using_camera);

	FeaturePtr property_feature;							// ͼ������
	VmbInt64_t frame_width = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", property_feature))
	{
		property_feature->GetValue(frame_width);			// ͼ����
	}
	m_frame_width = frame_width;

	VmbInt64_t frame_height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", property_feature))
	{
		property_feature->GetValue(frame_height);			// ͼ��߶�
	}
	m_frame_height = frame_height;

	VmbInt64_t payload = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("PayloadSize", property_feature))
	{
		property_feature->GetValue(payload);				// ͼ���ֽ���(���)
	}
	m_frame_area = payload;

	if (m_frame_width <= 0 || m_frame_height <= 0 || m_frame_area <= 0)
	{
		return camerabase::OpenFailed;						// ȷ����������ͼ��ߴ�
	}

	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("PixelFormat", property_feature))
	{
		property_feature->GetValue(m_frame_pixelformat);	// ͼ���ʽ VmbPixelFormatMono8 8bit rgb ��ͨ�� "VmbCommonTypes.h"
	}

	/* @todo ������ �������ļ���ȡ ������� */
	// COMMONFUNC::READCAMERACONFIG(CAMERACONFIG)

	/* �������� */
	/* �ɼ�ģʽ			AcquisitionMode */
	/* ֡��				AcquisitionFrameRateEnable & AcquisitionFrameRate */
	/* ��ƽ��			BlackLevel */
	/* �ع�ʱ��			ExposureAuto | ( ExposureAutoMax & ExposureAutoMin & ExposureTime) */
	/* ����				GainAuto | ��Gain & GainAutoMax & GainAutoMin */
	/* ����				IntensityControllerRate */

	std::string AcquisitionMode;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionMode", property_feature))
	{
		property_feature->GetValue(AcquisitionMode);
		if (0 != AcquisitionMode.compare("Continuous") &&
			VmbErrorSuccess != property_feature->SetValue("Continuous"))	// ����Ϊ�����ɼ�ģʽ
		{
			return camerabase::OpenFailed;
		}
	}

	bool AcquisitionFrameRateEnable = false;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionFrameRateEnable", property_feature))
	{
		property_feature->GetValue(AcquisitionFrameRateEnable);
		if (!AcquisitionFrameRateEnable &&
			VmbErrorSuccess != property_feature->SetValue(true))			// ���òɼ�֡��Ϊ������ģʽ
		{
			return camerabase::OpenFailed;
		}
	}
	double target_framerate = 60.0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionFrameRate", property_feature))
	{
		property_feature->GetValue(m_framerate);
		if (std::floor(m_framerate) != target_framerate &&
			VmbErrorSuccess != property_feature->SetValue(target_framerate))// ���òɼ�֡�� ע�����Ϊ������
		{
			return camerabase::OpenFailed;
		}
	}

	/* lzx 20200508 ����VimbaViewerԴ���ṩ�ı�׼(����...)������ʽ */
	static const int buffer_size = 8;							// 8
	m_AVT_framebuffer.resize(buffer_size);
	for (int i = 0; i < buffer_size; ++i)
	{
		m_AVT_framebuffer[i] = FramePtr(new Frame(payload));		// ���뻺��ռ�
	}
	for (int i = 0; i < buffer_size; ++i)
	{
		if (VmbErrorSuccess != m_AVT_framebuffer[i]->RegisterObserver(m_FrameObserver)// ע��۲���
			|| VmbErrorSuccess != m_using_camera->AnnounceFrame(m_AVT_framebuffer[i])	// �������֮����Ҫ�õ��Ļ���ռ�
			|| VmbErrorSuccess != m_using_camera->QueueFrame(m_AVT_framebuffer[i]))
		{
			return camerabase::OpenFailed;
		}
	}

	// AVT�����֮���� ����رպ����ȷ�򿪷�ʽ
	if (VmbErrorSuccess != m_using_camera->StartCapture() ||	// �����ʼ����
		VmbErrorSuccess != m_using_camera->EndCapture() ||
		VmbErrorSuccess != m_using_camera->StartCapture())
	{
		return camerabase::OpenFailed;
	}

	FeaturePtr command_feature;									// ����ִ�����������
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionStart", command_feature))	// ��ʼ�����ɼ�
	{
		if (VmbErrorSuccess != command_feature->RunCommand())
		{
			return camerabase::OpenFailed;
		}
	}

	m_framerate = 0;										// ����fps
	m_frame_obsr_cnt = 0;									// ���ûص���������
	m_save_frame_obsr_cnt = 0;								// ���ñ���Ļص�����
	m_is_connected = true;									// ����Ϊ������
	m_cnt_framerate_timer->start(1000);						// ��ʼ����fps	

	return OpenStatus::OpenSuccess;
}

bool AVTCamera::get_one_frame(cv::Mat & frame)
{
	if (m_framedata_queue.empty())
		return false;

	m_mutex.lock();
	uchar* data = m_framedata_queue.front();
	m_framedata_queue.pop();
	m_mutex.unlock();

	if (nullptr == data)
		return false;

	if (frame.cols != m_frame_width || frame.rows != m_frame_height)
	{
		frame = cv::Mat(cv::Size(m_frame_width, m_frame_height), CV_8UC1, data);
	}
	memcpy(frame.data, data, m_frame_area);

	return true;
}

int AVTCamera::closeCamera()
{
	if (SP_ISNULL(m_using_camera))
	{
		return camerabase::CloseFailed;
	}

	m_cnt_framerate_timer->stop();

	FeaturePtr command_feature;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionStop", command_feature))	// ֹͣ�ɼ�
	{
		if (VmbErrorSuccess != command_feature->RunCommand())
		{
			return camerabase::OpenFailed;
		}
	}

	if (VmbErrorSuccess != m_using_camera->EndCapture() ||			// ֹͣ��׽Ӱ��
		VmbErrorSuccess != m_using_camera->FlushQueue() ||			// ��ջ�����
		VmbErrorSuccess != m_using_camera->RevokeAllFrames() ||
		VmbErrorSuccess != m_using_camera->Close()					// �ر�����ͷ
		)
	{
		return camerabase::OpenFailed;
	}

	for (FramePtr& buffer : m_AVT_framebuffer)
	{
		buffer->UnregisterObserver();								// ע���۲���
		SP_RESET(buffer);											// ��������ָ�뼼�����ü��� ����
	}
	m_AVT_framebuffer.clear();

	if (!m_framedata_queue.empty())									// ������ݶ���
	{
		std::queue<uchar*> to_clear_cache;
		m_framedata_queue.swap(to_clear_cache);
	}

	m_is_connected = false;

	return camerabase::CloseSuccess;
}

double AVTCamera::get_framerate()
{
	if (m_framerate <= 0)
	{
		FeaturePtr cmd_feature;
		VmbInt64_t frame_rate = 0;
		if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionFrameRate", cmd_feature))
		{
			cmd_feature->GetValue(frame_rate);
			m_framerate = frame_rate;
		}
	}

	return m_framerate;
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

void AVTCamera::slot_cnt_framerate()
{
	// frame per second = ����һ���������ص��������ô��� = ����һ���ڵ��ü����ܺ� - ��һ�뱣�������ܺ�
	int cnt = m_frame_obsr_cnt;
	m_framerate = cnt - m_save_frame_obsr_cnt;
	m_save_frame_obsr_cnt = cnt;
}
