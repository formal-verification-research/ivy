#include <vector>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <string>
#include <fcntl.h>
#include <unistd.h>

namespace ivy {

    // Here we have the C++ representations of Ivy types. Every C++ type
    // used by Ivy provides certain traits:
    //
    // 1) A default constructor
    // 2) A constructor from (signed or unsigned) long long.
    // 3) A conversion to size_t
    // 4) A static predicate __is_seq
    // 5) A == overload
    // 6) A != overload
    // 7) A predicate __is_zero
    // 8) A class __hash of hashers
    //
    // For numeric types, the constructor from long long gives the
    // result of casting an integer to the type. For non-numeric
    // types, it gives the same result as the default
    // constructor. Conversion to size_t gives the result of casting
    // to a 64-bit integer for numeric types and zero for non-numeric
    // types. The __is_zero predicate returns true if the value
    // is the default value. The __hash method returns a hash of
    // the value of type size_t. We require that the hash of the default
    // value (i.e., 0) is always zero.
    // 
    // The predicate __is_seq gives true for unsigned integer types.
    

    // This struct implements the Ivy boolean type based on the native
    // C++ bool type. It provides the standard traits for Ivy values,
    // plus overloads for the standard boolean operations and
    // converstions to and from bool.

    struct native_bool {
        bool value;
        native_bool() : value(false) {}
        native_bool (bool value) : value(value) {}
        native_bool(long long value) : value(value) {}
        operator bool() const {
            return value;
        }
        operator std::size_t() const {
            return 0;
        }
        static bool __is_seq() {
            return false;
        }
        native_bool operator==(const native_bool &other) const {
            return native_bool(value == other.value);
        }
        native_bool operator!=(const native_bool &other) const {
            return native_bool(value != other.value);
        }
        bool __is_zero() const {
            return !value;
        }
        struct __hash {
            std::size_t operator()(const native_bool &x) const {
                return (std::size_t)(x.value);
            }
        };
        native_bool operator&(const native_bool & other) const {
            return native_bool(value && other.value);
        }
        native_bool operator|(const native_bool & other) const {
            return native_bool(value || other.value);
        }
        native_bool operator!() const {
            return native_bool(!value);
        }
    };

    // This is the basic (signed) integer type. In principle, this
    // should be implement with a multiprecision integer (e.g., GMP).
    // For now, it is just a wrapper around long long.

    struct integer {
        long long value;
        integer() {
            value = 0;
        }
        integer(long long v) {
            value = v;
        }
        integer &operator =(long long v) {
            value = v;
            return *this;
        }
        operator std::size_t() const {
            return value;
        }
        static bool __is_seq() {
            return false;
        }
        native_bool operator==(const integer &other) const {
            return native_bool(value == other.value);
        }
        native_bool operator!=(const integer &other) const {
            return native_bool(value != other.value);
        }
        bool __is_zero() const {
            return value == 0;
        }
        struct __hash {
            std::size_t operator()(const integer &x) const {
                return x.value;
            }
        };
    };

    // This is the basic (unsigned) natural type. In principle, this
    // should be implemented with a multiprecision integer (e.g., GMP).
    // For now, it is just a wrapper around unsigned long long.

    struct natural {
        unsigned long long value;
        natural() {
            value = 0;
        }
        natural(unsigned long long v) {
            value = v;
        }
        natural &operator =(unsigned long long v) {
            value = v;
            return *this;
        }
        operator std::size_t() const {
            return value;
        }
        static bool __is_seq() {
            return true;
        }
        native_bool operator==(const natural &other) const {
            return native_bool(value == other.value);
        }
        native_bool operator!=(const natural &other) const {
            return native_bool(value != other.value);
        }
        bool __is_zero() const {
            return value == 0;
        }
        struct __hash {
            std::size_t operator()(const natural &x) const {
                return x.value;
            }
        };
    };

    // This template represents a pointer to an object that may be
    // null. When the pointer is copied, the object is copied.

    template <class T> struct ptr_null {
        T *p;
        ptr_null() {p = 0;}
        ptr_null(T *p) : p(p) {}
        ptr_null(const ptr_null &x) {
            if (x.p) {
                p = new T(*(x.p));
            } else {
                p = 0;
            }
        }
        ptr_null & operator= (const ptr_null &x) {
            if (x.p) {
                p = new T(*(x.p));
            } else {
                p = 0;
            }
            return *this;
        }
        operator bool () const {
            return p;
        }
        T& operator* () const {
            return *p;
        }
        T* operator-> () const {
            return p;
        }
    };

    // This is a variadic class template for representing Ivy
    // functions. In addition to the standard traits, it provides:
    //
    // 1) const r operator() (d1,...,dn)
    // 2) r operator() (d1,...,dn)
    // 3) a static function resize(vector x, size_t size)
    //
    // where r is the range type of the function and d1,...,dn are
    // the domain types. The first is for use of the function as
    // an rvalue and the second is for use as an lvalue.
    //
    // For domain types that are sequences, this class dynamically
    // determines whether to use a dense (vector) or sparse
    // (unordered_map) representation. For purely dense
    // representation, the overhead of a lookup is just the array
    // bounds check.
    //
    // The resize operator converts to a vector representation of the
    // given size. The value of the resulting function for inputs
    // greater than size becomes the default value of the range type.
    // That is, of `f` is a function over a sequence type,
    // `resize(f,size)` is equivalent to:
    //
    //     lambda x. f(x) if x < size else f(x)
    //
    // and has the side effect of converting the representation to a
    // pure vector. If `f` is a function over a non-sequence type, the
    // result is `lambda x. 0`.
    //
    // The storage cost for this type is the cost of std::vector (usually
    // a pointer plus a size_t) plus one pointer.
    //
    // The template represents a function `f` with a vector `data` and
    // an optional unordered map `map`. To evaluate `f(x)`, when `x`
    // is a sequence type, we first consult `data`. If `x` is in the
    // range `[0..data.size())` we return `data[x]`. Failing this, we
    // return `map[x]` (creating `map` if needed).  As an exception ,
    // however, if `x = data.size()`, we increase the size of the
    // `data` by one to accomodate the new value. This allows the
    // vector to grow, provided values are appended sequentially. The
    // `resize` function produces a pure vector of the given size, even
    // if argument function is represented sparsely. 
    //
    // Functions of more than one argument are represented by currying.
    
    
    template <class T, class ... RestD> struct vector;
    
    // This specialization represents the base case: a function of
    // one argument.

    template<class T, class PrimaryD > struct vector<T, PrimaryD>  {
        typedef std::vector<T> type;
        type data;
        typedef std::unordered_map <PrimaryD,T,typename PrimaryD:: __hash> map_type;
        ptr_null< map_type  > map;

        vector () {
        }

        vector(long long v) {
        }

        operator std::size_t() const {
            return 0;
        }

        static bool __is_seq() {
            return false;
        }

        bool __value_eq(const PrimaryD &idx, const T &v) const {
            if (PrimaryD::__is_seq() && ((std::size_t)idx) < data.size()) {
                return data[((std::size_t)idx)] == v;
            }
            if (map) {
                auto it = map->find(idx);
                if (it != map->end())
                    return it->second == v;
            }
            return v.__is_zero();
        }
        
        native_bool operator==(const vector &other) const {
            if (PrimaryD::__is_seq()) {
                for (std::size_t idx = 0; idx < data.size(); ++idx) {
                    if (!other.__value_eq(idx,data[idx])) return false;
                }
                for (std::size_t idx = data.size(); idx < other.data.size(); ++idx) {
                    if (!__value_eq(idx,other.data[idx])) return false;
                }
            }
            if (map) {
                for (auto it = map->begin(); it != map->end(); ++it) {
                    const PrimaryD &idx = it->first;
                    if (!PrimaryD::__is_seq() || ((std::size_t)idx) >= data.size())
                        if (!other.__value_eq(((std::size_t)idx),it->second)) return false;
                }
            }
            if (other.map) {
                for (auto it = other.map->begin(); it != other.map->end(); ++it) {
                    const PrimaryD &idx = it->first;
                    if (!PrimaryD::__is_seq() || ((std::size_t)idx) >= other.data.size())
                        if (!__value_eq(idx,it->second)) return false;
                }
            }
            return true;
        }

        native_bool operator!=(const vector &other) const {
            return !((*this) == other);
        }

        bool __is_zero() const {
            if (PrimaryD::__is_seq()) {
                for (std::size_t idx = 0; idx < data.size(); ++idx) {
                    if (!data[idx].__is_zero()) return false;
                }
            }
            if (map) {
                for (auto it = map->begin(); it != map->end(); ++it) {
                    const PrimaryD &idx = it->first;
                    if (!PrimaryD::__is_seq() || ((std::size_t)idx) >= data.size())
                        if (!it->second.__is_zero()) return false;
                }
            }                        
            return true;
        }


        struct __hash {
            std::size_t operator()(const vector &x) const {
                std::size_t res = 0;
                if (PrimaryD::__is_seq()) {
                    for (std::size_t idx = 0; idx < x.data.size(); ++idx) {
                        typename T::__hash h;
                        res += h(x.data[idx]);
                    }
                }
                if (x.map) {
                    for (auto it = x.map->begin(); it != x.map->end(); ++it) {
                        const PrimaryD &idx = it->first;
                        if (!PrimaryD::__is_seq() || ((std::size_t)idx) >= x.data.size()) {
                            typename T::__hash h;
                            res += h(it->second);
                        }
                    }
                }                    
                return res;
            }
        };
        
        static T zero;  // apologies to Calvino

        const T& operator() (PrimaryD idx) const {
            if (PrimaryD::__is_seq()) {
                if (((std::size_t)idx) < data.size())
                    return data[((std::size_t)idx)];
            }
            if (map) {
                typename map_type::const_iterator it = map->find(idx);
                if (it != map->end()) {
                    return it->second;
                }
            }
            return zero;
        }
        
        T& operator() (PrimaryD idx) {
            if (PrimaryD::__is_seq()) {
                if (((std::size_t)idx) < data.size())
                    return data[((std::size_t)idx)];
                else if (((std::size_t)idx) == data.size()) {
                    data.resize(((std::size_t)idx)+1);
                    return data[((std::size_t)idx)];
                }
            }
            if (!map) {
                map = ptr_null< map_type  > (new map_type);
            }
            return (*map)[idx];
        }

        static void resize(vector &x, std::size_t size) {
            if (PrimaryD::__is_seq()) {
                std::size_t old_size = x.data.size();
                x.data.resize(size);
                if (x.map) {
                    for(typename map_type::const_iterator it = x.map->begin(),en = x.map->end(); it != en; ++it) {
                        std::size_t idx = (std::size_t)(it->first);
                        if (old_size <= idx && idx < size)
                            x.data[idx] = it->second;
                    }
                }
            }
        }

    };

    template<class T, class PrimaryD > T vector<T, PrimaryD>::zero;
    

    // This specialization represents the recursive case: a function of more than
    // one argument. 

    template<class T, class PrimaryD, class ... RestD > struct vector<T, PrimaryD, RestD...> {
        typedef PrimaryD index_type;
        typedef typename vector<T, RestD...>::type OneDimensionDownVectorT;
        typedef vector<OneDimensionDownVectorT,PrimaryD> type;
        type data;

        vector () {
        }

        vector(long long v) {
        }

        operator std::size_t() const {
            return 0;
        }

        static bool __is_seq() {
            return false;
        }

        native_bool operator==(const vector &other) const {
            return data == other.data;
        }

        native_bool operator!=(const vector &other) const {
            return data != other.data;
        }

        bool __is_zero() const {
            return data.__is_zero();
        }

        struct __hash {
            std::size_t operator()(const vector &x) const {
                return type::__hash(x.data);
            }
        };

        const T& operator() (PrimaryD idx, RestD... parameters) const {
            return data(idx)(parameters...);
        }

        T& operator() (PrimaryD idx, RestD... parameters) {
            return data(idx)(parameters...);
        }

        static void resize(vector &x, std::size_t size) {
            x.data.resize(size);
        }
    };

    // This template implements signed numeric types based on native C++
    // numeric types. It provides the standard traits for Ivy values,
    // plus overloads for the standard arithmetic operations.

    template <typename T> struct native_int {
        T value;
        native_int() : value(0) {}
        native_int(long long value) : value(value) {}
        operator std::size_t() const {
            return value;
        }
        static bool __is_seq() {
            return true;
        }
        native_bool operator==(const native_int &other) const {
            return native_bool(value == other.value);
        }
        native_bool operator!=(const native_int &other) const {
            return native_bool(value != other.value);
        }
        bool __is_zero() const {
            return value == 0;
        }
        struct __hash {
            std::size_t operator()(const native_int &x) const {
                return x.value;
            }
        };
        native_int operator+(const native_int & other) const {
            return native_int(value + other.value);
        }
        native_int operator-(const native_int & other) const {
            return native_int(value - other.value);
        }
        native_int operator*(const native_int & other) const {
            return native_int(value * other.value);
        }
        native_int operator/(const native_int & other) const {
            return native_int(value / other.value);
        }
        native_bool operator<(const native_int & other) const {
            return native_bool(value < other.value);
        }
        native_bool operator<=(const native_int & other) const {
            return native_bool(value <= other.value);
        }
        native_bool operator>(const native_int & other) const {
            return native_bool(value > other.value);
        }
        native_bool operator>=(const native_int & other) const {
            return native_bool(value >= other.value);
        }
    };


    // This template implements unsigned numeric types based on native C++
    // numeric types. The unsigned versions use saturation arithmetic, so
    // `0 - 1 = 0`. It provides the standard traits for Ivy values,
    // plus overloads for the standard arithmetic operations.

    template <typename T> struct native_unsigned {
        T value;
        native_unsigned() : value(0) {}
        native_unsigned(long long value) : value(value) {}
        operator std::size_t() const {
            return value;
        }
        static bool __is_seq() {
            return true;
        }
        native_bool operator==(const native_unsigned &other) const {
            return native_bool(value == other.value);
        }
        native_bool operator!=(const native_unsigned &other) const {
            return native_bool(value != other.value);
        }
        bool __is_zero() const {
            return value == 0;
        }
        struct __hash {
            std::size_t operator()(const native_unsigned &x) const {
                return x.value;
            }
        };
        native_unsigned operator+(const native_unsigned & other) const {
            return native_unsigned(value + other.value);
        }
        native_unsigned operator-(const native_unsigned & other) const {
            return native_unsigned((other.value > value) ? 0 : (value - other.value));
        }
        native_unsigned operator*(const native_unsigned & other) const {
            return native_unsigned(value * other.value);
        }
        native_unsigned operator/(const native_unsigned & other) const {
            return native_unsigned(value / other.value);
        }
        native_bool operator<(const native_unsigned & other) const {
            return native_bool(value < other.value);
        }
        native_bool operator<=(const native_unsigned & other) const {
            return native_bool(value <= other.value);
        }
        native_bool operator>(const native_unsigned & other) const {
            return native_bool(value > other.value);
        }
        native_bool operator>=(const native_unsigned & other) const {
            return native_bool(value >= other.value);
        }
    };


    // This template implements enumerated types based on native C++
    // enum types. It provides the standard traits for Ivy values.

    template <typename T> struct native_enum {
        T value;
        native_enum() : value((T)0) {}
        native_enum(long long value) : value((T)value) {}
        native_enum(T value) : value(value) {}
        operator std::size_t() const {
            return (std::size_t)value;
        }
        static bool __is_seq() {
            return true;
        }
        native_bool operator==(const native_enum &other) const {
            return value == other.value;
        }
        native_bool operator!=(const native_enum &other) const {
            return value != other.value;
        }
        bool __is_zero() const {
            return value == (T)0;
        }
        struct __hash {
            std::size_t operator()(const native_enum &x) const {
                return (std::size_t)x.value;
            }
        };
    };

    // This class template implements a copy-on-write pointer.
    //
    // Instances of class `ptr<T>` contain a pointer to a wrapper
    // object that in turn contains a reference count, and an object
    // of some derived class `S` of `T`. When the pointer is copied,
    // the reference count is increased. When it is deleted, the
    // reference count is decreased. The operator `->` returns a
    // pointer to the contained object of type `S`. If a non-const
    // pointer is reqruied, the pointer is first replaces by a pointer
    // to a copy of the wrapper, whose reference count is one, while
    // the reference count of the original wrapper is decremented. In
    // this way. we guarantee that the an object is modified through
    // a pointer, there is only one reference to that object.
    //
    // Copy-on-write pointers are one mechanism used by the Ivy
    // compiler to avoid deep copying, as the overhead of copying
    // a C-O-W pointer is just incrementing the reference count.
    //
    // TODO: The implementation is sub-optimal in that it used a
    // virtual function call to implement dereferencing, when simply
    // adding a constant to the pointer to offset into the wrapper
    // would be sufficient. Also, this and other Ivy classes should
    // have move constructors that would allow the compiler in some
    // cases to avoid the reference counting overhead.
    

    template <class T> struct ptr {

    // This is the base class for reference-counting wrappers.

        struct wrap {
            unsigned refs;
            wrap() : refs(1) {}
            wrap *dup() {refs++; return this;}
            bool deref() {return (--refs) != 0;}
            virtual wrap *clone() = 0;
            virtual const T *get() const = 0;
            virtual T *get() = 0;
            virtual std::size_t __hash() const = 0;
            virtual bool eq(const wrap &) const = 0;
            virtual ~wrap() {}
        };

        template <typename S> struct twrap : public wrap {

            S item;

            twrap() {}

            twrap(const S &item) : item(item) {}

            virtual wrap *clone() {return new twrap(item);}

            virtual const T *get() const {return &item;} 

            virtual T *get() {return &item;} 

            virtual std::size_t __hash() const {
                return typename S::__hash()(item);
            }

            virtual bool eq(const wrap &other) const {
                const twrap *p = dynamic_cast<const twrap *>(&other);
                return p && (item == p->item);
            }
        };

        wrap *p;

        ptr(){
            p = new twrap<T>();
        }

        //        ptr(wrap *p) : p(p) {}

        template <class S> ptr(const S& x) {
                p = new twrap<S>(x);
        }

        ptr(std::size_t) {
        }

        ptr(const ptr &other){
            p = other.p->dup();
        };

        ptr& operator=(const ptr &other){
            if (!p->deref()) delete p;
            p = other.p->dup();
            return *this;
        };

        const T* operator-> () const {
            return p->get();
        }

        T* get () {
            if (p->refs <= 1) {
                return p->get();
            } else {
                p->deref();
                p = p->clone();
                return p->get();
            }   
        }

        ~ptr() {
            if (!p->deref()) delete p;
        }

        operator std::size_t() const {
            return 0;
        }
        static bool __is_seq() {
            return false;
        }
        native_bool operator==(const ptr &other) const {
            return p->eq(*other.p);
        }
        native_bool operator!=(const ptr &other) const {
            return !((*this) == other);
        }
        bool __is_zero() const {
            const twrap<T> *q = dynamic_cast<const twrap<T> *>(p);
            return q && q->item.__is_zero();
        }
        struct __hash {
            std::size_t operator()(const ptr &x) const {
                return x.p->__hash();
            }
        };

        template <class S> native_bool isa() const {
            return dynamic_cast<const twrap<S> *>(p);
        }
    };

    template <class T> const T* to_ptr (const T &x) {
        return &x;
    }
    
    template <class T,class S> native_bool isa(const S& x) {
        return dynamic_cast<const T *>(&x);
    }

    /* template<typename T> T __num(long long x) { */
    /*     T res; */
    /*     res.value = x; */
    /*     return res; */
    /* } */


    // Put a character on standard out.

    static inline void put(int c) {
        std::cout.put(c);
    }

    // Get a character from standard in.

    static inline int get() {
        return std::cin.get();
    }

    // Get an environment variable

    template <class T> static inline T getenv(const T &name) {
        std::string sname;
        for (std::size_t idx = 0; idx < ((std::size_t) name.end); idx ++) {
            sname.push_back(name.value(idx));
        }
        const char *cres = ::getenv(sname.c_str());
        T res;
        if (cres) {
            for (; *cres; ++cres) {
                res.append(*cres);
            }
        }
        return res;
    }

    template <class N, class D> static inline void read_file(const N &name, D &data, __bool &ok) {
        std::string sname;
        for (std::size_t idx = 0; idx < ((std::size_t) name.end); idx ++) {
            sname.push_back(name.value(idx));
        }
        int fd = ::open(sname.c_str(),0);
        if (fd < 0) {
            ok = false;
        } else {
            std::vector<char> tmp;
            tmp.resize(2048);
            int len;
            while ((len = ::read(fd,&tmp[0],2048)) > 0) {
                for (std::size_t i = 0; i < len; i++)
                    data.append(tmp[i]);
            }
            ok = (len >= 0);
            ::close(fd);
        }
    }
    
    template <class N, class D> static inline bool write_file(const N &name, const D &data) {
        bool ok;
        std::string sname;
        for (std::size_t idx = 0; idx < ((std::size_t) name.end); idx ++) {
            sname.push_back(name.value(idx));
        }
        int fd = ::creat(sname.c_str(),0660);
        if (fd < 0) {
            ok = false;
        } else {
            std::vector<char> tmp;
            tmp.resize(data.end);
            for (std::size_t i = 0; i < tmp.size(); i++) {
                tmp[i] = data.value(i);
            }
            std::size_t bytes = ::write(fd,&tmp[0],tmp.size());
            ok = (bytes == tmp.size());
            ::close(fd);
        }
        return ok;
    }

    template <class N> static inline bool file_exists(const N &name) {
        std::string sname;
        for (std::size_t idx = 0; idx < ((std::size_t) name.end); idx ++) {
            sname.push_back(name.value(idx));
        }
        return ::access(sname.c_str(),F_OK) != -1;
    }

    // Resize a function.

    template <class T> static inline void resize(T &f, unsigned size) {
        T::resize(f,size);
    }

    // This template is used by the compiler to build string literals
    // of any type with string traits (that is, having array traits
    // and a numeric domain type).

    template <class T> static inline T from_str(const char *s) {
        T x;
        for (const char *p = s; *p; ++p) {
            x.append(*p);
        }
        return x;
    }

}

