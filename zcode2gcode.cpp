#include "zcode2gcode.h"
#include "ui_zcode2gcode.h"

#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QFileDialog>

#include "utils.h"

#define ORG "yingfeiyi"
#define APP "zcode2gcode"

#define qd qDebug()
#define echo(s) { ui->mPlainTextEdit->appendPlainText(s); }

ZCode2GCode::ZCode2GCode(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZCode2GCode)
{
    ui->setupUi(this);

    QString fname;
    QSettings s(ORG, APP, this);

    if(s.value("zcode_file").isValid())
    {
        fname = s.value("zcode_file").toString();
        QFile file(fname);
        if(file.exists())
            ui->mZCodeFileLineEdit->setText(fname);
    }

    if(s.value("gcode_file").isValid())
    {
        fname = s.value("gcode_file").toString();
        ui->mGCodeFileLineEdit->setText(fname);
    }

    if(s.value("head_temp").isValid())
        ui->mHeadTempSpinBox->setValue(s.value("head_temp").toInt());

    if(s.value("bed_temp").isValid())
        ui->mBedTempSpinBox->setValue(s.value("bed_temp").toInt());

    if(s.value("heatable_bed").isValid())
        ui->mBedTempGroupBox->setChecked(s.value("heatable_bed").toBool());

}

ZCode2GCode::~ZCode2GCode()
{
    delete ui;
}

void ZCode2GCode::on_mBrowseZCodePushButton_clicked()
{
    QString dd;
    QSettings s(ORG, APP, this);

    if(s.value("folder").isValid())
    {
        dd = s.value("folder").toString();
    }

    QFileDialog *fd = new QFileDialog(this, "Select a ZCode", dd, "*.zcode");
    fd->setFileMode(QFileDialog::ExistingFile);
    if(fd->exec() == QFileDialog::Accepted)
    {
        if(fd->selectedFiles().size())
        {
            QString fname = fd->selectedFiles().at(0);
            ui->mZCodeFileLineEdit->setText(fname);

            QFile file(fname);
            if(!QFile::exists(fname))
            {
                echo("File: "+fname+" does not exists!");
                return;
            }
            if(file.open(QFile::ReadOnly))
            {
                if(file.size() < 128)
                {
                    echo("File: "+fname+" is too small, probably not a ZCode!");
                    file.close();
                    return;
                }
                if(file.read(5) != "ZCode")
                {
                    QMessageBox::critical(this, "Error", "File: "+fname+" magic is shit!");
                    file.close();
                    return;
                }
                file.close();
            }
            ui->mGCodeFileLineEdit->setText(fname+".gcode");

            s.setValue("folder", fd->directory().absolutePath());
            s.setValue("zcode_file", fname);
            s.setValue("gcode_file", fname+".gcode");

            echo(fname+" selected");
        }
    }
}

QByteArray ZCode2GCode::convert(const QByteArray src)
{
    ui->mPlainTextEdit->clear();

    QSettings s(ORG, APP);
    if(ui->mGCodeFileLineEdit->text().size())
        s.setValue("gcode_file",ui->mGCodeFileLineEdit->text());

    if(!src.startsWith("ZCode")) return src;

    int ePercent = ui->mEScaleSpinBox->value();
    if(!inRange(ePercent, 10, 1000)) ePercent = 100;

    //axis scale factors
    const float XYScale = 1.f/ 640;
    const float ZScale  = 1.f/ 800;
    const float EScale  = 1.f/1200*(ePercent/100.f);
    const float scale[4] = { XYScale, XYScale, ZScale, EScale };

    int cZ=0, cE=0;

    QByteArray res, layer;

    #define ws(s) res.append(s)
    #define wi(i) appendInt(res,i)
    #define wf(f) appendFloat3(res,f)
    #define wsi(s,i) { ws(s);wi(i); }
    #define wsf(s,f) { ws(s);wf(f); }
    #define wln res.append("\r\n",2)
    #define wstln(s) { ws(QByteArray(";segType:")+s+"\r\n"); }

#define X 0
#define Y 1
#define Z 2
#define E 3

    #define ADDR_PRINT_DURATION 0x36
    #define ADDR_MATERIAL 0x3E

    const char *head = src.cbegin();

    QString materials[] = {"Z-ABS","Z-ULTRAT","Z-GLASS","Z-HIPS","Z-PCABS","Z-PETG"};

    echo("Material: "+materials[head[ADDR_MATERIAL]]);
    int *pd = (int*)(head+ADDR_PRINT_DURATION);
    echo("Print duration: "+toStr(*pd/60)+" minutes");

    const char *pnext = src.cbegin()+128;
    const char *pend = src.cend();
    int cnt=0;
    int lastSt = -1;
    QSet<int> unknownPathTypes;

    //M190 S60 ;set and wait bed temperature
    //M109 S215 ;set and wait head temperature

    ws("M104 S"); wi(ui->mHeadTempSpinBox->value()); wln;
    if(ui->mBedTempGroupBox->isChecked())
    {
        // set and wait for bed temp
        ws("M190 S"); wi(ui->mBedTempSpinBox->value()); wln;
    }
    // set and wait for head temp
    ws("M109 S"); wi(ui->mHeadTempSpinBox->value()); wln;

    while(pnext<pend)
    {
        QApplication::processEvents();

        cnt++;

        const char *p = pnext;
        int siz = p[0];
        p++; pnext = p+siz;
        if(pnext>pend){ echo("Error: Out of code"); break; }
        siz--;//without crc

        uchar p0 = p[0], p1 = p[1], p2 = p[2];

        if(p0==0x05){ echo("Cmd: 0x05: "+toHex(p+1,siz)); continue; }
        if(p0==0x08){ echo("Cmd: 0x08: "+toHex(p+1,siz)); continue; }
        if(p0==0x09){ echo("Cmd: 0x09: "+toHex(p+1,siz)); continue; }
        if(p0==0x0a){ echo("Cmd: 0x0a: "+toHex(p+1,siz)); continue; }
        if(p0==0x0b){ echo("Progress: "+toStr(p1)+"%"); continue; }
        if(p0==0x0e){ echo("Cmd: 0x0e: "+toHex(p+1,siz)); continue; }

        if(p0==0x02) //set feedrate
        {
            const int spd = *(int*)(p+1);
            wsi("G1 F", spd); wln;
            //layer.append("G1F"); appendInt(layer, spd); layer.append("\r\n");
            continue;
        }

        if(p0==0x04) //reset axes     p1: bit mask, XYZE, aztan annyi dword, ahany bit
        {
            int *val = (int*)(p+2);
            if(p1&0x07) //XYZ
            {
                // reset current Z
                if(p1&(1<<Z))
                    cZ = 0;

                ws("G28");
                static const QByteArray xyz("XYZ");
                for(int i=0; i<3; ++i)
                {
                    if(p1&(1<<i))
                    {
                        val++; //discard value
                        ws(xyz[i]+QByteArray("0")+" ");
                    }
                }
                wln;
                ws("; --- [ ZORTRAX RAFT ] ---");
                wln;
            }

            if(p1&0x08) //E
            {
                cE = *val;
                wsf("G92 E", (*val)*EScale); wln;
                val++;
            }

            if(p2&0xf0)
                echo("Unknown cmd: "+toStr(p0)+", values: "+toHex(p+1,siz));

            continue;
        }

        if(p0==0x01) //move XYZE, FAN and something unknown
        {
            int *val = (int*)(p+3);

            if(p2&0x0f)
            {
                //type
                if(lastSt!=p1)
                {
                    lastSt = p1;
                    if(p1<128)
                    {
                        switch(lastSt) //p1>=128 is travel
                        {
                            case 0: wstln("Perimeter"); break;
                            case 2: wstln("Loop"); break;
                            case 17: wstln("Loop"); break;
                            case 19: wstln("Loop"); break;

                            case 1: wstln("HShell"); break;
                            case 14: wstln("HShell"); break;
                            case 3: wstln("Infill"); break;

                            case 10: wstln("Raft"); break;
                            case 11: wstln("Raft"); break;
                            case 12: wstln("Raft"); break;
                            case 13: wstln("Support"); break; //top raft

                            case 4: wstln("Support"); break; //support perimeter
                            case 5: wstln("Support"); break; //inner support planes
                            case 7: wstln("WeakSupport"); break;
                            case 9: wstln("Support"); break;

                            case 15: wstln("HShell"); break;
                            case 18: wstln("HShell"); break;

                            case 20: wstln("Perimeter"); break; // small segment, default flow
                            case 21: wstln("Travel"); break; // some kind of travel

                            case 22:
                            case 23: wstln("WeakRaft"); wstln("Raft"); break; // object's first layer (weak)

                            case 27: wstln("HShell"); break;

                            default: wstln("Skirt " + toStr(lastSt)); unknownPathTypes << p1;
                        }
                    }
                }

                ws("G1 ");

                //layer.append("G1");

                static const QByteArray xyze("XYZE");
                for(int i=0; i<4; ++i)
                {
                    if(p2&(1<<i))
                    {
                        float f = (*val)*scale[i];

                        if(i == Z)
                        {
                            f = ((*val) - ui->mZOffsetDoubleSpinBox->value()*800) * scale[i];
                        }

                        if(i != 3 && (f<-1 || f>200))
                        {
                            echo("Out of range: "+toStr(f)+", "+xyze[i]+" "+toStr(p0)+toHex(p,siz));
                        }

                        if(i == Z)
                        {
                            int newZ = qRound(f*1000);
                            if(cZ != newZ) { cZ = newZ; }
                            echo("Z VAL: "+ toHex(p, siz));
                        }

                        //layer.append(xyze[i]); appendFloat3(layer, f);

                        wsf(xyze[i], f); ws(" ");
                        val++;
                    }
                }

                //layer.append("\r\n");
                wln;
            }

            for(int i=4; i<8; ++i)
            {
                if(p2&(1<<i))
                {
                    switch(i)
                    {
                        case 5: // FAN
                            wsi("M106 S", *val); wln;
                            //layer.append("M106 S"); appendInt(layer, *val); layer.append("\r\n");
                            val++;
                        break;
                        case 6:
                            ws(";Z CMD, VALUE: "); wi(*val); wln;
                            //layer.append(";Z CMD, VALUE: "); appendInt(layer, *val); layer.append("\r\n");
                            val++;
                        break;
                        default:
                            echo("UNKNOWN CMD, VALUE: "+toStr(i)+toStr(*val));
                            val++;
                        break;
                    }
                }
            }

            /*
            if(cZ > 0)
            {
                res.append(layer);
                layer.clear();
            }*/

            continue;
        }
        if(p0 == 0x12) {
            echo(toStr((int)pnext) + "," + toStr((int)pend));
            break;
        }
    }

    ws("G1 X0 Y0 F5000"); wln;

    if(!unknownPathTypes.empty())
    {
        echo("There are some unknown path types!");
        qd<<"code unknown path types"<<unknownPathTypes;
    }

    return res;
}

void ZCode2GCode::on_mConvertPushButton_clicked()
{
    QByteArray gcode;
    QFile file(ui->mZCodeFileLineEdit->text());
    if(file.open(QFile::ReadOnly))
    {
        gcode = convert(file.readAll());
        if(gcode.size())
        {
            QFile ofile(ui->mGCodeFileLineEdit->text());
            if(ofile.open(QFile::WriteOnly))
            {
                ofile.write(gcode);
                ofile.close();
            }
        }
        file.close();
    }
}

void ZCode2GCode::on_mHeadTempSpinBox_valueChanged(int arg1)
{
    QSettings s(ORG, APP);
    s.setValue("head_temp", arg1);
}

void ZCode2GCode::on_mBedTempSpinBox_valueChanged(int arg1)
{
    QSettings s(ORG, APP);
    s.setValue("bed_temp", arg1);
}

void ZCode2GCode::on_mBedTempGroupBox_toggled(bool arg1)
{
    QSettings s(ORG, APP);
    s.setValue("heatable_bed", arg1);
}
