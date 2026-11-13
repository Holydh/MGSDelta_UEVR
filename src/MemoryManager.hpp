#pragma once
#include <windows.h>
#include "Logger.hpp"
#include "utility/Scan.hpp"

struct MemoryBlock {
    uintptr_t address;
    size_t size;
    std::vector<uint8_t> bytes; // Stores the block of bytes

	// Constructor to initialize from address, size, and a contiguous hexadecimal string
	MemoryBlock(uintptr_t addr, size_t sz, uint64_t hexValue)
		: address(addr), size(sz) {
		// Convert the hexValue into a vector of bytes
		for (size_t i = 0; i < size; ++i) {
			uint8_t byte = static_cast<uint8_t>((hexValue >> (8 * (size - 1 - i))) & 0xFF);
			bytes.push_back(byte);
		}
	}
};

// MemoryManager class
class MemoryManager {
private:
	void NopMemory(std::vector<MemoryBlock>& memoryBlocks);
	void RestoreMemory(const std::vector<MemoryBlock>& memoryBlocks);

public:
	void InitMemoryManager();

	// enabled: the game instructions are NOPed, disabled: the original ones are restored
	void SetFrameOffsetCutsceneFixEnabled(bool enabled);
	bool FrameOffsetCutsceneFixInstructionsNoped = false;
};
