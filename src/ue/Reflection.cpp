#include "ue/Reflection.hpp"

#include <windows.h>

#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace ue {

std::string narrow(std::wstring_view text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), result.data(), size, nullptr, nullptr);
    return result;
}

namespace {

std::recursive_mutex g_mutex;

// Errors go to UEVR's log.txt (always, not only in debug mode), once per distinct message
template <typename... Args>
void log_error_once(const char* format, Args... args) {
    char message[1024];
    snprintf(message, sizeof(message), format, args...);
    static std::unordered_set<std::string> s_logged;
    std::scoped_lock lock{ g_mutex };
    if (s_logged.insert(message).second) {
        uevr::API::get()->log_error("[MGSDelta] %s", message);
    }
}

std::string struct_name(const UStruct* s) {
    return s != nullptr ? narrow(s->get_full_name()) : std::string{ "<null>" };
}

struct WStringHash {
    using is_transparent = void;
    size_t operator()(std::wstring_view s) const { return std::hash<std::wstring_view>{}(s); }
};
template <typename V>
using NameMap = std::unordered_map<std::wstring, V, WStringHash, std::equal_to<>>;

// Per-struct cache. The struct's name, first property and parent are kept to detect a struct
// unloaded and another one allocated at the same address (the cache is then flushed for that
// address). The name alone isn't enough: a Blueprint class reloaded at the same address has
// the same name, but new properties and functions.
struct StructCache {
    int32_t name_index{};
    int32_t name_number{};
    const void* first_property{};
    const UStruct* super{};
    NameMap<std::unique_ptr<detail::PropertyInfo>> properties; // nullptr = known missing
    NameMap<std::unique_ptr<detail::FunctionInfo>> functions;  // nullptr = known missing
};
std::unordered_map<const UStruct*, StructCache> g_cache;

// Flushed entries are kept alive: a FunctionInfo cached for another struct can still point to
// their PropertyInfos (function parameters are cached under the function's own entry).
std::list<StructCache> g_retired_caches;

StructCache& cache_for(const UStruct* owner) {
    const auto* fname = owner->get_fname();
    const void* first_property = owner->get_child_properties();
    const UStruct* super = owner->get_super();
    auto& entry = g_cache[owner];
    if (entry.name_index != fname->comparison_index || entry.name_number != fname->number ||
        entry.first_property != first_property || entry.super != super) {
        if (!entry.properties.empty() || !entry.functions.empty()) {
            g_retired_caches.push_back(std::move(entry));
        }
        entry = StructCache{};
        entry.name_index = fname->comparison_index;
        entry.name_number = fname->number;
        entry.first_property = first_property;
        entry.super = super;
    }
    return entry;
}

std::unique_ptr<detail::PropertyInfo> make_property_info(FProperty* prop) {
    auto info = std::make_unique<detail::PropertyInfo>();
    info->property = prop;
    info->offset = prop->get_offset();
    info->name = prop->get_fname()->to_string();
    if (const auto* field_class = prop->get_class(); field_class != nullptr) {
        info->field_class = field_class->get_name();
    }
    if (info->field_class == L"StructProperty") {
        const auto* script_struct = reinterpret_cast<uevr::API::FStructProperty*>(prop)->get_struct();
        if (script_struct != nullptr) {
            info->struct_name = script_struct->get_fname()->to_string();
            info->struct_type = script_struct;
        }
    }
    return info;
}

FProperty* find_property_in_chain(const UStruct* owner, std::wstring_view name) {
    for (const UStruct* s = owner; s != nullptr; s = s->get_super()) {
        for (auto* field = s->get_child_properties(); field != nullptr; field = field->get_next()) {
            if (field->get_fname()->to_string() == name) {
                return reinterpret_cast<FProperty*>(field);
            }
        }
    }
    return nullptr;
}

bool type_matches(const detail::PropertyInfo& info, const detail::TypeDesc& desc) {
    bool class_ok = false;
    for (const wchar_t* accepted : desc.field_classes) {
        if (accepted != nullptr && info.field_class == accepted) {
            class_ok = true;
            break;
        }
    }
    if (!class_ok) return false;
    return desc.struct_name == nullptr || info.struct_name == desc.struct_name;
}

std::string describe(const detail::PropertyInfo& info) {
    std::string result = narrow(info.field_class);
    if (!info.struct_name.empty()) result += " (" + narrow(info.struct_name) + ")";
    return result;
}

} // namespace

// ---------------------------------------------------------------------------

// The UEVR C API expects null-terminated strings, which std::wstring_view doesn't guarantee
UObject* find_object(std::wstring_view full_name) {
    return uevr::API::get()->find_uobject(std::wstring{ full_name });
}

UClass* find_class(std::wstring_view full_name) {
    auto* cls = uevr::API::get()->find_uobject<UClass>(std::wstring{ full_name });
    if (cls == nullptr) log_error_once("Class not found: %ls", std::wstring{ full_name }.c_str());
    return cls;
}

UScriptStruct* find_struct(std::wstring_view full_name) {
    auto* s = uevr::API::get()->find_uobject<UScriptStruct>(std::wstring{ full_name });
    if (s == nullptr) log_error_once("Struct not found: %ls", std::wstring{ full_name }.c_str());
    return s;
}

UObject* default_object(UClass* cls) {
    return cls != nullptr ? cls->get_class_default_object() : nullptr;
}

namespace {
StructRef struct_at(void* base, const UStruct* owner, std::wstring_view name) {
    if (base == nullptr || owner == nullptr) return {};
    const auto* p = detail::find_property(owner, name);
    if (p == nullptr) return {};
    if (p->struct_type == nullptr) {
        log_error_once("Property %ls in %s is %s, but the plugin expects a struct",
                       p->name.c_str(), struct_name(owner).c_str(), describe(*p).c_str());
        return {};
    }
    return { static_cast<uint8_t*>(base) + p->offset, p->struct_type };
}
} // namespace

StructRef StructRef::sub(std::wstring_view name) const {
    return struct_at(m_data, m_type, name);
}

const detail::PropertyInfo* StructRef::bool_member(std::wstring_view name) const {
    if (m_data == nullptr || m_type == nullptr) return nullptr;
    const auto* p = detail::find_property(m_type, name);
    if (p == nullptr || !detail::check_type(*p, detail::type_desc_v<bool>, m_type)) return nullptr;
    return p;
}

UObject* get_world() {
    auto* engine = uevr::API::get()->get_engine();
    return get<UObject*>(get<UObject*>(engine, L"GameViewport"), L"World");
}

StructRef get_struct(const UObject* obj, std::wstring_view name) {
    return obj != nullptr ? struct_at((void*)obj, obj->get_class(), name) : StructRef{};
}

FName::FName(std::wstring_view name) {
    const std::wstring terminated{ name }; // the UEVR C API expects null-terminated strings
    *this = call_static<FName>(lib::KismetString, L"Conv_StringToName", FStringArg{ terminated });
    if (IsNone() && !terminated.empty() && terminated != L"None") {
        // Fallback for games where KismetStringLibrary isn't usable
        const uevr::API::FName engine_name{ terminated };
        ComparisonIndex = engine_name.comparison_index;
        Number = engine_name.number;
        if (IsNone()) log_error_once("Failed to create FName \"%ls\"", terminated.c_str());
    }
}

UClass* ClassRef::get() const {
    if (auto* cls = m_cached.get()) {
        return static_cast<UClass*>(cls);
    }
    auto* cls = uevr::API::get()->find_uobject<UClass>(m_full_name);
    m_cached = cls;
    return cls;
}

UObject* SingletonRef::get() const {
    // is_valid also rejects an instance destroyed but not garbage collected yet: during a level
    // change the old and new instances can both be in the object list for a while
    if (auto* obj = m_cached.get(); obj != nullptr && is_valid(obj)) {
        return obj;
    }
    m_cached.reset();
    auto* cls = m_class.get();
    if (cls == nullptr) return nullptr;
    for (auto* obj : cls->get_objects_matching(false)) {
        if (is_valid(obj)) {
            m_cached = obj;
            return obj;
        }
    }
    return nullptr;
}

void WeakObject::assign(UObject* object) {
    reset();
    if (object == nullptr) return;
    m_object = object;
    m_class = object->get_class();
    // UObjectBase::InternalIndex, right after the vtable and the object flags (same in UE4 and UE5)
    const int32_t index = *reinterpret_cast<const int32_t*>(reinterpret_cast<const uint8_t*>(object) + 0xC);
    const auto* objects = uevr::API::FUObjectArray::get();
    if (objects != nullptr && index >= 0 && index < objects->get_object_count() && objects->get_object(index) == object) {
        m_index = index;
    } else {
        log_error_once("WeakObject: object index not found in GUObjectArray, using UObjectHook::exists instead");
    }
}

UObject* WeakObject::get() const {
    if (m_object == nullptr) return nullptr;
    if (m_index >= 0) {
        // Never reads the object itself: if it was freed, its slot is empty or holds another object
        const auto* objects = uevr::API::FUObjectArray::get();
        if (objects == nullptr || m_index >= objects->get_object_count() || objects->get_object(m_index) != m_object) {
            return nullptr;
        }
    } else if (!uevr::API::UObjectHook::exists(m_object)) {
        return nullptr;
    }
    // Same slot and address, but a different kind of object
    if (m_object->get_class() != m_class) return nullptr;
    return m_object;
}

namespace detail {

const PropertyInfo* find_property(const UStruct* owner, std::wstring_view name) {
    if (owner == nullptr) return nullptr;
    std::scoped_lock lock{ g_mutex };
    auto& cache = cache_for(owner);
    if (auto it = cache.properties.find(name); it != cache.properties.end()) {
        return it->second.get();
    }
    std::unique_ptr<PropertyInfo> info{};
    if (auto* prop = find_property_in_chain(owner, name); prop != nullptr) {
        info = make_property_info(prop);
    } else {
        log_error_once("Property %ls not found in %s", std::wstring{ name }.c_str(), struct_name(owner).c_str());
    }
    auto* result = info.get();
    cache.properties.emplace(std::wstring{ name }, std::move(info));
    return result;
}

bool check_type(const PropertyInfo& info, const TypeDesc& desc, const UStruct* owner) {
    std::scoped_lock lock{ g_mutex };
    for (auto* ok : info.validated) if (ok == &desc) return true;
    for (auto* bad : info.rejected) if (bad == &desc) return false;
    if (type_matches(info, desc)) {
        info.validated.push_back(&desc);
        return true;
    }
    info.rejected.push_back(&desc);
    log_error_once("Property %ls in %s is %s, but the plugin expects %s",
                   info.name.c_str(), struct_name(owner).c_str(), describe(info).c_str(), desc.cpp_name);
    return false;
}

const FunctionInfo* find_function(const UStruct* owner, std::wstring_view name, bool log_missing) {
    if (owner == nullptr) return nullptr;
    std::scoped_lock lock{ g_mutex };
    auto& cache = cache_for(owner);
    if (auto it = cache.functions.find(name); it != cache.functions.end()) {
        return it->second.get();
    }

    UFunction* fn = nullptr;
    const std::wstring terminated_name{ name };
    for (const UStruct* s = owner; s != nullptr && fn == nullptr; s = s->get_super()) {
        fn = s->find_function(terminated_name);
    }

    std::unique_ptr<FunctionInfo> info{};
    if (fn != nullptr) {
        info = std::make_unique<FunctionInfo>();
        info->function = fn;
        info->name = std::wstring{ name };
        info->params_size = fn->get_properties_size();
        info->alignment = fn->get_min_alignment();
        for (auto* field = fn->get_child_properties(); field != nullptr; field = field->get_next()) {
            auto* prop = reinterpret_cast<FProperty*>(field);
            if (!prop->is_param()) continue; // Blueprint functions also list their local variables
            const PropertyInfo* param = find_property(fn, prop->get_fname()->to_string());
            if (param == nullptr) continue;
            if (prop->is_return_param()) info->return_value = param;
            else info->params.push_back(param);
        }
    } else if (log_missing) {
        log_error_once("Function %ls not found in %s", std::wstring{ name }.c_str(), struct_name(owner).c_str());
    }
    auto* result = info.get();
    cache.functions.emplace(std::wstring{ name }, std::move(info));
    return result;
}

bool check_signature(const FunctionInfo& fn, const UStruct* owner, const void* signature_tag,
                     const TypeDesc* const* arg_types, size_t arg_count, const TypeDesc* return_type) {
    std::scoped_lock lock{ g_mutex };
    for (auto* ok : fn.validated_signatures) if (ok == signature_tag) return true;
    for (auto* bad : fn.rejected_signatures) if (bad == signature_tag) return false;

    const std::string where = narrow(fn.name) + " (" + struct_name(owner) + ")";
    bool ok = true;
    if (fn.params.size() != arg_count) {
        log_error_once("Function %s takes %d parameters, the plugin passes %d",
                       where.c_str(), (int)fn.params.size(), (int)arg_count);
        ok = false;
    } else {
        for (size_t i = 0; i < arg_count; ++i) {
            if (arg_types[i] != nullptr && !type_matches(*fn.params[i], *arg_types[i])) {
                log_error_once("Function %s parameter %d (%ls) is %s, the plugin passes %s",
                               where.c_str(), (int)i, fn.params[i]->name.c_str(),
                               describe(*fn.params[i]).c_str(), arg_types[i]->cpp_name);
                ok = false;
            }
        }
    }
    if (return_type != nullptr) {
        if (fn.return_value == nullptr) {
            log_error_once("Function %s has no return value, the plugin expects %s", where.c_str(), return_type->cpp_name);
            ok = false;
        } else if (!type_matches(*fn.return_value, *return_type)) {
            log_error_once("Function %s returns %s, the plugin expects %s",
                           where.c_str(), describe(*fn.return_value).c_str(), return_type->cpp_name);
            ok = false;
        }
    }
    (ok ? fn.validated_signatures : fn.rejected_signatures).push_back(signature_tag);
    return ok;
}

ParamBuffer::ParamBuffer(int32_t size, int32_t alignment) {
    const size_t bytes = size > 0 ? (size_t)size : 1;
    if (bytes <= sizeof(m_inline) && alignment <= 16) {
        m_data = m_inline;
    } else {
        m_heap = static_cast<uint8_t*>(_aligned_malloc(bytes, alignment > 16 ? (size_t)alignment : 16));
        m_data = m_heap;
    }
    std::memset(m_data, 0, bytes);
}

ParamBuffer::~ParamBuffer() {
    if (m_heap != nullptr) _aligned_free(m_heap);
}

int32_t struct_size(const UStruct* type) {
    return type != nullptr ? reinterpret_cast<const UScriptStruct*>(type)->get_struct_size() : 0;
}

bool struct_arg_matches(const PropertyInfo& p, const UStruct* arg_type, const FunctionInfo& fn) {
    if (p.struct_type == arg_type) return true;
    log_error_once("Function %s parameter %ls expects struct %ls, the plugin passes %s",
                   narrow(fn.name).c_str(), p.name.c_str(), p.struct_name.c_str(), struct_name(arg_type).c_str());
    return false;
}

void write_bool(const PropertyInfo& p, void* base, bool value) {
    reinterpret_cast<uevr::API::FBoolProperty*>(p.property)->set_value_in_object(base, value);
}

bool read_bool(const PropertyInfo& p, const void* base) {
    return reinterpret_cast<uevr::API::FBoolProperty*>(p.property)->get_value_from_object(const_cast<void*>(base));
}

} // namespace detail
} // namespace ue
