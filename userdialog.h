#ifndef USERDIALOG_H
#define USERDIALOG_H

#include <QDialog>
#include "user.h"

namespace Ui { class UserDialog; }

class UserDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UserDialog(QWidget *parent = nullptr, const User *user = nullptr);
    ~UserDialog();

    QString getLogin() const;
    QString getPassword() const;
    QString getFullName() const;
    QString getRole() const;

private:
    Ui::UserDialog *ui;
    bool m_isEditMode = false;
};

#endif // USERDIALOG_H
