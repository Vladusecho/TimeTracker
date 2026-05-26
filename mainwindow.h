#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include "user.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(const User &currentUser, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Tasks
    void onAddTask();
    void onEditTask();
    void onDeleteTask();

    // Timer
    void onStartStop();
    void onTimerTick();

    // History
    void onRefreshHistory();
    void onEditEntry();
    void onDeleteEntry();
    void onExportCsv();

    void onAdminPanel();

private:
    Ui::MainWindow *ui;

    User m_currentUser;  // Текущий пользователь

    QTimer  *m_timer;
    QTime    m_elapsed;
    bool     m_running   = false;
    int      m_activeEntryId = -1;

    void refreshTaskList();
    void refreshHistoryTable();
    void setupPermissions();      // Настройка прав доступа
    void updateTodayTotal();      // Обновление итога за сегодня

    void setupAdminPanel();

    static QString formatDuration(int secs);
};

#endif // MAINWINDOW_H
