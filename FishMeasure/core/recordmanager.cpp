#include "recordmanager.h"

RecordManager::RecordManager(QObject* parent) : QObject(parent) {}

void RecordManager::addRecord(const FishRecord& record) {
    FishRecord r = record;
    if (r.id.isEmpty()) {
        r.id = FishRecord::generateId(seqCounter_++);
    }
    records_.append(r);
    emit recordAdded(records_.size() - 1);
}

FishRecord* RecordManager::getRecord(int index) {
    if (index >= 0 && index < records_.size()) {
        return &records_[index];
    }
    return nullptr;
}

void RecordManager::clear() {
    records_.clear();
    seqCounter_ = 1;
    emit cleared();
}
