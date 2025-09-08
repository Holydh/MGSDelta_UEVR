#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <utility/Scan.hpp>
#include <utility/Module.hpp>
#include <utility/String.hpp>

#include "Plugin.hpp"

using namespace uevr;

#define MAX_DEBUG_MESSAGE_SIZE 2048
bool g_DebugEnabled = false;
float g_AdjustValueV = 0;
float g_AdjustValueH = 0;

static void DebugPrint(const char* Format, ...)
{
  char FormattedMessage[MAX_DEBUG_MESSAGE_SIZE];    
  va_list ArgPtr = NULL;  
  
  if(g_DebugEnabled)    
  {        
    /* Generate the formatted debug message. */        
    va_start(ArgPtr, Format);        
    vsprintf(FormattedMessage, Format, ArgPtr);        
    va_end(ArgPtr); 
    
    printf(FormattedMessage);
    API::get()->log_info(FormattedMessage);
  }
}

std::unique_ptr<MGS3Plugin> g_plugin = std::make_unique<MGS3Plugin>();

MGS3Plugin::~MGS3Plugin() {

}

void MGS3Plugin::on_initialize() {
    // Logs to the appdata UnrealVRMod log.txt file
    API::get()->log_info("%s", "Birria ramen");
    
    std::filesystem::path config_file = API::get()->get_persistent_dir() / "plugins" / "aimfix.txt";
    std::filesystem::path debug_file = API::get()->get_persistent_dir() / "debug";
    
    SetGunAdjustValuesFromConfig(config_file);
    API::get()->log_info("MGS3-AimFix.dll: using config file value for gun adjust v:%f h:%f", g_AdjustValueV, g_AdjustValueH);
    // Check if the file exists
    g_DebugEnabled = std::filesystem::exists(debug_file);
    
    if (g_DebugEnabled) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
    }
    DebugPrint("Hello, console!\n");
    hook_fire_fn();
}

int32_t hook_fn(std::wstring_view class_name, std::wstring_view fn_name, void* destination, void** original) {
    auto obj = (API::UClass*)API::get()->find_uobject(class_name);

    if (obj == nullptr) {
        PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", class_name.data());
        return -1;
    }

    auto fn = obj->find_function(fn_name);

    if (fn == nullptr) {
        PLUGIN_LOG_ONCE_ERROR("Failed to find %ls", fn_name.data());
        return -1;
    }

    auto native = fn->get_native_function();

    if (native == nullptr) {
        PLUGIN_LOG_ONCE_ERROR("Failed to get native function");
        return -1;
    }

    DebugPrint("%ls native: 0x%p", fn_name.data(), native);

    auto insn = utility::scan_disasm((uintptr_t)native, 0x1000, "E8 ? ? ? ?");

    if (!insn) {
        PLUGIN_LOG_ONCE_ERROR("Failed to find the instruction");
        DebugPrint("Not found");
        return -1;
    }

    uintptr_t insn_val = insn.value();

    auto relative_offset = *(int32_t*)(insn_val + 1);

    uintptr_t function_address = (insn_val + 5 ) + relative_offset;

    DebugPrint("Function address: 0x%p\n", (void*)function_address);

    return API::get()->param()->functions->register_inline_hook((void*)function_address, (void*)destination, original);
}

void MGS3Plugin::hook_fire_fn() {
    m_fire_hook_id = hook_fn(L"Class /Script/Gsr.GsrEquipController",L"Fire", on_get_fire, (void**)&m_fire_hook_fn);
}

void* MGS3Plugin::on_get_fire_internal(uevr::API::UObject* equipcontroller, FGsrFireRequest* FireRequest) {    
    
    DebugPrint("Function hooked\n");

    struct {
        API::UObject* value{nullptr};
    } obj;

    equipcontroller->call_function(L"GetCurrentWeapon", &obj);

    auto weapon = obj.value;

    if (weapon != nullptr) {
        const auto weapon_name = weapon->get_full_name();
        DebugPrint("Weapon: %ls\n", weapon_name.c_str());

        auto root_component = weapon->get_property<API::UObject*>(L"RootComponent");

        if (root_component != nullptr) {
            UEVR_Vector3d forward_params;

            root_component->call_function(L"GetForwardVector", &forward_params);
            DebugPrint("Forward params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

            FTransform muzzle_transform;

            weapon->call_function(L"GetMuzzleTransform", &muzzle_transform);
            auto location_params = muzzle_transform.Translation;
            DebugPrint("Location params: %f, %f, %f\n", location_params.x, location_params.y, location_params.z);

            FireRequest->FirePosition.x = location_params.x;
            FireRequest->FirePosition.y = location_params.y;
            FireRequest->FirePosition.z = location_params.z;
            
            float scaler = 8192.0;

            RotateAngleAxis rotate_params;
            static auto kismet_math_library_c = API::get()->find_uobject<API::UClass>(L"Class /Script/Engine.KismetMathLibrary");
            static auto kismet_math_library = kismet_math_library_c->get_class_default_object();

            rotate_params.InVect = forward_params;
            rotate_params.AngleDeg = 90.0f;
            root_component->call_function(L"GetUpVector", &rotate_params.Axis);
            DebugPrint("Up Vector: %f, %f, %f\n", rotate_params.Axis.x, rotate_params.Axis.y, rotate_params.Axis.z);
            
            kismet_math_library->call_function(L"RotateAngleAxis", &rotate_params);
            forward_params = rotate_params.Result;
            DebugPrint("Forward Params: %f, %f, %f\n", forward_params.x, forward_params.y, forward_params.z);

            DebugPrint("Old Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x,FireRequest->AimPosition.y, FireRequest->AimPosition.z);
            FireRequest->AimPosition.x = location_params.x + (forward_params.x * scaler);
            FireRequest->AimPosition.y = location_params.y + (forward_params.y * scaler) - g_AdjustValueH;
            FireRequest->AimPosition.z = location_params.z + (forward_params.z * scaler) - g_AdjustValueV;
            DebugPrint("New Aim Position: %f, %f, %f\n", FireRequest->AimPosition.x,FireRequest->AimPosition.y, FireRequest->AimPosition.z);
        }
    }


    //Call original function with our replaced parameters
    auto result = m_fire_hook_fn(equipcontroller, FireRequest);
    //PLUGIN_LOG_ONCE("Result: 0x%p", result);

    return result;
}

void MGS3Plugin::SetGunAdjustValuesFromConfig(const std::filesystem::path& ConfigFile) {
    // Check if the file exists
    if (!std::filesystem::exists(ConfigFile)) {
        g_AdjustValueV = 0.0f;
        g_AdjustValueH = 0.0f;
        return;
    }

    std::ifstream file(ConfigFile);
    std::string line;

    while (std::getline(file, line)) {
        // Trim leading and trailing spaces
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Skip comments and blank lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Look for the "GunAdjustV=" prefix
        if (line.rfind("GunAdjustV=", 0) == 0) {
            std::string value_str = line.substr(11); // Extract the value as a string
            char* end_ptr = nullptr;
            float value = std::strtof(value_str.c_str(), &end_ptr);

            // Validate conversion
            if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
                g_AdjustValueV = value; // Successfully parsed float
            } else {
                g_AdjustValueV = 0.0f; // Invalid float
            }
        }
        
        // Look for the "GunAdjustH=" prefix
        if (line.rfind("GunAdjustH=", 0) == 0) {
            std::string value_str = line.substr(11); // Extract the value as a string
            char* end_ptr = nullptr;
            float value = std::strtof(value_str.c_str(), &end_ptr);

            // Validate conversion
            if (end_ptr != value_str.c_str() && *end_ptr == '\0') {
                g_AdjustValueH = value; // Successfully parsed float
            } else {
                g_AdjustValueH = 0.0f; // Invalid float
            }
        }
    }
}