// Json.h - 极简 JSON 解析/序列化（零依赖）
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace minij {

class Json {
public:
    enum Type { Null, Bool, Num, Str, Arr, Obj };

    Type type = Null;
    bool b = false;
    double num = 0;
    std::string str;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;   // 保序，量小用线性查找即可

    Json() = default;

    static Json object() { Json j; j.type = Obj; return j; }
    static Json array()  { Json j; j.type = Arr; return j; }
    static Json stringVal(std::string s) { Json j; j.type = Str; j.str = std::move(s); return j; }
    static Json numVal(double v)         { Json j; j.type = Num; j.num = v; return j; }
    static Json boolVal(bool v)          { Json j; j.type = Bool; j.b = v; return j; }

    static Json parse(const std::string& text, std::string* err = nullptr);

    bool isNull() const { return type == Null; }
    bool isObj()  const { return type == Obj; }
    bool isArr()  const { return type == Arr; }
    bool isStr()  const { return type == Str; }
    bool isNum()  const { return type == Num; }
    bool isBool() const { return type == Bool; }

    bool contains(const std::string& key) const;
    const Json& at(const std::string& key) const;   // 缺失返回静态 null
    const Json& idx(size_t i) const;                // 越界返回静态 null
    void set(const std::string& key, Json v);
    void push(Json v);

    std::string dump() const;

    // 类型化读取（类型不符/缺失时给默认值）
    std::string asStr(const std::string& dflt = "") const;
    double      asNum(double dflt = 0) const;
    int64_t     asInt(int64_t dflt = 0) const;
    bool        asBool(bool dflt = false) const;
};

std::string numberToString(double v);

} // namespace minij
