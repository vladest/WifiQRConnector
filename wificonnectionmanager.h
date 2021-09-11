#ifndef WIFICONNECTIONMANAGER_H
#define WIFICONNECTIONMANAGER_H

#include <QThread>

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Connection>
#include <NetworkManagerQt/Device>
#include <NetworkManagerQt/Manager>
#include <NetworkManagerQt/AccessPoint>
#include <NetworkManagerQt/ConnectionSettings>
#include <NetworkManagerQt/Ipv4Setting>
#include <NetworkManagerQt/WirelessDevice>
#include <NetworkManagerQt/WirelessSecuritySetting>
#include <NetworkManagerQt/WirelessSetting>

class QubotConnectionManager : public QThread
{
    Q_OBJECT
public:

    explicit QubotConnectionManager(QObject *parent = nullptr);
    ~QubotConnectionManager() override;

    bool isWifiConnected();
    bool connectWifi(const QString& ssid, const QString& password);

    void showAllDevices();

public Q_SLOTS:
    void onWifiCredentialsChanged(const QString& ssid, const QString& password);

Q_SIGNALS:
    void connectionChanged(bool wifi, bool connected);

private Q_SLOTS:
    void onStateChanged(NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate,
                        NetworkManager::Device::StateChangeReason reason);

    void replyFinished(QDBusPendingCallWatcher *watcher);
private:
    inline bool isModem(NetworkManager::Device* dev) {
        return (dev->type() == NetworkManager::Device::Ethernet && dev->interfaceName().startsWith("usb") &&
                dev->driver().contains("rndis"));
    }

    Q_DISABLE_COPY_MOVE(QubotConnectionManager)

protected:
    void run() override;
private:
    bool m_wifiConnected = false;
    bool m_modemConnected = false;
    NetworkManager::ConnectionSettings *m_settings = nullptr;
    NetworkManager::WirelessDevice::List m_wifiDevicesList;
    QStringList m_modemDevicesList;
};

#endif // WIFICONNECTIONMANAGER_H
