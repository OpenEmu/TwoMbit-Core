#ifndef __STRING_H
#define __STRING_H

#include <sstream>
#include <string>
using namespace std;

class __String
{
public:
    template<typename T>
    static T hexToInt(const std::string& str) {
        if( str.size() == 0 ) {
            return 0;
        }
        istringstream iss(str);
        T result = 0;
        if( !( iss >> std::hex >> result) ) {
            throw;
        }
        return result;
    }

    template<typename T>
    static void removeChars( std::basic_string<T>& Str, const T * CharsToRemove )
    {
        typename std::basic_string<T>::size_type pos = 0;
        while (( pos = Str.find_first_of( CharsToRemove, pos )) != std::basic_string<T>::npos ) {
            Str.erase( pos, 1 );
        }
    }
};

#endif // __STRING_H
