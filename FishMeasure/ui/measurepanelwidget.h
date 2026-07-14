#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include "../core/fishrecord.h"

class MeasurePanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit MeasurePanelWidget(QWidget* parent = nullptr);
    
public slots:
    void updateData(const FishMorphology& morpho);
    void clearData();
    
private:
    QTableWidget* tableWidget_;
    QPushButton*  saveButton_;
};
