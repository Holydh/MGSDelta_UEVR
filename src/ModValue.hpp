#pragma once
#include <optional>
#include "uevr/API.hpp"

// A UEVR mod value (VR_*) driven by the plugin. Written only when the wanted value changes, so a value
// changed from the UEVR menu isn't overwritten every frame.
template <typename T>
class ModValue {
public:
	explicit ModValue(const char* key) : key(key) {}

	void Set(T value)
	{
		if (applied == value)
			return;
		uevr::API::VR::set_mod_value(key, value);
		applied = value;
	}

	// Written again on the next Set
	void Reset() { applied.reset(); }

private:
	const char* key;
	std::optional<T> applied;
};
