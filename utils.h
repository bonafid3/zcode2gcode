#ifndef UTILS_H
#define UTILS_H

#include <QString>

template <typename T>
Q_DECL_CONSTEXPR inline T inRange(const T &a, const T &mi, const T &ma) { return a>=mi && a<=ma; }
Q_DECL_CONSTEXPR inline QString toStr(int i) { return QString::number(i); }
Q_DECL_CONSTEXPR inline QString toStr(float f) { return QString::number(f); }

Q_DECL_CONSTEXPR inline bool checkAndSet  (bool& b) { bool res = !b; b = true ; return res; }

void appendInt(QByteArray& ba, int value);
void appendFloat3(QByteArray& ba, float fvalue);

QString padLeft(QString s, QChar ch, int size);
QString padRight(QString s, QChar ch, int size);
QString toHex(int i, int places);
QString toHex(const void *p, int size);

int divmod1000(int& value);

#define IROUND(a) (a)>=0.0f ? int((a)+0.5f) : int((a)-float(int((a)-1))+0.5f)+int((a)-1); //0.5 goes up (= qRound)
#define IFLOOR(a) ((a)>=0?(int)(a): (int)((a)-(int)(a)+1)+(int)(a)-1)
#define ICEIL(a)  (-IFLOOR(-(a)))

Q_DECL_CONSTEXPR inline int fast_iround(float a) { return IROUND(a); }
Q_DECL_CONSTEXPR inline int fast_ifloor(float a) { return IFLOOR(a); }
Q_DECL_CONSTEXPR inline int fast_iceil(float a) { return ICEIL(a); }

#define itrunc(a) ((int)(a))
#define iround(a) fast_iround(a)
#define ifloor(a) fast_ifloor(a)
#define iceil(a)  fast_iceil(a)

//void init_strToNormalFloat_table();

#endif // UTILS_H
