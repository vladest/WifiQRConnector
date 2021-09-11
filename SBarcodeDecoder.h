#ifndef QR_DECODER_H
#define QR_DECODER_H

#include <QObject>
#include <QVideoFrame>

class SBarcodeDecoder : public QObject
{
    Q_OBJECT

public:
    explicit SBarcodeDecoder(QObject *parent = nullptr);

    void clean();

    bool isDecoding() const;
    QString captured() const;

    static QImage videoFrameToImage(const QVideoFrame &videoFrame, const QRect &captureRect);
    static QImage imageFromVideoFrame(const QVideoFrame &videoFrame);

public Q_SLOTS:
    void process(const QImage &capturedImage);

Q_SIGNALS:
    void isDecodingChanged(bool isDecoding);
    void capturedChanged(const QString &captured);

private:
    bool _isDecoding = false;
    QString _captured = "";

    void setCaptured(const QString &captured);
    void setIsDecoding(bool isDecoding);
};

#endif // QR_DECODER_H
