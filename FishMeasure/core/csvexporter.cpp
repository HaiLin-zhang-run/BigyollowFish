#include "csvexporter.h"
#include <QFile>
#include <QTextStream>
#include <QDir>

bool CsvExporter::exportRecords(const QString& filePath, const QList<FishRecord>& records) {
    QFile file(filePath);
    bool fileExists = file.exists();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    // 写入 BOM 以支持 Excel 中的 UTF-8
    out.setEncoding(QStringConverter::Utf8);
    if (!fileExists) {
        out << "\xEF\xBB\xBF"; 
        out << "ID,Time,TotalLength,ForkLength,BodyLength,SnoutLength,EyeDiameter,HeadLength,HeadHeight,BodyHeight,CaudPedLength,CaudPedHeight,DorsalFinLen,PectoralFinLen,VentralFinLen,AnalFinLen,Thickness,Weight\n";
    }

    for (const auto& r : records) {
        out << r.id << ","
            << r.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ","
            << r.morphology.totalLength << ","
            << r.morphology.forkLength << ","
            << r.morphology.bodyLength << ","
            << r.morphology.snoutLength << ","
            << r.morphology.eyeDiameter << ","
            << r.morphology.headLength << ","
            << r.morphology.headHeight << ","
            << r.morphology.bodyHeight << ","
            << r.morphology.caudPedLength << ","
            << r.morphology.caudPedHeight << ","
            << r.morphology.dorsalFinLen << ","
            << r.morphology.pectoralFinLen << ","
            << r.morphology.ventralFinLen << ","
            << r.morphology.analFinLen << ","
            << r.morphology.thickness << ","
            << r.morphology.weight << "\n";
    }

    file.close();
    return true;
}

bool CsvExporter::appendRecord(const QString& filePath, const FishRecord& record) {
    QList<FishRecord> records;
    records.append(record);
    return exportRecords(filePath, records);
}
