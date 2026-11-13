#include "MemoryManager.hpp"

// Address found by InitMemoryManager's scan (0 = not found: never patched)
std::vector<MemoryBlock> frameOffsetCutsceneFixInstructionsAddresses = {
	{0, 7, 0x0F118330010000},
};

void MemoryManager::InitMemoryManager()
{
	auto frameOffsetCutsceneFixFunctionAddress = utility::scan(GetModuleHandle(nullptr), "0F 11 83 ? ? ? ? 41 0F 28 48 ? 0F 11 8B ? ? ? ? 41 0F 28 40 ? 0F 11 83 ? ? ? ? 41 0F 28 48 ? 0F 11 8B ? ? ? ? E8 ? ? ? ?");
	if (!frameOffsetCutsceneFixFunctionAddress) {
		uevr::API::get()->log_error("[MGSDelta] Cutscene frame offset fix: instruction pattern not found, fix disabled");
		return;
	}
	else
	{
		Logger::DebugPrint("Found frame Offset Cutscene Fix instructions address: 0x%p", (void*)frameOffsetCutsceneFixFunctionAddress.value());
	}
	frameOffsetCutsceneFixInstructionsAddresses[0].address = frameOffsetCutsceneFixFunctionAddress.value();

}

void MemoryManager::SetFrameOffsetCutsceneFixEnabled(bool enabled)
{
	if (enabled && !FrameOffsetCutsceneFixInstructionsNoped)
	{
		NopMemory(frameOffsetCutsceneFixInstructionsAddresses);
		FrameOffsetCutsceneFixInstructionsNoped = enabled;
		Logger::DebugPrint("Memory successfully modified");
	}
	if (!enabled && FrameOffsetCutsceneFixInstructionsNoped)
	{
		RestoreMemory(frameOffsetCutsceneFixInstructionsAddresses);
		FrameOffsetCutsceneFixInstructionsNoped = enabled;
		Logger::DebugPrint("Memory successfully restored");
	}
}

// NOPs a batch of instructions. Their current bytes are saved first, for RestoreMemory.
void MemoryManager::NopMemory(std::vector<MemoryBlock>& memoryBlocks) {
	for (auto& block : memoryBlocks) {
		DWORD oldProtect;
		if (block.address == 0) // not found by the scan (already logged)
			continue;
		if (!VirtualProtect((LPVOID)block.address, block.size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
			uevr::API::get()->log_error("[MGSDelta] Can't patch the code at 0x%p", (void*)block.address);
			continue;
		}

		for (size_t i = 0; i < block.size; ++i) {
			block.bytes[i] = *reinterpret_cast<uint8_t*>(block.address + i);
			*reinterpret_cast<uint8_t*>(block.address + i) = 0x90; // Write NOP
		}

		VirtualProtect((LPVOID)block.address, block.size, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)block.address, block.size);
	}
}

// Function to restore original bytes for a batch of addresses
void MemoryManager::RestoreMemory(const std::vector<MemoryBlock>& memoryBlocks) {
	for (const auto& block : memoryBlocks) {
		DWORD oldProtect;
		if (block.address == 0 || !VirtualProtect((LPVOID)block.address, block.size, PAGE_EXECUTE_READWRITE, &oldProtect))
			continue;

		for (size_t i = 0; i < block.size; ++i) {
            *reinterpret_cast<uint8_t*>(block.address + i) = block.bytes[i];
        }

		VirtualProtect((LPVOID)block.address, block.size, oldProtect, &oldProtect);
		FlushInstructionCache(GetCurrentProcess(), (LPCVOID)block.address, block.size);
	}
}
