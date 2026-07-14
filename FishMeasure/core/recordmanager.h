#pragma once
#include "../core/fishrecord.h"
#include <QObject>
#include <QList>

/**
 * @brief 测量记录管理器
 * 维护本次会话的所有鱼类记录，支持增删查
 */
class RecordManager : public QObject {
    Q_OBJECT
public:
    explicit RecordManager(QObject* parent = nullptr);

    void addRecord(const FishRecord& record);
    FishRecord* getRecord(int index);
    const QList<FishRecord>& records() const { return records_; }
    int count() const { return records_.size(); }
    void clear();

signals:
    void recordAdded(int index);
    void recordUpdated(int index);
    void cleared();

private:
    QList<FishRecord> records_;
    int seqCounter_ = 1;
};
