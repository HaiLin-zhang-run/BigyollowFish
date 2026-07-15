#pragma once
#include <QObject>
#include <QSerialPort>
#include <QByteArray>

/**
 * @brief 串口电子秤通信模块
 * 协议格式 (参考现有F项目):
 *   SOH(0x01) + STX(0x02) + STATUS + SIGN + WEIGHT(6bytes) + UNIT(2bytes) + EOT(0x04) + STA2
 */
class ScaleSerial : public QObject {
    Q_OBJECT
public:
    explicit ScaleSerial(QObject* parent = nullptr);
    ~ScaleSerial();

    // 扫描可用串口
    static QStringList availablePorts();

public slots:
    void connectPort(const QString& portName,
                     int baudRate = QSerialPort::Baud9600);
    void disconnectPort();
    void tare();   // 去皮 "T\r\n"
    void zero();   // 归零 "Z\r\n"
    void calibrate(double standardGrams); // 校准

    bool isConnected() const { return port_ && port_->isOpen(); }

signals:
    void weightUpdated(double grams);      // 新的重量值(g)
    void statusChanged(const QString& s);  // 状态文本
    void connected(bool ok);

private slots:
    void onReadyRead();
    void onError(QSerialPort::SerialPortError err);

private:
    QSerialPort* port_ = nullptr;
    QByteArray   buf_;

    void parsePacket(const QByteArray& data);
    bool sendCmd(const QString& cmd);
};
