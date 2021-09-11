#include "wificonnectionmanager.h"


#include <QObject>
#include <QDebug>
#include <QUuid>

using namespace NetworkManager;

QString typeAsString(const NetworkManager::Device::Type type)
{
    switch (type) {
    case Device::UnknownType:
        return QString("Unknown");
    case Device::Ethernet:
        return QString("Ethernet");
    case Device::Wifi:
        return QString("Wifi");
    case Device::Unused1:
        return QString("Unused1");
    case Device::Unused2:
        return QString("Unused2");
    case Device::Bluetooth:
        return QString("Bluetooth");
    case Device::OlpcMesh:
        return QString("OlpcMesh");
    case Device::Wimax:
        return QString("Wimax");
    case Device::Modem:
        return QString("Modem");
    case Device::Vlan:
        return QString("Vlan");
    case Device::Bridge:
        return QString("Bridge");
    case Device::Generic:
        return QString("Generic (Loopback)");
    default:
        return QString("Unknown");
    }
    return QString("Unknown");
}

void QubotConnectionManager::replyFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    if (reply.isError() || !reply.isValid()) {
        QString error = reply.error().message();
            const QString interface = watcher->property("interface").toString();
            qWarning() << "Wireless scan on" << interface << "failed:" << error;
    } else {
            qDebug() << "Wireless scan on" << watcher->property("interface").toString() << "succeeded";
    }

    watcher->deleteLater();
}


QubotConnectionManager::QubotConnectionManager(QObject *parent): QThread(parent)
{
    const Device::List& list = networkInterfaces();


    m_settings = new ConnectionSettings(ConnectionSettings::Wireless);
    // List device configuration, not including vpn connections, which do not
    // have a real device tied to them.
    for (const Device::Ptr &dev : list) {
        if (dev->type() == Device::Wifi) {
            NetworkManager::WirelessDevice::Ptr wifiDev = dev.objectCast<NetworkManager::WirelessDevice>();
            m_wifiDevicesList << wifiDev;
            qDebug() << "Requesting wifi scan on device" << wifiDev->interfaceName();
            QDBusPendingReply<> reply = wifiDev->requestScan();
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
            watcher->setProperty("interface", wifiDev->interfaceName());
            connect(watcher, &QDBusPendingCallWatcher::finished,
                    this, &QubotConnectionManager::replyFinished);

        } else if (isModem(dev.get())) {
            m_modemDevicesList << dev->uni();
        }

        if (dev->type() == Device::Wifi || isModem(dev.get())) {
            m_wifiConnected = (dev->type() == Device::Wifi && dev->state() == Device::Activated);
            m_modemConnected = (isModem(dev.get()) && dev->state() == Device::Activated);
            connect(dev.get(), &NetworkManager::Device::stateChanged, this,
                    &QubotConnectionManager::onStateChanged);
        }
    }
}

void QubotConnectionManager::showAllDevices() {
    QTextStream qout(stdout, QIODevice::WriteOnly);

    const NetworkManager::Device::List& list = NetworkManager::networkInterfaces();

    // List device configuration, not including vpn connections, which do not
    // have a real device tied to them.
    for (const NetworkManager::Device::Ptr &dev : list) {
        qout << "\n=====\n";
        qout << dev->uni() << "\n";
        qout << "type: " << typeAsString(dev->type()) << " (" << dev->type() << ")\n";
        qout << "managed: " << dev->managed() << "\n";
        qout << "interface name: " << dev->interfaceName() << "\n";
        qout << "state: " << dev->state() << "\n";
        qout << "driver: " << dev->driver() << "\n";

        NetworkManager::IpConfig ipConfig = dev->ipV4Config();
        if (ipConfig.isValid()) {
            // static IPv4 configuration.
            if (ipConfig.addresses().isEmpty()) {
                qout << "ip address: <not set>\n";
            } else {
                NetworkManager::IpAddress address = ipConfig.addresses().at(0);
                qout << "ip address: " << address.ip().toString() << "\n";
                qout << "gateway: " << address.gateway().toString() << "\n";
                qout << "ip address (raw): " << dev->ipV4Address().toString() << "\n";

                // Static routes.
                if (ipConfig.routes().isEmpty()) {
                    qout << "routers: <not set>\n";
                } else {
                    qout << "routers: " << ipConfig.routes().at(0).ip().toString() << "\n";
                }

                if (ipConfig.nameservers().isEmpty()) {
                    qout << "nameserver: <not set>\n";
                } else {
                    qout << "nameserver: " << ipConfig.nameservers().at(0).toString() << "\n";
                }
            }
            // DHCPv4 configuration.
            NetworkManager::Dhcp4Config::Ptr dhcp4Config = dev->dhcp4Config();
            if (!dhcp4Config) {
                qout << "dhcp info unavailable\n";
            } else {
                qout << "Dhcp4 options (" << dhcp4Config->path() << "): ";
                QVariantMap options = dhcp4Config->options();
                QVariantMap::ConstIterator it = options.constBegin();
                QVariantMap::ConstIterator end = options.constEnd();
                for (; it != end; ++it) {
                    qout << it.key() << "=" << it.value().toString() << " ";
                }
                qout << "\n";

                qout << "(dhcp) ip address: " << dhcp4Config->optionValue("ip_address") << "\n";
                qout << "(dhcp) network: " << dhcp4Config->optionValue("network_number") << '/' << dhcp4Config->optionValue("subnet_cidr") << " ("
                     << dhcp4Config->optionValue("subnet_mask") << ")\n";

                if (dhcp4Config->optionValue("routers").isEmpty()) {
                    qout << "(dhcp) gateway(s): <not set>\n";
                } else {
                    qout << "(dhcp) gateway(s): " << dhcp4Config->optionValue("routers") << "\n";
                }

                if (dhcp4Config->optionValue("domain_name_servers").isEmpty()) {
                    qout << "(dhcp) domain name server(s): <not set>\n";
                } else {
                    qout << "(dhcp) domain name server(s): " << dhcp4Config->optionValue("domain_name_servers") << "\n";
                }
            }
        }

        const NetworkManager::Connection::List connections = dev->availableConnections();

        qout << "available connections: ";

        for (const NetworkManager::Connection::Ptr &con : connections) {
            qout << "  con: ";
            NetworkManager::ConnectionSettings::Ptr settings = con->settings();
            qout << "\"" << settings->id() << "\" ";
        }
    }
    qout << "\n";
}

void QubotConnectionManager::onWifiCredentialsChanged(const QString &ssid, const QString &password)
{
    qWarning() << "new credentials:" << ssid << password;
    connectWifi(ssid, password);
}

void QubotConnectionManager::onStateChanged(NetworkManager::Device::State newstate, NetworkManager::Device::State oldstate,
                                           NetworkManager::Device::StateChangeReason reason)
{
    Device* dev = static_cast<Device*>(sender());
    if (dev == nullptr)
        return;
    qWarning() << "new state for interface:" << dev->interfaceName() << newstate;
    const bool modem = !isModem(dev);
    m_wifiConnected = !modem && newstate == Device::Activated;
    m_modemConnected = modem && newstate == Device::Activated;
    Q_EMIT connectionChanged(!modem, modem ? m_modemConnected : m_wifiConnected);
}

QubotConnectionManager::~QubotConnectionManager()
{

}

void QubotConnectionManager::run()
{
}

bool QubotConnectionManager::isWifiConnected()
{
    if (!m_wifiConnected) {
        qWarning() << "Wifi not connected";
    }
    return m_wifiConnected;
}

bool QubotConnectionManager::connectWifi(const QString &ssid, const QString &password)
{
    qWarning() << "connecting wifi" << ssid << password;
    if (m_wifiDevicesList.size() <= 0) {
        qWarning() << "No WiFi devices in the system";
        return false;
    }
    // Now we will prepare our new connection, we have to specify ID and create new UUID
    m_settings->setId(ssid);
    m_settings->setUuid(QUuid::createUuid().toString().mid(1, QUuid::createUuid().toString().length() - 2));

    // For wireless setting we have to specify SSID
    WirelessSetting::Ptr wirelessSetting = m_settings->setting(Setting::Wireless).
                                           dynamicCast<WirelessSetting>();
    wirelessSetting->setSsid(ssid.toUtf8());

    Ipv4Setting::Ptr ipv4Setting = m_settings->setting(Setting::Ipv4).dynamicCast<Ipv4Setting>();
    ipv4Setting->setMethod(Ipv4Setting::Automatic);

    // Optional password setting. Can be skipped if you do not need encryption.
    WirelessSecuritySetting::Ptr wifiSecurity =
        m_settings->setting(Setting::WirelessSecurity).dynamicCast<WirelessSecuritySetting>();
    wifiSecurity->setKeyMgmt(WirelessSecuritySetting::WpaPsk);
    wifiSecurity->setPsk(password);
    wifiSecurity->setInitialized(true);
    wirelessSetting->setSecurity("802-11-wireless-security");

    auto wifiDev = m_wifiDevicesList.at(0);
    auto wifiNetwork = wifiDev->findNetwork(ssid);
    if (wifiNetwork.isNull()) {
        qWarning() << "Cant find network for ssid" << ssid << wifiDev->networks().size();
        return false;
    }
    auto wifiAp = wifiNetwork->referenceAccessPoint();
    if (wifiAp.isNull()) {
        qWarning() << "Cant find reference AP for ssid" << ssid << wifiNetwork->accessPoints().size();
        return false;
    }

    // We try to add and activate our new wireless connection
    QDBusPendingReply<QDBusObjectPath, QDBusObjectPath> reply =
        addAndActivateConnection(m_settings->toMap(), m_wifiDevicesList.at(0)->uni(), wifiAp->uni());

    reply.waitForFinished();

    // Check if this connection was added successfuly
    if (reply.isValid()) {
        // Now our connection should be added in NetworkManager and we can print all settings pre-filled from NetworkManager
        Connection connection(reply.value().path());
        ConnectionSettings::Ptr newSettings = connection.settings();
        // Print resulting settings
        qDebug() << (*newSettings.data());

        // Continue with adding secrets
        WirelessSecuritySetting::Ptr wirelessSecuritySetting =
            newSettings->setting(Setting::WirelessSecurity).dynamicCast<WirelessSecuritySetting>();
        if (!wirelessSecuritySetting->needSecrets().isEmpty()) {
            qDebug() << "Need secrets: " << wirelessSecuritySetting->needSecrets();
            // TODO: fill missing secrets
        }

    } else {
        qDebug() << "Connection failed: " << reply.error();
    }

    return true;
}


