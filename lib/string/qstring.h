
#ifndef __QSTRING_H
#define __QSTRING_H

#include <QString>

class __QString
{
public:
    enum Type { STR, DEC, BIN, HEX };
    static void remove_quote(QString& str);
    static QString int_to_string(Type type, int input);
    static int string_to_int(Type type, QString input, bool& error);
};

#endif
