#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>
#include "user.h"

namespace Ui { class AuthDialog; }

class AuthDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AuthDialog(QWidget *parent = nullptr);
    ~AuthDialog();

    User getCurrentUser() const { return m_currentUser; }
    bool isAuthenticated() const { return m_authenticated; }

private slots:
    void onLoginClicked();
    void onCancelClicked();

private:
    Ui::AuthDialog *ui;
    User m_currentUser;
    bool m_authenticated = false;

    void showError(const QString &message);
};

#endif // AUTHDIALOG_H
