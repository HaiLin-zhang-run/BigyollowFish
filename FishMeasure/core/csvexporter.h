#pragma once
#include <QString>
#include <QList>
#include "fishrecord.h"

class CsvExporter {
public:
    // 导出多条记录到 CSV 文件
    static bool exportRecords(const QString& filePath, const QList<FishRecord>& records);
    
    // 导出单条记录到 CSV 文件 (追加模式)
    static bool appendRecord(const QString& filePath, const FishRecord& record);
};
