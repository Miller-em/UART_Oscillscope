#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QString>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QIODevice>
#include <QTextCodec>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qjsonvalue.h>

const int color[10]={
    0xff0000,0xffff00,0x00ff00,0x0000ff,0xff00ff,
    0xff8000,0xc0ff00,0x00ffff,0x8000ff,0x000001,
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/Images/Icons/Alien 2.png"));
    this->setWindowTitle(tr("串口示波器"));
    ui->baudRate->setCurrentIndex(1);
    this->system_init();  //系统各组件初始化
}

MainWindow::~MainWindow()
{
    delete ui;
}

/*-----------------------------------------------------------
 *                      functions
 * ----------------------------------------------------------*/

void MainWindow::system_init()
{
    // 这是statusbar的相关初始化
    QLabel *permanent = new QLabel(this);
    permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    permanent->setText(
      tr("<a href=\"http://www.xiaozhoua.top\">xiaozhoua.top</a>"));
    permanent->setTextFormat(Qt::RichText);
    permanent->setOpenExternalLinks(true);
    ui->statusbar->addPermanentWidget(permanent);
    statusLabel = new QLabel;
    statusLabel->setMinimumSize(150, 20); // 设置标签最小大小
    statusLabel->setFrameShape(QFrame::WinPanel); // 设置标签形状
    statusLabel->setFrameShadow(QFrame::Sunken); // 设置标签阴影
    ui->statusbar->addWidget(statusLabel);
    statusLabel->setText(tr("所属人： 周孝宗，钟雨彬，刘茂安"));
    statusLabel->setStyleSheet("color: white");
    statusBar()->setStyleSheet("background-color : rgb(30,30,30)");

    //PID 控制初始值控制
    ui->lineEdit->setText(tr("0"));
    ui->lineEdit_2->setText(tr("0"));
    ui->lineEdit_3->setText(tr("0"));
    ui->lineEdit_4->setText(tr("0"));

    // 串口初始化配置
    global_port.setParity(QSerialPort::NoParity);  //无奇偶校验位
    global_port.setDataBits(QSerialPort::Data8);   //8位校验位
    global_port.setStopBits(QSerialPort::OneStop); //停止位为1
    global_port.setFlowControl(QSerialPort::NoFlowControl);   //无流控制
    connect(&global_port, SIGNAL(readyRead()), this, SLOT(receiveInfo()));

    // 示波器初始化配置
    ui->customPlot->setBackground(QBrush(QColor("#474848")));
}

QStringList MainWindow::getPortNameList()
{
    /*--------------------------------
     *  获取系统串口信息
     *-------------------------------*/
    QStringList m_serialPortName;
    foreach(const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        m_serialPortName << info.portName();
        qDebug()<<"serialPortName:"<<info.portName();
    }
    return m_serialPortName;
}
//ui->customPlot->setBackground(QBrush(QColor("#000000")));
void MainWindow::buildChart()
{
    /*--------------------------------
     *  构造示波器
     *-------------------------------*/
    ui->customPlot->setBackground(QBrush(QColor("#474848")));
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);      //可拖拽+可滚轮缩放

    ui->customPlot->axisRect()->setupFullAxesBox();
    ui->customPlot->yAxis->setRange(0, 3.3);
}

void MainWindow::plotCustom(QByteArray info)
{
        //避免中文乱码
        QTextCodec *tc = QTextCodec::codecForName("GBK");
        QString tmpQStr = tc->toUnicode(info);

        // 若示波器打开，开始解析数据
        if (oscill_flag){
            QStringList datakeys;

            QJsonParseError jsonError;
            QJsonDocument jsonDoc = QJsonDocument::fromJson(info, &jsonError);

            if (jsonError.error != QJsonParseError::NoError)
            {
                qDebug() << "parse json object failed, " << jsonError.errorString();
                ui->textBrowser->append("parse json object failed!");
                return;
            }
            QJsonObject jsonObj = jsonDoc.object();

            qDebug() << "parse json object Successfully";
            datakeys = jsonObj.keys();      // 获取通道名称
            qDebug() << datakeys;

            if (Received_Keys != datakeys){
                // 只能够设置一次标签
                qDebug() << "update keys";
                Received_Keys = datakeys;

                this->index = 0;
                this->Ptext.clear();
                this->XData.clear();
                this->YData.clear();
                // 清楚画布
                ui->customPlot->clearGraphs();
                ui->customPlot->legend->setVisible(false);
                ui->customPlot->replot();

                ui->customPlot->legend->setVisible(true);  //右上角指示曲线的缩略框
                ui->customPlot->legend->setBrush(QColor(100, 100, 100, 0));//设置图例背景颜色，可设置透明
                ui->customPlot->legend->setTextColor(Qt::white);
                for (int i=0; i < datakeys.size(); i++){
                    ui->customPlot->addGraph();
                    ui->customPlot->graph(i)->setPen(QPen(color[i]));
                    ui->customPlot->graph(i)->setName(datakeys[i]);
                    Ptext.push_back(datakeys[i]);
                }
            }

            // 添加XData , YData 数据
            XData.push_back(index);
            QVector<double> Data;
            YData.resize(datakeys.size());

            for (int i = 0;  i < Ptext.size(); ++i) {
                Data.push_back(jsonObj.value(datakeys[i]).toDouble());
            }
            for (int i = 0;  i < Ptext.size(); ++i) {
                YData[i].push_back(Data[i]);
            }

            //向坐标值赋值
            for (int i=0; i < datakeys.size(); ++i){
                ui->customPlot->graph(i)->addData(XData[index], YData[i][index]);
            }
            this->index++;
            // 更新坐标
            ui->customPlot->xAxis->setRange((ui->customPlot->graph(0)->dataCount()>1000)?
                                                (ui->customPlot->graph(0)->dataCount()-1000):
                                                0,
                                            ui->customPlot->graph(0)->dataCount());
            ui->customPlot->replot(QCustomPlot::rpQueuedReplot);  //实现重绘
        }
        // 向接收区打印
        ui->textBrowser->append(tmpQStr);
        qDebug() << "Received Data: " << tmpQStr;
}
/*-----------------------------------------------------------
 *                      slots
 * ----------------------------------------------------------*/

void MainWindow::on_OpenorClose_clicked()
{
    /*--------------------------------
     *  打开或者关闭串口
     *-------------------------------*/
    if (!this->OpenorClose_Flag){
        // 打开串口
        ui->OpenorClose->setText("关闭串口");
        this->OpenorClose_Flag = true;

        //设置串口端口
        QStringList m_serialPortName;
        m_serialPortName = getPortNameList();  //配置
        qDebug()<<m_serialPortName;
        ui->portId->clear();
        ui->portId->addItems(m_serialPortName);

        //Serial 配置

        QString portId;
        portId = ui->portId->currentText();
        qDebug() << "portId: " << portId;
        global_port.setPortName(portId);  //配置端口号

        QString baudRate;
        baudRate = ui->baudRate->currentText();
        qDebug() << "baudRate: " << baudRate;
        global_port.setBaudRate(baudRate.toInt(), QSerialPort::AllDirections); //配置波特率

        // 配置完成,开启串口
        global_port.open(QIODevice::ReadWrite);
        qDebug() << "Open Serial Successfully";
    }else{
        // 关闭串口
        global_port.close();
        qDebug() << "Close Serial Successfully";
        ui->OpenorClose->setText("打开串口");
        this->OpenorClose_Flag = false;
    }
}

void MainWindow::on_sendPIDData_clicked()
{
    /*--------------------------------
     *  发送PID相关数据
     *-------------------------------*/
    if (global_port.isOpen()){
        QString P, I, D, targetVal;
        QJsonObject sendPIDData;

        P = ui->lineEdit->text();
        I = ui->lineEdit_2->text();
        D = ui->lineEdit_3->text();
        targetVal = ui->lineEdit_4->text();
        sendPIDData.insert("P", P.toFloat());
        sendPIDData.insert("I", I.toFloat());
        sendPIDData.insert("D", D.toFloat());
        sendPIDData.insert("Target", targetVal.toFloat());
        QJsonDocument document;
        document.setObject(sendPIDData);
        QByteArray pidbyte_array = document.toJson(QJsonDocument::Compact);
        QString pidjson_str(pidbyte_array);
        pidjson_str += '\n';
        qDebug() << QString::fromLocal8Bit("PID Data：") << pidjson_str;
        global_port.write(pidjson_str.toLocal8Bit());
    }else{
        QMessageBox box;
        box.setWindowTitle(tr("警告"));
        box.setWindowIcon(QIcon(":/Images/Icons/Alien 2.png"));
        box.setIcon(QMessageBox::Warning);
        box.setText(tr(" 未检测到串口，请确认串口是否打开？"));
        box.exec();
    }
}

void MainWindow::on_sendData_clicked()
{
    /*--------------------------------
     *  发送串口数据
     *-------------------------------*/
    if (global_port.isOpen()){
        QString sendData;

        sendData = ui->textEdit->toPlainText() + '\n';
        qDebug() << sendData;
        ui->textEdit->clear();
        global_port.write(sendData.toLocal8Bit());
    }else{
        QMessageBox box;
        box.setWindowTitle(tr("警告"));
        box.setWindowIcon(QIcon(":/Images/Icons/Alien 2.png"));
        box.setIcon(QMessageBox::Warning);
        box.setText(tr(" 未检测到串口，请确认串口是否打开？"));
        box.exec();
    }
}

void MainWindow::on_viewManual_clicked()
{
    /*--------------------------------
     *  查看帮助手册
     *-------------------------------*/
    QString strtext="1.串口通讯格式：用Json数据格式进行传输。发送数据的字符串不得超过45个字符，否则漏失数据较为严重\n"
                        "2.下位机MCU使用cJson进行发送数据\n"
                        "3.暂停示波器，看波形目前只支持开关串口，但这样会造成丢失数据\n"
                        "4.下位机发送字符串格式为： {\"xxx\":%.3f,\"xxx\":%.3f}";
    QMessageBox::information(this, "帮助信息", strtext, QMessageBox::Ok);
}

void MainWindow::on_pushButton_clicked()
{
    /*--------------------------------
     *  清空PID相关数据
     *-------------------------------*/
    ui->lineEdit->setText(tr("0"));
    ui->lineEdit_2->setText(tr("0"));
    ui->lineEdit_3->setText(tr("0"));
    ui->lineEdit_4->setText(tr("0"));
}

void MainWindow::on_openOscill_clicked()
{
    /*--------------------------------
     *  打开示波器
     *-------------------------------*/
    if (!oscill_flag){
        if (global_port.isOpen()){
            oscill_flag = true;
            buildChart();
            ui->openOscill->setText("关闭示波器");
        }else{
            QMessageBox box;
            box.setWindowTitle(tr("警告"));
            box.setWindowIcon(QIcon(":/Images/Icons/Alien 2.png"));
            box.setIcon(QMessageBox::Warning);
            box.setText(tr(" 未检测到串口，请确认串口是否打开？"));
            box.exec();
        }
    }else{
        // 置空所有状态值
        oscill_flag = false;
        this->index = 0;
        this->Ptext.clear();
        this->XData.clear();
        this->YData.clear();
        this->Received_Keys.clear();
        // 清楚画布
        ui->customPlot->clearGraphs();
        ui->customPlot->legend->setVisible(false);
        ui->customPlot->replot();
        ui->customPlot->setBackground(QBrush(QColor("#474848")));
        ui->openOscill->setText("打开示波器");
    }
}

void MainWindow::receiveInfo()
{
    /*--------------------------------
     *  接受串口信息
     *-------------------------------*/
    QString data;
    QByteArray info = global_port.readAll();
    QStringList list;

    if(!info.isEmpty())
    {
        if (info.contains('{')){
            data = info;
            list= data.split("{");
            if (list.length() == 2){
                if (frontData.isEmpty()){
                    frontData = list.at(1);
                    frontData.insert(0,'{');
                    return;
                }
                oneData = frontData + list.at(0);
                frontData = list.at(1);
                frontData.insert(0,'{');
                plotCustom(oneData.toLocal8Bit());  //绘图
                qDebug() << "One data: " << oneData;
            }

            if (list.length() == 3){
                oneData = frontData + list.at(0);
                plotCustom(oneData.toLocal8Bit());          //绘图
                qDebug() << "One data: " << oneData;
                oneData = list.at(1);
                oneData.insert(0,'{');
                plotCustom(oneData.toLocal8Bit());  //绘图
                qDebug() << "One data: " << oneData;
                frontData = list.at(2);
                frontData.insert(0,'{');
            }
         }
    }
}

void MainWindow::on_clearReceived_clicked()
{
    /*--------------------------------
     *  清空接收区
     *-------------------------------*/
    ui->textBrowser->clear();
}

void MainWindow::on_stopOscill_clicked()
{
    /*--------------------------------
     *  坐标轴缩放
     *-------------------------------*/
    if (stop_flag){
        ui->customPlot->axisRect()->setRangeZoom(Qt::Vertical);
        stop_flag = false;
        ui->stopOscill->setText("水平缩放");
    }else{
        ui->customPlot->axisRect()->setRangeZoom(Qt::Horizontal);
        stop_flag = true;
        ui->stopOscill->setText("垂直缩放");
    }
}
