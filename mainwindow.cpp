#include "mainwindow.h"
#include "database.h"
#include "editentrydialog.h"
#include "taskdialog.h"
#include "ui_mainwindow.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QHeaderView>

MainWindow::MainWindow(const User &currentUser, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_currentUser(currentUser)
{
    ui->setupUi(this);

    // Устанавливаем заголовок окна
    setWindowTitle(QString("Time Tracker - %1 (%2)")
                       .arg(m_currentUser.fullName)
                       .arg(m_currentUser.isAdmin() ? "Администратор" : "Сотрудник"));

    // Настройка прав доступа
    setupPermissions();

    // ── Timer ─────────────────────────────────────────────────────────────
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    // ── Default date range: today ─────────────────────────────────────────
    ui->dateFrom->setDate(QDate::currentDate());
    ui->dateTo->setDate(QDate::currentDate());

    // ── Signals ───────────────────────────────────────────────────────────
    connect(ui->btnAddTask,    &QPushButton::clicked, this, &MainWindow::onAddTask);
    connect(ui->btnEditTask,   &QPushButton::clicked, this, &MainWindow::onEditTask);
    connect(ui->btnDeleteTask, &QPushButton::clicked, this, &MainWindow::onDeleteTask);
    connect(ui->btnStartStop,  &QPushButton::clicked, this, &MainWindow::onStartStop);
    connect(ui->btnRefresh,    &QPushButton::clicked, this, &MainWindow::onRefreshHistory);
    connect(ui->btnEditEntry,  &QPushButton::clicked, this, &MainWindow::onEditEntry);
    connect(ui->btnDeleteEntry, &QPushButton::clicked, this, &MainWindow::onDeleteEntry);
    connect(ui->btnExportCsv,  &QPushButton::clicked, this, &MainWindow::onExportCsv);

    // double-click on history row → edit
    connect(ui->historyTable, &QTableWidget::doubleClicked, this, &MainWindow::onEditEntry);

    // Refresh history when switching to history tab
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int idx){
        if (idx == 1) onRefreshHistory();
    });

    refreshTaskList();
    refreshHistoryTable();

    // Today's total
    updateTodayTotal();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ── Настройка прав доступа ───────────────────────────────────────────────────
void MainWindow::setupPermissions()
{
    if (!m_currentUser.isAdmin()) {

        ui->statusFrame->setStyleSheet("QFrame { background-color: #2e2e3e; }");

        ui->lblStatus->setText("Режим: сотрудник");
    } else {
        ui->lblStatus->setText("👑 Режим: администратор");
    }
}

// ── Обновление итога за сегодня ──────────────────────────────────────────────
void MainWindow::updateTodayTotal()
{
    int userId = m_currentUser.isAdmin() ? -1 : m_currentUser.id;
    auto entries = Database::instance().getEntries(QDate::currentDate(), QDate::currentDate(), -1, userId);
    int total = 0;
    for (auto &e : entries) total += e.duration;
    ui->lblTodayTotal->setText("Итого: " + formatDuration(total));
}

// ── Close event ─────────────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_running) {
        auto btn = QMessageBox::question(this, "Таймер активен",
                                         "Таймер запущен. Остановить и сохранить запись перед выходом?",
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (btn == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (btn == QMessageBox::Yes) {
            Database::instance().stopEntry(m_activeEntryId);
        }
    }
    event->accept();
}

// ── Tasks ────────────────────────────────────────────────────────────────────
void MainWindow::refreshTaskList()
{
    ui->taskListWidget->clear();
    ui->comboTaskFilter->clear();
    ui->comboTaskFilter->addItem("Все задачи", -1);

    // Для работника показываем только его задачи
    // Для админа - все задачи
    int userId = m_currentUser.isAdmin() ? -1 : m_currentUser.id;

    for (const Task &t : Database::instance().getTasks(userId)) {
        QString label = t.project.isEmpty() ? t.name : QString("[%1] %2").arg(t.project, t.name);
        auto *item = new QListWidgetItem(label, ui->taskListWidget);
        item->setData(Qt::UserRole, t.id);
        ui->comboTaskFilter->addItem(label, t.id);
    }
}

void MainWindow::onAddTask()
{
    TaskDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted && !dlg.taskName().isEmpty()) {
        // Задача создается для текущего пользователя
        Database::instance().addTask(dlg.taskName(), dlg.project(), m_currentUser.id);
        refreshTaskList();
    }
}

void MainWindow::onEditTask()
{
    auto *item = ui->taskListWidget->currentItem();
    if (!item) {
        QMessageBox::information(this, "Редактирование", "Выберите задачу.");
        return;
    }

    int taskId = item->data(Qt::UserRole).toInt();
    int userId = m_currentUser.isAdmin() ? -1 : m_currentUser.id;

    for (const Task &t : Database::instance().getTasks(userId)) {
        if (t.id == taskId) {
            TaskDialog dlg(this, t.name, t.project);
            if (dlg.exec() == QDialog::Accepted && !dlg.taskName().isEmpty()) {
                Database::instance().updateTask(taskId, dlg.taskName(), dlg.project());
                refreshTaskList();
            }
            break;
        }
    }
}

void MainWindow::onDeleteTask()
{
    auto *item = ui->taskListWidget->currentItem();
    if (!item) {
        QMessageBox::information(this, "Удаление", "Выберите задачу.");
        return;
    }

    if (m_running && item->data(Qt::UserRole).toInt() == m_activeEntryId) {
        QMessageBox::warning(this, "Удаление", "Нельзя удалить задачу с активным таймером.");
        return;
    }

    if (QMessageBox::question(this, "Удаление", "Удалить задачу и все её записи?") == QMessageBox::Yes) {
        Database::instance().deleteTask(item->data(Qt::UserRole).toInt());
        refreshTaskList();
        refreshHistoryTable();
        updateTodayTotal();
    }
}

// ── Timer ─────────────────────────────────────────────────────────────────────
void MainWindow::onStartStop()
{
    if (!m_running) {
        auto *item = ui->taskListWidget->currentItem();
        if (!item) {
            QMessageBox::information(this, "Старт", "Выберите задачу из списка.");
            return;
        }

        int taskId = item->data(Qt::UserRole).toInt();

        // Запускаем таймер для текущего пользователя
        m_activeEntryId = Database::instance().startEntry(taskId, m_currentUser.id);
        if (m_activeEntryId < 0) {
            QMessageBox::critical(this, "Ошибка", "Не удалось создать запись.");
            return;
        }

        m_elapsed = QTime(0, 0, 0);
        m_running = true;
        m_timer->start();

        ui->lblTimer->setText("00:00:00");
        ui->lblStatus->setText("⏵ " + item->text());
        ui->lblActiveTask->setText("⏵ " + item->text());
        ui->lblActiveTask->setStyleSheet("color: #40a02b; font-size: 13px; font-weight: bold;");
        ui->btnStartStop->setText("⏸ Стоп");
        ui->btnStartStop->setStyleSheet(
            "QPushButton { background-color: #d20f39; color: white; font-size: 15px; font-weight: bold; border-radius: 8px; }"
            "QPushButton:hover { background-color: #f03050; }");
    } else {
        m_timer->stop();
        Database::instance().stopEntry(m_activeEntryId);
        m_running = false;
        m_activeEntryId = -1;

        updateTodayTotal();

        ui->lblStatus->setText(m_currentUser.isAdmin() ? "👑 Режим: администратор" : "👤 Режим: сотрудник");
        ui->lblActiveTask->setText("Выберите задачу из списка");
        ui->lblActiveTask->setStyleSheet("color: #888; font-size: 13px;");
        ui->lblTimer->setText("00:00:00");
        ui->btnStartStop->setText("▶ Старт");
        ui->btnStartStop->setStyleSheet(
            "QPushButton { background-color: #40a02b; color: white; font-size: 15px; font-weight: bold; border-radius: 8px; }"
            "QPushButton:hover { background-color: #50c030; }");

        refreshHistoryTable();  // Обновляем историю после остановки
    }
}

void MainWindow::onTimerTick()
{
    m_elapsed = m_elapsed.addSecs(1);
    ui->lblTimer->setText(m_elapsed.toString("HH:mm:ss"));
}

// ── History ───────────────────────────────────────────────────────────────────
void MainWindow::refreshHistoryTable()
{
    QDate from = ui->dateFrom->date();
    QDate to   = ui->dateTo->date();
    int taskId = ui->comboTaskFilter->currentData().toInt();

    // КЛЮЧЕВОЙ МОМЕНТ:
    // Для администратора показываем ВСЕ записи (userId = -1)
    // Для работника показываем ТОЛЬКО его записи (userId = m_currentUser.id)
    int userId = m_currentUser.isAdmin() ? -1 : m_currentUser.id;

    auto entries = Database::instance().getEntries(from, to, taskId > 0 ? taskId : -1, userId);

    ui->historyTable->setRowCount(0);
    ui->historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->historyTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);

    int totalSecs = 0;

    for (const TimeEntry &e : entries) {
        int row = ui->historyTable->rowCount();
        ui->historyTable->insertRow(row);

        auto cell = [&](int col, const QString &text){
            auto *it = new QTableWidgetItem(text);
            it->setData(Qt::UserRole, e.id);
            ui->historyTable->setItem(row, col, it);
        };

        cell(0, e.taskName);
        cell(1, e.project);
        cell(2, e.startTime.toString("dd.MM.yyyy HH:mm:ss"));
        cell(3, e.endTime.isValid() ? e.endTime.toString("dd.MM.yyyy HH:mm:ss") : "—");
        cell(4, formatDuration(e.duration));
        cell(5, e.description);

        totalSecs += e.duration;
    }

    ui->lblHistoryTotal->setText("Итого: " + formatDuration(totalSecs));

    // Добавляем информацию о том, чьи записи показываются
    if (m_currentUser.isAdmin()) {
        ui->lblHistoryTotal->setToolTip("Показаны записи всех сотрудников");
    } else {
        ui->lblHistoryTotal->setToolTip("Показаны только ваши записи");
    }
}

void MainWindow::onRefreshHistory()
{
    refreshHistoryTable();
}

void MainWindow::onEditEntry()
{
    int row = ui->historyTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Редактирование", "Выберите запись.");
        return;
    }

    int entryId = ui->historyTable->item(row, 0)->data(Qt::UserRole).toInt();

    // Получаем записи с учетом прав пользователя
    QDate from = ui->dateFrom->date();
    QDate to = ui->dateTo->date();
    int taskId = ui->comboTaskFilter->currentData().toInt();
    int userId = m_currentUser.isAdmin() ? -1 : m_currentUser.id;

    auto entries = Database::instance().getEntries(from, to, taskId > 0 ? taskId : -1, userId);

    for (const TimeEntry &e : entries) {
        if (e.id == entryId) {
            EditEntryDialog dlg(e, this);
            if (dlg.exec() == QDialog::Accepted) {
                if (dlg.startTime() >= dlg.endTime()) {
                    QMessageBox::warning(this, "Ошибка", "Время начала должно быть раньше конца.");
                    return;
                }
                Database::instance().updateEntry(entryId, dlg.startTime(), dlg.endTime(), dlg.description());
                refreshHistoryTable();
                updateTodayTotal();
            }
            break;
        }
    }
}

void MainWindow::onDeleteEntry()
{
    int row = ui->historyTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Удаление", "Выберите запись.");
        return;
    }

    if (QMessageBox::question(this, "Удаление", "Удалить эту запись?") == QMessageBox::Yes) {
        int entryId = ui->historyTable->item(row, 0)->data(Qt::UserRole).toInt();
        Database::instance().deleteEntry(entryId);
        refreshHistoryTable();
        updateTodayTotal();
    }
}

void MainWindow::onExportCsv()
{
    if (ui->historyTable->rowCount() == 0) {
        QMessageBox::information(this, "Экспорт", "Нет данных для экспорта.");
        return;
    }

    QString path = QFileDialog::getSaveFileName(this, "Сохранить CSV", "report.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл.");
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Добавляем BOM для UTF-8
    const char bom[] = { char(0xEF), char(0xBB), char(0xBF) };
    file.write(bom, 3);

    // Header
    out << "Задача;Проект;Начало;Конец;Длительность;Описание\n";

    for (int r = 0; r < ui->historyTable->rowCount(); ++r) {
        QStringList row;
        for (int c = 0; c < ui->historyTable->columnCount(); ++c) {
            QTableWidgetItem* item = ui->historyTable->item(r, c);
            row << (item ? item->text() : "");
        }
        out << row.join(";") << "\n";
    }

    file.close();
    QMessageBox::information(this, "Экспорт", "Файл сохранён:\n" + path);
}

// ── Helpers ───────────────────────────────────────────────────────────────────
QString MainWindow::formatDuration(int secs)
{
    int h = secs / 3600;
    int m = (secs % 3600) / 60;
    int s = secs % 60;
    return QString("%1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'));
}
