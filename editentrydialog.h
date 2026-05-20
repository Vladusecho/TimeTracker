#ifndef EDITENTRYDIALOG_H
#define EDITENTRYDIALOG_H

#include <QDialog>
#include <QDateTime>
#include "database.h"

namespace Ui { class EditEntryDialog; }

class EditEntryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditEntryDialog(const TimeEntry &entry, QWidget *parent = nullptr);
    ~EditEntryDialog();

    QDateTime startTime()   const;
    QDateTime endTime()     const;
    QString   description() const;

private:
    Ui::EditEntryDialog *ui;
};

#endif // EDITENTRYDIALOG_H
