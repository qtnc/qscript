#include<optional>
#include "../include/Swan.hpp"
#include "../include/SwanBinding.hpp"
#include "../include/cpprintf.hpp"
#include<iostream>
#include<sstream>
#include<fstream>
#include<exception>
#include<memory>
#include<cstdlib>
#include<cstdio>
using namespace std;

struct IO {
Swan::VM::EncodingConversionFn encoder;
Swan::VM::DecodingConversionFn decoder;
shared_ptr<istream> in;
shared_ptr<ostream> out;
IO (): in(nullptr), out(nullptr), encoder(nullptr), decoder(nullptr) {}
IO (shared_ptr<istream> i, const Swan::VM::DecodingConversionFn& c = nullptr): in(i), out(nullptr), encoder(nullptr), decoder(c) {}
IO (shared_ptr<ostream> o, const Swan::VM::EncodingConversionFn& c = nullptr): in(nullptr), out(o), encoder(c), decoder(nullptr)  {}
void flush () {
if (out && *out) *out << std::flush;
}
void close () { 
in=nullptr;
out=nullptr;
}
int tell () { return in && *in ? static_cast<int>(in->tellg()) : -1; }
int seek (int pos, optional<bool> absolute) {
bool abs = absolute.value_or(false);
if (!in || !*in) return -1;
if (!abs) in->seekg(pos, ios_base::cur);
else if (pos>=0) in->seekg(pos, ios_base::beg);
else in->seekg(pos, ios_base::end);
return tell();
}
bool operator! () { return (in && !*in) || (out && !*out); }
operator bool () { return (in && *in) || (out && *out); }
};
IO cstdin, cstdout, cstderr;

static IO& ioGetStdout () {
if (!cstdout.out) {
cstdout.out = make_shared<ostream>(cout.rdbuf());
cstdout.encoder = Swan::VM::getEncoder(("native"));
}
return cstdout;
}

static IO& ioGetStderr () {
if (!cstderr.out) {
cstderr.out = make_shared<ostream>(cerr.rdbuf());
cstderr.encoder = Swan::VM::getEncoder(("native"));
}
return cstderr;
}

static IO& ioGetStdin () {
if (!cstdin.in) {
cstdin.in = make_shared<istream>(cin.rdbuf());
cstdin.decoder = Swan::VM::getDecoder(("native"));
}
return cstdin;
}

static IO& ioSetStdout (IO& x) { return cstdout = x; }
static IO& ioSetStderr (IO& x) { return cstderr = x; }
static IO& ioSetStdin (IO& x) { return cstdin = x; }

static void print (Swan::Fiber& f) {
auto& io = ioGetStdout();
ostringstream p;
for (int i=0, n=f.getArgCount(); i<n; i++) {
if (i>0) p<<' ';
if (f.isString(i)) p << f.getString(i);
else {
f.pushCopy(i);
f.callMethod("toString", 1);
p << f.getString(-1);
f.pop();
}}
p << endl;
if (io.encoder) {
istringstream in(p.str());
io.encoder(in, *io.out);
}
else *io.out << p.str();
f.setUndefined(0);
}

static void ioWrite (Swan::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (io.encoder) {
istringstream in(f.getString(1));
Swan::ScopeUnlocker<Swan::VM> unlocker(f.getVM());
io.encoder(in, *io.out);
}
else {
int length;
const char* buffer = f.getBuffer<char>(1, &length);
Swan::ScopeUnlocker<Swan::VM> unlocker(f.getVM());
io.out->write(buffer, length);
}}

static void ioRead (Swan::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
int num = f.getOptionalNum(1, -1);
ostringstream out;
if (!io.in || !*io.in) { f.setUndefined(0); return; }
if (io.decoder) {
f.getVM().unlock();
if (num<0) io.decoder(*io.in, out, 0);
else while(--num>=0 && io.in && *io.in) io.decoder(*io.in, out, 1);
f.getVM().lock();
f.setString(0, out.str());
}
else if (num<0) {
f.getVM().unlock();
out << io.in->rdbuf();
string s = out.str();
f.getVM().lock();
f.setBuffer(0, s.data(), s.size());
}
else {
f.getVM().unlock();
auto buf = make_unique<char[]>(num);
io.in->read(&buf[0], num);
f.getVM().lock();
f.setBuffer(0, &buf[0], io.in->gcount());
}}

static void ioReadLine (Swan::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
if (!io.in || !*io.in) { f.setUndefined(0); return; }
if (io.decoder) {
f.getVM().unlock();
ostringstream out;
io.decoder(*io.in, out, 2);
f.getVM().lock();
f.setString(0, out.str());
}
else {
f.getVM().unlock();
string s;
getline(*io.in, s);
f.getVM().lock();
f.setBuffer(0, s.data(), s.size());
}}

static IO ioOpen (const string& target, optional<string> omode, optional<string> encoding, Swan::Fiber& f) {
Swan::ScopeUnlocker<Swan::VM> unlocker(f.getVM());
string mode = omode.value_or("r");
string enc = encoding.value_or("utf8");
if (mode.size()>3) { enc=mode; mode=""; }
bool reading = mode.find("r")!=string::npos;
bool writing = mode.find("w")!=string::npos;
bool appending = mode.find("a")!=string::npos;
bool binary = mode.find("b")!=string::npos;
if (reading) {
if (binary) IO(make_shared<ifstream>(target, ios::binary));
else return IO(make_shared<ifstream>(target), Swan::VM::getDecoder(enc));
}
else if (writing) {
if (binary) IO(make_shared<ofstream>(target, ios::binary));
else return IO(make_shared<ofstream>(target), Swan::VM::getEncoder(enc));
}
else if (appending) {
if (binary) IO(make_shared<ofstream>(target, ios::binary | ios::app));
else return IO(make_shared<ofstream>(target, ios::app), Swan::VM::getEncoder(enc));
}
throw std::logic_error(format("Unknown open mode: %s", mode));
}

static IO ioCreate (optional<string> encoding) {
if (encoding) return IO(make_shared<ostringstream>(), Swan::VM::getEncoder(encoding.value_or("utf8")));
else return IO(make_shared<ostringstream>());
}

static void ioOf (Swan::Fiber& f) {
string enc = f.getOptionalString(2, "");
if (f.isBuffer(1)) {
int length;
const void* buf = f.getBufferV(1, &length);
string s(reinterpret_cast<const char*>(buf), length);
auto st = make_shared<istringstream>(s);
if (enc.empty()) f.emplaceUserObject<IO>(0, st);
else f.emplaceUserObject<IO>(0, st, Swan::VM::getDecoder(enc));
}
else if (f.isString(1)) {
string s = f.getString(1);
auto st = make_shared<istringstream>(s);
if (enc.empty()) f.emplaceUserObject<IO>(0, st);
else f.emplaceUserObject<IO>(0, st, Swan::VM::getDecoder(enc));
}
else throw std::logic_error("Expecting a buffer or a string");
}

static void ioToBuffer (Swan::Fiber& f) {
IO& io = f.getUserObject<IO>(0);
auto out = dynamic_pointer_cast<ostringstream>(io.out);
if (!out) throw std::logic_error("Couldn't convert to buffer");
string s = out->str();
f.setBuffer(0, s.data(), s.size());
}

std::function<double(double)> test (double a) {
return [=](double b){ return a*b; };
}

void registerIO (Swan::Fiber& f) {
f.loadGlobal("Iterable");
f.registerClass<IO>("IO", 1);
f.registerDestructor<IO>();
f.registerStaticMethod("open", STATIC_METHOD(ioOpen));
f.registerStaticMethod("create", STATIC_METHOD(ioCreate));
f.registerStaticMethod("of", ioOf);
f.registerMethod("read", ioRead);
f.registerMethod("readLine", ioReadLine);
f.registerMethod("next", ioReadLine);
f.registerMethod("!", METHOD(IO, operator!));
f.registerMethod("?", METHOD(IO, operator bool));
f.registerMethod("hasNext", METHOD(IO, operator bool));
f.registerMethod("write", ioWrite);
f.registerMethod("flush", METHOD(IO, flush));
f.registerMethod("seek", METHOD(IO, seek));
f.registerMethod("tell", METHOD(IO, tell));
f.registerMethod("close", METHOD(IO, close));
f.registerMethod("toBuffer", ioToBuffer);
f.pop();

f.pushNewForeignClass("System", 0, 0, 0);
f.storeGlobal("System");
f.registerStaticProperty("out", STATIC_METHOD(ioGetStdout), STATIC_METHOD(ioSetStdout));
f.registerStaticProperty("err", STATIC_METHOD(ioGetStderr), STATIC_METHOD(ioSetStderr));
f.registerStaticProperty("in", STATIC_METHOD(ioGetStdin), STATIC_METHOD(ioSetStdin));
f.pop();

f.registerFunction("print", print);
f.registerFunction("test", FUNCTION(test));
}

