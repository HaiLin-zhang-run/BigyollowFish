#include "fishlistwidget.h"

FishListWidget::FishListWidget(RecordManager* manager, QWidget* parent) 
    : QWidget(parent), manager_(manager) 
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    listWidget_ = new QListWidget(this);
    layout->addWidget(listWidget_);
    
    if (manager_) {
        connect(manager_, &RecordManager::recordAdded, this, &FishListWidget::onRecordAdded);
        connect(manager_, &RecordManager::recordUpdated, this, &FishListWidget::onRecordUpdated);
        connect(manager_, &RecordManager::cleared, this, &FishListWidget::onCleared);
    }
    
    connect(listWidget_, &QListWidget::itemSelectionChanged, this, &FishListWidget::onItemSelectionChanged);
}

void FishListWidget::onRecordAdded(int index) {
    if (!manager_) return;
    FishRecord* record = manager_->getRecord(index);
    if (record) {
        QString text = QString("%1 (%2)").arg(record->id, record->timestamp.toString("HH:mm:ss"));
        listWidget_->addItem(text);
        // 自动选中最新的一条
        listWidget_->setCurrentRow(listWidget_->count() - 1);
    }
}

void FishListWidget::onRecordUpdated(int index) {
    if (!manager_) return;
    FishRecord* record = manager_->getRecord(index);
    if (record && index < listWidget_->count()) {
        QString text = QString("%1 (%2)").arg(record->id, record->timestamp.toString("HH:mm:ss"));
        listWidget_->item(index)->setText(text);
    }
}

void FishListWidget::onCleared() {
    listWidget_->clear();
}

void FishListWidget::onItemSelectionChanged() {
    int row = listWidget_->currentRow();
    if (row >= 0) {
        emit recordSelected(row);
    }
}
