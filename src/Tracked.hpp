#pragma once
#include <concepts>

// A value and the one it had at the end of the previous frame. Reads and writes like the value itself;
// the owner calls Commit() once per frame, at the end of the frame (its EndFrame).
// Pass Get() to printf-style functions: the object itself isn't converted there.
template <typename T>
class Tracked {
public:
	constexpr Tracked(T initial = T{}) : current(initial), previous(initial) {}
	Tracked& operator=(T value) { current = value; return *this; }

	operator T() const { return current; }
	T Get() const { return current; }
	T Previous() const { return previous; }

	bool Changed() const { return current != previous; }
	// Has the value this frame, didn't have it last frame
	bool Entered(T value) const { return current == value && previous != value; }
	// Had the value last frame, doesn't have it this frame
	bool Left(T value) const { return current != value && previous == value; }
	bool Rose() const requires std::same_as<T, bool> { return current && !previous; }
	bool Fell() const requires std::same_as<T, bool> { return !current && previous; }

	void Commit() { previous = current; }
	// What the next Changed()/Rose()... compare against, e.g. to start over after a level change
	void SetPrevious(T value) { previous = value; }

private:
	T current;
	T previous;
};
