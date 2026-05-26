#include "userdialog.h"
#include "ui_userdialog.h"
#include <QMessageBox>

UserDialog::UserDialog(QWidget *parent, const User *user)
    : QDialog(parent)
    , ui(new Ui::UserDialog)
{
    ui->setupUi(this);

    if (user) {
        // Режим редактирования
        m_isEditMode = true;
        setWindowTitle("Редактирование пользователя");
        ui->editLogin->setText(user->login);
        ui->editFullName->setText(user->fullName);
        ui->comboRole->setCurrentText(user->role);
        ui->editPassword->setPlaceholderText("Оставьте пустым, чтобы не менять пароль");
        ui->editPassword->setEnabled(true);
    } else {
        // Режим добавления
        setWindowTitle("Добавление пользователя");
        ui->editPassword->setPlaceholderText("Введите пароль");
    }

    // Настройка валидации
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (getLogin().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Логин не может быть пустым");
            return;
        }
        if (!m_isEditMode && getPassword().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Пароль не может быть пустым");
            return;
        }
        if (getFullName().isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "ФИО не может быть пустым");
            return;
        }
        accept();
    });
}

UserDialog::~UserDialog()
{
    delete ui;
}

QString UserDialog::getLogin() const
{
    return ui->editLogin->text().trimmed();
}

QString UserDialog::getPassword() const
{
    return ui->editPassword->text();
}

QString UserDialog::getFullName() const
{
    return ui->editFullName->text().trimmed();
}

QString UserDialog::getRole() const
{
    return ui->comboRole->currentText() == "Администратор" ? "admin" : "worker";
}
