#include "wifiqrrecognizer.h"

void processImage(SBarcodeDecoder *decoder, const QImage &image)
{
    decoder->process(image);
};

WifiQRRecognizer::WifiQRRecognizer(QObject *parent) : QObject(parent)
{
    m_decoderSurface = new DecoderVideoSurface;
}

bool WifiQRRecognizer::startRecognizing(int cameraIndex)
{
    if (m_camera != nullptr) {
        stopRecognizing();
    }

    m_decoder = new SBarcodeDecoder;
    QObject::connect(m_decoder, &SBarcodeDecoder::capturedChanged, this, &WifiQRRecognizer::onCaptureChanged,
                     Qt::QueuedConnection);
    m_decoderSurface->setDecoder(m_decoder);

    m_camera = new QCamera(QCameraInfo::availableCameras().at(cameraIndex));
    if (!m_camera) {
        qWarning() << "cant allocate camera instance";
        return false;
    }
    m_camera->setCaptureMode(QCamera::CaptureViewfinder);

    connect(m_camera, &QCamera::stateChanged, this, &WifiQRRecognizer::onCameraStateChanged);
    connect(m_camera, &QCamera::errorOccurred, this, &WifiQRRecognizer::displayCameraError);

    m_camera->load();
    QThread::msleep(500);

    return true;
}

void WifiQRRecognizer::displayCameraError(QCamera::Error error)
{
    qWarning() << "Camera error" << error << m_camera->errorString();
}

void WifiQRRecognizer::stopRecognizing()
{
    if (m_camera != nullptr) {
        m_camera->stop();
        delete m_camera;
        m_camera = nullptr;
    }
    if (m_decoder != nullptr) {
        delete m_decoder;
        m_decoder = nullptr;
    }
}

QStringList WifiQRRecognizer::camerasDevices()
{
    QStringList cameraDevs;
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    qWarning() << "num cameras detected:" << cameras.size();
    for (const QCameraInfo &cameraInfo : cameras) {
        cameraDevs += cameraInfo.deviceName();
    }
    return cameraDevs;
}

void WifiQRRecognizer::onCaptureChanged(const QString &captured)
{
    qWarning() << "captured" << captured;
    if (!captured.isEmpty()) {
        // "WIFI:S:Sunrise_5GHz_22F6A9;T:WPA;P:w9t5m7zJw1ke;H:false;;"
        QString ssid;
        QString password;
        const auto wifiParams = captured.split(";");
        for (const auto& wifiParam : wifiParams) {
            const auto params = wifiParam.split(":");
            if (params.size() < 2) {
                qWarning() << "Invalid wifi parameter" << params;
                continue;
            }
            if (params.size() == 3 && params.at(0) == "WIFI" && params.at(1) == "S") {
                ssid = params.at(2);
            }
            if (params.at(0) == "P") {
                password = params.at(1);
            }
        }
        if (ssid.isEmpty() || password.isEmpty()) {
            qWarning() << "Credentials not found";
        }
        stopRecognizing();
        Q_EMIT wifiRecognized(ssid, password);
        startRecognizing(0);
    }
}

void WifiQRRecognizer::onCameraStateChanged(QCamera::State state)
{
    qWarning() << "Camera state" << state;
    if (state == QCamera::LoadedState) {
        qWarning() << "supported wf framerates:";
        for (const auto& rng : m_camera->supportedViewfinderFrameRateRanges()) {
            qWarning() << "   " << rng.minimumFrameRate << rng.maximumFrameRate;
        }

        qWarning() << "supported wf pixel formats:" << m_camera->supportedViewfinderPixelFormats();
        qWarning() << "supported wf res:" << m_camera->supportedViewfinderResolutions();
        qWarning() << "supported wf settings:";
        for (const auto& set : m_camera->supportedViewfinderSettings()) {
            qWarning() << "   " << set.minimumFrameRate() << set.maximumFrameRate() << set.pixelAspectRatio() << set.pixelFormat() << set.resolution();
        }
        qWarning() << "wf settings:";
        qWarning() << "   "
                   << m_camera->viewfinderSettings().minimumFrameRate()
                   << m_camera->viewfinderSettings().maximumFrameRate()
                   << m_camera->viewfinderSettings().pixelAspectRatio()
                   << m_camera->viewfinderSettings().pixelFormat()
                   << m_camera->viewfinderSettings().resolution();

        m_camera->imageProcessing()->setColorFilter(QCameraImageProcessing::ColorFilterGrayscale);

        QCameraViewfinderSettings viewfinderSettings;
        viewfinderSettings.setResolution(1280, 720);
        viewfinderSettings.setMinimumFrameRate(10.0);
        viewfinderSettings.setMaximumFrameRate(10.0);
        m_camera->setViewfinderSettings(viewfinderSettings);
        m_camera->setViewfinder(m_decoderSurface);
        m_camera->start();

    }
}

QList<QVideoFrame::PixelFormat> DecoderVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    Q_UNUSED(handleType);
    return QList<QVideoFrame::PixelFormat>({QVideoFrame::Format_NV12});
}

bool DecoderVideoSurface::present(const QVideoFrame &frame)
{
    const QImage croppedCapturedImage = SBarcodeDecoder::videoFrameToImage(frame, QRect(0, 0, frame.width(), frame.height()));
    m_imageFuture = QtConcurrent::run(processImage, m_decoder, croppedCapturedImage);

    return true;
}

void DecoderVideoSurface::setDecoder(SBarcodeDecoder *newDecoder)
{
    m_decoder = newDecoder;
}
