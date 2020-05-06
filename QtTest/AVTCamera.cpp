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
	int reg_res = qRegisterMetaType<FramePtr>("FramePtr");	// 注册为Qt元类型 发射信号要用到
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

// GetFeatureByName函数 所有可用feature(属性)
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
}

AVTCamera::~AVTCamera()
{

}

int AVTCamera::openCamera()
{
	/*	AVT相机开启的通用流程 详见AlliedVision提供的Cpp案例源码 没啥好说的	*/

	// 启动Vimba系统
	if (VmbErrorSuccess != m_vimba_system.Startup())
	{
		return camerabase::NoCameras;
	}
	
	if (VmbErrorSuccess != m_vimba_system.RegisterCameraListObserver(m_CameraObserver))	// 注册相机拔插观察者
	{
		return camerabase::NoCameras;
	}	

	m_vimba_system.GetCameras(m_avaliable_camera_list);	// 获取相机列表
	if (m_avaliable_camera_list.empty())
	{
		return camerabase::NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();	// 默认使用第一个
	if (VmbErrorSuccess != m_using_camera->Open(VmbAccessModeFull) || SP_ISNULL(m_using_camera))
	{
		return camerabase::OpenFailed;
	}

	// lzx 20200506 command_feature(用于执行GVSPAdjustPacketSize命令)往下的这段代码 从案例直接抄过来的 在这个相机上面没有这个属性
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

	FeaturePtr format_feature;						// 图像属性
	VmbInt64_t frame_width = 0, frame_height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", format_feature))
	{
		format_feature->GetValue(frame_width);		// 图像宽度
	}
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", format_feature))
	{
		format_feature->GetValue(frame_height);		// 图像高度
	}

	if (frame_width <= 0 || frame_height <= 0)		// 确保是正常的图像尺寸
		return camerabase::OpenFailed;
	m_frame_width = (int)frame_width;
	m_frame_height = (int)frame_height;
	m_frame_area = m_frame_width * m_frame_height;

	m_frame_pixelformat = -1;
	if (VmbErrorSuccess == SP_ACCESS(m_using_camera)->GetFeatureByName("PixelFormat", format_feature))
	{
		format_feature->GetValue(m_frame_pixelformat);	// VmbPixelFormatMono8 8bit rgb 单通道
	}

	// 给3个帧的空间作为相机的缓冲区大小 并开始连续采集 观察者开始工作
	if (VmbErrorSuccess != m_using_camera->StartContinuousImageAcquisition(3, m_FrameObserver))
	{
		return OpenStatus::OpenFailed;
	}

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

	// 安全性行为 确保目标数据与相机获取的帧大小一致
	if (frame->cols != m_frame_width || frame->rows != m_frame_height)
	{
		if (!frame->empty())
			frame->release();

		cv::Mat to_copy(m_frame_height, m_frame_width, CV_8UC1);
		to_copy.copyTo(*frame);
	}
	
	memcpy(frame->data, data, m_frame_area);	// frame的内存由调用者自行管理
	return true;
}

int AVTCamera::closeCamera()
{
	decltype(m_framedata_queue) to_clear_cache;
	m_framedata_queue.swap(to_clear_cache);			// 清除数据队列

	m_vimba_system.Shutdown();
	if (!SP_ISNULL(m_using_camera))
	{
		m_using_camera->StopContinuousImageAcquisition();
		if (VmbErrorSuccess != m_using_camera->Close())
			return camerabase::CloseFailed;
	}

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
	if (SP_ISNULL(frame))
		return;

	VmbUchar_t* new_frame_buffer = nullptr;
	frame->GetImage(new_frame_buffer);			// 解析成通用格式

	m_mutex.lock();
	m_framedata_queue.push(new_frame_buffer);	// 存储
	m_mutex.unlock();

	m_using_camera->QueueFrame(frame);			// 清理AVT相机的缓存空间 否则会进入呆滞状态
}
