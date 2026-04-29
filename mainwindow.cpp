#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_running(false)
{
    ui->setupUi(this);
    setWindowTitle("Time Tracker for Workers");

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTimer);

    connect(ui->btnAdd,       &QPushButton::clicked, this, &MainWindow::addTask);
    connect(ui->btnDelete,    &QPushButton::clicked, this, &MainWindow::deleteTask);
    connect(ui->btnStartStop, &QPushButton::clicked, this, &MainWindow::startStopTimer);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addTask()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Новая задача",
                                         "Название задачи:", QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty())
        ui->taskList->addItem(name.trimmed());
}

void MainWindow::deleteTask()
{
    QListWidgetItem *item = ui->taskList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Удаление", "Выберите задачу.");
        return;
    }
    if (m_running) {
        QMessageBox::warning(this, "Удаление", "Остановите таймер перед удалением.");
        return;
    }
    delete ui->taskList->takeItem(ui->taskList->row(item));
    ui->lblActiveTask->setText("Активная задача: —");
}

void MainWindow::startStopTimer()
{
    if (!m_running) {
        if (!ui->taskList->currentItem()) {
            QMessageBox::information(this, "Старт", "Выберите задачу из списка.");
            return;
        }
        m_elapsed = QTime(0, 0, 0);
        ui->lblTimer->setText("00:00:00");
        ui->lblActiveTask->setText("▶ " + ui->taskList->currentItem()->text());
        ui->btnStartStop->setText("⏹  Стоп");
        ui->btnStartStop->setStyleSheet(
            "background-color: #f44336; color: white; font-weight: bold; border-radius: 6px;");
        m_timer->start();
        m_running = true;
    } else {
        m_timer->stop();
        m_running = false;
        QString spent = ui->lblTimer->text();
        ui->lblActiveTask->setText("Активная задача: —");
        ui->btnStartStop->setText("▶  Старт");
        ui->btnStartStop->setStyleSheet(
            "background-color: #4CAF50; color: white; font-weight: bold; border-radius: 6px;");
        QMessageBox::information(this, "Готово",
            QString("Задача: %1\nВремя: %2")
                .arg(ui->taskList->currentItem()
                         ? ui->taskList->currentItem()->text() : "—")
                .arg(spent));
    }
}

void MainWindow::updateTimer()
{
    m_elapsed = m_elapsed.addSecs(1);
    ui->lblTimer->setText(m_elapsed.toString("HH:mm:ss"));
}
