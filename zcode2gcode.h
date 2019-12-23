#ifndef ZCODE2GCODE_H
#define ZCODE2GCODE_H

#include <QDialog>

namespace Ui {
class ZCode2GCode;
}

class ZCode2GCode : public QDialog
{
    Q_OBJECT

public:
    explicit ZCode2GCode(QWidget *parent = 0);
    ~ZCode2GCode();

private slots:
    void on_mBrowseZCodePushButton_clicked();
    void on_mConvertPushButton_clicked();

    void on_mHeadTempSpinBox_valueChanged(int arg1);

    void on_mBedTempSpinBox_valueChanged(int arg1);

    void on_mBedTempGroupBox_toggled(bool arg1);

private:
    Ui::ZCode2GCode *ui;
    void check(const QByteArray src);
    QByteArray convert(const QByteArray src);
};

#endif // ZCODE2GCODE_H
