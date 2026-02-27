#pragma once
#include <windows.h>
#include "Logger.hpp"
#include "utility/Scan.hpp"

// Define the OriginalByte struct
struct OriginalByte {
    uintptr_t address; // The memory address (offset)
    uint8_t value;     // The original byte value
};

struct Vector3Addresses {
	uintptr_t x;
	uintptr_t y;
	uintptr_t z;
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
	uintptr_t FindAddressFromPointerOffsets(uintptr_t baseAddress, const std::vector<unsigned int>& offsets);
	void NopMemory(const std::vector<MemoryBlock>& memoryBlocks);
	void RestoreMemory(const std::vector<MemoryBlock>& memoryBlocks);
	uintptr_t baseAddressGameEXE = NULL;

public:
	Vector3Addresses AimPositionAddresses{0x53E2580, 0x0, 0x0}; // x, y, z

	void InitMemoryManager();
	void ResolveAddresses();


	void ToggleAimRelatedMemoryInstructions(bool enableOriginalInstructions);
	void ToggleFrameOffsetCutsceneFixMemoryInstructions(bool enableOriginalInstructions);
	bool AimRelatedMemoryInstructionsNoped = false;
	bool FrameOffsetCutsceneFixInstructionsNoped = false;
	
	//void GetAllBytes();
	//void WriteBytesToIniFile(const char* header, const std::vector<std::pair<uintptr_t, size_t>>& addresses);
};