#pragma once
/*
UeMeta.h — the two things a C++ declaration cannot say by itself.

A C++ name does not record which UE package its class lives in, nor its UE spelling (which
drops the A/U prefix), and an import table needs both. UE_CLASS carries them as a constexpr
string rather than an annotate attribute, because clang's JSON AST dump keeps string literals
and drops attribute arguments — a literal is the only channel that survives to the generator.

This is the one hand-written file here; UeApi.h beside it is generated.
*/
#include "Types.h" // int32/int64/uint*, reached by every UeApi header

#define UE_CLASS(Package, UeName)                                                                                      \
  static constexpr const char *UeClassMeta = Package ":" UeName;                                                       \
  static class UClass         *StaticClass()

/*
UE_CLASS for a mod's own Blueprint class, named by its owner's UE_MOD_PACKAGE alone:
`UE_CLASS_IN("/Game/_MyMods/Turrets");` in UTurretDef is
UE_CLASS("/Game/_MyMods/Turrets/UTurretDef", "UTurretDef_C"). The compiler appends
the class's namespaces and name, as UE_STRUCT_IN does, so the name is written once.
*/
#define UE_CLASS_IN(ModPackage)                                                                                        \
  static constexpr const char *UeClassInMeta = ModPackage;                                                             \
  static class UClass         *StaticClass()

/* Marks a struct to be cooked as a UserDefinedStruct asset in the mod package. */
#define UE_STRUCT static constexpr bool UeStructMeta = true

/*
UE_STRUCT for a struct in a header other mods include: `UE_STRUCT_IN("/Game/_MyMods/Turrets");`.
Only the source whose UE_MOD_PACKAGE is that path cooks it; every other mod imports
<Path>/<Name>, the way a UE_CLASS outside its package is imported. It names one owner, so
a struct cannot carry both macros.
*/
#define UE_STRUCT_IN(ModPackage) static constexpr const char *UeStructMeta = ModPackage

/*
A uint8 enum cooked as a UserDefinedEnum, after its declaration: `enum class EMood : uint8 { Calm, Angry };
UE_ENUM(EMood);`. Its enumerators keep their C++ names and values. UE_ENUM_IN(EMood, "/Game/...") in a
shared header names the mod that cooks it, as UE_STRUCT_IN does.
*/
#define UE_ENUM(Enum) static constexpr bool Enum##__UeEnum = true

/*
A table of an enum's names that follows the enum, declared as a member: `UE_ENUM_MAP(EMood, FName, Names);` and
`UE_ENUM_MAP(FName, EMood, Moods);` are `TMap<EMood, FName> Names` and `TMap<FName, EMood> Moods` with defaults the
compiler fills at build time, one pair per enumerator, the name being its C++ identifier. FString works in place of
FName. Any enum, a game's included. The one-argument form is the default alone, for a member you declare:
`TMap<EMood, FName> Names = UE_ENUM_MAP(EMood);`.
*/
template <class E> struct __EnumMapInit__ {
  operator TMap<E, FName>() const { return {}; }
  operator TMap<FName, E>() const { return {}; }
  operator TMap<E, FString>() const { return {}; }
  operator TMap<FString, E>() const { return {}; }
};
template <class E> __EnumMapInit__<E> __EnumMap__() { return {}; }
/* The enum of a map's two types, for the three-argument form: one of them an enum, the other FName or FString. A pair
   that is not one still names a Type, an empty enum whose table converts to anything, so the static_assert in
   __EnumMapCheck__ is the only error, and it names both types. The traits are plain C++, no compiler builtin, so the
   MSVC IntelliSense BpMods.vcxproj runs reads them as clang does: an enum is what an int becomes by a cast but not by
   itself, and has no members to point at. */
enum class __EnumMapNone__ : unsigned char {};
template <> struct __EnumMapInit__<__EnumMapNone__> {
  template <class M> operator M() const { return {}; }
};
template <class A, class B> constexpr bool __EnumMapSame__ = false;
template <class A> constexpr bool __EnumMapSame__<A, A> = true;
template <class T> void __EnumMapTake__(T);
template <class T> void __EnumMapMember__(int T::*);
template <class T>
constexpr bool __EnumMapEnum__ = requires(int I) { static_cast<T>(I); } && !requires(int I) { __EnumMapTake__<T>(I); }
                                 && !requires { __EnumMapMember__<T>(nullptr); };
template <class T> constexpr bool __EnumMapText__ = __EnumMapSame__<T, FName> || __EnumMapSame__<T, FString>;
template <class K, class V>
constexpr bool __EnumMapPair__ = (__EnumMapEnum__<K> && __EnumMapText__<V>) || (__EnumMapEnum__<V> && __EnumMapText__<K>);
template <class K, class V, bool KeyIsEnum = __EnumMapEnum__<K> && __EnumMapText__<V>,
          bool ValueIsEnum = __EnumMapEnum__<V> && __EnumMapText__<K>>
struct __EnumMapSide__ { using Type = __EnumMapNone__; };
template <class K, class V> struct __EnumMapSide__<K, V, true, false> { using Type = K; };
template <class K, class V> struct __EnumMapSide__<K, V, false, true> { using Type = V; };
template <class K, class V> struct __EnumMapCheck__ {
  static_assert(__EnumMapPair__<K, V>, "UE_ENUM_MAP(Key, Value, Name): one of Key and Value is an enum and the other "
                                       "FName or FString; for a TEnum<E>, write E");
  using Type = typename __EnumMapSide__<K, V>::Type;
};
#define UE_ENUM_MAP__1(Enum) __EnumMap__<Enum>()
#define UE_ENUM_MAP__3(Key, Value, Name) TMap<Key, Value> Name = __EnumMap__<__EnumMapCheck__<Key, Value>::Type>()
/* Any other count of arguments, none or more than three included. */
#define UE_ENUM_MAP__N(...)                                                                                            \
  static_assert(false, "UE_ENUM_MAP takes (Enum), or (Key, Value, Name) to declare the member")
#define UE_ENUM_MAP__PICK(_1, _2, _3, _4, _5, _6, _7, _8, Form, ...) Form
#define UE_ENUM_MAP__EXPAND(X) X
/* __VA_OPT__ tells no arguments from one. MSVC's traditional preprocessor, which BpMods.vcxproj's IntelliSense runs, has
   none: there UE_ENUM_MAP() counts as the one-argument form given nothing, an error of its own. */
#ifdef __clang__
#define UE_ENUM_MAP(...)                                                                                               \
  UE_ENUM_MAP__EXPAND(UE_ENUM_MAP__PICK(__VA_ARGS__ __VA_OPT__(, ) UE_ENUM_MAP__N, UE_ENUM_MAP__N, UE_ENUM_MAP__N,    \
                                        UE_ENUM_MAP__N, UE_ENUM_MAP__N, UE_ENUM_MAP__3, UE_ENUM_MAP__N,                \
                                        UE_ENUM_MAP__1, UE_ENUM_MAP__N, )(__VA_ARGS__))
#else
#define UE_ENUM_MAP(...)                                                                                               \
  UE_ENUM_MAP__EXPAND(UE_ENUM_MAP__PICK(__VA_ARGS__, UE_ENUM_MAP__N, UE_ENUM_MAP__N, UE_ENUM_MAP__N, UE_ENUM_MAP__N,   \
                                        UE_ENUM_MAP__N, UE_ENUM_MAP__3, UE_ENUM_MAP__N, UE_ENUM_MAP__1, )(__VA_ARGS__))
#endif
#define UE_ENUM_IN(Enum, ModPackage) static constexpr const char *Enum##__UeEnum = ModPackage

/*
Final through one leaf: `UE_FINAL_AS(UMyBase, MyLeaf);` at namespace scope (the leaf's package path, as for any class)
declares `class MyLeaf final : public UMyBase {}`, the one class ever made. UMyBase is then compiled as if final - its
calls on `this` bound and copied in, its functions FUNC_Final - and cooked Abstract; any other subclass of it is
refused. Put it in the header beside UMyBase, so a mod that includes the header is refused a subclass too.
*/
#define UE_FINAL_AS(Base, Leaf)                                                                                        \
  class Leaf final : public Base {                                                                                     \
  public:                                                                                                              \
    static constexpr bool UeFinalAsLeaf = true;                                                                        \
  }

/*
An asset some other package holds, a game one or another mod's, named so `&ED_Spider_Grunt` can point at it:
`UE_ASSET_AT(UEnemyDescriptor, ED_Spider_Grunt, "/Game/Enemies/Spider/Grunt/ED_Spider_Grunt");`. The asset's
object name is the path's last segment, unless the path spells it: "/Game/Dir/Package.Object". It may sit in a
namespace, as every one in UeAssets/ does. An asset this mod cooks needs none of this: it is a namespace-scope
variable with braces, `UMoodDef MD_Big = { .Health = 500 };`, and `&MD_Big` points at it.
Another mod's asset of a class declared in a shared header needs that class pinned to its owner with UE_CLASS;
otherwise every mod including the header cooks its own copy of the class, and the reference would load as null
(refused).
*/
#define UE_ASSET_AT(Class, Name, Path)                                                                                 \
  extern Class                 Name;                                                                                   \
  static constexpr const char *Name##__UeAsset = Path

/*
S38 - editing an asset the game holds, rather than cooking a new one:

    UE_ASSET_EDIT(UeAssets::UEnemyDescriptor::Game::Enemies::Spider::Grunt::ED_Spider_Grunt) { .SpawnSpread = 800 };

The target is a UE_ASSET_AT (every one in UeAssets/ is). The build reads the asset's package as the game cooked it
(`assetgen compile --game <extracted pak>/FSD/Content`, `game_content` in mods.yaml), replaces or adds the tags the
braces name and writes the package to the asset's own path, so the mod pak's copy is the one the game loads. A member
the braces leave out keeps the asset's value, and every other byte stays the game's. A zero is written like any value.

Part of a member's value - one struct member, one TArray element - is assigned by path, in a block of statements:

    UE_ASSET_EDITS {
      ED_Spider_Grunt.SpawnRarityModifiers[1].Rarity = 2.0f;    // the rest of the array, and of that element, stay
      ED_Spider_Exploder.SpawnSpread = 100.0f;                  // a whole member, as UE_ASSET_EDIT's braces do
    }

A step is `.Member` or `[i]` (a TArray element, the index a constant), to any depth. An element past the array's end
is refused. A member the asset has no value of (it takes its class's default) can take a path through structs
written as tags - the new value holds only that path, and the engine fills in the rest - but not through a native
struct (FVector, FRotator, ...) or an element, whose other parts are unknown here; assign the whole member then.

A game Blueprint's class defaults are edited by a patch of the class:

    class MyGruntPatch : public Game::Enemies::Spider::Grunt::ENE_Spider_Grunt_Normal_C {
      UE_PATCH;
      UE_DEFAULTS {
        SomeMember = 5;                     // a member of the class or of any class above it
        HealthComponent->MaxHealth = 90;    // a component's: the export the engine builds the component from
        PrimaryActorTick.bCanEverTick = true;               // part of a member's value, by path as above
        MeleeAttack->Montages[0] = &SomeMontage;
      }
    };

which edits the tags of that class's default object (Default__<Name>_C) in the class's own package, and of the
component exports beside it: a native component's default subobject, the class's own SCS template, or the template of
the override record a parent Blueprint's component has in this class. A parent's component the class does not override
has no such export yet; patch the Blueprint that declares it.

A method of the patch replaces the function of the same name that the Blueprint itself defines:

      void GetEnemySpawnedCount(int32 &SpawnCount) { SpawnCount = 42; }

Its parameters must be the game function's, in order, type, name and direction (a Blueprint output is a `T&`). The
function keeps the game's flags and parameters, so every caller still fits, and takes the method's locals and code.
`ENE_Spider_Grunt_Normal_C::GetEnemySpawnedCount(SpawnCount)` inside it runs the game's body, kept beside it as
GetEnemySpawnedCount__Vanilla (an RPC's body runs there as a plain function, where the RPC already arrived). A method
the Blueprint does not define is added to it: a helper the other methods call, or an override of a function it
inherits (`void ReceiveTick(float DeltaSeconds)`), which changes this class and its children only, where patching the
parent that defines it changes them all. A latent call in a patch's method is refused. A patch is not a class of its
own and cooks nothing else: a member or an interface of its own is refused, not dropped.
*/
#define UE_ASSET_EDIT__CAT2(A, B) A##B
#define UE_ASSET_EDIT__CAT(A, B) UE_ASSET_EDIT__CAT2(A, B)
#define UE_ASSET_EDIT__(Asset, N)                                                                                      \
  [[maybe_unused]] static constexpr auto *UE_ASSET_EDIT__CAT(UeEditOf__, N) = &Asset;                                \
  [[maybe_unused]] static decltype(Asset) UE_ASSET_EDIT__CAT(UeEdit__, N)
#define UE_ASSET_EDIT(Asset) UE_ASSET_EDIT__(Asset, __COUNTER__)
#define UE_ASSET_EDITS [[maybe_unused]] static void UE_ASSET_EDIT__CAT(UeAssetEdits__, __COUNTER__)()
#define UE_PATCH static constexpr bool UePatchMeta = true

/*
`All`: every UE_ASSET_AT in this namespace and the ones inside it whose class is Class or derives from it, as soft
pointers, so nothing loads until asked. Each UeAssets/<Class>.h declares one, `UeAssets::USoundWave::All`. Like any
namespace-scope variable a mod uses, it is kept in the default object of a class the compiler generates for it.
*/
#define UE_ASSET_ALL(Class)                                                                                            \
  extern TArray<TSoftObjectPtr<Class>> All;                                                                            \
  static constexpr bool                All__UeAssetAll = true

/*
An interface a mod declares, cooked as its own asset:
    class ITargetable {
      UE_INTERFACE;
      void OnTargeted(class AActor *By);
    };
    class Turret : public AActor, public ITargetable {
      void OnTargeted(class AActor *By) { ... }     // the implementation, an ordinary method
    };
The UE parent comes first in the base list and every base after it is an implemented interface, the
same as for the game's own.

An interface may extend one other, `class IMarkable : public ITargetable { UE_INTERFACE; ... }`: a class
that implements IMarkable implements both, and is found by a cast to either.

A variable on an interface is AssetGen's own idea (an engine interface holds no state): it becomes a
property of each class that implements the interface, once, and not again below it. Read it on an
object of such a class (`this`, `Beacon->Marks`), never through the interface itself. Its initializer is
the default; an implementing class's UE_DEFAULTS may give its own.
*/
#define UE_INTERFACE static constexpr bool UeInterfaceMeta = true

/*
A component on a mod actor, the "Add Component" list in the editor:
    UE_COMPONENT(UStaticMeshComponent, Mesh);
Declares Mesh as an ordinary class variable AND an SCS node that instantiates a
Mesh_GEN_VARIABLE archetype into it at spawn. The first scene component declared becomes the
actor's root; later ones attach to it, unless UE_DEFAULTS places one with SetupAttachment
(below). Set its defaults in the UE_DEFAULTS block, not with an
initializer - they live on the archetype, not on the actor CDO.

Per-component and inherited-property defaults, the static-init block the editor's details panel
writes for you:
    UE_DEFAULTS {
      Mesh->RelativeScale3D = FVector(2, 2, 2);      // a tag on Mesh_GEN_VARIABLE
      Mesh->SetupAttachment(Lamp);                   // Mesh's SCS node under Lamp, own or inherited
    }
It is not a constructor and never runs: AssetGen reads the assignments and writes them as
defaults, and each SetupAttachment as where the component's node hangs. A plain member of this
class takes its default from its own initializer instead.
*/
#define UE_COMPONENT(Type, Name)                                                                                       \
  static constexpr const char *Name##__UeComponent = #Type;                                                            \
  Type                        *Name
#define UE_DEFAULTS void UeDefaults__()

/*
An event dispatcher on a mod class: `UE_DISPATCHER(OnScored, int32 Points, AActor* By);`. Declares the
dispatcher OnScored and its signature function OnScored__DelegateSignature, which is where the
parameter names survive the AST dump.
*/
#define UE_DISPATCHER(Name, ...)                                                                                       \
  void                                        Name##__DelegateSignature(__VA_ARGS__);                                  \
  TMulticastInlineDelegate<void(__VA_ARGS__)> Name

/*
Replicated variables. The macro declares the variable, so an initializer follows it:
    UE_REPLICATED(int32, Health) = 100;
    UE_REPLICATED_USING(int32, Health, OnRep_Health);            // OnRep_Health() runs when a new value arrives
    UE_REPLICATED_IF(FVector, Aim, SkipOwner);                   // an ELifetimeCondition without the COND_ prefix
    UE_REPLICATED_USING_IF(int32, Ammo, OnRep_Ammo, OwnerOnly);
Assigning a RepNotify variable also runs its OnRep function right there, as the editor's Set node does, so a
listen-server host sees the change too. The class gets bReplicates.
*/
#define UE_REPLICATED(Type, Name)                                                                                      \
  static constexpr const char *Name##__Replicated = ":";                                                               \
  Type                         Name
#define UE_REPLICATED_USING(Type, Name, Notify)                                                                        \
  static constexpr const char *Name##__Replicated = #Notify ":";                                                       \
  Type                         Name
#define UE_REPLICATED_IF(Type, Name, Cond)                                                                             \
  static constexpr const char *Name##__Replicated = ":" #Cond;                                                         \
  Type                         Name
#define UE_REPLICATED_USING_IF(Type, Name, Notify, Cond)                                                               \
  static constexpr const char *Name##__Replicated = #Notify ":" #Cond;                                                 \
  Type                         Name

/*
Remote procedure calls: `UE_SERVER void Fire(FVector At);`, `UE_MULTICAST UE_RELIABLE void PlayHit();`.
Server runs on the server when called by the owning client, Client on the owning client, Multicast on the
server and every client. An RPC returns void; a reference parameter arrives as a copy, so writing through one is
a warning. UE_AUTHORITY_ONLY (BlueprintAuthorityOnly) runs only with authority, UE_COSMETIC (BlueprintCosmetic)
never on a dedicated server; the call is skipped otherwise. UeApi marks engine and game functions the same way. Like
UE_PURE these are attribute kinds, the one thing the JSON dump keeps of an attribute; the GNU attributes chosen do
nothing under -fsyntax-only. `noinline` is not one of them: it keeps its own meaning, that a call to the function is
never expanded in place.
*/
#ifdef __clang__
#define UE_SERVER [[gnu::hot]]
#define UE_CLIENT [[gnu::cold]]
#define UE_MULTICAST [[gnu::flatten]]
#define UE_RELIABLE [[gnu::nodebug]]
#define UE_AUTHORITY_ONLY [[gnu::no_stack_protector]]
#define UE_COSMETIC [[gnu::no_instrument_function]]
#else
#define UE_SERVER
#define UE_CLIENT
#define UE_MULTICAST
#define UE_RELIABLE
#define UE_AUTHORITY_ONLY
#define UE_COSMETIC
#endif

/*
UE_NO_OPTIMIZE: AssetGen lowers the function as written: an unused pure call and an unread local stay, and a
harmless `&&` / `||` keeps its branch. It is the real attribute, so `#pragma clang optimize off` /
`#pragma optimize("", off)` over a run of functions does the same.
*/
#ifdef __clang__
#define UE_NO_OPTIMIZE [[clang::optnone]]
#else
#define UE_NO_OPTIMIZE
#endif

/*
UE_AWAIT(Proxy->OnCompleted): the rest of the function runs when that dispatcher next fires, as the editor's
async action node continues from its output pin. The value is the dispatcher's parameter when it has exactly one.
A UBlueprintAsyncActionBase is activated after the bind. Like a latent call, the function moves into the
ubergraph (Actor / ActorComponent, returns nothing, no non-const reference parameters). The event stays
bound: a dispatcher that fires again re-enters the code after the await.
*/
template <class Sig> struct TMulticastInlineDelegate;
template <class A> A       __Await__(TMulticastInlineDelegate<void(A)> &Dispatcher);
template <class... A> void __Await__(TMulticastInlineDelegate<void(A...)> &Dispatcher);
#define UE_AWAIT(Dispatcher) __Await__(Dispatcher)

/*
switch on an FName:

  switch (UE_NAME_SWITCH(ClassName)) {
  case UE_NAME_CASE("IntProperty"): ...
  }

C++ switches on integers, so clang sees a constexpr hash of each case's text (a duplicate hash is a duplicate case
error). AssetGen compares the names: the value is stored once and each case is a NotEqual_NameName test.
*/
struct FName;
int32           __NameSwitch__(FName Name);
constexpr int32 __NameCase__(const char *Text) {
  unsigned H = 2166136261u;
  while (*Text)
    H = (H ^ (unsigned char)*Text++) * 16777619u;
  return int32(H & 0x7fffffff);
}
#define UE_NAME_SWITCH(Name) __NameSwitch__(Name)
#define UE_NAME_CASE(Text) __NameCase__(Text)

/*
`Obj->GetOuter()`, `GetClass()`, `GetName()` - on this or any object - are UObject's in C++ and not UFunctions; the
generated UObject declares them and forwards each to what does it, the object as the first argument: GetOuter to a
read of OuterPrivate off the object (no engine call; the mod declares FDeref as for any pointer read, and Obj must
not be null), GetClass / GetName to the Kismet statics GetObjectClass / GetObjectName. genueapi's UOBJECT_FORWARDS
is the list; a target with `::` is a library static, without it a free inline function.
*/

/*
A subsystem: `UUGCSubsystem::Get()`, or `GetSubsystem<UUGCSubsystem>()`. genueapi gives every class under a subsystem
kind (engine, game instance, world, local player) a static Get calling USubsystemBlueprintLibrary's getter for that
kind, as the editor's Get node does. A world context left out is this, the node's hidden pin; `Get(Other)` asks
Other's world, and a local player subsystem also takes a player controller.
*/
template <class T> inline T *GetSubsystem() { return T::Get(); }

/*
Calling the parent's implementation needs no macro either: inside an override, `Base::Method(args)` runs the
parent's Method on this object and comes back - the editor's "Add call to parent function". It works for a mod
parent and for a game or engine one (`AActor::ReceiveBeginPlay()` is a no-op there unless the parent is a
Blueprint: a native event has no script body). Write the class you derive from; `Super` is not a name C++ knows.
*/

/*
Access specifiers need no macro: `public:` / `protected:` / `private:` on a mod class mean in the editor what they
mean in C++. A function is cooked FUNC_Public / FUNC_Protected (the class and its subclasses) / FUNC_Private (the
class alone), and the editor API stub carries it, so a Blueprint cannot place a call the specifier forbids. An
override keeps its parent's access. A Blueprint variable knows private only, and only as editor metadata: a private
field is left out of the API stub, a protected one stays visible. Nothing is enforced at runtime - the VM checks
no access, and clang has already refused what C++ forbids. Mind that `class` starts private.

Read-only needs none either: a `const` member (`const int32 Limit = 3;`) is cooked BlueprintReadOnly, its
initializer being the default - the editor offers a Get node and no Set. The VM does not enforce that either.
*/

/*
BlueprintReadOnly that may still be written: `UE_READONLY int32 Limit = 3;`. It is cooked as a `const` member is, but
a subclass's UE_DEFAULTS can give its default, which `const` forbids, and a write from code compiles with a warning
- the editor would refuse it, the VM does not (setting a deferred spawn's members before FinishSpawning is the use).
UeApi marks the engine's and game's BlueprintReadOnly properties with it. It is `mutable` to the compiler, the one
specifier a member can carry without changing its type, kept in the AST dump as the field's "mutable".
*/
#define UE_READONLY mutable

/*
An inline class variable needs no macro: C++'s own `static inline const` (or `static constexpr`) member

    static inline const float HoldTime = 0.5f;
    static inline const TArray<TSoftClassPtr<AItem>> Chargeable = {"/Game/A.A_C", "/Game/B.B_C"};

is no Blueprint variable. Nothing is cooked for it, no property and no default: each use is its initializer, lowered
where it is used (a number folded to its literal), a braced list a Make Array / Set / Map there. A range-for over an
inline array of constants makes no array; each pass picks its element with a switch on the index. Contains on one
makes none either: it compares the item with each element (not for text or structs, whose == differs from Contains').
A class has no static storage, so a static that is not const is refused where it is used.
*/

/*
The editor category of what follows it in a class, as an access specifier is the access of what follows it:

    UE_CATEGORY("Turret|Setup");
    static void Configure(...);      // listed under Turret > Setup in the editor's menus and My Blueprint
    int32 Charges;
    UE_CATEGORY("");                 // back to none

`|` parts a subcategory, as in the editor. It reaches the generated editor API stub only (generate_api): a cooked
class has no categories, they are editor metadata. The stub lists the functions a Blueprint can call - every
method but an override of a parent's event - and the variables that are not private.
*/
#define UE_CATEGORY__JOIN2(A, B) A##B
#define UE_CATEGORY__JOIN(A, B) UE_CATEGORY__JOIN2(A, B)
#define UE_CATEGORY(Text) static constexpr const char *UE_CATEGORY__JOIN(UeCategory__, __COUNTER__) = Text

/*
The /Game package that the classes in a mod source are written into. A namespace is a folder: a class, struct,
interface, UE_ENUM or asset in `namespace Weapons::Rifles` goes to <Path>/Weapons/Rifles. A namespace that starts at
Game is a /Game path of its own, `namespace Game::Weapons::Rifles { class X ... }` being /Game/Weapons/Rifles/X, the
namespace UeApi gives a game Blueprint there, so a child can sit beside the class it extends. A path outside <Path>
is written under the Content folder the output directory sits in, as bpbuild stages a mod.
*/
#define UE_MOD_PACKAGE(Path) static constexpr const char *UeModPackage = Path

/*
Marks a method BlueprintPure: `UE_PURE FName MakeKey(FText Prefix)`, `UE_PURE static int32 F()`.
An attribute survives the JSON dump as its kind alone, which is all a flag needs. clang drops
gnu::pure (with a warning) from a function returning void, so a pure method returns its value.
Only AssetGen's clang reads it; an MSVC-mode IntelliSense would flag the attribute (C5030).
*/
#ifdef __clang__
#define UE_PURE [[gnu::pure]]
#else
#define UE_PURE
#endif
