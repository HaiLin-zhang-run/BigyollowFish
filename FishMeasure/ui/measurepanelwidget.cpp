#include "measurepanelwidget.h"
#include <QHeaderView>

MeasurePanelWidget::MeasurePanelWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("鱼类表型具体数据", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #d4d4d4;");
    layout->addWidget(titleLabel);
    
    tableWidget_ = new QTableWidget(18, 2, this);
    tableWidget_->setHorizontalHeaderLabels(QStringList() << "测量项" << "数值");
    tableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableWidget_->verticalHeader()->setVisible(false);
    
    // 工业风暗色表格样式
    tableWidget_->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
            gridline-color: #3f3f46;
            border: 1px solid #3f3f46;
            border-radius: 4px;
        }
        QHeaderView::section {
            background-color: #252526;
            color: #007acc;
            font-weight: bold;
            border: none;
            border-bottom: 1px solid #3f3f46;
            border-right: 1px solid #3f3f46;
            padding: 4px;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QTableWidget::item:selected {
            background-color: #333337;
            color: #ffffff;
        }
    )");
    
    QStringList items = {
        "全长", "体长", "头长", "躯干长", "尾长", "吻长", "眼长",
        "眼后头长", "尾柄长", "体高", "尾柄高", "尾鳍长度", "臀鳍长度",
        "厚度", "体重", "胸鳍长度", "眼鳍距", "黄蓝值"
    };
    
    for (int i = 0; i < items.size(); ++i) {
        QTableWidgetItem* nameItem = new QTableWidgetItem(items[i]);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        tableWidget_->setItem(i, 0, nameItem);
        
        QTableWidgetItem* valItem = new QTableWidgetItem("0.0");
        tableWidget_->setItem(i, 1, valItem);
    }
    
    layout->addWidget(tableWidget_);
    
    // Bottom buttons can be added here
}

void MeasurePanelWidget::updateData(const FishMorphology& morpho) {
    if (!morpho.isValid()) return;
    
    QList<float> values = {
        morpho.totalLength, morpho.bodyLength, morpho.headLength, morpho.trunkLength,
        morpho.tailLength, morpho.snoutLength, morpho.eyeLength, morpho.postEyeHeadLength,
        morpho.caudPedLength, morpho.bodyHeight, morpho.caudPedHeight,
        morpho.caudalFinLength, morpho.analFinLength, morpho.thickness, morpho.weight, morpho.pectoralFinLength, morpho.eyeFinLength, morpho.yellowBlueValue
    };
    
    for (int i = 0; i < values.size(); ++i) {
        QTableWidgetItem* item = tableWidget_->item(i, 1);
        if (item) {
            item->setText(QString::number(values[i], 'f', 2));
        }
    }
}

void MeasurePanelWidget::clearData() {
    for (int i = 0; i < tableWidget_->rowCount(); ++i) {
        QTableWidgetItem* item = tableWidget_->item(i, 1);
        if (item) {
            item->setText("0.0");
        }
    }
}
