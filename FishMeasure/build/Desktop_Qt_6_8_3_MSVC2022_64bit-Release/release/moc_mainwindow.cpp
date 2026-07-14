/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ui/mainwindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10MainWindowE = QtMocHelpers::stringData(
    "MainWindow",
    "onCameraFrame",
    "",
    "rgb",
    "cv::Mat",
    "depth",
    "onWeightUpdated",
    "weight",
    "onScaleStatusChanged",
    "status",
    "onCameraError",
    "err",
    "on_btnStartCamera_clicked",
    "on_btnStopCamera_clicked",
    "on_btnCapture_clicked",
    "on_btnClose_clicked",
    "on_btnTare_clicked",
    "on_btnZero_clicked",
    "on_btnBrowsePath_clicked",
    "on_btnConnectSerial_clicked",
    "on_btnConfirmFishId_clicked",
    "on_btnModifyFishId_clicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10MainWindowE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   98,    2, 0x08,    1 /* Private */,
       6,    1,  103,    2, 0x08,    4 /* Private */,
       8,    1,  106,    2, 0x08,    6 /* Private */,
      10,    1,  109,    2, 0x08,    8 /* Private */,
      12,    0,  112,    2, 0x08,   10 /* Private */,
      13,    0,  113,    2, 0x08,   11 /* Private */,
      14,    0,  114,    2, 0x08,   12 /* Private */,
      15,    0,  115,    2, 0x08,   13 /* Private */,
      16,    0,  116,    2, 0x08,   14 /* Private */,
      17,    0,  117,    2, 0x08,   15 /* Private */,
      18,    0,  118,    2, 0x08,   16 /* Private */,
      19,    0,  119,    2, 0x08,   17 /* Private */,
      20,    0,  120,    2, 0x08,   18 /* Private */,
      21,    0,  121,    2, 0x08,   19 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage, 0x80000000 | 4,    3,    5,
    QMetaType::Void, QMetaType::Double,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ZN10MainWindowE.offsetsAndSizes,
    qt_meta_data_ZN10MainWindowE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10MainWindowE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onCameraFrame'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QImage, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        // method 'onWeightUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'onScaleStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onCameraError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_btnStartCamera_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnStopCamera_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnCapture_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnClose_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnTare_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnZero_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnBrowsePath_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnConnectSerial_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnConfirmFishId_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_btnModifyFishId_clicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onCameraFrame((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[2]))); break;
        case 1: _t->onWeightUpdated((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 2: _t->onScaleStatusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->onCameraError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->on_btnStartCamera_clicked(); break;
        case 5: _t->on_btnStopCamera_clicked(); break;
        case 6: _t->on_btnCapture_clicked(); break;
        case 7: _t->on_btnClose_clicked(); break;
        case 8: _t->on_btnTare_clicked(); break;
        case 9: _t->on_btnZero_clicked(); break;
        case 10: _t->on_btnBrowsePath_clicked(); break;
        case 11: _t->on_btnConnectSerial_clicked(); break;
        case 12: _t->on_btnConfirmFishId_clicked(); break;
        case 13: _t->on_btnModifyFishId_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10MainWindowE.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}
QT_WARNING_POP
