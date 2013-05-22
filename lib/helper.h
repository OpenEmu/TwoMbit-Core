#ifndef HELPER_H
#define HELPER_H

#define free_mem(__m) if(__m) { delete[](__m); __m = 0; }

#ifndef _max
#define _max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef _min
#define _min(a,b) (((a) < (b)) ? (a) : (b))
#endif

template<typename T> inline T _minmax(const T x, const T min, const T max) {
    return (x < min) ? min : ((x > max) ? max : x);
}

inline signed sclamp(unsigned bits, const signed x) {
    const signed b = 1U << (bits - 1);
    const signed m = (1U << (bits - 1)) - 1;
    return (x > m) ? m : (x < -b) ? -b : x;
}

template<typename T> inline void _sort(T arr[], unsigned size) {
    T swap=0;
    for(unsigned i=0; i < (size-1); i++) {
        for(unsigned k=0; k < (size-1-i); k++) {
            if(arr[k]>arr[k+1]) {
                swap=arr[k];
                arr[k]=arr[k+1];
                arr[k+1]=swap;
            }
        }
    }
}

#define _getBit(reg, bit) ( (reg >> (bit)) & 1 )
#define _setBit(reg, bit) reg |= (1 << (bit))
#define _resBit(reg, bit) reg &= ~(1 << (bit))

#endif

