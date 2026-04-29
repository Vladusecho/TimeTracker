#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void addTask();
    void deleteTask();
    void startStopTimer();
    void updateTimer();

private:
    Ui::MainWindow *ui;

    QTimer *m_timer;
    QTime   m_elapsed;
    bool    m_running;
};

#endif // MAINWINDOW_H
