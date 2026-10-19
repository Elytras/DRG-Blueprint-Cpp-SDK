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
parent that defines it changes them all. A method may wait (Delay): it resumes from an ubergraph the patch adds, and
what it keeps across the wait is transient variables of the Blueprint. A member of the patch's own (`int32 Hits = 0;`)
is a variable added to the Blueprint, its initializer on the default object; an interface it lists is added to the
Blueprint with its methods and variables. A patch adds no component, widget or dispatcher, and its members do not
replicate: each is refused, not dropped.
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

With generate_api, the editor API stub of an interface is a Blueprint Interface asset, so a Blueprint made in the
editor can implement it too. Its variables are not in the stub, and such a Blueprint does not have them.
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
class takes its default from its own initializer instead. A default constructor is read the same
way, as the class's defaults: its member initializers and assignments to the class's own members
as their initializers, its other statements as this block's, after them.
*/
#define UE_COMPONENT(Type, Name)                                                                                       \
  static constexpr const char *Name##__UeComponent = #Type;                                                            \
  Type                        *Name
#define UE_DEFAULTS void UeDefaults__()

/*
A widget of a UUserWidget class's designer tree, which the class's constructor lays out:
    UE_WIDGET(UCanvasPanel, Root);                   // the first UE_WIDGET is the tree's root
    UE_WIDGET(UTextBlock, Title);
    UMyCard() {
      Root->AddChildToCanvas(Title)->SetPosition(FVector2D(20, 10));
      Title->SetText(FText("Score"));
    }
Declares Title as a class variable (the editor's "Is Variable") and a widget of the tree, cooked
as the editor saves one. Like UE_DEFAULTS, the constructor never runs: AddChild... calls place
the widgets, and each setter is read as the property it sets, on the widget or on its slot.
*/
#define UE_WIDGET(Type, Name)                                                                                          \
  static constexpr const char *Name##__UeWidget = #Type;                                                               \
  Type                        *Name

/*
The layout's other lines, beside its AddChild calls:
    Play->OnClicked.Add(this, &UMyCard::OnPlay);     // a widget's event, bound as each widget is made
    UE_SLOT(Frame->Body, Inner);                     // Inner fills the named slot Body of the user widget Frame
    UE_BIND(Title, Text, &UMyCard::GetScoreText);     // Title's Text, read from GetScoreText as it is drawn
UE_SLOT's slot is a UNamedSlot of the nested widget's class (a UE_WIDGET of a mod class, a variable of a
game one), and Inner one of this class's UE_WIDGETs, in no panel. UE_BIND's function returns the
property's type and takes nothing; the widget must have a <Property>Delegate. Neither is a function
that runs: like the rest of the layout they are read, and only in the constructor.
*/
#define UE_SLOT(Slot, Content) UeSlot__(Slot, Content)
#define UE_BIND(Widget, Property, Function) UeBind__(Widget->Property##Delegate, Widget->Property, Function)
void                                                 UeSlot__(class UNamedSlot *Slot, class UWidget *Content);
template <class D, class C, class V> void UeBind__(D &Delegate, const V &Property, V (C::*Function)() const);
template <class D, class C, class V> void UeBind__(D &Delegate, const V &Property, V (C::*Function)());

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
async action node continues from its output pin. The value is the dispatcher's parameter when it has exactly one;
of two or more, bind them: `auto [Who, Points] = UE_AWAIT(OnScored);`. A UBlueprintAsyncActionBase is activated
after the bind. Like a latent call, the function moves into the ubergraph (Actor / ActorComponent, returns
nothing, no non-const reference parameters). The event stays bound: a dispatcher that fires again re-enters the
code after the await.
ponytail: TAwaitValues has members for up to nine values, the most a UeApi dispatcher has; a line adds more.
*/
template <class Sig> struct TMulticastInlineDelegate;
template <class... A> struct TAwaitValues {};
template <class A, class B> struct TAwaitValues<A, B> { A V0; B V1; };
template <class A, class B, class C> struct TAwaitValues<A, B, C> { A V0; B V1; C V2; };
template <class A, class B, class C, class D> struct TAwaitValues<A, B, C, D> { A V0; B V1; C V2; D V3; };
template <class A, class B, class C, class D, class E> struct TAwaitValues<A, B, C, D, E> { A V0; B V1; C V2; D V3; E V4; };
template <class A, class B, class C, class D, class E, class F>
struct TAwaitValues<A, B, C, D, E, F> { A V0; B V1; C V2; D V3; E V4; F V5; };
template <class A, class B, class C, class D, class E, class F, class G>
struct TAwaitValues<A, B, C, D, E, F, G> { A V0; B V1; C V2; D V3; E V4; F V5; G V6; };
template <class A, class B, class C, class D, class E, class F, class G, class H>
struct TAwaitValues<A, B, C, D, E, F, G, H> { A V0; B V1; C V2; D V3; E V4; F V5; G V6; H V7; };
template <class A, class B, class C, class D, class E, class F, class G, class H, class I>
struct TAwaitValues<A, B, C, D, E, F, G, H, I> { A V0; B V1; C V2; D V3; E V4; F V5; G V6; H V7; I V8; };
void __Await__(TMulticastInlineDelegate<void()> &Dispatcher);
template <class A> A __Await__(TMulticastInlineDelegate<void(A)> &Dispatcher);
template <class A, class B, class... R> TAwaitValues<A, B, R...> __Await__(TMulticastInlineDelegate<void(A, B, R...)> &Dispatcher);
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
The tooltip and the search keywords of the method that follows, as the editor's Details panel of a function sets them:

    UE_TOOLTIP("Adds charges, up to the cap.") UE_KEYWORDS("refill reload") void Recharge(int32 By);

The tooltip shows over the call node and in the action menu; the keywords are extra words the action menu's search
finds the function by. Like UE_CATEGORY, they reach the generated editor API stub only (generate_api): a cooked game
keeps no editor metadata, so the pak carries neither.
*/
#define UE_TOOLTIP(Text) static constexpr const char *UE_CATEGORY__JOIN(UeToolTip__, __COUNTER__) = Text;
#define UE_KEYWORDS(Text) static constexpr const char *UE_CATEGORY__JOIN(UeKeywords__, __COUNTER__) = Text;

/*
A method the engine calls on a key or an input action, as the editor's InputKey and InputAction events:

    UE_INPUT_KEY("F5") void Toggle();                        // F5 pressed
    UE_INPUT_KEY("F5", Released) void Stop();                // F5 released
    UE_INPUT_KEY("S", Ctrl, Shift) void Save();              // Ctrl+Shift+S
    UE_INPUT_ACTION("Use", NoConsume) void Used(FKey Key);   // the game's Use action, which the game still gets

The key is the engine's name of it (F5, SpaceBar, One, LeftMouseButton, Gamepad_FaceButton_Bottom: EKeys), and one the
engine does not have is refused with the closest it does; the action is one of the game's input settings, which AssetGen
cannot check. The method returns void and takes nothing or the key that fired it. Options: Pressed and Released (both:
one call each; neither: Pressed), Shift, Ctrl, Alt and Cmd (a key's), NoConsume (the game's own use of the input still
runs: without it the mod takes the input from the game), WhenPaused (fires while the game is paused) and KeepParent (a
binding a parent class made of the same input and event stays). Only an actor gets input: a pawn while possessed and a
player controller always, any other once it calls EnableInput(PlayerController) or its UE_DEFAULTS set
AutoReceiveInput = EAutoReceiveInput::Player0.
*/
#define UE_INPUT_KEY(Key, ...)                                                                                         \
  static constexpr const char *UE_CATEGORY__JOIN(UeInputKey__, __COUNTER__) = Key "|" #__VA_ARGS__;
#define UE_INPUT_ACTION(Action, ...)                                                                                   \
  static constexpr const char *UE_CATEGORY__JOIN(UeInputAction__, __COUNTER__) = Action "|" #__VA_ARGS__;

/*
A timeline of an actor, as the editor's Timeline node: its length in seconds and options, its tracks, and the methods it
calls.

    UE_TIMELINE(Fade, 2.0f, AutoPlay);                                  // UTimelineComponent *Fade
    UE_FLOAT_TRACK(Fade, Alpha, (0, 0), (1, 1, Cubic), (2, 0));        // float Alpha, set each frame it plays
    UE_VECTOR_TRACK(Fade, Lift, (0, 0, 0, 0), (2, 0, 0, 100));          // FVector Lift: (time, x, y, z)
    UE_COLOR_TRACK(Fade, Tint, (0, 1, 1, 1, 1), (2, 1, 0, 0, 1));       // FLinearColor Tint: (time, r, g, b, a)
    UE_TIMELINE_UPDATE(Fade) void OnFade();                             // each frame it plays, after the tracks
    UE_TIMELINE_FINISHED(Fade) void OnFaded();                          // at its end, or its start in reverse
    UE_TIMELINE_EVENT(Fade, 0.5, 1.5) void Blink();                     // an event track: as it passes each time

The options are AutoPlay, Loop, Replicated, IgnoreTimeDilation and LastKeyframe (the length is the last key's time). A
key is Linear to the next unless Cubic (Auto tangents) or Constant follows its values; its numbers are literals. The
methods return void and take nothing. Play it through the component: Fade->Play(), PlayFromStart(), Reverse(), Stop(),
SetNewTime(T).
*/
#define UE_TIMELINE(Name, Length, ...)                                                                                 \
  static constexpr const char *Name##__UeTimeline = #Length "|" #__VA_ARGS__;                                         \
  UTimelineComponent          *Name
#define UE_FLOAT_TRACK(Timeline, Name, ...)                                                                            \
  static constexpr const char *Name##__UeTrack = #Timeline "|" #__VA_ARGS__;                                          \
  float                        Name
#define UE_VECTOR_TRACK(Timeline, Name, ...)                                                                           \
  static constexpr const char *Name##__UeTrack = #Timeline "|" #__VA_ARGS__;                                          \
  FVector                      Name
#define UE_COLOR_TRACK(Timeline, Name, ...)                                                                            \
  static constexpr const char *Name##__UeTrack = #Timeline "|" #__VA_ARGS__;                                          \
  FLinearColor                 Name
/*
A track over a curve asset, the editor's External Curve: the timeline plays Curve's keys, as they are in the game, rather
than keys written here. Curve is an asset a UE_ASSET_AT names (the UeAssets headers name every game curve), another mod's
or the game's, or one this mod declares with UE_FLOAT_CURVE and kin; a float track plays a UCurveFloat, a vector track a
UCurveVector, a color track a UCurveLinearColor.

    UE_FLOAT_TRACK_CURVE(Fade, Alpha, UeAssets::UCurveFloat::Game::LevelElements::Blueprints::Hologram_FadeIn);
*/
#define UE_FLOAT_TRACK_CURVE(Timeline, Name, Curve)                                                                    \
  static constexpr const char *Name##__UeTrack = #Timeline "|";                                                        \
  [[maybe_unused]] static constexpr auto *Name##__UeTrackCurve = &Curve;                                               \
  float                        Name
#define UE_VECTOR_TRACK_CURVE(Timeline, Name, Curve)                                                                   \
  static constexpr const char *Name##__UeTrack = #Timeline "|";                                                        \
  [[maybe_unused]] static constexpr auto *Name##__UeTrackCurve = &Curve;                                               \
  FVector                      Name
#define UE_COLOR_TRACK_CURVE(Timeline, Name, Curve)                                                                    \
  static constexpr const char *Name##__UeTrack = #Timeline "|";                                                        \
  [[maybe_unused]] static constexpr auto *Name##__UeTrackCurve = &Curve;                                               \
  FLinearColor                 Name

/*
A curve asset of the mod's own, at namespace scope, as the Content Browser's Miscellaneous > Curve makes one: cooked at
<UE_MOD_PACKAGE>/<namespace folders>/<Name>, its keys written as a timeline track's are. `&Name` points at it, from a
track (UE_FLOAT_TRACK_CURVE), a default or code, and another mod names it with UE_ASSET_AT.

    UE_FLOAT_CURVE(Ramp, (0, 0), (1, 1, Cubic));                        // a UCurveFloat
    UE_VECTOR_CURVE(Path, (0, 0, 0, 0), (2, 0, 0, 100));                // a UCurveVector: (time, x, y, z)
    UE_COLOR_CURVE(Glow, (0, 1, 1, 1, 1), (2, 1, 0, 0, 1));             // a UCurveLinearColor: (time, r, g, b, a)
*/
#define UE_FLOAT_CURVE(Name, ...)                                                                                      \
  static constexpr const char *Name##__UeCurve = "F|" #__VA_ARGS__;                                                    \
  extern UCurveFloat           Name
#define UE_VECTOR_CURVE(Name, ...)                                                                                     \
  static constexpr const char *Name##__UeCurve = "V|" #__VA_ARGS__;                                                    \
  extern UCurveVector          Name
#define UE_COLOR_CURVE(Name, ...)                                                                                      \
  static constexpr const char *Name##__UeCurve = "C|" #__VA_ARGS__;                                                    \
  extern UCurveLinearColor     Name

#define UE_TIMELINE_UPDATE(Timeline)                                                                                   \
  static constexpr const char *UE_CATEGORY__JOIN(UeTimelineUpdate__, __COUNTER__) = #Timeline;
#define UE_TIMELINE_FINISHED(Timeline)                                                                                 \
  static constexpr const char *UE_CATEGORY__JOIN(UeTimelineFinished__, __COUNTER__) = #Timeline;
#define UE_TIMELINE_EVENT(Timeline, ...)                                                                               \
  static constexpr const char *UE_CATEGORY__JOIN(UeTimelineEvent__, __COUNTER__) = #Timeline "|" #__VA_ARGS__;

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
