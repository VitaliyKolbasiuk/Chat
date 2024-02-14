#ifndef CHAT_SETTINGSDIALOG_H
#define CHAT_SETTINGSDIALOG_H

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class SettingsDialog; }
QT_END_NAMESPACE

class SettingsDialog : public QDialog {
Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    ~SettingsDialog() override;

private slots:
    void on_buttonBox_accepted();

    void on_buttonBox_rejected();

private:
    Ui::SettingsDialog *ui;
};

#endif //CHAT_SETTINGSDIALOG_H
