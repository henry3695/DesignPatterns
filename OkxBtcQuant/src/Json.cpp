// Json.cpp - 极简 JSON 实现（递归下降解析器）
#include "Json.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace minij {

namespace {

struct Parser {
    const char* p;
    const char* end;
    std::string err;
    int depth = 0;

    Parser(const char* b, const char* e) : p(b), end(e) {}

    bool fail(const char* m) { if (err.empty()) err = m; return false; }

    void skip() {
        while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p;
    }

    bool lit(const char* s) {
        size_t n = strlen(s);
        if (end - p < (ptrdiff_t)n || memcmp(p, s, n) != 0) return fail("bad literal");
        p += n;
        return true;
    }

    static void utf8(std::string& s, unsigned cp) {
        if (cp < 0x80) s += (char)cp;
        else if (cp < 0x800) { s += (char)(0xC0 | (cp >> 6)); s += (char)(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) { s += (char)(0xE0 | (cp >> 12)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
        else { s += (char)(0xF0 | (cp >> 18)); s += (char)(0x80 | ((cp >> 12) & 0x3F)); s += (char)(0x80 | ((cp >> 6) & 0x3F)); s += (char)(0x80 | (cp & 0x3F)); }
    }

    unsigned hex4() {
        unsigned v = 0;
        for (int i = 0; i < 4; ++i) {
            if (p >= end) return v;
            char c = *p++;
            v <<= 4;
            if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
            else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
            else { return v; }
        }
        return v;
    }

    bool stringVal(std::string& out) {
        ++p; // 跳过 "
        out.clear();
        while (p < end) {
            unsigned char c = (unsigned char)*p++;
            if (c == '"') return true;
            if (c == '\\') {
                if (p >= end) return fail("bad escape");
                char e = *p++;
                switch (e) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case '/':  out += '/';  break;
                    case 'b':  out += '\b'; break;
                    case 'f':  out += '\f'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    case 'u': {
                        if (end - p < 4) return fail("bad \\u");
                        unsigned cp = hex4();
                        if (cp >= 0xD800 && cp <= 0xDBFF) {
                            if (end - p >= 6 && p[0] == '\\' && p[1] == 'u') {
                                p += 2;
                                unsigned lo = hex4();
                                if (lo >= 0xDC00 && lo <= 0xDFFF)
                                    cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                                else return fail("bad surrogate");
                            } else return fail("bad surrogate");
                        }
                        utf8(out, cp);
                        break;
                    }
                    default: return fail("bad escape");
                }
            } else {
                out += (char)c;
            }
        }
        return fail("unterminated string");
    }

    Json number() {
        char* e = nullptr;
        double v = strtod(p, &e);
        if (e == p) { fail("bad number"); return Json(); }
        p = e;
        Json j;
        j.type = Json::Num;
        j.num = v;
        return j;
    }

    Json object() {
        if (++depth > 64) { fail("too deep"); return Json(); }
        ++p;
        Json j; j.type = Json::Obj;
        skip();
        if (p < end && *p == '}') { ++p; --depth; return j; }
        while (true) {
            skip();
            if (p >= end || *p != '"') { fail("expected key"); break; }
            std::string k;
            if (!stringVal(k)) break;
            skip();
            if (p >= end || *p != ':') { fail("expected ':'"); break; }
            ++p;
            Json v = value();
            j.obj.emplace_back(std::move(k), std::move(v));
            skip();
            if (p < end && *p == ',') { ++p; continue; }
            if (p < end && *p == '}') { ++p; --depth; return j; }
            fail("expected ',' or '}'");
            break;
        }
        --depth;
        return j;
    }

    Json array() {
        if (++depth > 64) { fail("too deep"); return Json(); }
        ++p;
        Json j; j.type = Json::Arr;
        skip();
        if (p < end && *p == ']') { ++p; --depth; return j; }
        while (true) {
            Json v = value();
            j.arr.push_back(std::move(v));
            skip();
            if (p < end && *p == ',') { ++p; continue; }
            if (p < end && *p == ']') { ++p; --depth; return j; }
            fail("expected ',' or ']'");
            break;
        }
        --depth;
        return j;
    }

    Json value() {
        skip();
        if (p >= end) { fail("unexpected end"); return Json(); }
        char c = *p;
        if (c == '{') return object();
        if (c == '[') return array();
        if (c == '"') {
            Json j;
            j.type = Json::Str;
            if (!stringVal(j.str)) { j.type = Json::Null; }
            return j;
        }
        if (c == 't') { if (lit("true")) { Json j; j.type = Json::Bool; j.b = true; return j; } return Json(); }
        if (c == 'f') { if (lit("false")) { Json j; j.type = Json::Bool; j.b = false; return j; } return Json(); }
        if (c == 'n') { if (lit("null")) return Json(); return Json(); }
        return number();
    }
};

void dumpString(const std::string& s, std::string& out) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += (char)c;
                }
        }
    }
    out += '"';
}

void dumpValue(const Json& j, std::string& out) {
    switch (j.type) {
        case Json::Null: out += "null"; break;
        case Json::Bool: out += j.b ? "true" : "false"; break;
        case Json::Num:  out += numberToString(j.num); break;
        case Json::Str:  dumpString(j.str, out); break;
        case Json::Arr: {
            out += '[';
            bool first = true;
            for (const auto& v : j.arr) {
                if (!first) out += ',';
                first = false;
                dumpValue(v, out);
            }
            out += ']';
            break;
        }
        case Json::Obj: {
            out += '{';
            bool first = true;
            for (const auto& kv : j.obj) {
                if (!first) out += ',';
                first = false;
                dumpString(kv.first, out);
                out += ':';
                dumpValue(kv.second, out);
            }
            out += '}';
            break;
        }
    }
}

} // namespace

Json Json::parse(const std::string& text, std::string* err) {
    Parser ps(text.data(), text.data() + text.size());
    Json j = ps.value();
    if (!ps.err.empty()) {
        if (err) *err = ps.err;
        return Json();
    }
    if (err) err->clear();
    return j;
}

bool Json::contains(const std::string& key) const {
    if (type != Obj) return false;
    for (const auto& kv : obj) if (kv.first == key) return true;
    return false;
}

const Json& Json::at(const std::string& key) const {
    if (type == Obj) {
        for (const auto& kv : obj) if (kv.first == key) return kv.second;
    }
    static Json nullJson;
    return nullJson;
}

const Json& Json::idx(size_t i) const {
    if (type == Arr && i < arr.size()) return arr[i];
    static Json nullJson;
    return nullJson;
}

void Json::set(const std::string& key, Json v) {
    if (type != Obj) { type = Obj; }
    for (auto& kv : obj) {
        if (kv.first == key) { kv.second = std::move(v); return; }
    }
    obj.emplace_back(key, std::move(v));
}

void Json::push(Json v) {
    if (type != Arr) type = Arr;
    arr.push_back(std::move(v));
}

std::string Json::dump() const {
    std::string out;
    dumpValue(*this, out);
    return out;
}

std::string Json::asStr(const std::string& dflt) const {
    if (type == Str) return str;
    if (type == Num) return numberToString(num);
    if (type == Bool) return b ? "true" : "false";
    return dflt;
}

double Json::asNum(double dflt) const {
    if (type == Num) return num;
    if (type == Str) { char* e = nullptr; double v = strtod(str.c_str(), &e); if (e != str.c_str()) return v; }
    if (type == Bool) return b ? 1 : 0;
    return dflt;
}

int64_t Json::asInt(int64_t dflt) const {
    if (type == Num) return (int64_t)llround(num);
    if (type == Str) { char* e = nullptr; double v = strtod(str.c_str(), &e); if (e != str.c_str()) return (int64_t)llround(v); }
    return dflt;
}

bool Json::asBool(bool dflt) const {
    if (type == Bool) return b;
    if (type == Num) return num != 0;
    if (type == Str) return str == "true" || str == "1" || str == "yes";
    return dflt;
}

std::string numberToString(double v) {
    if (std::isnan(v) || std::isinf(v)) return "null";
    char buf[40];
    if (v == (double)(int64_t)v && std::fabs(v) < 1e15) {
        snprintf(buf, sizeof(buf), "%lld", (long long)v);
    } else {
        snprintf(buf, sizeof(buf), "%.17g", v);
        // 去掉多余的浮点尾数噪声：用最短可往返表示
        for (int prec = 1; prec <= 17; ++prec) {
            char tmp[40];
            snprintf(tmp, sizeof(tmp), "%.*g", prec, v);
            double back = strtod(tmp, nullptr);
            if (back == v) { return tmp; }
        }
    }
    return buf;
}

} // namespace minij
