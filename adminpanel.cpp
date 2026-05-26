#include "adminpanel.h"
#include "ui_adminpanel.h"
#include "database.h"
#include "userdialog.h"
#include "passwordmanager.h"
#include <QMessageBox>
#include <QHeaderView>

AdminPanel::AdminPanel(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AdminPanel)
{
    ui->setupUi(this);
    setWindowTitle("Управление пользователями");

    // Настройка таблицы
    ui->userTable->setColumnCount(4);
    ui->userTable->setHorizontalHeaderLabels({"ID", "Логин", "ФИО", "Роль"});
    ui->userTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->userTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->userTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Подключаем кнопки
    connect(ui->btnAdd, &QPushButton::clicked, this, &AdminPanel::onAddUser);
    connect(ui->btnEdit, &QPushButton::clicked, this, &AdminPanel::onEditUser);
    connect(ui->btnDelete, &QPushButton::clicked, this, &AdminPanel::onDeleteUser);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &AdminPanel::onRefresh);

    // Загружаем пользователей
    loadUsers();
}

AdminPanel::~AdminPanel()
{
    delete ui;
}

void AdminPanel::loadUsers()
{
    ui->userTable->setRowCount(0);

    QList<User> users = Database::instance().getAllUsers();

    for (const User &user : users) {
        int row = ui->userTable->rowCount();
        ui->userTable->insertRow(row);

        ui->userTable->setItem(row, 0, new QTableWidgetItem(QString::number(user.id)));
        ui->userTable->setItem(row, 1, new QTableWidgetItem(user.login));
        ui->userTable->setItem(row, 2, new QTableWidgetItem(user.fullName));

        QString roleText = user.isAdmin() ? "Администратор" : "Сотрудник";
        ui->userTable->setItem(row, 3, new QTableWidgetItem(roleText));

        // Сохраняем ID пользователя в данные строки
        ui->userTable->item(row, 0)->setData(Qt::UserRole, user.id);
    }
}

void AdminPanel::onAddUser()
{
    UserDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        if (Database::instance().addUser(
                dlg.getLogin(),
                dlg.getPassword(),
                dlg.getFullName(),
                dlg.getRole()
                )) {
            QMessageBox::information(this, "Успех", "Пользователь добавлен");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось добавить пользователя. Возможно, логин уже существует.");
        }
    }
}

void AdminPanel::onEditUser()
{
    int row = ui->userTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Редактирование", "Выберите пользователя");
        return;
    }

    int userId = ui->userTable->item(row, 0)->data(Qt::UserRole).toInt();
    User user = Database::instance().getUserById(userId);

    if (user.id == -1) {
        QMessageBox::warning(this, "Ошибка", "Пользователь не найден");
        return;
    }

    UserDialog dlg(this, &user);
    if (dlg.exec() == QDialog::Accepted) {
        // Обновляем данные пользователя
        if (Database::instance().updateUser(userId, dlg.getLogin(), dlg.getFullName(), dlg.getRole())) {
            // Если пароль введен, меняем его
            if (!dlg.getPassword().isEmpty()) {
                Database::instance().changePassword(userId, dlg.getPassword());
            }
            QMessageBox::information(this, "Успех", "Пользователь обновлен");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось обновить пользователя");
        }
    }
}

void AdminPanel::onDeleteUser()
{
    int row = ui->userTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "Удаление", "Выберите пользователя");
        return;
    }

    int userId = ui->userTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString userName = ui->userTable->item(row, 2)->text();

    // Проверяем, не удаляем ли мы самого себя
    // Нужно передать текущего пользователя. Пока сделаем простое предупреждение
    if (QMessageBox::question(this, "Удаление",
                              QString("Удалить пользователя '%1'?\nВсе его задачи и записи времени будут удалены.").arg(userName),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

        if (Database::instance().deleteUser(userId)) {
            QMessageBox::information(this, "Успех", "Пользователь удален");
            loadUsers();
        } else {
            QMessageBox::warning(this, "Ошибка", "Не удалось удалить пользователя (возможно, это последний администратор)");
        }
    }
}

void AdminPanel::onRefresh()
{
    loadUsers();
}
