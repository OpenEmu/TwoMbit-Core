
#include "qstring.h"

void __QString::remove_quote(QString& str) {
    int s = str.length();
    if(s < 2) return;
    else if(str.at(0) == '\"' && str.at(s - 1) == '\"');
    else if(str.at(0) == '\'' && str.at(s - 1) == '\'');
    else return;

    str.mid(1, str.length()-2);
}

QString __QString::int_to_string(__QString::Type type, int input) {
    switch(type) {
        case DEC:
        default:
            return QString::number(input, 10);
        case BIN:
            return input ? "true" : "false";
        case HEX:
            return QString::number(input, 16).prepend("0x");
    }
}

int __QString::string_to_int(__QString::Type type, QString input, bool& error) {
    int value;
    bool ok;
    error = false;

    switch(type) {
        case BIN:
            if ((input == "true") || (input == "enabled") || (input == "on") || (input == "yes") || (input == "1"))
                return (int)true;

            if((input == "false") || (input == "disabled") || (input == "off") || (input == "no") || (input == "0"))
                return (int)false;
            break;
        case HEX:
            if ((input.length() < 3) || (input.length() > 6)) break;
            value = input.toInt(&ok, 16);
            if (!ok) break;
            return value;
        case DEC:
        default:
            value = input.toInt(&ok, 10);
            if (!ok) break;
            return value;
    }
    error = true;
    return 0;
}

