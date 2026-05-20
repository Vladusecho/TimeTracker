#include "editentrydialog.h"
#include "ui_editentrydialog.h"

EditEntryDialog::EditEntryDialog(const TimeEntry &entry, QWidget *parent)
    : QDialog(parent), ui(new Ui::EditEntryDialog)
{
    ui->setupUi(this);
    ui->dtStart->setDateTime(entry.startTime);
    ui->dtEnd->setDateTime(entry.endTime.isValid() ? entry.endTime : QDateTime::currentDateTime());
    ui->editDesc->setText(entry.description);
}

EditEntryDialog::~EditEntryDialog() { delete ui; }

QDateTime EditEntryDialog::startTime()   const { return ui->dtStart->dateTime(); }
QDateTime EditEntryDialog::endTime()     const { return ui->dtEnd->dateTime(); }
QString   EditEntryDialog::description() const { return ui->editDesc->text().trimmed(); }
