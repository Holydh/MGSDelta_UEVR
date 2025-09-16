#pragma once

#include <unordered_map>
#include <cstdint>
#include <iostream>
#include <windows.h>
#include <array>

// Define the OriginalByte struct
struct OriginalByte {
    uintptr_t address; // The memory address (offset)
    uint8_t value;     // The original byte value
};

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
	uintptr_t GetModuleBaseAddress(LPCTSTR moduleName);
    void AdjustAddresses();
	uintptr_t baseAddressGameEXE = NULL;

public:
	std::array<uintptr_t, 3> AimPositionAddresses; // x, y, z

	uintptr_t AimPositionXAddress = 0x53E2580;

	void InitMemoryManager();
	void ToggleAimPositionMemoryInstructions(bool enableOriginalInstructions);

	bool AimPositionMemoryInstructionsNoped = true;

	//void GetAllBytes();
	//void WriteBytesToIniFile(const char* header, const std::vector<std::pair<uintptr_t, size_t>>& addresses);
};