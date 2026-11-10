#pragma once
// Engine value types with the memory layout of Unreal Engine 5 (double precision).
// These layouts are fixed by the engine version, not by the game, so game updates don't
// affect them. Default construction zero-fills, like the Dumper-7 SDK types they replace.
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include "uevr/API.hpp"

#if MGSDELTA_HAS_SDK
// TEMPORARY while the Dumper-7 SDK is still used: implicit conversions so ported and
// not-yet-ported code can exchange values. Layouts are identical (checked by static_assert).
#include "SDK/Basic.hpp"
#include "SDK/CoreUObject_structs.hpp"
#include "SDK/Engine_structs.hpp"
#define UE_SDK_INTEROP(Type)                                                              \
    Type(const ::SDK::Type& other) {                                                      \
        static_assert(sizeof(Type) == sizeof(::SDK::Type), #Type " layout mismatch");     \
        std::memcpy(static_cast<void*>(this), &other, sizeof(Type));                      \
    }                                                                                     \
    operator ::SDK::Type() const {                                                        \
        ::SDK::Type result{};                                                             \
        std::memcpy(static_cast<void*>(&result), this, sizeof(Type));                     \
        return result;                                                                    \
    }
#else
#define UE_SDK_INTEROP(Type)
#endif

namespace ue {

struct FVector {
    double X{}, Y{}, Z{};

    constexpr FVector() = default;
    constexpr FVector(double x, double y, double z) : X(x), Y(y), Z(z) {}
    UE_SDK_INTEROP(FVector)

    constexpr FVector operator+(const FVector& o) const { return { X + o.X, Y + o.Y, Z + o.Z }; }
    constexpr FVector operator-(const FVector& o) const { return { X - o.X, Y - o.Y, Z - o.Z }; }
    constexpr FVector operator*(double s) const { return { X * s, Y * s, Z * s }; }
    constexpr FVector operator/(double s) const { return { X / s, Y / s, Z / s }; }
    constexpr FVector operator-() const { return { -X, -Y, -Z }; }
    constexpr FVector& operator+=(const FVector& o) { X += o.X; Y += o.Y; Z += o.Z; return *this; }
    constexpr FVector& operator-=(const FVector& o) { X -= o.X; Y -= o.Y; Z -= o.Z; return *this; }
    constexpr FVector& operator*=(double s) { X *= s; Y *= s; Z *= s; return *this; }
    constexpr bool operator==(const FVector&) const = default;
};
static_assert(sizeof(FVector) == 0x18);

struct FVector2D {
    double X{}, Y{};

    constexpr FVector2D() = default;
    constexpr FVector2D(double x, double y) : X(x), Y(y) {}
    UE_SDK_INTEROP(FVector2D)

    constexpr bool operator==(const FVector2D&) const = default;
};
static_assert(sizeof(FVector2D) == 0x10);

struct FRotator {
    double Pitch{}, Yaw{}, Roll{};

    constexpr FRotator() = default;
    constexpr FRotator(double pitch, double yaw, double roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
    UE_SDK_INTEROP(FRotator)

    constexpr FRotator operator+(const FRotator& o) const { return { Pitch + o.Pitch, Yaw + o.Yaw, Roll + o.Roll }; }
    constexpr FRotator operator-(const FRotator& o) const { return { Pitch - o.Pitch, Yaw - o.Yaw, Roll - o.Roll }; }
    constexpr bool operator==(const FRotator&) const = default;
};
static_assert(sizeof(FRotator) == 0x18);

struct alignas(16) FQuat {
    double X{}, Y{}, Z{}, W{};

    constexpr FQuat() = default;
    constexpr FQuat(double x, double y, double z, double w) : X(x), Y(y), Z(z), W(w) {}
    UE_SDK_INTEROP(FQuat)

    static constexpr FQuat Identity() { return { 0.0, 0.0, 0.0, 1.0 }; }
};
static_assert(sizeof(FQuat) == 0x20);

struct alignas(16) FTransform {
    FQuat Rotation{};
    FVector Translation{};
    uint8_t Pad_38[0x8]{};
    FVector Scale3D{};
    uint8_t Pad_58[0x8]{};

    constexpr FTransform() = default;
    constexpr FTransform(const FQuat& rotation, const FVector& translation, const FVector& scale)
        : Rotation(rotation), Translation(translation), Scale3D(scale) {}
    UE_SDK_INTEROP(FTransform)

    static constexpr FTransform Identity() { return { FQuat::Identity(), {}, { 1.0, 1.0, 1.0 } }; }
};
static_assert(sizeof(FTransform) == 0x60);
static_assert(offsetof(FTransform, Translation) == 0x20 && offsetof(FTransform, Scale3D) == 0x40);

struct FLinearColor {
    float R{}, G{}, B{}, A{};

    constexpr FLinearColor() = default;
    constexpr FLinearColor(float r, float g, float b, float a) : R(r), G(g), B(b), A(a) {}
    UE_SDK_INTEROP(FLinearColor)

    constexpr bool operator==(const FLinearColor&) const = default;
};
static_assert(sizeof(FLinearColor) == 0x10);

// Opaque: the plugin only passes it to engine functions as an out parameter.
struct FHitResult {
    alignas(8) uint8_t Data[0xE8]{};

    FHitResult() = default;
    UE_SDK_INTEROP(FHitResult)
};
static_assert(sizeof(FHitResult) == 0xE8);

// Same layout as the engine's FName (and uevr::API::FName).
struct FName {
    int32_t ComparisonIndex{};
    int32_t Number{};

    constexpr FName() = default; // "None"
    explicit FName(std::wstring_view name) {
        const std::wstring terminated{ name }; // the UEVR C API expects a null-terminated string
        const uevr::API::FName engine_name{ terminated };
        ComparisonIndex = engine_name.comparison_index;
        Number = engine_name.number;
    }
    UE_SDK_INTEROP(FName)

    std::wstring ToWString() const { return reinterpret_cast<const uevr::API::FName*>(this)->to_string(); }
    std::string ToString() const;

    constexpr bool IsNone() const { return ComparisonIndex == 0 && Number == 0; }
    constexpr bool operator==(const FName&) const = default;
};
static_assert(sizeof(FName) == 0x8);

// Narrow (UTF-8) conversion, for logging
std::string narrow(std::wstring_view text);

inline std::string FName::ToString() const { return narrow(ToWString()); }

} // namespace ue

#undef UE_SDK_INTEROP
