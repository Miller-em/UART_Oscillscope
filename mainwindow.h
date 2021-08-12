#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

/*-------------include----------------*/
#include <QSerialPort>
#include <QSerialPortInfo>
#include "qcustomplot.h"
//#include <QtCharts>

/*-------------class------------------*/
class QLabel;
class QString;
class QTimer;
class QDebug;

/*-------------define-----------------*/
#define cout qDebug() << "[" << __FILE__ << ":" << __LINE__ << "]"

/*-------------namespace--------------*/
//using namespace QtCharts;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void buildChart();                                      //创建图表函数
    void plotCustom(QByteArray info);

private slots:
    void on_OpenorClose_clicked();

    void on_sendPIDData_clicked();

    void on_sendData_clicked();

    void on_viewManual_clicked();

    void on_pushButton_clicked();

    void on_openOscill_clicked();

    void receiveInfo();

    void on_clearReceived_clicked();

    void on_stopOscill_clicked();

private:
    Ui::MainWindow *ui;

    /*-------------function----------------*/
    void system_init();
    QStringList getPortNameList();

    /*-------------variable----------------*/
    // 串口声明
    QSerialPort global_port;
    // 状态栏声明
    QLabel *statusLabel;
    QLabel *msgLabel;

    // 串口相关
    QString receivedDdata = "";
    bool receivedFlag = false;
    bool OpenorClose_Flag = false;
    bool stop_flag = true;

    // 示波器相关
    bool oscill_flag = false;
    QVector <QString>   Ptext;  // 每条曲线上的显示的文字
    QVector <QVector<double>>   YData;  // Y轴数据，可具有多个数据曲线
    QVector <double>            XData;  // x轴数据
    double index = 0;                       // 索引
    QString frontData;
    QString backData;
    QString oneData;
    QStringList Received_Keys;  // 标签值
};
#endif // MAINWINDOW_H
