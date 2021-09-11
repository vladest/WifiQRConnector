#ifndef WIFIQRRECOGNIZER_H
#define WIFIQRRECOGNIZER_H

#include <QObject>
#include <QCamera>
#include <QCameraInfo>
#include <QElapsedTimer>
#include <QtConcurrent>

#include "qabstractvideosurface.h"
#include "SBarcodeDecoder.h"

class DecoderVideoSurface : public QAbstractVideoSurface
{
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;
    bool present(const QVideoFrame &frame);

public:
    void setDecoder(SBarcodeDecoder *newDecoder);

private:
    QFuture<void> m_imageFuture;
    SBarcodeDecoder* m_decoder = nullptr;
};

class WifiQRRecognizer : public QObject
{
    Q_OBJECT
public:
    explicit WifiQRRecognizer(QObject *parent = nullptr);
    bool startRecognizing(int cameraIndex);
    void stopRecognizing();
    static QStringList camerasDevices();

private Q_SLOTS:
    void onCaptureChanged(const QString& captured);
    void onCameraStateChanged(QCamera::State state);
    void displayCameraError(QCamera::Error error);

Q_SIGNALS:
    void wifiRecognized(const QString& ssid, const QString& password);

private:
    SBarcodeDecoder* m_decoder = nullptr;
    DecoderVideoSurface* m_decoderSurface = nullptr;
    QCamera *m_camera = nullptr;
    bool m_cameraFocused = false;
};

#endif // WIFIQRRECOGNIZER_H
