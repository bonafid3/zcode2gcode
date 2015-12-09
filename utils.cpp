#include "utils.h"

bool _intToStrTableValid = false;
int _intToStrTable[1000];

void init_intToStrTable()
{
  if(checkAndSet(_intToStrTableValid)){
    for(int i=0; i<1000; ++i){
      _intToStrTable[i]= ((i/100%10+0x30)<<8 )+
                         ((i/ 10%10+0x30)<<16)+
                         ((i/  1%10+0x30)<<24);
    }
  }
}

void appendInt(QByteArray& ba, int value)
{
  if(!value){ ba.append("0",1); return;}

  bool neg = value<0;
  if(neg) value = -value;

  char buff[16];
  char *p = &buff[15]; //starter pos

  while(value) {
    int remainder = divmod1000(value);
    p-=3;
    *((int*)p)=_intToStrTable[remainder];
  }

  ++p; while(*p=='0') ++p;  //clear back zeroes

  if(neg){ --p; *p = '-'; }

  ba.append(p, &buff[16]-p);
}

void appendFloat3(QByteArray& ba, float fvalue)
{
  //bypass ba.append(QByteArray::number(fvalue, 'f', 4)); return;

  int value = iround(fvalue*1000);

  bool neg = value<0;
  if(neg) value = -value;

  char buff[16];
  char *p = &buff[15]; //starter pos

  //first 3 digits is a must
  int remainder = divmod1000(value);
  p-=3;
  *((int*)p)=_intToStrTable[remainder];

  *p = '.'; --p; //decimal

  do{
    int remainder = divmod1000(value);
    p-=3;
    *((int*)p)=_intToStrTable[remainder];
  }while(value);

  ++p; while(*p=='0') ++p;  //clear back zeroes

  if(*p=='.') --p; //must have one '0' before decimal point

  if(neg){ --p; *p = '-'; }

  ba.append(p, &buff[16]-p);
}

QString padLeft(QString s, QChar ch, int size)
{
  return QString(size-s.size(), ch)+s;
}

QString padRight(QString s, QChar ch, int size)
{
  return s+QString(size-s.size(), ch);
}

QString toHex(int i, int places)
{
  return padLeft(QString::number((uint)i, 16), '0', places);
}

QString toHex(const void *p, int size)
{
  QString res;
  if(!p) return res;
  for(int i=0; i<size; ++i)
    res += toHex(((const uchar*)p)[i], 2)+" ";
  if(!res.isEmpty()) res.remove(res.size()-1, 1);
  return "["+res+"]";
}

int divmod1000(int& value)
{
  int res = value % 1000;
  value = value / 1000;
  return res;
}

class InitStrNumTables{
public:
  InitStrNumTables() {
    init_intToStrTable();
  }
};

InitStrNumTables init;
