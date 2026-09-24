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

/* Marks a struct to be cooked as a UserDefinedStruct asset in the mod package. */
#define UE_STRUCT static constexpr bool UeStructMeta = true

/*
UE_STRUCT for a struct in a header other mods include: `UE_STRUCT_IN("/Game/_ElytrasMods/ReadProperty");`.
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
A table of an enum's names that follows the enum: `TMap<EMood, FName> Names = UE_ENUM_MAP(EMood);` and
`TMap<FName, EMood> Moods = UE_ENUM_MAP(EMood);` are defaults the compiler fills at build time, one pair per
enumerator, the name being its C++ identifier. FString works in place of FName. Any enum, a game's included.
*/
template <class E> struct __EnumMapInit__ {
  operator TMap<E, FName>() const { return {}; }
  operator TMap<FName, E>() const { return {}; }
  operator TMap<E, FString>() const { return {}; }
  operator TMap<FString, E>() const { return {}; }
};
template <class E> __EnumMapInit__<E> __EnumMap__() { return {}; }
#define UE_ENUM_MAP(Enum) __EnumMap__<Enum>()
#define UE_ENUM_IN(Enum, ModPackage) static constexpr const char *Enum##__UeEnum = ModPackage

/*
An asset some other package holds, a game one or another mod's, named so `&ED_Spider_Grunt` can point at it:
`UE_ASSET_AT(UEnemyDescriptor, ED_Spider_Grunt, "/Game/Enemies/Spider/Grunt/ED_Spider_Grunt");`. The asset's
object name is the path's last segment, unless the path spells it: "/Game/Dir/Package.Object". It may sit in a
namespace, as every one in UeAssets/ does. An asset this mod cooks needs none of this: it is a namespace-scope
variable with braces, `UMoodDef MD_Big = { .Health = 500 };`, and `&MD_Big` points at it.
*/
#define UE_ASSET_AT(Class, Name, Path)                                                                                 \
  extern Class                 Name;                                                                                   \
  static constexpr const char *Name##__UeAsset = Path

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
actor's root; later ones attach to it. Set its defaults in the UE_DEFAULTS block, not with an
initializer - they live on the archetype, not on the actor CDO.

Per-component and inherited-property defaults, the static-init block the editor's details panel
writes for you:
    UE_DEFAULTS {
      Mesh->RelativeScale3D = FVector(2, 2, 2);      // a tag on Mesh_GEN_VARIABLE
    }
It is not a constructor and never runs: AssetGen reads the assignments and writes them as
defaults. A plain member of this class takes its default from its own initializer instead.
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
nothing under -fsyntax-only.
*/
#ifdef __clang__
#define UE_SERVER [[gnu::hot]]
#define UE_CLIENT [[gnu::cold]]
#define UE_MULTICAST [[gnu::flatten]]
#define UE_RELIABLE [[gnu::nodebug]]
#define UE_AUTHORITY_ONLY [[gnu::noinline]]
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
ubergraph (Actor / ActorComponent, returns nothing, no reference parameters). The event stays bound: a
dispatcher that fires again re-enters the code after the await.
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
The editor category of what follows it in a class, as an access specifier is the access of what follows it:

    UE_CATEGORY("Teleporter|Setup");
    static void Configure(...);      // listed under Teleporter > Setup in the editor's menus and My Blueprint
    int32 Charges;
    UE_CATEGORY("");                 // back to none

`|` parts a subcategory, as in the editor. It reaches the generated editor API stub only (generate_api): a cooked
class has no categories, they are editor metadata. The stub lists the functions a Blueprint can call - every
method but an override of a parent's event - and the variables that are not private.
*/
#define UE_CATEGORY__JOIN2(A, B) A##B
#define UE_CATEGORY__JOIN(A, B) UE_CATEGORY__JOIN2(A, B)
#define UE_CATEGORY(Text) static constexpr const char *UE_CATEGORY__JOIN(UeCategory__, __COUNTER__) = Text

/* The /Game package that the classes in a mod source are written into. */
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
