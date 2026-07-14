/****************************************************************************
** Meta object code from reading C++ file 'capturedetailwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../ui/capturedetailwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'capturedetailwindow.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN19CaptureDetailWindowE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN19CaptureDetailWindowE = QtMocHelpers::stringData(
    "CaptureDetailWindow",
    "detectionFinished",
    "",
    "cv::Mat",
    "annotatedBgr",
    "rawBgr",
    "depthMat",
    "FishMorphology",
    "morpho",
    "FishKeypoints",
    "kps",
    "fx",
    "fy",
    "cx",
    "cy",
    "hasFish",
    "onDetectionDone",
    "onSaveClicked",
    "onCancelClicked"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN19CaptureDetailWindowE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,   10,   38,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      16,   10,   59,    2, 0x08,   12 /* Private */,
      17,    0,   80,    2, 0x08,   23 /* Private */,
      18,    0,   81,    2, 0x08,   24 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 7, 0x80000000 | 9, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Bool,    4,    5,    6,    8,   10,   11,   12,   13,   14,   15,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 3, 0x80000000 | 7, 0x80000000 | 9, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Float, QMetaType::Bool,    4,    5,    6,    8,   10,   11,   12,   13,   14,   15,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject CaptureDetailWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ZN19CaptureDetailWindowE.offsetsAndSizes,
    qt_meta_data_ZN19CaptureDetailWindowE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN19CaptureDetailWindowE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<CaptureDetailWindow, std::true_type>,
        // method 'detectionFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<FishMorphology, std::false_type>,
        QtPrivate::TypeAndForceComplete<FishKeypoints, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onDetectionDone'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<cv::Mat, std::false_type>,
        QtPrivate::TypeAndForceComplete<FishMorphology, std::false_type>,
        QtPrivate::TypeAndForceComplete<FishKeypoints, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<float, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onSaveClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onCancelClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void CaptureDetailWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CaptureDetailWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->detectionFinished((*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<FishMorphology>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<FishKeypoints>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[8])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[9])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[10]))); break;
        case 1: _t->onDetectionDone((*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<cv::Mat>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<FishMorphology>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<FishKeypoints>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[6])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[7])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[8])),(*reinterpret_cast< std::add_pointer_t<float>>(_a[9])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[10]))); break;
        case 2: _t->onSaveClicked(); break;
        case 3: _t->onCancelClicked(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (CaptureDetailWindow::*)(cv::Mat , cv::Mat , cv::Mat , FishMorphology , FishKeypoints , float , float , float , float , bool );
            if (_q_method_type _q_method = &CaptureDetailWindow::detectionFinished; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *CaptureDetailWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CaptureDetailWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN19CaptureDetailWindowE.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CaptureDetailWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void CaptureDetailWindow::detectionFinished(cv::Mat _t1, cv::Mat _t2, cv::Mat _t3, FishMorphology _t4, FishKeypoints _t5, float _t6, float _t7, float _t8, float _t9, bool _t10)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t7))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t8))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t9))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t10))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
