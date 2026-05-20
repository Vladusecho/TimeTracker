#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>

namespace Ui { class TaskDialog; }

class TaskDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TaskDialog(QWidget *parent = nullptr,
                        const QString &name = {},
                        const QString &project = {});
    ~TaskDialog();

    QString taskName() const;
    QString project()  const;

private:
    Ui::TaskDialog *ui;
};

#endif // TASKDIALOG_H
