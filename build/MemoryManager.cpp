#include "MemoryManager.hpp"

DWORD PID;

std::vector<MemoryBlock> aimPositionInstructionsAddresses = {
	{0x5DFDBB1, 7, 0x0F118070030000},
	{0x5DFDBC9, 7, 0x0F118070030000},
	{0x5DFDBD8, 8, 0xF20F118880030000},
};

std::vector<MemoryBlock> cameraPitchInstructionsAddresses = {
	{0x5DAE48F, 8, 0xF20F118730010000},
	{0x5DAE4C7, 7, 0x0F118730010000},
	{0x5DA5667, 7, 0x0F118730010000},
};

std::vector<unsigned int> aimPositionXOffsets = { 0x0C7DE270, 0x10, 0x18, 0xE8, 0xA8, 0x98, 0x280, 0x370 };

void MemoryManager::InitMemoryManager()
{
	baseAddressGameEXE = GetModuleBaseAddress(nullptr);
	for (auto& [address, size, bytes] : aimPositionInstructionsAddresses) { address += baseAddressGameEXE; }

	auto cameraPitchFunctionAddress = utility::scan((uintptr_t)baseAddressGameEXE, 0x9999999, "F2 0F 11 87 ? ? ? ? F2 0F 11 8F ? ? ? ? E8 ? ? ? ?");
	if (!cameraPitchFunctionAddress) {
		Logger::DebugPrint("Failed to find the instruction");
		return;
	}
	else
	{
		Logger::DebugPrint("Found camera pitch function 0 instructions address: 0x%p", (void*)cameraPitchFunctionAddress.value());
	}

	cameraPitchInstructionsAddresses[0].address = cameraPitchFunctionAddress.value();

	auto cameraPitchFunctionAddress1 = utility::scan((uintptr_t)baseAddressGameEXE, 0x9999999, "0F 11 87 ? ? ? ? F2 0F 10 48 ? F2 0F 11 8F ? ? ? ? 49 8B E3 5F C3");
	if (!cameraPitchFunctionAddress1) {
		Logger::DebugPrint("Failed to find the instruction");
		return;
	}
	else
	{
		Logger::DebugPrint("Found camera pitch function 1 instructions address: 0x%p", (void*)cameraPitchFunctionAddress1.value());
	}
	cameraPitchInstructionsAddresses[1].address = cameraPitchFunctionAddress1.value();

	auto cameraPitchFunctionAddress2 = utility::scan((uintptr_t)baseAddressGameEXE, 0x9999999, "0F 11 87 ? ? ? ? F2 0F 10 48 ? F2 0F 11 8F ? ? ? ? 0F 28 CF E8 ? ? ? ?");
	if (!cameraPitchFunctionAddress2) {
		Logger::DebugPrint("Failed to find the instruction");
		return;
	}
	else
	{
		Logger::DebugPrint("Found camera pitch function 2 instructions address: 0x%p", (void*)cameraPitchFunctionAddress2.value());
	}
	cameraPitchInstructionsAddresses[2].address = cameraPitchFunctionAddress2.value();

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

void MemoryManager::ResolveAddresses()
{

	//AimPositionAddresses.x = FindAddressFromPointerOffsets(baseAddressGameEXE, aimPositionXOffsets);
	//AimPositionAddresses.y = AimPositionAddresses.x + 8;
	//AimPositionAddresses.z = AimPositionAddresses.x + 16;
	//Logger::DebugPrint("AimPositionAddresses.x: %f, y: %f, z: %f", *(reinterpret_cast<double*>(AimPositionAddresses.x)),  *(reinterpret_cast<double*>(AimPositionAddresses.y)),  *(reinterpret_cast<double*>(AimPositionAddresses.z)));
}



void MemoryManager::ToggleAimRelatedMemoryInstructions(bool enableOriginalInstructions)
{
	//Logger::DebugPrint("ToggleAimPositionMemoryInstructions called. enableOriginal=%d, noped=%d",enableOriginalInstructions, AimRelatedMemoryInstructionsNoped);
	if (!enableOriginalInstructions && !AimRelatedMemoryInstructionsNoped)
	{
		//NopMemory(aimPositionInstructionsAddresses);
		NopMemory(cameraPitchInstructionsAddresses);
		AimRelatedMemoryInstructionsNoped = !enableOriginalInstructions;
		Logger::DebugPrint("Memory successfully modified");
	}
	if (enableOriginalInstructions && AimRelatedMemoryInstructionsNoped)
	{
		//RestoreMemory(aimPositionInstructionsAddresses);
		RestoreMemory(cameraPitchInstructionsAddresses);
		AimRelatedMemoryInstructionsNoped = !enableOriginalInstructions;
		Logger::DebugPrint("Memory successfully restored");
	}
}

// Function to NOP a batch of addresses
void MemoryManager::NopMemory(const std::vector<MemoryBlock>& memoryBlocks) {
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
void MemoryManager::RestoreMemory(const std::vector<MemoryBlock>& memoryBlocks) {
	for (const auto& block : memoryBlocks) {
		DWORD oldProtect;
		VirtualProtect((LPVOID)block.address, block.size, PAGE_EXECUTE_READWRITE, &oldProtect);

		for (size_t i = 0; i < block.size; ++i) {
            *reinterpret_cast<uint8_t*>(block.address + i) = block.bytes[i];
        }

		VirtualProtect((LPVOID)block.address, block.size, oldProtect, &oldProtect);
	}
}

//Finds address from pointer offsets found in cheat engine
uintptr_t MemoryManager::FindAddressFromPointerOffsets(uintptr_t baseAddress, const std::vector<unsigned int>& offsets) {
	uintptr_t addr = baseAddress + offsets[0]; //first offset is already an offset. No need to dereference the pointer here.
		//Logger::DebugPrint("addr 0x%p", addr);

	for (size_t i = 1; i < offsets.size(); ++i) {
		if (addr == 0) {
			// If at any point the address is invalid, return 0
			Logger::DebugPrint("%s", "Can't find address from pointer offsets");
			return 0;
		}
		//Logger::DebugPrint("Found address from pointer offsets %d : 0x%p", i, (void*)addr);
		
		// Dereference the pointer
		addr = *reinterpret_cast<uintptr_t*>(addr);

		// Add the offset
		addr += offsets[i];
	}
	//Logger::DebugPrint("Found address from pointer offsets 0x%p",addr);
	return addr;
}