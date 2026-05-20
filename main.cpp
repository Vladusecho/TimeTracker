#include "mainwindow.h"
#include "database.h"
#include "authdialog.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("TimeTracker");
    app.setOrganizationName("WorkTrack");

    if (!Database::instance().init()) {
        QMessageBox::critical(nullptr, "Ошибка",
                              "Не удалось открыть базу данных.\nПриложение будет закрыто.");
        return 1;
    }

    User testUser = Database::instance().getUserByLogin("worker");
    if (testUser.id == -1) {
        Database::instance().addUser("worker", "worker123", "Тестовый сотрудник", "worker");
    }

    // Показываем диалог авторизации
    AuthDialog authDialog;
    if (authDialog.exec() != QDialog::Accepted || !authDialog.isAuthenticated()) {
        return 0; // Пользователь отменил вход
    }

    MainWindow w(authDialog.getCurrentUser());
    w.show();
    return app.exec();
}
