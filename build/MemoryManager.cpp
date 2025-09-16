#include "MemoryManager.hpp"

DWORD PID;

std::vector<MemoryBlock> aimPositionInstructionsAddresses = {
	//{0x110BB78, 3, 0x418945},	
	//{0x110BB7B, 1, 0x08},
	//{0x110C5A4, 3, 0x418945},	
	//{0x110C5A7, 1, 0x08},
	//{0x110C59E, 5, 0xf2410f1145},	
	//{0x110C5A3, 1, 0x00},
	//{0x110BB68, 5, 0xf2410f114d},	
	//{0x110BB6D, 1, 0x00},
	//{0x110CE81, 3, 0x894208},
	//{0x110CE7A, 3, 0xf20f11}, 
	//{0x110CE7D, 1, 0x02}
};

void MemoryManager::InitMemoryManager()
{
	baseAddressGameEXE = GetModuleBaseAddress(nullptr);
	AdjustAddresses();
}

uintptr_t MemoryManager::GetModuleBaseAddress(LPCTSTR moduleName)
{
	HMODULE hModule = GetModuleHandle(moduleName);
	if (hModule == nullptr) {
		//uevr::API::get()->log_info("Failed to get the base address of the module.");
		return 0;
	}
	return reinterpret_cast<uintptr_t>(hModule);
}

void MemoryManager::AdjustAddresses()
{
	AimPositionXAddress += baseAddressGameEXE;
}



void MemoryManager::ToggleAimPositionMemoryInstructions(bool enableOriginalInstructions)
{
	if (!enableOriginalInstructions)
	{
		NopMemory(aimPositionInstructionsAddresses);
	}
	if (enableOriginalInstructions)
	{
		RestoreMemory(aimPositionInstructionsAddresses);
	}
}

// Function to NOP a batch of addresses
void NopMemory(const std::vector<MemoryBlock>& memoryBlocks) {
	for (const auto& [address, size, bytes] : memoryBlocks) {
		DWORD oldProtect;
		VirtualProtect((LPVOID)address, size, PAGE_EXECUTE_READWRITE, &oldProtect);

		for (size_t i = 0; i < size; ++i) {
			uintptr_t currentAddr = address + i;
			*reinterpret_cast<uint8_t*>(currentAddr) = 0x90; // Write NOP
		}

		VirtualProtect((LPVOID)address, size, oldProtect, &oldProtect);
	}
}

// Function to restore original bytes for a batch of addresses
void RestoreMemory(const std::vector<MemoryBlock>& memoryBlocks) {
	for (const auto& block : memoryBlocks) {
		DWORD oldProtect;
		VirtualProtect((LPVOID)block.address, block.size, PAGE_EXECUTE_READWRITE, &oldProtect);

		for (size_t i = 0; i < block.size; ++i) {
            *reinterpret_cast<uint8_t*>(block.address + i) = block.bytes[i];
        }

		VirtualProtect((LPVOID)block.address, block.size, oldProtect, &oldProtect);
	}
}