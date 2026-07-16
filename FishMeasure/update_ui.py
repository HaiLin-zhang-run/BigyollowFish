import xml.etree.ElementTree as ET

xml_file = 'd:/QPRO/FishMeasure/ui/mainwindow.ui'
tree = ET.parse(xml_file)
root = tree.getroot()

# Find centerFrame and remove cameraControls
centerFrame = root.find(".//widget[@name='centerFrame']/layout")
if centerFrame is not None:
    for item in centerFrame.findall('item'):
        layout = item.find('layout')
        if layout is not None and layout.get('name') == 'cameraControls':
            centerFrame.remove(item)

# Find rightLayout and rebuild it
rightLayout = root.find(".//widget[@name='rightFrame']/layout")

if rightLayout is not None:
    widgetSettings_layout = root.find(".//widget[@name='widgetSettings']/layout")
    id_layout = None
    path_layout = None
    if widgetSettings_layout is not None:
        for item in widgetSettings_layout.findall('item'):
            lyt = item.find('layout')
            if lyt is not None:
                if lyt.get('name') == 'horizontalLayoutId':
                    id_layout = item
                elif lyt.get('name') == 'horizontalLayoutPath':
                    path_layout = item

    # Clear rightLayout
    for child in list(rightLayout):
        rightLayout.remove(child)

    def create_widget_item(widget_xml):
        item = ET.Element('item')
        item.append(ET.fromstring(widget_xml))
        return item

    def create_layout_item(layout_xml):
        item = ET.Element('item')
        item.append(ET.fromstring(layout_xml))
        return item

    # 1. Camera GroupBox
    camera_group_xml = '''
    <widget class="QGroupBox" name="groupBoxCamera">
     <property name="title"><string>相机</string></property>
     <layout class="QGridLayout" name="gridLayoutCamera">
      <item row="0" column="0"><widget class="QLabel" name="label_2"><property name="text"><string>型号</string></property></widget></item>
      <item row="0" column="1"><widget class="QComboBox" name="cbCameraModel"/></item>
      <item row="0" column="2"><widget class="QPushButton" name="btnRefreshCamera"><property name="text"><string>刷新</string></property></widget></item>
      <item row="0" column="3"><widget class="QPushButton" name="btnConnectCamera"><property name="text"><string>连接</string></property></widget></item>
      <item row="1" column="0"><widget class="QLabel" name="label_3"><property name="text"><string>曝光时间</string></property></widget></item>
      <item row="1" column="1"><widget class="QLineEdit" name="editExposureTime"/></item>
      <item row="1" column="2"><widget class="QPushButton" name="btnSetExposure"><property name="text"><string>设置</string></property></widget></item>
      <item row="2" column="0"><widget class="QLabel" name="label_4"><property name="text"><string>分辨率</string></property></widget></item>
      <item row="2" column="1" colspan="3"><widget class="QLineEdit" name="editResolution"/></item>
     </layout>
    </widget>
    '''
    rightLayout.append(create_widget_item(camera_group_xml))

    # 2. Scale GroupBox
    scale_group_xml = '''
    <widget class="QGroupBox" name="groupBoxScale">
     <property name="title"><string>电子称</string></property>
     <layout class="QGridLayout" name="gridLayoutScale">
      <item row="0" column="0"><widget class="QLabel" name="label_5"><property name="text"><string>串口</string></property></widget></item>
      <item row="0" column="1"><widget class="QComboBox" name="cbSerialPort"/></item>
      <item row="0" column="2"><widget class="QPushButton" name="btnRefreshSerial"><property name="text"><string>刷新</string></property></widget></item>
      <item row="0" column="3"><widget class="QPushButton" name="btnConnectSerial"><property name="text"><string>连接</string></property></widget></item>
      <item row="1" column="0"><widget class="QLabel" name="label_6"><property name="text"><string>单位</string></property></widget></item>
      <item row="1" column="1" colspan="3">
       <widget class="QComboBox" name="cbWeightUnit">
        <item><property name="text"><string>kg</string></property></item>
        <item><property name="text"><string>g</string></property></item>
       </widget>
      </item>
      <item row="2" column="0"><widget class="QLabel" name="label_7"><property name="text"><string>重量</string></property></widget></item>
      <item row="2" column="1"><widget class="QLineEdit" name="editWeight"><property name="readOnly"><bool>true</bool></property></widget></item>
      <item row="2" column="2" colspan="2">
       <layout class="QHBoxLayout" name="horizontalLayoutScaleBtns">
        <property name="spacing"><number>2</number></property>
        <item><widget class="QPushButton" name="btnCalibrateScale"><property name="text"><string>校准</string></property></widget></item>
        <item><widget class="QPushButton" name="btnZero"><property name="text"><string>归零</string></property></widget></item>
        <item><widget class="QPushButton" name="btnTare"><property name="text"><string>去皮</string></property></widget></item>
       </layout>
      </item>
     </layout>
    </widget>
    '''
    rightLayout.append(create_widget_item(scale_group_xml))

    # 3. Settings Box (ID & Path)
    settings_box_xml = '''
    <widget class="QGroupBox" name="groupBoxSettings">
     <property name="title"><string>基本信息</string></property>
     <layout class="QVBoxLayout" name="verticalLayoutSettings">
     </layout>
    </widget>
    '''
    settings_box_item = create_widget_item(settings_box_xml)
    vbox = settings_box_item.find('.//layout')
    if id_layout is not None: vbox.append(id_layout)
    if path_layout is not None: vbox.append(path_layout)
    rightLayout.append(settings_box_item)

    # 4. Spacer
    rightLayout.append(create_layout_item('''<spacer name="verticalSpacerRight"><property name="orientation"><enum>Qt::Vertical</enum></property><property name="sizeHint" stdset="0"><size><width>20</width><height>40</height></size></property></spacer>'''))

    # 5. lblScaleStatus
    rightLayout.append(create_widget_item('''<widget class="QLabel" name="lblScaleStatus"><property name="font"><font><pointsize>12</pointsize><weight>75</weight><bold>true</bold></font></property><property name="text"><string>电子秤状态</string></property></widget>'''))

    # 6. btnCapture
    rightLayout.append(create_widget_item('''<widget class="QPushButton" name="btnCapture"><property name="minimumSize"><size><width>0</width><height>50</height></size></property><property name="text"><string>拍照查看</string></property></widget>'''))

    # 7. btnClose
    rightLayout.append(create_widget_item('''<widget class="QPushButton" name="btnClose"><property name="minimumSize"><size><width>0</width><height>50</height></size></property><property name="text"><string>退出系统</string></property></widget>'''))

tree.write(xml_file, encoding='UTF-8', xml_declaration=True)
print('Updated UI file successfully.')
