#include <QCoreApplication>
#include <QDebug>

#include <wificonnectionmanager.h>
#include <wifiqrrecognizer.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QubotConnectionManager connMan;
    connMan.showAllDevices();
    WifiQRRecognizer wifiRecognizer;
    QObject::connect(&wifiRecognizer, &WifiQRRecognizer::wifiRecognized, &connMan,
                     &QubotConnectionManager::onWifiCredentialsChanged, Qt::QueuedConnection);
    if (!connMan.isWifiConnected()) {
        qWarning() << "No WiFi connected" << WifiQRRecognizer::camerasDevices();
        wifiRecognizer.startRecognizing(0);
    }

    return a.exec();
}
