#include "taskdialog.h"
#include "ui_taskdialog.h"

TaskDialog::TaskDialog(QWidget *parent, const QString &name, const QString &project)
    : QDialog(parent), ui(new Ui::TaskDialog)
{
    ui->setupUi(this);
    ui->editName->setText(name);
    ui->editProject->setText(project);
    ui->editName->setFocus();
}

TaskDialog::~TaskDialog() { delete ui; }

QString TaskDialog::taskName() const { return ui->editName->text().trimmed(); }
QString TaskDialog::project()  const { return ui->editProject->text().trimmed(); }
