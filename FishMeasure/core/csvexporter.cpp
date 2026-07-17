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
        out << "ID,Time,TotalLength,BodyLength,HeadLength,TrunkLength,TailLength,SnoutLength,EyeLength,PostEyeHeadLength,CaudPedLength,BodyHeight,CaudPedHeight,PectoralFinLength,CaudalFinLength,AnalFinLength,Thickness,Weight,EyeFinLength,YellowBlueValue\n";
    }

    for (const auto& r : records) {
        out << r.id << ","
            << r.timestamp.toString("yyyy-MM-dd HH:mm:ss") << ","
            << r.morphology.totalLength << ","
            << r.morphology.bodyLength << ","
            << r.morphology.headLength << ","
            << r.morphology.trunkLength << ","
            << r.morphology.tailLength << ","
            << r.morphology.snoutLength << ","
            << r.morphology.eyeLength << ","
            << r.morphology.postEyeHeadLength << ","
            << r.morphology.caudPedLength << ","
            << r.morphology.bodyHeight << ","
            << r.morphology.caudPedHeight << ","
            << r.morphology.pectoralFinLength << ","
            << r.morphology.caudalFinLength << ","
            << r.morphology.analFinLength << ","
            << r.morphology.thickness << ","
            << r.morphology.weight << ","
            << r.morphology.eyeFinLength << ","
            << r.morphology.yellowBlueValue << "\n";
    }

    file.close();
    return true;
}

bool CsvExporter::appendRecord(const QString& filePath, const FishRecord& record) {
    QList<FishRecord> records;
    records.append(record);
    return exportRecords(filePath, records);
}
