#include "authdialog.h"
#include "ui_authdialog.h"
#include "database.h"
#include "passwordmanager.h"
#include <QMessageBox>
#include <QDebug>

AuthDialog::AuthDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AuthDialog)
{
    ui->setupUi(this);
    setWindowTitle("Авторизация");
    setModal(true);

    connect(ui->btnLogin, &QPushButton::clicked, this, &AuthDialog::onLoginClicked);
    connect(ui->btnCancel, &QPushButton::clicked, this, &AuthDialog::onCancelClicked);

    // Настройка внешнего вида
    ui->editPassword->setEchoMode(QLineEdit::Password);

    // Фокус на поле логина
    ui->editLogin->setFocus();
}

AuthDialog::~AuthDialog()
{
    delete ui;
}

void AuthDialog::onLoginClicked()
{
    QString login = ui->editLogin->text().trimmed();
    QString password = ui->editPassword->text();

    if (login.isEmpty()) {
        showError("Введите логин");
        return;
    }

    if (password.isEmpty()) {
        showError("Введите пароль");
        return;
    }

    // Ищем пользователя в БД
    User user = Database::instance().getUserByLogin(login);

    if (user.id == -1) {
        showError("Пользователь не найден");
        return;
    }

    // Проверяем пароль
    if (!PasswordManager::verifyPassword(password, user.passwordHash)) {
        showError("Неверный пароль");
        return;
    }

    m_currentUser = user;
    m_authenticated = true;

    accept();
}

void AuthDialog::onCancelClicked()
{
    reject();
}

void AuthDialog::showError(const QString &message)
{
    QMessageBox::warning(this, "Ошибка входа", message);
    ui->editPassword->clear();
    ui->editPassword->setFocus();
}
