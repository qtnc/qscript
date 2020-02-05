#ifndef _____SWAN_STRING_HPP_____
#define _____SWAN_STRING_HPP_____
#include "Iterable.hpp"
#include<string>

void appendToString (QFiber& f, QV x, std::string& out);

struct QString: QSequence {
size_t length;
char data[];
static QString* create (QVM& vm, const std::string& str);
static QString* create (QVM& vm, const char* str, int length = -1);
static inline QString* create (QVM& vm, const char* start, const char* end) { return create(vm, start, end-start); }
static QString* create (QString*);
QString (QVM& vm, size_t len);
inline std::string asString () { return std::string(data, length); }
inline char* begin () { return data; }
inline char* end () { return data+length; }
~QString ();
inline size_t getMemSize () { return sizeof(*this) + sizeof(char) * (length+1); }
inline int getLength () { return length; }
};

struct QStringIterator: QObject {
QString& str;
char* iterator;
QStringIterator (QVM& vm, QString& m);
bool gcVisit ();
~QStringIterator() = default;
};
#endif
