/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QFrame *leftFrame;
    QVBoxLayout *leftLayout;
    QLabel *label;
    QFrame *line;
    QFrame *centerFrame;
    QVBoxLayout *centerLayout;
    QHBoxLayout *cameraControls;
    QPushButton *btnStartCamera;
    QPushButton *btnStopCamera;
    QComboBox *cbSerialPort;
    QPushButton *btnConnectSerial;
    QSpacerItem *horizontalSpacer;
    QFrame *rightFrame;
    QVBoxLayout *rightLayout;
    QWidget *widgetSettings;
    QVBoxLayout *verticalLayoutRight;
    QHBoxLayout *horizontalLayoutId;
    QLabel *labelId;
    QLineEdit *editFishId;
    QPushButton *btnConfirmFishId;
    QPushButton *btnModifyFishId;
    QHBoxLayout *horizontalLayoutPath;
    QLabel *labelPath;
    QLineEdit *editSavePath;
    QPushButton *btnBrowsePath;
    QHBoxLayout *horizontalLayoutTare;
    QPushButton *btnTare;
    QPushButton *btnZero;
    QSpacerItem *verticalSpacerRight;
    QLabel *lblWeight;
    QLabel *lblScaleStatus;
    QPushButton *btnCapture;
    QPushButton *btnClose;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1280, 720);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName("horizontalLayout");
        leftFrame = new QFrame(centralwidget);
        leftFrame->setObjectName("leftFrame");
        leftFrame->setMinimumSize(QSize(250, 0));
        leftLayout = new QVBoxLayout(leftFrame);
        leftLayout->setObjectName("leftLayout");
        label = new QLabel(leftFrame);
        label->setObjectName("label");

        leftLayout->addWidget(label);

        line = new QFrame(leftFrame);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        leftLayout->addWidget(line);


        horizontalLayout->addWidget(leftFrame);

        centerFrame = new QFrame(centralwidget);
        centerFrame->setObjectName("centerFrame");
        QSizePolicy sizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centerFrame->sizePolicy().hasHeightForWidth());
        centerFrame->setSizePolicy(sizePolicy);
        centerLayout = new QVBoxLayout(centerFrame);
        centerLayout->setObjectName("centerLayout");
        cameraControls = new QHBoxLayout();
        cameraControls->setObjectName("cameraControls");
        btnStartCamera = new QPushButton(centerFrame);
        btnStartCamera->setObjectName("btnStartCamera");

        cameraControls->addWidget(btnStartCamera);

        btnStopCamera = new QPushButton(centerFrame);
        btnStopCamera->setObjectName("btnStopCamera");

        cameraControls->addWidget(btnStopCamera);

        cbSerialPort = new QComboBox(centerFrame);
        cbSerialPort->setObjectName("cbSerialPort");
        cbSerialPort->setMinimumSize(QSize(120, 0));

        cameraControls->addWidget(cbSerialPort);

        btnConnectSerial = new QPushButton(centerFrame);
        btnConnectSerial->setObjectName("btnConnectSerial");

        cameraControls->addWidget(btnConnectSerial);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        cameraControls->addItem(horizontalSpacer);


        centerLayout->addLayout(cameraControls);


        horizontalLayout->addWidget(centerFrame);

        rightFrame = new QFrame(centralwidget);
        rightFrame->setObjectName("rightFrame");
        rightFrame->setMinimumSize(QSize(250, 0));
        rightLayout = new QVBoxLayout(rightFrame);
        rightLayout->setObjectName("rightLayout");
        widgetSettings = new QWidget(rightFrame);
        widgetSettings->setObjectName("widgetSettings");
        verticalLayoutRight = new QVBoxLayout(widgetSettings);
        verticalLayoutRight->setObjectName("verticalLayoutRight");
        horizontalLayoutId = new QHBoxLayout();
        horizontalLayoutId->setObjectName("horizontalLayoutId");
        labelId = new QLabel(widgetSettings);
        labelId->setObjectName("labelId");

        horizontalLayoutId->addWidget(labelId);

        editFishId = new QLineEdit(widgetSettings);
        editFishId->setObjectName("editFishId");

        horizontalLayoutId->addWidget(editFishId);

        btnConfirmFishId = new QPushButton(widgetSettings);
        btnConfirmFishId->setObjectName("btnConfirmFishId");

        horizontalLayoutId->addWidget(btnConfirmFishId);

        btnModifyFishId = new QPushButton(widgetSettings);
        btnModifyFishId->setObjectName("btnModifyFishId");
        btnModifyFishId->setEnabled(false);

        horizontalLayoutId->addWidget(btnModifyFishId);


        verticalLayoutRight->addLayout(horizontalLayoutId);

        horizontalLayoutPath = new QHBoxLayout();
        horizontalLayoutPath->setObjectName("horizontalLayoutPath");
        labelPath = new QLabel(widgetSettings);
        labelPath->setObjectName("labelPath");

        horizontalLayoutPath->addWidget(labelPath);

        editSavePath = new QLineEdit(widgetSettings);
        editSavePath->setObjectName("editSavePath");

        horizontalLayoutPath->addWidget(editSavePath);

        btnBrowsePath = new QPushButton(widgetSettings);
        btnBrowsePath->setObjectName("btnBrowsePath");
        btnBrowsePath->setMaximumSize(QSize(40, 16777215));

        horizontalLayoutPath->addWidget(btnBrowsePath);


        verticalLayoutRight->addLayout(horizontalLayoutPath);

        horizontalLayoutTare = new QHBoxLayout();
        horizontalLayoutTare->setObjectName("horizontalLayoutTare");
        btnTare = new QPushButton(widgetSettings);
        btnTare->setObjectName("btnTare");

        horizontalLayoutTare->addWidget(btnTare);

        btnZero = new QPushButton(widgetSettings);
        btnZero->setObjectName("btnZero");

        horizontalLayoutTare->addWidget(btnZero);


        verticalLayoutRight->addLayout(horizontalLayoutTare);


        rightLayout->addWidget(widgetSettings);

        verticalSpacerRight = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        rightLayout->addItem(verticalSpacerRight);

        lblWeight = new QLabel(rightFrame);
        lblWeight->setObjectName("lblWeight");
        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        lblWeight->setFont(font);

        rightLayout->addWidget(lblWeight);

        lblScaleStatus = new QLabel(rightFrame);
        lblScaleStatus->setObjectName("lblScaleStatus");
        QFont font1;
        font1.setPointSize(12);
        font1.setBold(true);
        lblScaleStatus->setFont(font1);

        rightLayout->addWidget(lblScaleStatus);

        btnCapture = new QPushButton(rightFrame);
        btnCapture->setObjectName("btnCapture");
        btnCapture->setMinimumSize(QSize(0, 50));

        rightLayout->addWidget(btnCapture);

        btnClose = new QPushButton(rightFrame);
        btnClose->setObjectName("btnClose");
        btnClose->setMinimumSize(QSize(0, 50));

        rightLayout->addWidget(btnClose);


        horizontalLayout->addWidget(rightFrame);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1280, 21));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\345\244\247\351\273\204\351\261\274\350\241\250\345\236\213\346\231\272\350\203\275\346\265\213\351\207\217\347\263\273\347\273\237", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "\345\216\206\345\217\262\350\256\260\345\275\225", nullptr));
        btnStartCamera->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200\347\233\270\346\234\272", nullptr));
        btnStopCamera->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255\347\233\270\346\234\272", nullptr));
        btnConnectSerial->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245", nullptr));
        labelId->setText(QCoreApplication::translate("MainWindow", "\351\261\274\347\232\204\347\274\226\345\217\267:", nullptr));
        btnConfirmFishId->setText(QCoreApplication::translate("MainWindow", "\347\241\256\350\256\244", nullptr));
        btnModifyFishId->setText(QCoreApplication::translate("MainWindow", "\344\277\256\346\224\271", nullptr));
        labelPath->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\350\267\257\345\276\204:", nullptr));
        btnBrowsePath->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        btnTare->setText(QCoreApplication::translate("MainWindow", "\345\216\273\347\232\256", nullptr));
        btnZero->setText(QCoreApplication::translate("MainWindow", "\345\275\222\351\233\266", nullptr));
        lblWeight->setText(QCoreApplication::translate("MainWindow", "\345\275\223\345\211\215\351\207\215\351\207\217: 0.0 g", nullptr));
        lblScaleStatus->setText(QCoreApplication::translate("MainWindow", "\347\224\265\345\255\220\347\247\244\347\212\266\346\200\201", nullptr));
        btnCapture->setText(QCoreApplication::translate("MainWindow", "\346\213\215\347\205\247\346\237\245\347\234\213", nullptr));
        btnClose->setText(QCoreApplication::translate("MainWindow", "\351\200\200\345\207\272\347\263\273\347\273\237", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
