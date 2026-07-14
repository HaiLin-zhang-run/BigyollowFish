#pragma once
#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include "../core/recordmanager.h"

class FishListWidget : public QWidget {
    Q_OBJECT
public:
    explicit FishListWidget(RecordManager* manager, QWidget* parent = nullptr);
    
public slots:
    void onRecordAdded(int index);
    void onRecordUpdated(int index);
    void onCleared();

signals:
    void recordSelected(int index);

private slots:
    void onItemSelectionChanged();

private:
    RecordManager* manager_ = nullptr;
    QListWidget* listWidget_ = nullptr;
};
