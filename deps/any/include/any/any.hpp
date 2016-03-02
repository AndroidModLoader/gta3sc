///
/// Implementation of N4562 std::experimental::any plus a proposed fix on LWG Defect 2509.
///
/// See also:
///   + http://en.cppreference.com/w/cpp/experimental/any
///   + http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4562.html#any
///   + https://cplusplus.github.io/LWG/lwg-active.html#2509
///
///
///
/// This is free and unencumbered software released into the public domain.
/// 
/// Anyone is free to copy, modify, publish, use, compile, sell, or
/// distribute this software, either in source code form or as a compiled
/// binary, for any purpose, commercial or non - commercial, and by any
/// means.
/// 
/// In jurisdictions that recognize copyright laws, the author or authors
/// of this software dedicate any and all copyright interest in the
/// software to the public domain.We make this dedication for the benefit
/// of the public at large and to the detriment of our heirs and
/// successors.We intend this dedication to be an overt act of
/// relinquishment in perpetuity of all present and future rights to this
/// software under copyright law.
/// 
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
/// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
/// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
/// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
/// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/// OTHER DEALINGS IN THE SOFTWARE.
/// 
/// For more information, please refer to <http://unlicense.org/>
///
#include <typeinfo>
#include <type_traits>
#include <stdexcept>

namespace linb
{

class bad_any_cast : public std::bad_cast
{
public:
    const char* what() const noexcept override
    {
        return "bad any cast";
    }
};

class any final
{
public:
    /// Constructs an object of type any with an empty state.
    any() :
        vtable(nullptr)
    {
    }

    /// Constructs an object of type any with an equivalent state as other.
    any(const any& rhs) :
        vtable(rhs.vtable)
    {
        if(!rhs.empty())
        {
            rhs.vtable->copy(rhs.storage, this->storage);
        }
    }

    /// Constructs an object of type any with a state equivalent to the original state of other.
    /// rhs is left in a valid but otherwise unspecified state.
    any(any&& rhs) noexcept :
    vtable(rhs.vtable)
    {
        if(!rhs.empty())
        {
            rhs.vtable->move(rhs.storage, this->storage);
            rhs.vtable = nullptr;
        }
    }

    /// Same effect as this->clear().
    ~any()
    {
        this->clear();
    }

    /// Constructs an object of type any that contains an object of type T direct-initialized with std::forward<ValueType>(value).
    ///
    /// T shall satisfy the CopyConstructible requirements, except for the requirements for MoveConstructible.
    /// (this is unlike N4562 specifies, see LWG Defect 2509).
    template<typename ValueType, typename = std::enable_if_t<!std::is_same<std::decay_t<ValueType>, any>::value>>
    any(ValueType&& value)
    {
        this->construct(std::forward<ValueType>(value));
    }

    /// Has the same effect as any(rhs).swap(*this). No effects if an exception is thrown.
    any& operator=(const any& rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    /// Has the same effect as any(std::move(rhs)).swap(*this).
    ///
    /// The state of *this is equivalent to the original state of rhs and rhs is left in a valid
    /// but otherwise unspecified state.
    any& operator=(any&& rhs) noexcept
    {
        any(std::move(rhs)).swap(*this);
        return *this;
    }

    /// Has the same effect as any(std::forward<ValueType>(value)).swap(*this). No effect if a exception is thrown.
    ///
    /// T shall satisfy the CopyConstructible requirements, except for the requirements for MoveConstructible.
    /// (this is unlike N4562 specifies, see LWG Defect 2509).
    template<typename ValueType, typename = std::enable_if_t<!std::is_same<std::decay_t<ValueType>, any>::value>>
    any& operator=(ValueType&& value)
    {
        any(std::forward<ValueType>(value)).swap(*this);
        return *this;
    }

    /// If not empty, destroys the contained object.
    void clear() noexcept
    {
        if(!empty())
        {
            this->vtable->destroy(storage);
            this->vtable = nullptr;
        }
    }

    /// Returns true if *this has no contained object, otherwise false.
    bool empty() const noexcept
    {
        return this->vtable == nullptr;
    }

    /// If *this has a contained object of type T, typeid(T); otherwise typeid(void).
    const std::type_info& type() const noexcept
    {
        return empty()? typeid(void) : this->vtable->type();
    }

    /// Exchange the states of *this and rhs.
    void swap(any& rhs) noexcept
    {
        if(this->vtable != rhs.vtable)
        {
            any tmp(std::move(rhs));

            // move from *this to rhs.
            rhs.vtable = this->vtable;
            if(this->vtable != nullptr)
            {
                this->vtable->move(this->storage, rhs.storage);
                //this->vtable = nullptr; -- uneeded, see below
            }

            // move from tmp (previously rhs) to *this.
            this->vtable = tmp.vtable;
            if(tmp.vtable != nullptr)
            {
                tmp.vtable->move(tmp.storage, this->storage);
                tmp.vtable = nullptr;
            }
        }
        else // same types
        {
            if(this->vtable != nullptr)
                this->vtable->swap(this->storage, rhs.storage);
        }
    }

private: // Storage and Virtual Method Table

    union storage_union
    {
        void*                                                       dynamic;
        std::aligned_storage_t<2 * sizeof(void*), alignof(void*)>   stack;      // 2 words for e.g. shared_ptr
    };

    /// Base VTable specification.
    struct vtable_type
    {
        // Note: The caller is responssible for doing .vtable = nullptr after destructful operations
        // such as destroy() and/or move().

        /// The type of the object this vtable is for.
        const std::type_info& (*type)() noexcept;

        /// Destroys the object in the union.
        /// The state of the union after this call is unspecified, caller must ensure not to use src anymore.
        void(*destroy)(storage_union&) noexcept;

        /// Copies the **inner** content of the src union into the yet unitialized dest union.
        /// As such, both inner objects will have the same state, but on separate memory locations.
        void(*copy)(const storage_union& src, storage_union& dest);

        /// Moves the storage from src to the yet unitialized dest union.
        /// The state of src after this call is unspecified, caller must ensure not to use src anymore.
        void(*move)(storage_union& src, storage_union& dest) noexcept;

        /// Exchanges the storage between lhs and rhs.
        void(*swap)(storage_union& lhs, storage_union& rhs) noexcept;
    };

    /// VTable for dynamically allocated storage.
    template<typename T>
    struct vtable_dynamic
    {
        static const std::type_info& type() noexcept
        {
            return typeid(T);
        }

        static void destroy(storage_union& storage) noexcept
        {
            //assert(reinterpret_cast<T*>(storage.dynamic));
            delete reinterpret_cast<T*>(storage.dynamic);
        }

        static void copy(const storage_union& src, storage_union& dest)
        {
            dest.dynamic = new T(*reinterpret_cast<const T*>(src.dynamic));
        }

        static void move(storage_union& src, storage_union& dest) noexcept
        {
            dest.dynamic = src.dynamic;
            src.dynamic = nullptr;
        }

        static void swap(storage_union& lhs, storage_union& rhs) noexcept
        {
            // just exchage the storage pointers.
            std::swap(lhs.dynamic, rhs.dynamic);
        }
    };

    /// VTable for stack allocated storage.
    template<typename T>
    struct vtable_stack
    {
        static const std::type_info& type() noexcept
        {
            return typeid(T);
        }

        static void destroy(storage_union& storage) noexcept
        {
            reinterpret_cast<T*>(&storage.stack)->~T();
        }

        static void copy(const storage_union& src, storage_union& dest)
        {
            new (&dest.stack) T(reinterpret_cast<const T&>(src.stack));
        }

        static void move(storage_union& src, storage_union& dest) noexcept
        {
            // one of the conditions for using vtable_stack is a nothrow move constructor,
            // so this move constructor will never throw a exception.
            new (&dest.stack) T(std::move(reinterpret_cast<T&>(src.stack)));
            destroy(src);
        }

        static void swap(storage_union& lhs, storage_union& rhs) noexcept
        {
            std::swap(reinterpret_cast<T&>(lhs.stack), reinterpret_cast<T&>(rhs.stack));
        }
    };

    /// Whether the type T must be dynamically allocated or can be stored on the stack.
    template<typename T>
    static constexpr bool requires_allocation()
    {
        return !(std::is_nothrow_move_constructible<T>::value      // N4562 §6.3/3 [any.class]
            && sizeof(T) <= sizeof(storage_union::stack)
            && alignof(T) <= alignof(decltype(storage_union::stack)));
    }

    /// Returns the pointer to the vtable of the type T.
    template<typename T>
    static vtable_type* vtable_for_type()
    {
        using VTableType = std::conditional_t<requires_allocation<T>(), vtable_dynamic<T>, vtable_stack<T>>;
        static vtable_type table = {
            VTableType::type, VTableType::destroy,
            VTableType::copy, VTableType::move,
            VTableType::swap,
        };
        return &table;
    }

protected:
    template<typename T>
    friend const T* any_cast(const any* operand) noexcept;
    template<typename T>
    friend T* any_cast(any* operand) noexcept;

    /// Same effect as is_same(this->type(), t);
    bool is_typed(const std::type_info& t) const
    {
        return is_same(this->type(), t);
    }

    /// Checks if two type infos are the same.
    ///
    /// If ANY_IMPL_FAST_TYPE_INFO_COMPARE is defined, checks only the address of the
    /// type infos, otherwise does an actual comparision. Checking addresses is
    /// only a valid approach when there's no interaction with outside sources
    /// (other shared libraries and such).
    static bool is_same(const std::type_info& a, const std::type_info& b)
    {
#ifdef ANY_IMPL_FAST_TYPE_INFO_COMPARE
        return &a == &b;
#else
        return a == b;
#endif
    }

    /// Casts (with no type_info checks) the storage pointer as const T*.
    template<typename T>
    const T* cast() const noexcept
    {
        return requires_allocation<T>()?
            reinterpret_cast<const T*>(storage.dynamic) :
            reinterpret_cast<const T*>(&storage.stack);
    }

    /// Casts (with no type_info checks) the storage pointer as T*.
    template<typename T>
    T* cast() noexcept
    {
        return requires_allocation<T>()?
            reinterpret_cast<T*>(storage.dynamic) :
            reinterpret_cast<T*>(&storage.stack);
    }

private:
    storage_union storage; // on offset(0) so no padding for align
    vtable_type*  vtable;

    /// Chooses between stack and dynamic allocation for the type decay_t<ValueType>,
    /// assigns the correct vtable, and constructs the object on our storage.
    template<typename ValueType>
    void construct(ValueType&& value)
    {
        using T = std::decay_t<ValueType>;

        this->vtable = vtable_for_type<T>();

        if(requires_allocation<T>())
            storage.dynamic = new T(std::forward<ValueType>(value));
        else
            new (&storage.stack) T(std::forward<ValueType>(value));
    }
};

namespace detail
{
    template<typename ValueType>
    inline ValueType any_cast_move_if_true(std::remove_reference_t<ValueType>* p, std::true_type)
    {
        return std::move(*p);
    }

    template<typename ValueType>
    inline ValueType any_cast_move_if_true(std::remove_reference_t<ValueType>* p, std::false_type)
    {
        return *p;
    }
}

/// Performs *any_cast<add_const_t<remove_reference_t<ValueType>>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(const any& operand)
{
    auto p = any_cast<std::add_const_t<std::remove_reference_t<ValueType>>>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return *p;
}

/// Performs *any_cast<remove_reference_t<ValueType>>(&operand), or throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(any& operand)
{
    auto p = any_cast<std::remove_reference_t<ValueType>>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return *p;
}

/// If ValueType is MoveConstructible and isn't a lvalue reference, performs
/// std::move(*any_cast<remove_reference_t<ValueType>>(&operand)), otherwise
/// *any_cast<remove_reference_t<ValueType>>(&operand). Throws bad_any_cast on failure.
template<typename ValueType>
inline ValueType any_cast(any&& operand)
{
    // https://cplusplus.github.io/LWG/lwg-active.html#2509

    using can_move = std::integral_constant<bool,
        std::is_move_constructible<ValueType>::value
        && !std::is_lvalue_reference<ValueType>::value>;

    auto p = any_cast<std::remove_reference_t<ValueType>>(&operand);
    if(p == nullptr) throw bad_any_cast();
    return detail::any_cast_move_if_true<ValueType>(p, can_move());
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename T>
inline const T* any_cast(const any* operand) noexcept
{
    if(operand == nullptr || !operand->is_typed(typeid(T)))
        return nullptr;
    else
        return operand->cast<T>();
}

/// If operand != nullptr && operand->type() == typeid(ValueType), a pointer to the object
/// contained by operand, otherwise nullptr.
template<typename T>
inline T* any_cast(any* operand) noexcept
{
    if(operand == nullptr || !operand->is_typed(typeid(T)))
        return nullptr;
    else
        return operand->cast<T>();
}

}

namespace std
{
    inline void swap(linb::any& lhs, linb::any& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}
