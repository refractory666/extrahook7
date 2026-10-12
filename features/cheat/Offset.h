#pragma once

#include <cstdint>
#include "obf_nums.h"

namespace Offsets
{
	inline constexpr int BotTeamId = 255;
	namespace Il2CppString_
	{
		inline uintptr_t Length = OBF_UPTR(0x8u);
		inline uintptr_t Chars = OBF_UPTR(0xCu);
	}
	namespace Il2CppClass_
	{
		inline uintptr_t Parent = OBF_UPTR(0x24u);
		inline uintptr_t StaticFields = OBF_UPTR(0x50u);
	}
	namespace Il2CppStatics_
	{
		inline uintptr_t FromKlass = OBF_UPTR(0x5Cu);
	}
	namespace Il2CppArray_
	{
		inline uintptr_t Length = OBF_UPTR(0xCu);
		inline uintptr_t Data = OBF_UPTR(0x10u);
	}
	namespace ManagedDictValue_
	{
		inline uintptr_t Int32OrBoxed = OBF_UPTR(0x08u);
	}

	// =====================================================================
	// 1.0.0 Entry point: ClientWorld (njh) — replaces PlayerManager.
	// Found by heap scan for an object whose klass == *(libunity + ClientWorld_TypeInfo).
	// =====================================================================
	inline uintptr_t ClientWorld_TypeInfo = OBF_UPTR(0xA3B1720u);

	namespace ClientWorld_
	{
		inline uintptr_t Options = OBF_UPTR(0x0Cu);          // -> ClientWorldOptions
		inline uintptr_t InputCommand = OBF_UPTR(0x14u);     // -> xre (wrapper), +0x08 -> owv command
		inline uintptr_t Snapshots = OBF_UPTR(0x18u);       // -> aqn, +0x08 -> obn<yfq> ring buffer
		inline uintptr_t NetworkObjectRegistry = OBF_UPTR(0x20u); // -> gat (registry)
		inline uintptr_t Tick = OBF_UPTR(0x6Cu);            // uint tick
		inline uintptr_t Transport = OBF_UPTR(0x78u);        // -> hbu (transport), +0x68 -> stn (actor)
	}

	// ClientWorldOptions (at ClientWorld +0x0C)
	namespace ClientWorldOptions_
	{
		inline uintptr_t InterpolationBufferSeconds = OBF_UPTR(0x08u);
		inline uintptr_t InputPrediction = OBF_UPTR(0x0Cu); // bool
	}

	// Transport (hbu) — at ClientWorld +0x78
	namespace Transport_
	{
		inline uintptr_t LocalActor = OBF_UPTR(0x68u); // -> stn (our actor)
	}

	// =====================================================================
	// 2.2 Object registry and list — replaces PlayerManager cached dict.
	// njh+0x20 -> gat -> gat+0x18 -> gzx -> gzx+0x0C -> yjk<krm> array
	// =====================================================================

	// gat: registry of all network objects
	inline uintptr_t NetObjectRegistry_TypeInfo = OBF_UPTR(0xA3B0458u);
	namespace NetObjectRegistry_
	{
		inline uintptr_t World = OBF_UPTR(0x10u);    // -> njh (back-ref to world)
		inline uintptr_t Storage = OBF_UPTR(0x18u);  // -> gzx
	}

	// gzx: storage containing all network objects
	inline uintptr_t NetObjectStorage_TypeInfo = OBF_UPTR(0xA3B06B8u);
	namespace NetObjectStorage_
	{
		inline uintptr_t ObjectsList = OBF_UPTR(0x0Cu);  // -> yjk<krm> sorted list of ALL net objects
		// yjk<krm>: +0x08 array ptr, +0x0C count; array elements at +0x10 + i*4
	}

	// Dictionary<Type, dtq<krm>> — objects by type
	namespace NetObjectStorage_
	{
		inline uintptr_t ObjectsByType = OBF_UPTR(0x14u);
	}

	// krm: wrapper entry in the list (one per network object)
	// Array: pointers at +0x10 + i*4 (each points to an mfc-based object)
	inline uintptr_t NetObjectEntry_TypeInfo = OBF_UPTR(0xA3B1424u);

	// Object class TypeInfos — for filtering by klass
	inline uintptr_t Player_TypeInfo = OBF_UPTR(0xA3AF5DCu);   // ake — игрок
	inline uintptr_t Grenade_TypeInfo = OBF_UPTR(0xA3AFA88u);   // cim — летит
	inline uintptr_t GrenadeCol_TypeInfo = OBF_UPTR(0xA3C4B54u); // qak — коллизии
	inline uintptr_t Bomb_TypeInfo = OBF_UPTR(0xA3B372Cu);     // zno — бомба
	inline uintptr_t WeaponDrop_TypeInfo = OBF_UPTR(0xA3B171Cu); // njg — дроп
	inline uintptr_t Ragdoll_TypeInfo = OBF_UPTR(0xA3B1298u);   // lob — труп
	inline uintptr_t ModePlant_TypeInfo = OBF_UPTR(0xA3B1674u); // nbs — Закладка
	inline uintptr_t ModeRankedDefuse_TypeInfo = OBF_UPTR(0xA3B31ACu); // xlm
	inline uintptr_t ModeTDM_TypeInfo = OBF_UPTR(0xA3B3464u);  // ymn
	inline uintptr_t ModeDuel_TypeInfo = OBF_UPTR(0xA3B1EB4u); // qin
	inline uintptr_t ModeRankedDuel_TypeInfo = OBF_UPTR(0xA3B0824u); // hog

	// =====================================================================
	// 2.3 Base network object mfc (all net objects)
	// =====================================================================
	inline uintptr_t NetObject_TypeInfo = OBF_UPTR(0xA3B1424u);
	namespace NetObject_
	{
		// From il2cpp.h: NetObjectId_o* lpmx (pointer at +0x08 in _Fields = obj+0x08)
		// NetObjectId layout (when dereferenced): id(4)@0x08, OwnerId(byte)@0x0C, TypeId(byte)@0x0D, InstanceId(ushort)@0x0E
		// Note: must dereference pointer at +0x08 to access NetObjectId fields
		inline uintptr_t NetObjectIdPtr = OBF_UPTR(0x08u);  // pointer to NetObjectId* (boxed)
		// Helper: read OwnerId = Read(Read(net_obj+0x08) + 0x0C)
		inline uintptr_t OwnerId = OBF_UPTR(0x08u);  // pointer to NetObjectId, then +0x0C for OwnerId
		inline uintptr_t World = OBF_UPTR(0x10u);      // -> njh
		inline uintptr_t Systems = OBF_UPTR(0x14u);    // -> wmz
		inline uintptr_t Parts = OBF_UPTR(0x18u);      // -> rct (List<part>), +0x08 items, +0x0C size; +0x10 array
		inline uintptr_t ActivityFlags = OBF_UPTR(0x28u); // bool flags (0x28/0x29/0x2A)
	}

	// =====================================================================
	// 2.4 Player ake — TypeInfo 0xA3AF5DC
	// =====================================================================
	inline uintptr_t Player_TypeInfo_Real = OBF_UPTR(0xA3AF5DCu);
	namespace Player_
	{
		// ake inherits NetObject (mfc). NetObjectId is a pointer to boxed struct.
		// To read OwnerId: Read(ake + 0x08) -> netobj_ptr; Read(netobj_ptr + 0x0C) -> OwnerId
		inline uintptr_t NetObjectId = OBF_UPTR(0x08u);    // pointer to boxed NetObjectId
		inline uintptr_t OwnerId = OBF_UPTR(0x0Cu);        // byte -> ActorId (read via NetObject_::NetObjectIdPtr helper)
		inline uintptr_t PlayerConfig = OBF_UPTR(0x38u);   // PlayerObjectConfiguration (+0x18 CenterOffset)
		inline uintptr_t TransformPart = OBF_UPTR(0x40u);  // uint32_t -> dwc* (pointer)
		inline uintptr_t StatsPart = OBF_UPTR(0x44u);      // uint32_t -> ylw* (pointer)
		inline uintptr_t CameraLinks = OBF_UPTR(0x48u);    // uint32_t -> yyl* (pointer)
		inline uintptr_t State = OBF_UPTR(0x4Cu);          // byte-enum состояние игрока (жив/мёртв)
		inline uintptr_t Visibility = OBF_UPTR(0x50u);     // -> qdp
		inline uintptr_t Team = OBF_UPTR(0x54u);           // byte команда (1=TR 2=CT)
	}

	// PlayerObjectConfiguration (at ake +0x38)
	namespace PlayerObjectConfiguration_
	{
		inline uintptr_t CenterOffset = OBF_UPTR(0x18u); // Vector3
	}

	// =====================================================================
	// 2.5 Transform part dwc (ake+0x40) — TypeInfo 0xA3AFE84
	// =====================================================================
	inline uintptr_t TransformPart_TypeInfo = OBF_UPTR(0xA3AFE84u);
	namespace TransformPart_
	{
		inline uintptr_t RootTransform = OBF_UPTR(0x14u);   // Transform корня игрока (позиция ног)
		inline uintptr_t SecondTransform = OBF_UPTR(0x18u); // Transform второй (корень модели/камеры)
		inline uintptr_t PlayerBindings = OBF_UPTR(0x1Cu);  // -> PlayerBindings
		inline uintptr_t CharacterController = OBF_UPTR(0x20u);
		inline uintptr_t CapsuleCollider = OBF_UPTR(0x24u);
		inline uintptr_t Trigger = OBF_UPTR(0x28u);
	}

	// PlayerBindings — TypeInfo 0xA3BDD8C
	inline uintptr_t PlayerBindings_TypeInfo = OBF_UPTR(0xA3BDD8Cu);
	namespace PlayerBindings_
	{
		inline uintptr_t CameraPlaceholder = OBF_UPTR(0x18u); // -> Transform (точка глаз / держатель камеры)
	}

	// =====================================================================
	// 2.6 Model/Bones/Hitboxes jgi — from parts list
	// =====================================================================
	inline uintptr_t ModelPart_TypeInfo = OBF_UPTR(0xA3B0C64u);
	namespace ModelPart_
	{
		inline uintptr_t GameObject = OBF_UPTR(0x08u);
		inline uintptr_t HitBoxColliders = OBF_UPTR(0x0Cu);  // -> HitBoxCollider[]
		inline uintptr_t RootTransform = OBF_UPTR(0x10u);
		inline uintptr_t Animator = OBF_UPTR(0x14u);
		inline uintptr_t CharacterBindings = OBF_UPTR(0x18u);
		inline uintptr_t CharacterVariant = OBF_UPTR(0x1Cu); // вид персонажа
	}

	// CharacterBindings — TypeInfo 0xA3A87D0
	inline uintptr_t CharacterBindings_TypeInfo = OBF_UPTR(0xA3A87D0u);
	namespace CharacterBindings_
	{
		inline uintptr_t BonesDictionary = OBF_UPTR(0x10u); // -> BonesDictionary
		inline uintptr_t BodyMesh = OBF_UPTR(0x1Cu);       // SkinnedMeshRenderer тело
		inline uintptr_t HandMesh = OBF_UPTR(0x20u);       // SkinnedMeshRenderer руки
		inline uintptr_t GloveMesh = OBF_UPTR(0x24u);      // SkinnedMeshRenderer перчатки
		inline uintptr_t UntouchableMaterial = OBF_UPTR(0x28u); // Material неуязвимости
		inline uintptr_t OutlineMaterial = OBF_UPTR(0x2Cu);     // Material обводки
	}

	// HitBoxCollider (MonoBehaviour) — TypeInfo 0xA3D178C
	// Layout: +0x08 m_CachedPtr, +0x0C m_Enabled, +0x10 HitBox, +0x18 Collider, +0x1C owner
	inline uintptr_t HitBoxCollider_TypeInfo = OBF_UPTR(0xA3D178Cu);
	namespace HitBoxCollider_
	{
		inline uintptr_t HitBox = OBF_UPTR(0x10u);           // embedded struct: bone(+0x08), lossPenetrationPower(+0x0C)
		inline uintptr_t Bone = OBF_UPTR(0x18u);             // HumanBodyBones (inside HitBox at +0x08)
		inline uintptr_t LossPenetrationPower = OBF_UPTR(0x1Cu); // int (inside HitBox at +0x0C)
		inline uintptr_t Collider = OBF_UPTR(0x18u);         // Collider (after HitBox struct)
		inline uintptr_t Owner = OBF_UPTR(0x1Cu);            // владелец (rph)
	}

	// BonesDictionary = Dictionary<HumanBodyBones, Transform> — TypeInfo 0xA3A84C4
	// Dictionary 32-бит layout: +0x0C entries (массив), +0x10 count
	// entry 16 байт {int hash, int next, int key(HumanBodyBones), Transform value}, с +0x10 массива
	inline uintptr_t BonesDictionary_TypeInfo = OBF_UPTR(0xA3A84C4u);
	namespace BonesDictionary_
	{
		inline uintptr_t Entries = OBF_UPTR(0x0Cu);
		inline uintptr_t Count = OBF_UPTR(0x10u);
	}

	// =====================================================================
	// 2.7 Stats ylw (ake+0x44) — TypeInfo 0xA3B344C
	// =====================================================================
	inline uintptr_t StatsPart_TypeInfo = OBF_UPTR(0xA3B344Cu);
	namespace StatsPart_
	{
		// +0x08 +0x0A +0x0C +0x0E +0x10 +0x12  ushort x6 (HP, maxHP, броня, maxБроня ...)
		inline uintptr_t Health = OBF_UPTR(0x08u);   // ushort HP
		inline uintptr_t MaxHealth = OBF_UPTR(0x0Au);
		inline uintptr_t Armor = OBF_UPTR(0x0Cu);    // ushort броня
		inline uintptr_t MaxArmor = OBF_UPTR(0x0Eu);
		inline uintptr_t Field_10 = OBF_UPTR(0x10u);
		inline uintptr_t Field_12 = OBF_UPTR(0x12u);
		inline uintptr_t Helmet = OBF_UPTR(0x14u);   // bool шлем
		inline uintptr_t Field_18 = OBF_UPTR(0x18u); // float
	}

	// UntouchableStatusComponent tbq — TypeInfo 0xA3B2580 (spawned-immunity)
	inline uintptr_t UntouchableStatus_TypeInfo = OBF_UPTR(0xA3B2580u);
	namespace UntouchableStatus_
	{
		inline uintptr_t IsUntouchable = OBF_UPTR(0x08u); // bool неуязвим
		inline uintptr_t Field_09 = OBF_UPTR(0x09u);
		inline uintptr_t Field_0A = OBF_UPTR(0x0Au);
		inline uintptr_t Field_0C = OBF_UPTR(0x0Cu); // float
		inline uintptr_t Field_10 = OBF_UPTR(0x10u); // float (время/длительность)
	}

	// =====================================================================
	// 2.8 Visibility qdp (ake+0x50) — TypeInfo 0xA3B1E3C
	// =====================================================================
	inline uintptr_t Visibility_TypeInfo = OBF_UPTR(0xA3B1E3Cu);
	namespace Visibility_
	{
		inline uintptr_t Field_08 = OBF_UPTR(0x08u); // bool
		inline uintptr_t Field_09 = OBF_UPTR(0x09u); // byte-enum
		inline uintptr_t Field_0A = OBF_UPTR(0x0Au); // bool
		inline uintptr_t Field_0B = OBF_UPTR(0x0Bu);
		inline uintptr_t Field_0C = OBF_UPTR(0x0Cu);
		inline uintptr_t Field_0D = OBF_UPTR(0x0Du);
		// +0x10 Bounds, +0x28 float3, +0x34 float3[] точки
	}

	// =====================================================================
	// 2.9 Actors: ник, свой игрок
	// =====================================================================
	// stn actor — TypeInfo 0xA3B24B0
	inline uintptr_t Actor_TypeInfo = OBF_UPTR(0xA3B24B0u);
	namespace Actor_
	{
		inline uintptr_t ActorId = OBF_UPTR(0x08u);  // byte ActorId (== ake.OwnerId -> это мы)
		inline uintptr_t Nickname = OBF_UPTR(0x0Cu); // string ник
		inline uintptr_t Field_10 = OBF_UPTR(0x10u); // bool
		inline uintptr_t IsBot = OBF_UPTR(0x11u);    // bool (бот?)
		inline uintptr_t UID = OBF_UPTR(0x20u);      // long uid
	}

	// =====================================================================
	// 2.10 Weapon nrk (part of player)
	// =====================================================================
	inline uintptr_t WeaponPart_TypeInfo = OBF_UPTR(0xA3B1804u);
	namespace WeaponPart_
	{
		inline uintptr_t Slots = OBF_UPTR(0x10u);         // -> kyz (kyz+0x08 dtq<qjf>)
		inline uintptr_t WeaponList = OBF_UPTR(0x14u);     // -> vtl (+0x08 List<qjf>)
		inline uintptr_t WeaponContext = OBF_UPTR(0x18u);  // встроено; относительные оффсеты ниже
	}

	// WeaponContext — allocated on the heap (nrk holds a pointer at +0x18).
	// All offsets relative to WeaponContext object base (after Il2CppObject header).
	// From dump.cs (verified field order). Each *field* below is a pointer unless noted.
	// deref needed: Owner, NavigationContext, AccuracyContext, RecoilContext, ActiveWeapon, WeaponState, AimState, RecoilState, ReloadState
	namespace WeaponContext_
	{
		inline uintptr_t Owner = OBF_UPTR(0x08u);           // NetObjectId* (deref for fields @+0x0C OwnerId)
		inline uintptr_t Team = OBF_UPTR(0x10u);            // byte (1=TR 2=CT)
		inline uintptr_t HitTransform = OBF_UPTR(0x14u);   // Transform*
		inline uintptr_t NavigationContext = OBF_UPTR(0x18u);  // NavContext* (deref, see NavigationContext_)
		inline uintptr_t AccuracyContext = OBF_UPTR(0x38u);    // AccuracyContext* (deref, float@+0x08/+0x0C)
		inline uintptr_t RecoilContext = OBF_UPTR(0x40u);      // RecoilContext* (deref, float@+0x08)
		inline uintptr_t ActiveWeapon = OBF_UPTR(0x44u);       // qjf* (pointer to weapon instance)
		inline uintptr_t WeaponState = OBF_UPTR(0x48u);       // State<WeaponState>* (boxed, mode@+0x08)
		inline uintptr_t AimState = OBF_UPTR(0x54u);          // State<Aim>* (boxed)
		inline uintptr_t RecoilState = OBF_UPTR(0x60u);        // RecoilState* (deref, see RecoilState_)
		inline uintptr_t ReloadState = OBF_UPTR(0x6Cu);       // State<ReloadState>* (boxed)
		inline uintptr_t CurrentTime = OBF_UPTR(0x70u);       // float
	}

	/// NavigationContext — pointer at WeaponContext+0x20, must dereference.
	/// Inner fields (verified from dump.cs): IsCrouch@0x8, IsJump@0x9, Speed@0x0C, Position@0x10, Velocity@0x1C
	namespace NavigationContext_
	{
		inline uintptr_t Crouch = OBF_UPTR(0x08u);    // bool
		inline uintptr_t Jump = OBF_UPTR(0x09u);      // bool
		inline uintptr_t Speed = OBF_UPTR(0x0Cu);     // float
		inline uintptr_t Position = OBF_UPTR(0x10u); // Vector3
		inline uintptr_t Velocity = OBF_UPTR(0x1Cu);  // Vector3
	}

	/// RecoilState — pointer at WeaponContext+0x60, must dereference.
	/// Inner fields (verified from dump.cs): Progress@0x8, PrevProgress@0x0C,
	///   PrevPoint@0x10(Vector2), ActualPoint@0x18(Vector2), ApproachDeltaDist@0x20, LastShotTime@0x24
	namespace RecoilState_
	{
		inline uintptr_t Progress = OBF_UPTR(0x08u);
		inline uintptr_t PrevProgress = OBF_UPTR(0x0Cu);
		inline uintptr_t PrevPoint = OBF_UPTR(0x10u);  // Vector2* (pointer, deref for x@+0x08, y@+0x0C)
		inline uintptr_t ActualPoint = OBF_UPTR(0x18u); // Vector2* (pointer, deref for x@+0x08, y@+0x0C)
		inline uintptr_t ApproachDeltaDist = OBF_UPTR(0x20u);
		inline uintptr_t LastShotTime = OBF_UPTR(0x24u);
	}

	// WeaponState enum (byte)
	namespace WeaponState_
	{
		inline constexpr uint8_t Idle = 0;
		inline constexpr uint8_t Attack = 1;
		inline constexpr uint8_t Reload = 2;
		inline constexpr uint8_t Inspect = 3;
		inline constexpr uint8_t Take = 4;
		inline constexpr uint8_t Charge = 5;
	}

	// qjf weapon instance — TypeInfo 0xA3B1ED0
	inline uintptr_t WeaponInstance_TypeInfo = OBF_UPTR(0xA3B1ED0u);
	namespace WeaponInstance_
	{
		inline uintptr_t WeaponId = OBF_UPTR(0x08u);  // byte
		inline uintptr_t MetaData = OBF_UPTR(0x10u);  // -> WeaponMetaData (OwnerId@0x08, SkinId@0x0C, CharmId@0x10, SkinPatternId@0x38)
		inline uintptr_t MetaId = OBF_UPTR(0x48u);
		inline uintptr_t Field_4A = OBF_UPTR(0x4Au);  // byte
		inline uintptr_t AmmoInfo = OBF_UPTR(0x4Cu);  // ushort (патроны?)
		inline uintptr_t Field_50 = OBF_UPTR(0x50u);  // float
	}

	// WeaponSnapshot (network) — TypeInfo not specified in dump
	namespace WeaponSnapshot_
	{
		inline uintptr_t Id = OBF_UPTR(0x08u);          // byte
		inline uintptr_t MetaId = OBF_UPTR(0x0Cu);     //
		inline uintptr_t Ammo = OBF_UPTR(0x10u);       // short
		inline uintptr_t RestAmmo = OBF_UPTR(0x12u);    // short
		inline uintptr_t LastAttackTime = OBF_UPTR(0x14u); // float
		inline uintptr_t Slot = OBF_UPTR(0x18u);       // byte
	}

	// WeaponMetaData (embedded in qjf at +0x10) — offsets relative to WeaponMetaData start
	// From dump.cs: OwnerId@0x08, SkinId@0x0C, CharmId@0x10, SkinPatternId@0x38
	namespace WeaponMetaData_
	{
		inline uintptr_t OwnerId = OBF_UPTR(0x08u);   // byte
		inline uintptr_t SkinId = OBF_UPTR(0x0Cu);    // int (СКИНЧЕНДЖЕР)
		inline uintptr_t CharmId = OBF_UPTR(0x10u);   // int
		inline uintptr_t SkinPatternId = OBF_UPTR(0x38u); // int
	}

	// Weapon provider ijq — TypeInfo 0xA3B0A04, +0x0C Dictionary<WeaponId, definition>
	inline uintptr_t WeaponProvider_TypeInfo = OBF_UPTR(0xA3B0A04u);
	// WeaponDefinition — TypeInfo 0xA3C7070
	inline uintptr_t WeaponDefinition_TypeInfo = OBF_UPTR(0xA3C7070u);
	namespace WeaponDefinition_
	{
		inline uintptr_t WeaponId = OBF_UPTR(0x0Cu);
		inline uintptr_t Name = OBF_UPTR(0x10u);      // string
		inline uintptr_t Parameters = OBF_UPTR(0x1Cu); // -> WeaponParameters
	}

	// WeaponParameters (контент) +0x0C слот, +0x10 категория, +0x18 ushort цена
	namespace WeaponParameters_
	{
		inline uintptr_t Slot = OBF_UPTR(0x0Cu);
		inline uintptr_t Category = OBF_UPTR(0x10u);
		inline uintptr_t Price = OBF_UPTR(0x18u); // ushort
	}
	// GunParameters (наследник WeaponBuiltIn) — TypeInfo 0xA3AA5B8
	inline uintptr_t GunParameters_TypeInfo = OBF_UPTR(0xA3AA5B8u);
	namespace GunParameters_
	{
		inline uintptr_t RecoilControl = OBF_UPTR(0x1Cu);
		inline uintptr_t AccurateRange = OBF_UPTR(0x20u);
		inline uintptr_t MovementRate = OBF_UPTR(0x24u);
		inline uintptr_t Capacity = OBF_UPTR(0x28u);    // ushort
		inline uintptr_t MagazineCapacity = OBF_UPTR(0x2Au); // byte
		inline uintptr_t ReloadDuration = OBF_UPTR(0x2Cu);   // float
		inline uintptr_t Rate = OBF_UPTR(0x3Cu);        // short скорострельность
		inline uintptr_t Distance = OBF_UPTR(0x40u);    // float
		inline uintptr_t WeaponDamage = OBF_UPTR(0x4Cu); // -> WeaponDamage
		inline uintptr_t RecoilParameters = OBF_UPTR(0x50u);
		inline uintptr_t AccuracyParameters = OBF_UPTR(0x54u);
	}

	// WeaponDamage — content (nested in GunParameters at +0x4C)
	namespace WeaponDamage_
	{
		inline uintptr_t HeadDamage = OBF_UPTR(0x08u);      // short
		inline uintptr_t ChestAndArmsDamage = OBF_UPTR(0x0Au); // short
		inline uintptr_t StomachDamage = OBF_UPTR(0x0Cu);   // short
		inline uintptr_t LegsDamage = OBF_UPTR(0x0Eu);      // short
		inline uintptr_t DamageCurve = OBF_UPTR(0x10u);     // кривая урона по дистанции
		inline uintptr_t ArmorCurve = OBF_UPTR(0x14u);      // кривая брони по дистанции
		inline uintptr_t PenetrationPower = OBF_UPTR(0x1Cu); // int
		inline uintptr_t ArmorPenetration = OBF_UPTR(0x20u); // int
	}

	namespace RecoilParameters_
	{
		inline uintptr_t RecoilMultOnCrouch = OBF_UPTR(0x08u);
		inline uintptr_t RecoilAimMult = OBF_UPTR(0x0Cu);
		inline uintptr_t RecoilAimMultOnCrouch = OBF_UPTR(0x10u);
		inline uintptr_t HorizontalRange = OBF_UPTR(0x14u); // float
		inline uintptr_t VerticalRange = OBF_UPTR(0x18u);   // float
		inline uintptr_t CameraDeviationCoeff = OBF_UPTR(0x38u);
		inline uintptr_t MaxApproachSpeed = OBF_UPTR(0x3Cu);
		inline uintptr_t ProgressFillingShotsCount = OBF_UPTR(0x48u); // int
	}

	namespace AccuracyParameters_
	{
		inline uintptr_t AccuracyAimMult = OBF_UPTR(0x08u);
		inline uintptr_t AccuracyMultOnCrouch = OBF_UPTR(0x0Cu);
		inline uintptr_t AccuracyAimMultOnCrouch = OBF_UPTR(0x10u);
		inline uintptr_t AccuracyMultOnJump = OBF_UPTR(0x14u);
		inline uintptr_t SpreadCurve1 = OBF_UPTR(0x18u);
		inline uintptr_t SpreadCurve2 = OBF_UPTR(0x1Cu);
	}

	// =====================================================================
	// 2.11 Input / buttons / command znn
	// =====================================================================
	inline uintptr_t InputManager_TypeInfo = OBF_UPTR(0xA3B3728u);
	namespace InputManager_
	{
		inline uintptr_t AxisInputs = OBF_UPTR(0x0Cu);     // -> yjk<InputEntry<AxisInput>> ("Look", "Move")
		inline uintptr_t ButtonInputs = OBF_UPTR(0x10u);   // -> yjk<InputEntry<ButtonInput>> ("Attack","Jump","Crouch","Scope","Reload")
	}

	// ButtonInput — в боксе (+0x8/+0x9/+0xA), т.к. обращение через указатель на boxed-объект
	namespace ButtonInput_
	{
		inline uintptr_t WasPressedInThisFrame = OBF_UPTR(0x08u);
		inline uintptr_t WasReleasedInThisFrame = OBF_UPTR(0x09u);
		inline uintptr_t IsPressed = OBF_UPTR(0x0Au);
	}

	// AxisInput Vector2 Value
	namespace AxisInput_
	{
		inline uintptr_t Value = OBF_UPTR(0x00u); // Vector2
	}

	// InputEntry<T> = {string имя, T значение}
	// Layout: +0x08 string name, +0x10 T value (ButtonInput at +0x08/+0x9/+0xA bools in box)
	// Command owv (отправляется серверу каждый тик) — TypeInfo 0xA3B1AF8
	inline uintptr_t InputCommand_TypeInfo = OBF_UPTR(0xA3B1AF8u);
	namespace InputCommand_
	{
		inline uintptr_t Tick = OBF_UPTR(0x08u);          // uint тик
		inline uintptr_t Field_0C = OBF_UPTR(0x0Cu);     // float (база xkg)
		inline uintptr_t Field_10 = OBF_UPTR(0x10u);     // bool
		inline uintptr_t LookAxisNullable = OBF_UPTR(0x14u); // Nullable<Vector2>
		inline uintptr_t LookAxis = OBF_UPTR(0x20u);    // Vector2 (углы/направление взгляда)
		// +0x28..+0x36 bool-кнопки (выстрел, прицел, прыжок, присед...)
		inline uintptr_t ButtonsStart = OBF_UPTR(0x28u);
		inline uintptr_t TargetNetObjectId = OBF_UPTR(0x38u); // Nullable<NetObjectId>
	}

	// Текущая команда: njh +0x14 (xre) +0x08.
	// =====================================================================
	// 2.12 Camera / angles / FOV
	// =====================================================================
	// ios camera component — TypeInfo 0xA3B0A90
	inline uintptr_t CameraComponent_TypeInfo = OBF_UPTR(0xA3B0A90u);
	namespace CameraComponent_
	{
		inline uintptr_t RotationPart = OBF_UPTR(0x1Cu);   // -> eir (поворот)
		inline uintptr_t PlayerStats = OBF_UPTR(0x20u);    // -> ylw
		inline uintptr_t CameraLinks = OBF_UPTR(0x24u);    // -> yyl
		inline uintptr_t CameraTransform = OBF_UPTR(0x28u); // Transform
		inline uintptr_t CameraHolderTransform = OBF_UPTR(0x2Cu); // Transform (держатель/камера)
	}

	// eir rotation part — TypeInfo 0xA3AFFFC
	inline uintptr_t RotationPart_TypeInfo = OBF_UPTR(0xA3AFFFCu);
	namespace RotationPart_
	{
		inline uintptr_t ViewRotation = OBF_UPTR(0x08u);  // Quaternion (взгляд / view)
		inline uintptr_t BodyRotation = OBF_UPTR(0x18u);  // Quaternion (тело / body)
	}

	// nrl camera controller (LateTick) — TypeInfo 0xA3CD204
	inline uintptr_t CameraController_TypeInfo = OBF_UPTR(0xA3CD204u);
	namespace CameraController_
	{
		inline uintptr_t Transform = OBF_UPTR(0x0Cu);
		inline uintptr_t Camera = OBF_UPTR(0x10u); // managed +0x08 -> native; матрица по оффсету в native]
	}

	// Native camera matrix offset — Unity 2019+ (перепроверить для новой Unity)
	namespace NativeCamera_
	{
		inline uintptr_t MatrixOffset = OBF_UPTR(0xE0u);  // view matrix в native Camera
		inline uintptr_t FieldOfView = OBF_UPTR(0xDCu);  // native camera fieldOfView (новая Unity)
	}

	// PlayerCameraConfiguration (TypeInfo not in dump — find via scan or DI)
	namespace PlayerCameraConfiguration_
	{
		inline uintptr_t StandCameraHeight = OBF_UPTR(0x0Cu);
		inline uintptr_t CrouchCameraHeight = OBF_UPTR(0x10u);
	}

	// NavigationComponentConfiguration
	namespace NavigationComponentConfiguration_
	{
		inline uintptr_t MinX = OBF_UPTR(0x10u);  // _maximumX
		inline uintptr_t MaxX = OBF_UPTR(0x14u);  // _minimumX
		inline uintptr_t HeadDirectiveAngles = OBF_UPTR(0x18u); // Vector3
	}

	// =====================================================================
	// 2.13 Movement (bunny hop, speed) — nip
	// =====================================================================
	inline uintptr_t NavigationPart_TypeInfo = OBF_UPTR(0xA3B1710u);
	namespace NavigationPart_
	{
		inline uintptr_t Transforms = OBF_UPTR(0x18u);
		inline uintptr_t Config = OBF_UPTR(0x1Cu);
		inline uintptr_t CharacterController = OBF_UPTR(0x20u);
		inline uintptr_t MovementState = OBF_UPTR(0x24u); // -> tof
		inline uintptr_t StatsPart = OBF_UPTR(0x30u);    // -> ylw
	}

	// tof movement state — TypeInfo 0xA3B26C8
	inline uintptr_t MovementState_TypeInfo = OBF_UPTR(0xA3B26C8u);
	namespace MovementState_
	{
		inline uintptr_t Position = OBF_UPTR(0x08u);  // Vector3 (позиция/скорость)
		inline uintptr_t Velocity1 = OBF_UPTR(0x14u); // Vector3
		inline uintptr_t Velocity2 = OBF_UPTR(0x20u); // Vector3 (позиция/скорость)
		// +0x2C..+0x48 float x8
		inline uintptr_t HitDelayStart = OBF_UPTR(0x4Cu); // HitDuration
		inline uintptr_t HitDelayActive = OBF_UPTR(0x50u); // HitAffect
		inline uintptr_t OnGround = OBF_UPTR(0x54u); // bool (на земле)
		inline uintptr_t Crouching = OBF_UPTR(0x55u); // bool (присед)
		inline uintptr_t Field_58 = OBF_UPTR(0x58u); // float
		inline uintptr_t Field_5C = OBF_UPTR(0x5Cu); // float
	}

	// PlayerTranslationParameters (скорости, прыжок)
	namespace PlayerTranslationParameters_
	{
		inline uintptr_t G = OBF_UPTR(0x08u);              // гравитация
		inline uintptr_t TargetSpeed = OBF_UPTR(0x18u);
		inline uintptr_t AirSpeed = OBF_UPTR(0x44u);
		inline uintptr_t BunnyJumpMaxSpeed = OBF_UPTR(0x48u);
		inline uintptr_t JumpHeight = OBF_UPTR(0x4Cu);
	}

	// =====================================================================
	// 2.14 Grenades — cim (летит), qak (коллизии)
	// =====================================================================
	// cim grenade — TypeInfo 0xA3AFA88
	inline uintptr_t Grenade_TypeInfo_Real = OBF_UPTR(0xA3AFA88u);
	namespace Grenade_
	{
		inline uintptr_t TransformPart = OBF_UPTR(0x38u); // часть трансформа
		inline uintptr_t ExplosionData = OBF_UPTR(0x40u); // -> jar: +0x08 NetObjectId бросившего, +0x10 float время, +0x14 WeaponId
	}

	// qak grenade (collisions) — TypeInfo 0xA3C4B54
	inline uintptr_t GrenadeCollision_TypeInfo = OBF_UPTR(0xA3C4B54u);
	namespace GrenadeCollision_
	{
		inline uintptr_t TransformPart = OBF_UPTR(0x38u); // -> aih: +0x08 NetObjectId бросившего, +0x10 WeaponId, +0x14 float время
		inline uintptr_t Position = OBF_UPTR(0x3Cu);      // -> uyb: +0x0C Transform (позиция)
	}

	// GrenadeParameters
	namespace GrenadeParameters_
	{
		inline uintptr_t NetObjectTypeId = OBF_UPTR(0x1Bu); // byte
		inline uintptr_t Lifetime = OBF_UPTR(0x40u);        // float
	}
	// Smoke: GrenadeSmokeParameters +0x50 DetonationTime, +0x54 BlindRadius
	namespace GrenadeSmokeParameters_
	{
		inline uintptr_t DetonationTime = OBF_UPTR(0x50u);
		inline uintptr_t BlindRadius = OBF_UPTR(0x54u);
	}
	// Molotov: GrenadeFlameParameters +0x5C Radius
	namespace GrenadeFlameParameters_
	{
		inline uintptr_t Radius = OBF_UPTR(0x5Cu);
	}

	// =====================================================================
	// 2.15 Bomb — zno
	// =====================================================================
	inline uintptr_t Bomb_TypeInfo_Real = OBF_UPTR(0xA3B372Cu);
	namespace Bomb_
	{
		inline uintptr_t ExplosionData = OBF_UPTR(0x38u); // -> ogn
	}
	namespace BombExplosion_
	{
		inline uintptr_t CarrierNetObjectId = OBF_UPTR(0x08u);  // NetObjectId (носитель/заложивший)
		inline uintptr_t Field_10 = OBF_UPTR(0x10u);  // float
		inline uintptr_t Field_14 = OBF_UPTR(0x14u);  // float
		inline uintptr_t Field_18 = OBF_UPTR(0x18u);  // float (таймер детонации / дефуза)
		inline uintptr_t IsPlanted = OBF_UPTR(0x1Cu); // bool заложена
		inline uintptr_t IsDefusing = OBF_UPTR(0x1Du); // bool дефузится
		inline uintptr_t DefuserNetObjectId = OBF_UPTR(0x20u); // NetObjectId дефузящий
		inline uintptr_t Position = OBF_UPTR(0x28u);  // Vector3 ПОЗИЦИЯ
		inline uintptr_t Rotation = OBF_UPTR(0x34u);   // Quaternion
		inline uintptr_t PathPoints = OBF_UPTR(0x44u); // List<Vector3>
	}

	// BombParameters (WeaponBuiltIn) +0x24 float DetonateTime, +0x2C WeaponDamage, +0x30 float Distance
	namespace BombParameters_
	{
		inline uintptr_t DetonateTime = OBF_UPTR(0x24u);
		inline uintptr_t Damage = OBF_UPTR(0x2Cu);
		inline uintptr_t Distance = OBF_UPTR(0x30u);
	}

	// =====================================================================
	// 2.16 Weapon drop — njg
	// =====================================================================
	inline uintptr_t WeaponDrop_TypeInfo = OBF_UPTR(0xA3B171Cu);
	namespace WeaponDrop_
	{
		inline uintptr_t PhysicsPart = OBF_UPTR(0x38u); // физика (интерфейс: позиция/поворот)
		inline uintptr_t VisualName = OBF_UPTR(0x3Cu); // string имя, ValueTransform
		inline uintptr_t ItemData = OBF_UPTR(0x40u);   // -> vsv: +0x1C Vector3, +0x28 Vector3, +0x34 WeaponId, +0x38 int (патроны?)
		inline uintptr_t Visibility = OBF_UPTR(0x44u); // -> qdp
	}

	// =====================================================================
	// 2.17 Map / mode — GlobalOptions
	// =====================================================================
	inline uintptr_t GlobalOptions_TypeInfo = OBF_UPTR(0xA3AA368u);
	namespace GlobalOptions_
	{
		inline uintptr_t GameId = OBF_UPTR(0x08u);      // string
		inline uintptr_t IP = OBF_UPTR(0x0Cu);           // string
		inline uintptr_t Port = OBF_UPTR(0x10u);         // int
		inline uintptr_t GameMode = OBF_UPTR(0x18u);     // string
		inline uintptr_t Region = OBF_UPTR(0x1Cu);       // string
		inline uintptr_t LevelId = OBF_UPTR(0x20u);      // string ИМЯ КАРТЫ
		inline uintptr_t IsPublicGame = OBF_UPTR(0x24u); // bool
		inline uintptr_t IsRankedGame = OBF_UPTR(0x25u); // bool
	}
	// Find: RootLifetimeScope (TypeInfo 0xA3CAC28) +0x3C -> GlobalOptions, or heap scan by class.
	inline uintptr_t RootLifetimeScope_TypeInfo = OBF_UPTR(0xA3CAC28u);
	namespace RootLifetimeScope_
	{
		inline uintptr_t GlobalOptions = OBF_UPTR(0x3Cu);
	}

	// =====================================================================
	// 2.18 Skins — in-match weapon instance qjf
	// WeaponMetaData is accessed via pointer at qjf +0x10.
	// Use WeaponMetaData_:: namespace for offsets within WeaponMetaData.
	// =====================================================================


	// =====================================================================
	// 2.19 Damage / hit-log (for events)
	// =====================================================================
	// DamageSnapshot
	namespace DamageSnapshot_
	{
		inline uintptr_t Damage = OBF_UPTR(0x08u);       // int
		inline uintptr_t Producer = OBF_UPTR(0x0Cu);     // NetObjectId (кто стрелял)
		inline uintptr_t WeaponId = OBF_UPTR(0x14u);
		inline uintptr_t Bone = OBF_UPTR(0x18u);         // HumanBodyBones
		inline uintptr_t HitPoint = OBF_UPTR(0x1Cu);     // Vector3
		inline uintptr_t ShotPoint = OBF_UPTR(0x28u);    // Vector3
		inline uintptr_t Penetrated = OBF_UPTR(0x34u);   // bool
		inline uintptr_t Time = OBF_UPTR(0x38u);         // float
	}

	// RagdollInstantiateArgs (смерть)
	namespace RagdollInstantiateArgs_
	{
		inline uintptr_t KillerNetObjectId = OBF_UPTR(0x0Cu);
		inline uintptr_t Victim = OBF_UPTR(0x1Cu);
		inline uintptr_t LastHitBone = OBF_UPTR(0x44u);
		// +0x7C/0x7D/0x7E попадания голова/тело/ноги
		inline uintptr_t HeadHit = OBF_UPTR(0x7Cu);
		inline uintptr_t BodyHit = OBF_UPTR(0x7Du);
		inline uintptr_t LegsHit = OBF_UPTR(0x7Eu);
	}

	// =====================================================================
	// Il2Cpp core structures (32-bit ARM)
	// =====================================================================
	namespace Il2Cpp_
	{
		inline uintptr_t ObjectHeaderSize = OBF_UPTR(0x8u);  // klass + monitor before fields (object, not array)
		inline uintptr_t StringLength = OBF_UPTR(0x8u);     // +0x8 длина, +0xC UTF-16
		inline uintptr_t StringChars = OBF_UPTR(0xCu);
		inline uintptr_t ArrayLength = OBF_UPTR(0xCu);      // +0xC длина
		inline uintptr_t ArrayData = OBF_UPTR(0x10u);       // данные с +0x10 (32-bit ARM: 8-byte header + 4-byte length + 4-byte padding)
	}

	// List<T>: +0x8 items, +0xC size
	namespace Il2CppList_
	{
		inline uintptr_t Items = OBF_UPTR(0x08u);
		inline uintptr_t Size = OBF_UPTR(0x0Cu);
		inline uintptr_t Count = OBF_UPTR(0x0Cu);
	}

	// Dictionary<TKey, TValue> 32-bit layout
	namespace Il2CppDictionary_
	{
		// Dictionary 32-бит layout (верифицировано из дампа):
		inline uintptr_t Entries = OBF_UPTR(0x0Cu);  // массив entries
		inline uintptr_t Count = OBF_UPTR(0x10u);    // count
		// entry 16 байт {int hash, int next, TKey key, TValue value}
		inline uintptr_t EntryHash = OBF_UPTR(0x00u);
		inline uintptr_t EntryNext = OBF_UPTR(0x04u);
		inline uintptr_t EntryKey = OBF_UPTR(0x08u);
		inline uintptr_t EntryValue = OBF_UPTR(0x0Cu);
		inline constexpr int EntryStride = 16;
	}

	// HumanBodyBones enum (Unity)
	namespace HumanBodyBones_
	{
		inline constexpr int Hips = 0;
		inline constexpr int LeftUpperLeg = 1;
		inline constexpr int RightUpperLeg = 2;
		inline constexpr int LeftLowerLeg = 3;
		inline constexpr int RightLowerLeg = 4;
		inline constexpr int LeftFoot = 5;
		inline constexpr int RightFoot = 6;
		inline constexpr int Spine = 7;
		inline constexpr int Chest = 8;
		inline constexpr int Neck = 9;
		inline constexpr int Head = 10;
		inline constexpr int LeftShoulder = 11;
		inline constexpr int RightShoulder = 12;
		inline constexpr int LeftUpperArm = 13;
		inline constexpr int RightUpperArm = 14;
		inline constexpr int LeftLowerArm = 15;
		inline constexpr int RightLowerArm = 16;
		inline constexpr int LeftHand = 17;
		inline constexpr int RightHand = 18;
		inline constexpr int LeftToes = 19;
		inline constexpr int RightToes = 20;
		inline constexpr int UpperChest = 54;
	}

	// =====================================================================
	// Transform native offsets (native+0x18 иерархия, +0x1C индекс)
	// =====================================================================
	namespace TransformBone_
	{
		inline uintptr_t TransObj = OBF_UPTR(0x08u);  // klass + monitor is 8 bytes on x86
	}
	namespace TransformMatrix_
	{
		inline uintptr_t List = OBF_UPTR(0x18u);    // native+0x18 иерархия
		inline uintptr_t Indices = OBF_UPTR(0x1Cu); // native+0x1C индекс
	}

	namespace Transform_
	{
		inline uintptr_t Internal1 = OBF_UPTR(0x64u);
		inline uintptr_t Internal2 = OBF_UPTR(0x38u);
		inline uintptr_t Position = OBF_UPTR(0x3Cu);
	}
	namespace TransformLocalWrite_
	{
		inline uintptr_t Y = OBF_UPTR(0xCu);
		inline uintptr_t Z = OBF_UPTR(0x10u);
	}

	// Managed Transform localPosition (for thirdperson camera holder offset)
	namespace ManagedTransform_
	{
		inline uintptr_t LocalPositionY = OBF_UPTR(0x34u);
		inline uintptr_t LocalPositionZ = OBF_UPTR(0x38u);
	}

	// =====================================================================
	// 2.1 Weapon provider dictionary access
	// =====================================================================
	// ijq +0x0C Dictionary<WeaponId, WeaponDefinition>
	// To get weapon definition: read ijq +0x0C -> dict, lookup by WeaponId -> WeaponDefinition
}
