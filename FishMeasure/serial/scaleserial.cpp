#include "scaleserial.h"
#include <QSerialPortInfo>
#include <QDebug>

ScaleSerial::ScaleSerial(QObject* parent) : QObject(parent) {}

ScaleSerial::~ScaleSerial() {
    disconnectPort();
}

QStringList ScaleSerial::availablePorts() {
    QStringList list;
    for (const auto& info : QSerialPortInfo::availablePorts())
        list << info.portName();
    return list;
}

void ScaleSerial::connectPort(const QString& portName, int baudRate) {
    if (port_ && port_->isOpen()) port_->close();
    delete port_;

    port_ = new QSerialPort(this);
    port_->setPortName(portName);
    port_->setBaudRate(baudRate);
    port_->setDataBits(QSerialPort::Data8);
    port_->setParity(QSerialPort::NoParity);
    port_->setStopBits(QSerialPort::OneStop);
    port_->setFlowControl(QSerialPort::NoFlowControl);

    connect(port_, &QSerialPort::readyRead, this, &ScaleSerial::onReadyRead);
    connect(port_, &QSerialPort::errorOccurred, this, &ScaleSerial::onError);

    bool ok = port_->open(QIODevice::ReadWrite);
    emit connected(ok);
    if (!ok)
        emit statusChanged(QString("连接失败: %1").arg(port_->errorString()));
}

void ScaleSerial::disconnectPort() {
    if (port_ && port_->isOpen()) port_->close();
    delete port_; port_ = nullptr;
    emit connected(false);
}

void ScaleSerial::tare() { sendCmd("T"); }
void ScaleSerial::zero() { sendCmd("Z"); }

bool ScaleSerial::sendCmd(const QString& cmd) {
    if (!port_ || !port_->isOpen()) return false;
    return port_->write((cmd + "\r\n").toUtf8()) > 0;
}

void ScaleSerial::onReadyRead() {
    buf_ += port_->readAll();
    // 按EOT(0x04)拆包
    int eotIdx = buf_.indexOf(0x04);
    while (eotIdx != -1) {
        QByteArray pkt = buf_.left(eotIdx + 2);
        parsePacket(pkt);
        buf_ = buf_.mid(eotIdx + 2);
        eotIdx = buf_.indexOf(0x04);
    }
}

void ScaleSerial::parsePacket(const QByteArray& data) {
    // 协议: SOH(0x01) STX(0x02) STATUS SIGN WEIGHT(6) UNIT(2) EOT(0x04) STA2
    if (data.length() < 15 || (uchar)data[0] != 0x01 || (uchar)data[1] != 0x02)
        return;

    char status = data[2];
    QString statusStr;
    if      (status == 'S') statusStr = "稳定";
    else if (status == 'U') statusStr = "不稳定";
    else if (status == 'F') statusStr = "溢出";
    else                    statusStr = "未知";

    QString sign   = (data[3] == '-') ? "-" : "";
    QString weight = sign + QString::fromLatin1(data.mid(4, 6)).trimmed();
    QString unit   = QString::fromLatin1(data.mid(10, 2)).trimmed();

    bool ok;
    double grams = weight.toDouble(&ok);
    // 单位换算
    if (unit == "kg") grams *= 1000.0;
    else if (unit == "lb") grams *= 453.592;

    if (ok) emit weightUpdated(grams);
    emit statusChanged(QString("%1 | %2 %3").arg(statusStr, weight, unit));
}

void ScaleSerial::onError(QSerialPort::SerialPortError err) {
    if (err != QSerialPort::NoError)
        emit statusChanged(QString("串口错误: %1").arg(port_->errorString()));
}
