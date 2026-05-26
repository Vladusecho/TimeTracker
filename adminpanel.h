#ifndef ADMINPANEL_H
#define ADMINPANEL_H

#include <QDialog>
#include <QTableWidgetItem>
#include "user.h"

namespace Ui { class AdminPanel; }

class AdminPanel : public QDialog
{
    Q_OBJECT
public:
    explicit AdminPanel(QWidget *parent = nullptr);
    ~AdminPanel();

private slots:
    void onAddUser();
    void onEditUser();
    void onDeleteUser();
    void onRefresh();

private:
    Ui::AdminPanel *ui;
    void loadUsers();
};

#endif // ADMINPANEL_H
