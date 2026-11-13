#pragma once
#include <algorithm>
#include <iterator>
#include "ue/Enums.hpp"

// How the plugin handles each player weapon
struct WeaponTraits
{
	bool mainHandShouldAttachToWeapon;
	bool canBeTwoHanded;
	// Held with two hands, the weapon points from the right hand to the left one
	bool twoHandedPivot;
	// Pivot: where the left hand holds the weapon, along the weapon's up axis
	float leftHandWeaponGrabOffset;
	// The game holds an object in the left hand with this weapon (CQC knife, TNT)
	bool hasObjectInLeftHand;
	// That object is the CQC knife: a dummy knife replaces it while the left hand holds the weapon
	bool holdCQCKnifeInLeftHand;
	bool hasScope;
};

// Indexed by EGsrEquipId
inline constexpr WeaponTraits weaponTraitsTable[] = {
	//                            main hand  two hands  pivot  grab offset  object in LH  CQC knife in LH  scope
	/* WP_None */               { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Knife */              { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Fork */               { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_CigarPistol */        { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_Handkerchief */       { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_Mk22 */               { true,      true,      false, 0.0f,        true,         true,            false },
	/* WP_Gove */               { true,      true,      false, 0.0f,        true,         true,            false },
	/* WP_EasyGun */            { true,      true,      false, 0.0f,        true,         true,            false },
	/* WP_SAArmy */             { true,      true,      false, 0.0f,        false,        false,           false },
	/* WP_PatriotPostol */      { true,      true,      true,  -4.68f,      false,        false,           false },
	/* WP_Scorpion */           { true,      true,      true,  -1.75f,      false,        false,           false },
	/* WP_M16a1 */              { true,      true,      true,  -4.90f,      false,        false,           false },
	/* WP_Akm */                { true,      true,      true,  -3.94f,      false,        false,           false },
	/* WP_M63 */                { true,      true,      true,  -3.59f,      false,        false,           false },
	/* WP_Ithaca */             { true,      true,      true,  1.91f,       false,        false,           false },
	/* WP_Dragnov */            { true,      true,      true,  -3.22f,      false,        false,           true },
	/* WP_MosinNagant */        { true,      true,      true,  -3.32f,      false,        false,           true },
	/* WP_Rpg */                { true,      true,      false, 0.0f,        false,        false,           true },
	/* WP_Torch */              { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_Grenade */            { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_FireGrenade */        { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_StunGrenade */        { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_ChaffGrenade */       { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_SmokeGrenade */       { true,      false,     false, 0.0f,        false,        false,           false },
	/* WP_Magazine */           { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Tnt */                { false,     false,     false, 0.0f,        true,         false,           false },
	/* WP_C3 */                 { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Claymore */           { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Book */               { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_RatTrap */            { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_Microphone */         { false,     false,     false, 0.0f,        false,        false,           false },
	/* WP_MAX_PLAYER_WEAPONS */ { false,     false,     false, 0.0f,        false,        false,           false },
};

// Food and other items (every ID from WP_MAX_PLAYER_WEAPONS up) share the last entry
inline const WeaponTraits& GetWeaponTraits(ue::EGsrEquipId id)
{
	static_assert(std::size(weaponTraitsTable) == (size_t)ue::EGsrEquipId::WP_MAX_PLAYER_WEAPONS + 1);
	return weaponTraitsTable[(size_t)std::min(id, ue::EGsrEquipId::WP_MAX_PLAYER_WEAPONS)];
}
