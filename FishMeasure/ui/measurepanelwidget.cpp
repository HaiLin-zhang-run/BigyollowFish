#include "measurepanelwidget.h"
#include <QHeaderView>

MeasurePanelWidget::MeasurePanelWidget(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    QLabel* titleLabel = new QLabel("鱼类表型具体数据", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #d4d4d4;");
    layout->addWidget(titleLabel);
    
    tableWidget_ = new QTableWidget(16, 2, this);
    tableWidget_->setHorizontalHeaderLabels(QStringList() << "测量项" << "数值(mm/g)");
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
        "全长", "叉长", "体长", "吻长", "眼径", "头长", "头高", "体高",
        "尾柄长", "尾柄高", "背鳍长", "胸鳍长", "腹鳍长", "臀鳍长", "厚度", "体重(g)"
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
        morpho.totalLength, morpho.forkLength, morpho.bodyLength, morpho.snoutLength,
        morpho.eyeDiameter, morpho.headLength, morpho.headHeight, morpho.bodyHeight,
        morpho.caudPedLength, morpho.caudPedHeight, morpho.dorsalFinLen, morpho.pectoralFinLen,
        morpho.ventralFinLen, morpho.analFinLen, morpho.thickness, morpho.weight
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
