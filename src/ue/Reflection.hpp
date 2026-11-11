#pragma once
// Runtime reflection helpers on top of the UEVR API.
//
// Everything is resolved by name at runtime (property offsets, functions, parameter
// layouts), so the plugin keeps working when a game update moves things around in
// memory. Lookups are cached per class. Every access is type-checked once against the
// reflected property type: on a mismatch (or a missing property/function) an error is
// logged once to UEVR's log.txt and the access is skipped instead of corrupting memory.
//
//   auto mesh  = ue::get<UObject*>(actor, L"Mesh");
//   ue::set(component, L"bHiddenInGame", true);               // bitfield-safe for bools
//   auto child = ue::call<UObject*>(component, L"GetChildComponent", 0);
//   ue::call(component, L"K2_SetRelativeLocation", location, false, nullptr, false);
//   auto rot   = ue::call_static<FRotator>(ue::lib::KismetMath, L"MakeRotFromX", forward);
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "uevr/API.hpp"
#include "ue/Types.hpp"
#include "ue/Enums.hpp"
#include "ue/WeakObject.hpp"

namespace ue {

class StructRef;

using UObject = uevr::API::UObject;
using UClass = uevr::API::UClass;
using UStruct = uevr::API::UStruct;
using UFunction = uevr::API::UFunction;
using UScriptStruct = uevr::API::UScriptStruct;
using FProperty = uevr::API::FProperty;
template <typename T> using TArray = uevr::API::TArray<T>;

// ---------------------------------------------------------------------------
// Object and class lookup
// ---------------------------------------------------------------------------

// Full names look like L"Class /Script/Engine.Actor"
UObject* find_object(std::wstring_view full_name);           // silent if missing
UClass* find_class(std::wstring_view full_name);             // logs once if missing
UScriptStruct* find_struct(std::wstring_view full_name);     // logs once if missing
UObject* default_object(UClass* cls);                        // class default object (CDO)

// Class resolved on first use and cached once found (Blueprint classes load late).
// Looked up again if the class gets unloaded.
class ClassRef {
public:
    constexpr explicit ClassRef(const wchar_t* full_name) : m_full_name(full_name) {}
    UClass* get() const;
    operator UClass*() const { return get(); }
    const wchar_t* name() const { return m_full_name; }

private:
    const wchar_t* m_full_name;
    mutable WeakObject m_cached{};
};

// GameEngine->GameViewport->World
UObject* get_world();

// Null-safe UObject::is_a
inline bool is_a(const UObject* obj, UClass* cls) { return obj != nullptr && cls != nullptr && obj->is_a(cls); }

// Engine function libraries, called through their class default object with call_static
namespace lib {
inline constinit ClassRef KismetMath{ L"Class /Script/Engine.KismetMathLibrary" };
inline constinit ClassRef KismetSystem{ L"Class /Script/Engine.KismetSystemLibrary" };
inline constinit ClassRef KismetString{ L"Class /Script/Engine.KismetStringLibrary" };
inline constinit ClassRef KismetRendering{ L"Class /Script/Engine.KismetRenderingLibrary" };
inline constinit ClassRef GameplayStatics{ L"Class /Script/Engine.GameplayStatics" };
}

// ---------------------------------------------------------------------------
// Type descriptions (implementation details used by the templates below)
// ---------------------------------------------------------------------------
namespace detail {

// What reflected property type a C++ type must match
struct TypeDesc {
    const wchar_t* field_classes[3]{}; // accepted FFieldClass names, e.g. L"DoubleProperty"
    const wchar_t* struct_name{};      // for StructProperty: L"Vector", L"Rotator", ...
    const char* cpp_name{};            // for error messages
};

template <typename T> struct is_value_struct : std::false_type {};
template <> struct is_value_struct<FVector> : std::true_type {};
template <> struct is_value_struct<FVector2D> : std::true_type {};
template <> struct is_value_struct<FRotator> : std::true_type {};
template <> struct is_value_struct<FQuat> : std::true_type {};
template <> struct is_value_struct<FTransform> : std::true_type {};
template <> struct is_value_struct<FLinearColor> : std::true_type {};
template <> struct is_value_struct<FHitResult> : std::true_type {};
template <> struct is_value_struct<FName> : std::true_type {};
template <> struct is_value_struct<FStringArg> : std::true_type {};
template <> struct is_value_struct<StructRef> : std::true_type {};

template <typename T> struct is_tarray : std::false_type {};
template <typename T> struct is_tarray<TArray<T>> : std::true_type {};

// Value types we know how to check (anything else passed by pointer is an object pointer)
template <typename T>
inline constexpr bool is_value_type_v = std::is_arithmetic_v<T> || std::is_enum_v<T> ||
                                        is_value_struct<T>::value || is_tarray<T>::value;

// Pointer to a non-value type = UObject pointer (uevr::API::UObject and subclasses)
template <typename T>
inline constexpr bool is_object_ptr_v = std::is_pointer_v<T> && !is_value_type_v<std::remove_cv_t<std::remove_pointer_t<T>>>;

template <typename T>
constexpr TypeDesc type_desc() {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, bool>) return { { L"BoolProperty" }, nullptr, "bool" };
    else if constexpr (std::is_enum_v<U>) return { { L"ByteProperty", L"EnumProperty" }, nullptr, "enum" };
    else if constexpr (std::is_same_v<U, int8_t>) return { { L"Int8Property" }, nullptr, "int8" };
    else if constexpr (std::is_same_v<U, uint8_t>) return { { L"ByteProperty" }, nullptr, "uint8" };
    else if constexpr (std::is_same_v<U, int16_t>) return { { L"Int16Property" }, nullptr, "int16" };
    else if constexpr (std::is_same_v<U, uint16_t>) return { { L"UInt16Property" }, nullptr, "uint16" };
    else if constexpr (std::is_same_v<U, int32_t>) return { { L"IntProperty" }, nullptr, "int32" };
    else if constexpr (std::is_same_v<U, uint32_t>) return { { L"UInt32Property" }, nullptr, "uint32" };
    else if constexpr (std::is_same_v<U, int64_t>) return { { L"Int64Property" }, nullptr, "int64" };
    else if constexpr (std::is_same_v<U, uint64_t>) return { { L"UInt64Property" }, nullptr, "uint64" };
    else if constexpr (std::is_same_v<U, float>) return { { L"FloatProperty" }, nullptr, "float" };
    else if constexpr (std::is_same_v<U, double>) return { { L"DoubleProperty" }, nullptr, "double" };
    else if constexpr (std::is_same_v<U, FName>) return { { L"NameProperty" }, nullptr, "FName" };
    else if constexpr (std::is_same_v<U, FStringArg>) return { { L"StrProperty" }, nullptr, "FString" };
    // In-place struct (StructRef): any StructProperty here, its exact type is checked at call time
    else if constexpr (std::is_same_v<U, StructRef>) return { { L"StructProperty" }, nullptr, "struct" };
    else if constexpr (std::is_same_v<U, FVector>) return { { L"StructProperty" }, L"Vector", "FVector" };
    else if constexpr (std::is_same_v<U, FVector2D>) return { { L"StructProperty" }, L"Vector2D", "FVector2D" };
    else if constexpr (std::is_same_v<U, FRotator>) return { { L"StructProperty" }, L"Rotator", "FRotator" };
    else if constexpr (std::is_same_v<U, FQuat>) return { { L"StructProperty" }, L"Quat", "FQuat" };
    else if constexpr (std::is_same_v<U, FTransform>) return { { L"StructProperty" }, L"Transform", "FTransform" };
    else if constexpr (std::is_same_v<U, FLinearColor>) return { { L"StructProperty" }, L"LinearColor", "FLinearColor" };
    else if constexpr (std::is_same_v<U, FHitResult>) return { { L"StructProperty" }, L"HitResult", "FHitResult" };
    else if constexpr (is_tarray<U>::value) return { { L"ArrayProperty" }, nullptr, "TArray" };
    else if constexpr (is_object_ptr_v<U> || std::is_same_v<U, std::nullptr_t> || std::is_same_v<U, WeakObject>)
        return { { L"ObjectProperty", L"ClassProperty", L"ObjectPtrProperty" }, nullptr, "UObject*" };
    else static_assert(sizeof(U) == 0, "ue: unsupported property type");
}

// A resolved property: offset plus what's needed to type-check accesses
struct PropertyInfo {
    FProperty* property{};
    int32_t offset{ -1 };
    std::wstring field_class;  // e.g. L"StructProperty"
    std::wstring struct_name;  // for StructProperty
    const UStruct* struct_type{}; // for StructProperty
    std::wstring name;
    mutable std::vector<const TypeDesc*> validated; // types already checked OK
    mutable std::vector<const TypeDesc*> rejected;  // types already reported as mismatching
};

// owner_name is used in error messages
const PropertyInfo* find_property(const UStruct* owner, std::wstring_view name);
bool check_type(const PropertyInfo& info, const TypeDesc& desc, const UStruct* owner);

template <typename T>
inline constexpr TypeDesc type_desc_v = type_desc<T>();

struct FunctionInfo {
    UFunction* function{};
    int32_t params_size{};
    int32_t alignment{};
    std::vector<const PropertyInfo*> params; // in declaration order, return value excluded
    const PropertyInfo* return_value{};
    std::wstring name;
    mutable std::vector<const void*> validated_signatures;
    mutable std::vector<const void*> rejected_signatures;
};

// log_missing = false for functions that are allowed not to exist (see ue::try_call)
const FunctionInfo* find_function(const UStruct* owner, std::wstring_view name, bool log_missing = true);

// Signature check, memoized per (function, C++ signature)
bool check_signature(const FunctionInfo& fn, const UStruct* owner, const void* signature_tag,
                     const TypeDesc* const* arg_types, size_t arg_count, const TypeDesc* return_type);

template <typename... Ts> struct signature_tag { static inline const char tag{}; };

// Zero-initialized, suitably aligned parameter buffer for process_event
class ParamBuffer {
public:
    ParamBuffer(int32_t size, int32_t alignment);
    ~ParamBuffer();
    ParamBuffer(const ParamBuffer&) = delete;
    ParamBuffer& operator=(const ParamBuffer&) = delete;
    uint8_t* data() { return m_data; }

private:
    alignas(16) uint8_t m_inline[512];
    uint8_t* m_heap{ nullptr };
    uint8_t* m_data{ nullptr };
};

void write_bool(const PropertyInfo& p, void* base, bool value);
bool read_bool(const PropertyInfo& p, const void* base);

int32_t struct_size(const UStruct* type);
// A StructRef argument must be an instance of the parameter's own struct type (logged once if not)
bool struct_arg_matches(const PropertyInfo& p, const UStruct* arg_type, const FunctionInfo& fn);

template <typename T>
bool arg_runtime_ok(const PropertyInfo& p, const T& value, const FunctionInfo& fn) {
    if constexpr (std::is_same_v<std::remove_cvref_t<T>, StructRef>) {
        return static_cast<bool>(value) && struct_arg_matches(p, value.type(), fn);
    } else {
        return true;
    }
}

template <typename T>
void write_arg(const PropertyInfo& p, uint8_t* buffer, const T& value) {
    using U = std::remove_cvref_t<T>;
    uint8_t* dst = buffer + p.offset;
    if constexpr (std::is_same_v<U, std::nullptr_t>) {
        // null object, or "don't care" out parameter: leave zeroed
    } else if constexpr (std::is_pointer_v<U> && !is_object_ptr_v<U>) {
        // pointer to a value = out (or in/out) parameter
        using V = std::remove_cv_t<std::remove_pointer_t<U>>;
        if (value != nullptr) {
            if constexpr (std::is_same_v<V, bool>) write_bool(p, buffer, *value);
            else *reinterpret_cast<V*>(dst) = *value;
        }
    } else if constexpr (std::is_same_v<U, bool>) {
        write_bool(p, buffer, value);
    } else if constexpr (std::is_same_v<U, StructRef>) {
        std::memcpy(dst, value.data(), (size_t)struct_size(value.type())); // bitwise, like the SDK did
    } else if constexpr (is_object_ptr_v<U>) {
        *reinterpret_cast<void**>(dst) = (void*)value;
    } else if constexpr (std::is_same_v<U, WeakObject>) {
        *reinterpret_cast<void**>(dst) = (void*)value.get(); // nullptr once the object is gone
    } else {
        *reinterpret_cast<U*>(dst) = value;
    }
}

template <typename T>
void read_back_arg(const PropertyInfo& p, uint8_t* buffer, const T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_pointer_v<U> && !is_object_ptr_v<U>) {
        using V = std::remove_cv_t<std::remove_pointer_t<U>>;
        if (value != nullptr) {
            if constexpr (std::is_same_v<V, bool>) *const_cast<V*>(value) = read_bool(p, buffer);
            else *const_cast<V*>(value) = *reinterpret_cast<const V*>(buffer + p.offset);
        }
    }
}

// Type an argument must match: out-parameter pointers are checked as their pointee
template <typename T>
constexpr const TypeDesc* arg_type_desc() {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_pointer_v<U> && !is_object_ptr_v<U>) return &type_desc_v<std::remove_cv_t<std::remove_pointer_t<U>>>;
    else if constexpr (std::is_same_v<U, std::nullptr_t>) return nullptr; // matches anything
    else return &type_desc_v<U>;
}

template <typename R>
R default_value() {
    if constexpr (!std::is_void_v<R>) return R{};
}

template <typename R, typename... Args>
R invoke_resolved(UObject* target, const UStruct* owner, const FunctionInfo& fn_info, const Args&... args);

template <typename R, typename... Args>
R invoke(UObject* target, const UStruct* owner, std::wstring_view fn_name, const Args&... args) {
    if (target == nullptr || owner == nullptr) return detail::default_value<R>();
    const FunctionInfo* fn = find_function(owner, fn_name);
    if (fn == nullptr) return detail::default_value<R>();
    return invoke_resolved<R>(target, owner, *fn, args...);
}

template <typename R, typename... Args>
R invoke_resolved(UObject* target, const UStruct* owner, const FunctionInfo& fn_info, const Args&... args) {
    const FunctionInfo* fn = &fn_info;

    static constexpr const TypeDesc* arg_types[] = { arg_type_desc<Args>()..., nullptr };
    const TypeDesc* return_type = nullptr;
    if constexpr (!std::is_void_v<R>) return_type = &type_desc_v<R>;
    if (!check_signature(*fn, owner, &signature_tag<R, Args...>::tag, arg_types, sizeof...(Args), return_type)) return detail::default_value<R>();
    {
        size_t k = 0;
        if (!(arg_runtime_ok(*fn->params[k++], args, *fn) && ... && true)) return detail::default_value<R>();
    }

    ParamBuffer buffer{ fn->params_size, fn->alignment };
    size_t i = 0;
    (write_arg(*fn->params[i++], buffer.data(), args), ...);
    target->process_event(fn->function, buffer.data());
    i = 0;
    (read_back_arg(*fn->params[i++], buffer.data(), args), ...);

    if constexpr (std::is_void_v<R>) {
        return;
    } else if constexpr (std::is_same_v<R, bool>) {
        return read_bool(*fn->return_value, buffer.data());
    } else {
        return *reinterpret_cast<const R*>(buffer.data() + fn->return_value->offset);
    }
}

template <typename T>
T* member_ptr(void* base, const UStruct* owner, std::wstring_view name) {
    if (base == nullptr || owner == nullptr) return nullptr;
    const PropertyInfo* p = find_property(owner, name);
    if (p == nullptr) return nullptr;
    if constexpr (!std::is_void_v<T>) {
        if (!check_type(*p, type_desc_v<T>, owner)) return nullptr;
    }
    return reinterpret_cast<T*>(static_cast<uint8_t*>(base) + p->offset);
}

} // namespace detail

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

// Pointer to a property's data inside an object (nullptr if missing/mismatching).
// Not for bool properties: they may be bitfields, use get/set.
template <typename T>
T* get_ptr(const UObject* obj, std::wstring_view name) {
    static_assert(!std::is_same_v<T, bool>, "use ue::get<bool>/ue::set for bool properties (may be bitfields)");
    if (obj == nullptr) return nullptr;
    return detail::member_ptr<T>((void*)obj, obj->get_class(), name);
}

// Not for TArray: UEVR's TArray frees its memory when destroyed, so a copy would free the
// game's array. Use get_ptr<TArray<T>> to access arrays in place.
template <typename T>
T get(const UObject* obj, std::wstring_view name, T fallback = T{}) {
    static_assert(!detail::is_tarray<T>::value, "use ue::get_ptr<TArray<T>> (a TArray copy would free the game's array)");
    if (obj == nullptr) return fallback;
    if constexpr (std::is_same_v<T, bool>) {
        const auto* p = detail::find_property(obj->get_class(), name);
        if (p == nullptr || !detail::check_type(*p, detail::type_desc_v<bool>, obj->get_class())) return fallback;
        return detail::read_bool(*p, obj);
    } else {
        const T* ptr = get_ptr<T>(obj, name);
        return ptr != nullptr ? *ptr : fallback;
    }
}

template <typename T>
bool set(UObject* obj, std::wstring_view name, const T& value) {
    static_assert(!detail::is_tarray<T>::value, "use ue::get_ptr<TArray<T>> to modify arrays in place");
    if (obj == nullptr) return false;
    if constexpr (std::is_same_v<T, bool>) {
        const auto* p = detail::find_property(obj->get_class(), name);
        if (p == nullptr || !detail::check_type(*p, detail::type_desc_v<bool>, obj->get_class())) return false;
        detail::write_bool(*p, obj, value);
        return true;
    } else {
        T* ptr = get_ptr<T>(obj, name);
        if (ptr == nullptr) return false;
        *ptr = value;
        return true;
    }
}

// A struct instance inside an object or another struct, e.g.
//   auto pov = ue::get_struct(cameraManager, L"CameraCachePrivate").sub(L"POV");
//   auto location = pov.get<ue::FVector>(L"Location");
// The struct type is taken from the reflected property, so no struct name is hard-coded.
class StructRef {
public:
    StructRef() = default;
    StructRef(void* data, const UStruct* type) : m_data(data), m_type(type) {}

    explicit operator bool() const { return m_data != nullptr && m_type != nullptr; }
    void* data() const { return m_data; }
    const UStruct* type() const { return m_type; }

    // Not for bool members: they may be bitfields, use get/set
    template <typename T>
    T* member(std::wstring_view name) const {
        static_assert(!std::is_same_v<T, bool>, "use get<bool>/set for bool members (may be bitfields)");
        return detail::member_ptr<T>(m_data, m_type, name);
    }
    template <typename T>
    T get(std::wstring_view name, T fallback = T{}) const {
        static_assert(!detail::is_tarray<T>::value, "use member<TArray<T>> (a TArray copy would free the game's array)");
        if constexpr (std::is_same_v<T, bool>) {
            const auto* p = bool_member(name);
            return p != nullptr ? detail::read_bool(*p, m_data) : fallback;
        } else {
            const T* ptr = member<T>(name);
            return ptr != nullptr ? *ptr : fallback;
        }
    }
    template <typename T>
    bool set(std::wstring_view name, const T& value) const {
        static_assert(!detail::is_tarray<T>::value, "use member<TArray<T>> to modify arrays in place");
        if constexpr (std::is_same_v<T, bool>) {
            const auto* p = bool_member(name);
            if (p == nullptr) return false;
            detail::write_bool(*p, m_data, value);
            return true;
        } else {
            T* ptr = member<T>(name);
            if (ptr == nullptr) return false;
            *ptr = value;
            return true;
        }
    }
    StructRef sub(std::wstring_view name) const; // nested struct member

private:
    const detail::PropertyInfo* bool_member(std::wstring_view name) const;

    void* m_data{};
    const UStruct* m_type{};
};

StructRef get_struct(const UObject* obj, std::wstring_view name);

// Member of a struct instance (e.g. a game struct passed to a hooked native function)
template <typename T>
T* struct_member(void* data, const UStruct* struct_type, std::wstring_view name) {
    return detail::member_ptr<T>(data, struct_type, name);
}

// Parameter of a UFunction inside a parameter buffer (e.g. in a hook)
template <typename T>
T* param(const UFunction* fn, void* params, std::wstring_view name) {
    return detail::member_ptr<T>(params, fn, name);
}

// ---------------------------------------------------------------------------
// Function calls (arguments in declaration order, like the engine signature)
// ---------------------------------------------------------------------------
// - Out parameters: pass a pointer to a value (copied in, then back), or nullptr to ignore.
// - The return value is the template argument (default void).
// - Returns R{} if the object is null, or the function is missing/mismatching (logged once).

template <typename R = void, typename... Args>
R call(UObject* obj, std::wstring_view fn_name, const Args&... args) {
    if (obj == nullptr) return detail::default_value<R>();
    return detail::invoke<R>(obj, obj->get_class(), fn_name, args...);
}

// Same as call, for functions that may legitimately not exist on the object's class
// (e.g. GetMuzzleTransform on whatever weapon is equipped): a missing function is not
// logged. Signature mismatches still are.
template <typename R = void, typename... Args>
R try_call(UObject* obj, std::wstring_view fn_name, const Args&... args) {
    if (obj == nullptr) return detail::default_value<R>();
    const auto* fn = detail::find_function(obj->get_class(), fn_name, false);
    if (fn == nullptr) return detail::default_value<R>();
    return detail::invoke_resolved<R>(obj, obj->get_class(), *fn, args...);
}

// Static function (e.g. Kismet libraries), called on the class default object
template <typename R = void, typename... Args>
R call_static(UClass* cls, std::wstring_view fn_name, const Args&... args) {
    UObject* cdo = default_object(cls);
    return detail::invoke<R>(cdo, cls, fn_name, args...);
}

// ---------------------------------------------------------------------------
// Convenience
// ---------------------------------------------------------------------------

// UKismetSystemLibrary::IsValid, null-safe: false for an object destroyed but not garbage
// collected yet. Reads the object, so never use it on a pointer that may have been freed:
// keep such pointers in a ue::WeakObject, which returns nullptr once the object is freed.
inline bool is_valid(const UObject* obj) {
    return obj != nullptr && call_static<bool>(lib::KismetSystem, L"IsValid", obj);
}

// Copy of an object array property (e.g. AttachChildren): safe to iterate while the
// engine modifies the array, e.g. when destroying components inside the loop.
inline std::vector<UObject*> array_snapshot(const UObject* obj, std::wstring_view name) {
    const auto* array = get_ptr<TArray<UObject*>>(obj, name);
    if (array == nullptr || array->empty()) return {};
    return { array->begin(), array->end() };
}

} // namespace ue
