#pragma once
#include <cstdint>

#include "uevr/API.hpp"

namespace ue {

// Non-owning reference to a UObject that knows when the engine has freed it, like the engine's
// TWeakObjectPtr. Use it for every object kept from one frame to the next: level loads, cutscenes
// and the Lua scripts destroy objects at any time, and a raw pointer then points to freed memory.
//
// It remembers the object's slot in the engine's object array (GUObjectArray). Once the object is
// freed, its slot is emptied or given to another object, so get() returns nullptr without ever
// reading the freed memory. An object destroyed but not garbage collected yet is still returned
// (its memory is valid): use ue::is_valid to reject those too.
//
// Converts implicitly to UObject* (nullptr once the object is gone) and can be passed to
// ue::get, ue::set and ue::call, including as a function argument.
class WeakObject {
public:
    constexpr WeakObject() = default;
    WeakObject(uevr::API::UObject* object) { assign(object); }
    WeakObject& operator=(uevr::API::UObject* object) { assign(object); return *this; }

    uevr::API::UObject* get() const;
    operator uevr::API::UObject*() const { return get(); }
    uevr::API::UObject* operator->() const { return get(); }

    // Was set to an object that has been freed since
    bool expired() const { return m_object != nullptr && get() == nullptr; }
    void reset() { *this = WeakObject{}; }

private:
    void assign(uevr::API::UObject* object);

    uevr::API::UObject* m_object{};
    int32_t m_index{ -1 };
    const uevr::API::UClass* m_class{};
};

} // namespace ue
