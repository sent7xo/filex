// =====================================
//           SAYANA'S DEFER
// =====================================
#ifndef SYN_DEFER_H
#define SYN_DEFER_H

template <typename F>
struct _Defer {
    F f;
    _Defer(F f) : f(f) {}
    ~_Defer() { f(); }
};

struct _Defer2 {
    template <typename F>
    _Defer<F> operator-(F f) {
        return _Defer<F>(f);
    }
};

#define DEFER_0(x, y) x##y
#define DEFER_1(x, y) DEFER_0(x, y)
#define defer auto DEFER_1(_defer_, __COUNTER__) = _Defer2()-[&]()
// #define defer(code) auto DEFER_1(_defer_, __COUNTER__) = _Defer2()-[&](){code;}
#endif
