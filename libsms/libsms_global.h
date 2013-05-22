#ifndef LIBSMS_GLOBAL_H
#define LIBSMS_GLOBAL_H

#if defined(LIBSMS_LIBRARY)
#  define LIBSMSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define LIBSMSSHARED_EXPORT
#endif

#endif // LIBSMS_GLOBAL_H
