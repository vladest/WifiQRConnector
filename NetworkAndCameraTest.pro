TEMPLATE = app
QT -= gui
QT += network multimedia multimedia-private concurrent dbus
CONFIG += console
CONFIG -= app_bundle
CONFIG += c++17
CONFIG+=link_pkgconfig

include(SCodes.pri)

PKGCONFIG += libnm
contains(QMAKE_CXX, .*arm.*)|contains(QMAKE_CXX, aarch64-linux-gnu-g++): {
    message("Building for ARM")
} else {
    message("Building for x64")
    LIBS += -L/usr/local/lib/x86_64-linux-gnu -L/usr/local/lib/x86_64-linux-gnu/libcamera
    INCLUDEPATH += /usr/local/include/libcamera
}
#PKGCONFIG += glib-2.0

LIBS += -lKF5NetworkManagerQt
DEFINES += QT_NO_KEYWORDS

INCLUDEPATH += /usr/include/KF5/NetworkManagerQt
SOURCES += \
        main.cpp \
        wificonnectionmanager.cpp \
        wifiqrrecognizer.cpp

HEADERS += \
    wificonnectionmanager.h \
    wifiqrrecognizer.h

unix: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


