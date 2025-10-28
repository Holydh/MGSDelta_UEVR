#pragma once
#include "Utilities.hpp"
#include "uevr/API.hpp"
#include "MemoryManager.hpp"
#include "SettingsManager.hpp"
#include "Logger.hpp"
#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#define GLM_FORCE_QUAT_DATA_XYZW

#include "SDK/CoreUObject_structs.hpp"
#include "SDK/Engine_parameters.hpp"
#include "SDK/Engine_Classes.hpp"
#include "SDK/Gsr_parameters.hpp"
#include "SDK/UMG_classes.hpp"
#include "SDK/CobraUI_classes.hpp"

#include "CameraController.hpp"
#include "PlayerManager.hpp"

class WeaponController {
private:
	MemoryManager* const memoryManager;
	SettingsManager* const settingsManager;
	PlayerManager* const playerManager;

	void ApplyRecoil(float delta);
	float RandomRange(float min, float max);
	void spawnScope();
	void getRenderTarget(SDK::UWorld* world);
	void spawnScopeStaticMeshComponent();
	void spawnSceneCaptureComponent();
	void spawnScopeWidget();
	void attachScopeToWeapon();

	int cameraRenderTargetResolution = 1024;
public:
	WeaponController(MemoryManager* mm, SettingsManager* sm, PlayerManager* pm) : memoryManager(mm), settingsManager(sm), playerManager(pm) {};
	bool IsShooting = false;
	uevr::API::UObject* EquipController = nullptr;
	uevr::API::UObject* CurrentWeapon = nullptr;
	uevr::API::UObject* WeaponRoot = nullptr;
	SDK::USkeletalMeshComponent* ReceiverMesh = nullptr;

	SDK::AActor* ScopeActor = nullptr;
	SDK::UWidgetComponent* ScopeWidget = nullptr;

	SDK::UStaticMeshComponent* ScopeCylinderComponent = nullptr;
	SDK::UStaticMeshComponent* RedDotCylinderComponent = nullptr;
	SDK::USceneCaptureComponent2D* SceneCaptureComponent = nullptr;
	SDK::UStaticMeshComponent* SceneCaptureUStaticMeshComponent = nullptr;

	SDK::UTextureRenderTarget2D* sniperRenderTarget = nullptr;

	glm::fquat DefaultWeaponRotation{};
	glm::fvec3 DefaultWeaponPosition{};
	glm::fquat CurrentWeaponRotation{};
	glm::fvec3 CurrentWeaponRotationEuler{};
	glm::fvec3 CurrentWeaponPosition{};

	glm::dvec3 FirePosition{};
	glm::dvec3 AimPosition{};
	SDK::FRotator CurrentWeaponWorldRotation{};

	

	//glm::dvec3 AkmDefaultPosition{-6.5970001220703125,-4.460999965667725,-0.5180000066757202 };
	//glm::fquat AkmDefaultRotation{ -0.6113685965538025,-0.08241719752550125,0.7799869775772095,-0.10514845699071884 };
	//glm::dvec3 ChaffGrenadeDefaultPosition{ 0,0,0 };
	//glm::fquat ChaffGrenadeDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 CigarPistolDefaultPosition{ 0,0,0 };
	//glm::fquat CigarPistolDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 DragnovDefaultPosition{ 0,0,0 };
	//glm::fquat DragnovDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 EasyGunDefaultPosition{ 0,0,0 };
	//glm::fquat EasyGunDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 FireGrenadeDefaultPosition{ 0,0,0 };
	//glm::fquat FireGrenadeDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 GrenadeDefaultPosition{ 0,0,0 };
	//glm::fquat GrenadeDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 HandkerchiefDefaultPosition{ 0,0,0 };
	//glm::fquat HandkerchiefDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 M16a1DefaultPosition{ 0,0,0 };
	//glm::fquat M16a1DefaultRotation{ 1,0,0,0 };
	//glm::dvec3 IthacaDefaultPosition{ 0,0,0 };
	//glm::fquat IthacaDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 M63DefaultPosition{ 0,0,0 };
	//glm::fquat M63DefaultRotation{ 1,0,0,0 };
	//glm::dvec3 GoveDefaultPosition{ 0,0,0 };
	//glm::fquat GoveDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 Mk22DefaultPosition{ 0,0,0 };
	//glm::fquat Mk22DefaultRotation{ 1,0,0,0 };
	//glm::dvec3 MosinNagantDefaultPosition{ 0,0,0 };
	//glm::fquat MosinNagantDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 PatriotPistolDefaultPosition{ 0,0,0 };
	//glm::fquat PatriotPistolDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 RpgDefaultPosition{ 0,0,0 };
	//glm::fquat RpgDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 ScorpionDefaultPosition{ 0,0,0 };
	//glm::fquat ScorpionDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 SAArmyDefaultPosition{ 0,0,0 };
	//glm::fquat SAArmyDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 SmokeGrenadeDefaultPosition{ 0,0,0 };
	//glm::fquat SmokeGrenadeDefaultRotation{ 1,0,0,0 };
	//glm::dvec3 StunGrenadeDefaultPosition{ 0,0,0 };
	//glm::fquat StunGrenadeDefaultRotation{ 1,0,0,0 };


	enum EGsrEquipId : uint8_t
	{
		WP_None = 0,
		WP_Knife = 1,
		WP_Fork = 2,
		WP_CigarPistol = 3,
		WP_Handkerchief = 4,
		WP_Mk22 = 5,
		WP_Gove = 6,
		WP_EasyGun = 7,
		WP_SAArmy = 8,
		WP_PatriotPostol = 9,
		WP_Scorpion = 10,
		WP_M16a1 = 11,
		WP_Akm = 12,
		WP_M63 = 13,
		WP_Ithaca = 14,
		WP_Dragnov = 15,
		WP_MosinNagant = 16,
		WP_Rpg = 17,
		WP_Torch = 18,
		WP_Grenade = 19,
		WP_FireGrenade = 20,
		WP_StunGrenade = 21,
		WP_ChaffGrenade = 22,
		WP_SmokeGrenade = 23,
		WP_Magazine = 24,
		WP_Tnt = 25,
		WP_C3 = 26,
		WP_Claymore = 27,
		WP_Book = 28,
		WP_RatTrap = 29,
		WP_Microphone = 30,
		WP_MAX_PLAYER_WEAPONS = 31,
		WP_MAX_PLAYER = 31,
		FD_FoodSlot0 = 31,
		FD_FoodSlot1 = 32,
		FD_FoodSlot2 = 33,
		FD_FoodSlot3 = 34,
		FD_FoodSlot4 = 35,
		FD_FoodSlot5 = 36,
		FD_FoodSlot6 = 37,
		FD_FoodSlot7 = 38,
		FD_FoodSlot8 = 39,
		FD_FoodSlot9 = 40,
		FD_FoodSlot10 = 41,
		FD_FoodSlot11 = 42,
		FD_FoodSlot12 = 43,
		FD_FoodSlot13 = 44,
		FD_FoodSlot14 = 45,
		FD_FoodSlot15 = 46,
		FD_MAX_FOODSLOT_FOODS = 47,
		FD_MAX_FOODSLOT = 16,
		FD_AnimalSlot0 = 47,
		FD_AnimalSlot1 = 48,
		FD_AnimalSlot2 = 49,
		FD_MAX_ANIMALSLOT_FOODS = 50,
		FD_MAX_ANIMALSLOT = 19,
		WP_MAX_MENU_WEAPONS = 50,
		WP_MAX_MENU = 50,
		FD_FoodHebiA = 50,
		FD_FoodHebiB = 51,
		FD_FoodHebiC = 52,
		FD_FoodHebiD = 53,
		FD_FoodHebiE = 54,
		FD_FoodHebiF = 55,
		FD_FoodHebiG = 56,
		FD_FoodHebiH = 57,
		FD_FoodHebiI = 58,
		FD_FoodHebiJ = 59,
		FD_FoodHebiK = 60,
		FD_FoodCrocodile = 61,
		FD_FoodFrogA = 62,
		FD_FoodFrogB = 63,
		FD_FoodFrogC = 64,
		FD_FoodRat = 65,
		FD_FoodRabbit = 66,
		FD_FoodSquirrel = 67,
		FD_FoodMutton = 68,
		FD_FoodBat = 69,
		FD_FoodBeehive = 70,
		FD_FoodScorpion = 71,
		FD_FoodSpider = 72,
		FD_FoodBirdA = 73,
		FD_FoodBirdB = 74,
		FD_FoodBirdC = 75,
		FD_FoodBirdD = 76,
		FD_FoodBirdE = 77,
		FD_FoodFishA = 78,
		FD_FoodFishB = 79,
		FD_FoodFishC = 80,
		FD_FoodCrab = 81,
		FD_FoodMushroomA = 82,
		FD_FoodMushroomB = 83,
		FD_FoodMushroomC = 84,
		FD_FoodMushroomD = 85,
		FD_FoodMushroomE = 86,
		FD_FoodMushroomF = 87,
		FD_FoodMushroomG = 88,
		FD_FoodFruitA = 89,
		FD_FoodFruitB = 90,
		FD_FoodFruitC = 91,
		FD_FoodVegetableA = 92,
		FD_FoodNoodle = 93,
		FD_FoodRation = 94,
		FD_FoodPotato = 95,
		FD_FoodBeehivePain = 96,
		FD_FoodTutinoko = 97,
		FD_MAX_FOOD_FOODS = 98,
		FD_MAX_FOOD = 48,
		FD_AnimalHebiA = 98,
		FD_AnimalHebiB = 99,
		FD_AnimalHebiC = 100,
		FD_AnimalHebiD = 101,
		FD_AnimalHebiE = 102,
		FD_AnimalHebiF = 103,
		FD_AnimalHebiG = 104,
		FD_AnimalHebiH = 105,
		FD_AnimalHebiI = 106,
		FD_AnimalHebiJ = 107,
		FD_AnimalHebiK = 108,
		FD_AnimalCrocodile = 109,
		FD_AnimalFrogA = 110,
		FD_AnimalFrogB = 111,
		FD_AnimalFrogC = 112,
		FD_AnimalRat = 113,
		FD_AnimalRabbit = 114,
		FD_AnimalSquirrel = 115,
		FD_AnimalMutton = 116,
		FD_AnimalBat = 117,
		FD_AnimalBeehive = 118,
		FD_AnimalScorpion = 119,
		FD_AnimalSpider = 120,
		FD_AnimalBirdA = 121,
		FD_AnimalBirdB = 122,
		FD_AnimalBirdC = 123,
		FD_AnimalBirdD = 124,
		FD_AnimalBirdE = 125,
		FD_AnimalFishA = 126,
		FD_AnimalFishB = 127,
		FD_AnimalFishC = 128,
		FD_AnimalCrab = 129,
		FD_AnimalTutinoko = 130,
		FD_MAX_ANIMAL_FOODS = 131,
		FD_MAX_ANIMAL = 33,
		WP_MAX_WEAPONS = 131,
		WP_Mauser = 131,
		WP_Makarov = 132,
		WP_Amd65 = 133,
		WP_FlameThrower = 134,
		WP_FlameThrower2 = 135,
		WP_Tompson = 136,
		WP_Dshk = 137,
		WP_Body = 138,
		WP_AkmKnife = 139,
		WP_FpJet = 140,
		WP_CORE = 141,
		WP_LittleJoe = 142,
		WP_WilliamTell = 143,
		WP_Zu23 = 144,
		WP_Debeers = 145,
		WP_LargePlasma = 146,
		WP_DrumCan = 147,
		WP_GroundFire = 148,
		WP_MAX_ENEMY_WEAPONS = 149,
		WP_MAX_ENEMY = 99,
		WP_Kick = 149,
		WP_PunchLeft = 150,
		WP_PunchRight = 151,
		WP_Pendulum = 152,
		WP_BowTrap = 153,
		WP_BowTrap_Poison = 154,
		WP_CQCKnife = 155,
		WP_RollingAttack = 156,
		WP_DoorAttack = 157,
		WP_AnimalWeak = 158,
		WP_AnimalStrong = 159,
		WP_TntTrigger = 160,
		WP_CriticalKnife = 161,
		WP_ExplosionSmall = 162,
		WP_ExplosionMiddle = 163,
		WP_ExplosionLarge = 164,
		WP_Shield = 165,
		WP_Missile_AT_2 = 166,
		WP_Rocket_UB32 = 167,
		WP_12mmNoseGun = 168,
		WP_BridgeShake = 169,
		WP_Scop = 170,
		WP_Wireless = 171,
		WP_Wireless2 = 172,
		WP_LaserSight = 173,
		WP_BulletPunch = 174,
		WP_SmallPlasma = 175,
		WP_RollingDrum = 176,
		WP_SnakeBite = 177,
		WP_VehicleDamage = 178,
		WP_VehicleShake = 179,
		WP_FPLightBreak = 180,
		WP_BeeSting = 181,
		WP_VehicleNoDamage = 182,
		WP_CrocodileBite = 183,
		WP_Grass = 184,
		WP_DamDefense = 185,
		WP_Stomp = 186,
		WP_ShagoBodyAttack = 187,
		WP_SidecarAttack = 188,
		WP_BeePain_Weak = 189,
		WP_BeePain = 190,
		WP_BulletBee = 191,
		WP_BodyPlasma = 192,
		WP_BeeDark = 193,
		WP_Honey = 194,
		WP_Shago_Vulcan = 195,
		WP_Shago_100Pod = 196,
		WP_KickNoDamage = 197,
		WP_HagetakaEat = 198,
		WP_PunchBodyblow = 199,
		WP_PunchUpper = 200,
		WP_Mortar = 201,
		WP_PipeGas = 202,
		WP_PunchBodyblowL = 203,
		WP_BulletPunchL = 204,
		WP_PunchJab = 205,
		WP_WiredGrn = 206,
		WP_Strike = 207,
		WP_PainBody = 208,
		WP_PunchStrong = 209,
		WP_KickStrong = 210,
		WP_SaruRock = 211,
		WP_SaruPunch = 212,
		WP_SaruKick = 213,
		WP_Death = 214,
		WP_StinkingBlow = 215,
		WP_Sprinkler = 216,
		WP_GoldCrasher = 217,
		WP_DogBite = 219,
		WP_Wheel = 220,
		WP_BombermanBomb = 221,
		WP_MAX_OTHER_WEAPONS = 222,
		WP_MAX_OTHER = 73,
		EGsrEquipId_MAX = 223,
	};
	EGsrEquipId CurrentWeaponEquippedID = WP_None;

	inline const char* WeaponIdToKey(EGsrEquipId id);
	void FetchWeaponOffsets(EGsrEquipId currentWeapon,
		glm::fvec3& outPos,
		glm::fquat& outRot);

	//void Initialize();
	void FetchEquipController();
	void ProcessWeapon(float delta);
	void FetchWeaponMeshAimData();
	void ApplyAimDataToMemory();

	void SetAimDirection();

	void SwitchScopeState(bool enable);
};