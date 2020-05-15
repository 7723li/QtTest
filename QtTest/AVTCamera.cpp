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
	int reg_res = qRegisterMetaType<FramePtr>("FramePtr");	// 注册为Qt元类型 发射信号要用到
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

// 所有可用feature(属性) from GetFeatures
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

	m_vimba_system.Startup();											// 启动Vimba系统
	m_vimba_system.RegisterCameraListObserver(m_CameraObserver);		// 注册相机拔插观察者
}

AVTCamera::~AVTCamera()
{

}

int AVTCamera::openCamera()
{
	/*	AVT的相机开启流程 从源码缝合而成 所以有些东西也不知道什么意思(!-_-) 有兴趣详见AlliedVision提供的手册与Cpp案例源码 */

	if (VmbErrorSuccess != m_vimba_system.GetCameras(m_avaliable_camera_list) ||	// 获取相机列表
		m_avaliable_camera_list.empty())
	{
		return camerabase::NoCameras;
	}

	m_using_camera = m_avaliable_camera_list.front();		// 默认使用第一个
	auto a = m_using_camera->Open(VmbAccessModeFull);
	if (SP_ISNULL(m_using_camera) || VmbErrorSuccess != a)
	{
		return camerabase::OpenFailed;
	}
	SP_DYN_CAST(m_FrameObserver, FrameObserver)->set_camera(m_using_camera);

	FeaturePtr property_feature;							// 图像属性
	VmbInt64_t frame_width = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Width", property_feature))
	{
		property_feature->GetValue(frame_width);			// 图像宽度
	}
	m_frame_width = frame_width;

	VmbInt64_t frame_height = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("Height", property_feature))
	{
		property_feature->GetValue(frame_height);			// 图像高度
	}
	m_frame_height = frame_height;

	VmbInt64_t payload = 0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("PayloadSize", property_feature))
	{
		property_feature->GetValue(payload);				// 图像字节数(面积)
	}
	m_frame_area = payload;

	if (m_frame_width <= 0 || m_frame_height <= 0 || m_frame_area <= 0)
	{
		return camerabase::OpenFailed;						// 确保是正常的图像尺寸
	}

	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("PixelFormat", property_feature))
	{
		property_feature->GetValue(m_frame_pixelformat);	// 图像格式 VmbPixelFormatMono8 8bit rgb 单通道 "VmbCommonTypes.h"
	}

	/* @todo 待补充 从配置文件读取 设置相机 */
	// COMMONFUNC::READCAMERACONFIG(CAMERACONFIG)

	/* 可配置项 */
	/* 采集模式			AcquisitionMode */
	/* 帧率				AcquisitionFrameRateEnable & AcquisitionFrameRate */
	/* 白平衡			BlackLevel */
	/* 曝光时间			ExposureAuto | ( ExposureAutoMax & ExposureAutoMin & ExposureTime) */
	/* 增益				GainAuto | （Gain & GainAutoMax & GainAutoMin */
	/* 亮度				IntensityControllerRate */

	std::string AcquisitionMode;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionMode", property_feature))
	{
		property_feature->GetValue(AcquisitionMode);
		if (0 != AcquisitionMode.compare("Continuous") &&
			VmbErrorSuccess != property_feature->SetValue("Continuous"))	// 设置为连续采集模式
		{
			return camerabase::OpenFailed;
		}
	}

	bool AcquisitionFrameRateEnable = false;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionFrameRateEnable", property_feature))
	{
		property_feature->GetValue(AcquisitionFrameRateEnable);
		if (!AcquisitionFrameRateEnable &&
			VmbErrorSuccess != property_feature->SetValue(true))			// 设置采集帧率为可设置模式
		{
			return camerabase::OpenFailed;
		}
	}
	double target_framerate = 60.0;
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionFrameRate", property_feature))
	{
		property_feature->GetValue(m_framerate);
		if (std::floor(m_framerate) != target_framerate &&
			VmbErrorSuccess != property_feature->SetValue(target_framerate))// 设置采集帧率 注意必须为浮点数
		{
			return camerabase::OpenFailed;
		}
	}

	/* lzx 20200508 根据VimbaViewer源码提供的标准(大嘘...)启动方式 */
	static const int buffer_size = 8;							// 8
	m_AVT_framebuffer.resize(buffer_size);
	for (int i = 0; i < buffer_size; ++i)
	{
		m_AVT_framebuffer[i] = FramePtr(new Frame(payload));		// 申请缓冲空间
	}
	for (int i = 0; i < buffer_size; ++i)
	{
		if (VmbErrorSuccess != m_AVT_framebuffer[i]->RegisterObserver(m_FrameObserver)// 注册观察者
			|| VmbErrorSuccess != m_using_camera->AnnounceFrame(m_AVT_framebuffer[i])	// 声明相机之后需要用到的缓存空间
			|| VmbErrorSuccess != m_using_camera->QueueFrame(m_AVT_framebuffer[i]))
		{
			return camerabase::OpenFailed;
		}
	}

	// AVT相机迷之机制 意外关闭后的正确打开方式
	if (VmbErrorSuccess != m_using_camera->StartCapture() ||	// 相机开始工作
		VmbErrorSuccess != m_using_camera->EndCapture() ||
		VmbErrorSuccess != m_using_camera->StartCapture())
	{
		return camerabase::OpenFailed;
	}

	FeaturePtr command_feature;									// 用来执行命令的属性
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionStart", command_feature))	// 开始连续采集
	{
		if (VmbErrorSuccess != command_feature->RunCommand())
		{
			return camerabase::OpenFailed;
		}
	}

	m_framerate = 0;										// 重置fps
	m_frame_obsr_cnt = 0;									// 重置回调函数计数
	m_save_frame_obsr_cnt = 0;								// 重置保存的回调计数
	m_is_connected = true;									// 设置为已连接
	m_cnt_framerate_timer->start(1000);						// 开始计算fps	

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
	if (VmbErrorSuccess == m_using_camera->GetFeatureByName("AcquisitionStop", command_feature))	// 停止采集
	{
		if (VmbErrorSuccess != command_feature->RunCommand())
		{
			return camerabase::OpenFailed;
		}
	}

	if (VmbErrorSuccess != m_using_camera->EndCapture() ||			// 停止捕捉影像
		VmbErrorSuccess != m_using_camera->FlushQueue() ||			// 清空缓冲区
		VmbErrorSuccess != m_using_camera->RevokeAllFrames() ||
		VmbErrorSuccess != m_using_camera->Close()					// 关闭摄像头
		)
	{
		return camerabase::OpenFailed;
	}

	for (FramePtr& buffer : m_AVT_framebuffer)
	{
		buffer->UnregisterObserver();								// 注销观察者
		SP_RESET(buffer);											// 减少智能指针技术引用技术 析构
	}
	m_AVT_framebuffer.clear();

	if (!m_framedata_queue.empty())									// 清除数据队列
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
	if (SP_ISNULL(frame))						// 安全措施 避免错误数据
		return;

	++m_frame_obsr_cnt;							// 增加回调函数调用计数
	if (m_frame_obsr_cnt == INT_MAX)			// 防止数值溢出
	{
		m_frame_obsr_cnt = 1;
		m_save_frame_obsr_cnt = 0;
	}

	VmbUchar_t* new_frame_buffer = nullptr;
	frame->GetImage(new_frame_buffer);			// 解析成通用格式

	m_mutex.lock();
	m_framedata_queue.push(new_frame_buffer);	// 存储至队列等待取出
	m_mutex.unlock();

	m_using_camera->QueueFrame(frame);			// 清理AVT相机的缓存空间 否则会进入呆滞状态
}

void AVTCamera::slot_cnt_framerate()
{
	// frame per second = 最新一秒内新增回调函数调用次数 = 最新一秒内调用计数总和 - 上一秒保留计数总和
	int cnt = m_frame_obsr_cnt;
	m_framerate = cnt - m_save_frame_obsr_cnt;
	m_save_frame_obsr_cnt = cnt;
}
