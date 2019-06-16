#ifndef _____SWAN_BOUND_FUNCTION_HPP_____
#define _____SWAN_BOUND_FUNCTION_HPP_____
#include "Object.hpp"
#include "Value.hpp"

struct BoundFunction: QObject {
QV object, method;
BoundFunction (QVM& vm, const QV& o, const QV& m);
virtual bool gcVisit () override;
virtual ~BoundFunction () = default;
virtual size_t getMemSize () override { return sizeof(*this); }
};

#endif
