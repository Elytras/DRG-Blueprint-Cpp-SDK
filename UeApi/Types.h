#pragma once
/* UE integer spellings for clang; nothing here has behaviour. */
using int8  = signed char;
using int16 = short;
using int32 = int;
using int64 = long long;

using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using WChar = wchar_t;

/*
FString / FName / FText exist so clang accepts a mod source; AssetGen lowers by the clang types
alone. Conv.h supplies one constructor per Kismet Conv_XToY (implicit) and one explicit operator
per scalar target, so `Key = Label + Count` and `int(Label)` both spell a Kismet conversion.
Every `+` yields an FString: AssetGen converts each operand to a string, concatenates, and
converts the result to the destination type.
*/
struct FName;
struct FText;
#include "Conv.h"

/* Methods_<T>.inc, included last in FString, FName, FText and the soft pointers: every library static taking the
   value first as a method (`Path.MakeSoftClassPath()`), which AssetGen calls with the value first, and each
   one-parameter Conv_ as To<Target>() (`Path.ToSoftClassPtr()`). Methods.h declares what they name. */
#include "Methods.h"

struct FString {
  WChar *Data;
  int32  Length;
  int32  Capacity;

  FString() = default;
  FString(const WChar *) {}
  FString(const char *) {}
  FString(double) {} // a FloatingLiteral, so `Str + 0.5` is not ambiguous across the scalar ctors
  UE_CONV_FString

    int32
    Len() const {
    return Length;
  }

#include "Methods_FString.inc"
};

struct FName {
  WChar *Val;

  FName() = default;
  FName(const WChar *) {}
  FName(const char *) {}
  FName(double) {}
  UE_CONV_FName
  explicit operator bool() const; // `if (Name)`: Name != None

#include "Methods_FName.inc"
};

struct FText {
  WChar *Val;

  FText() = default;
  FText(const WChar *) {}
  FText(const char *) {}
  FText(double) {}
  UE_CONV_FText

#include "Methods_FText.inc"
};

inline FString operator+(const FString &, const FString &) { return FString(); }

/* Class and soft references, and Cast<T>: AssetGen lowers them to ClassProperty /
   SoftObjectProperty / SoftClassProperty, EX_ObjectConst, EX_SoftObjectConst and EX_DynamicCast. */
class UClass;
class UObject;

template <class T> struct TSubclassOf {
  UClass *Ptr;
  TSubclassOf() = default;
  TSubclassOf(UClass *) {}
  operator UClass *() const { return Ptr; }
};

/* An enum that knows its names: `TEnum<EMood> Mood = EMood::Calm;` compiles to a plain EMood - the same property,
   the same bytes - and converts both ways, so it switches, compares and assigns like one. `Mood.Name()` is the
   enumerator's name and `Mood.String()` its display name, read off the UEnum by the engine (KismetNodeHelperLibrary's
   GetEnumeratorName / GetEnumeratorUserFriendlyName, what the editor's Enum to Name / Enum to String nodes call).
   UeApi spells every enum field and return value this way. A uint8 enum only: those two take a uint8. */
template <class E> struct TEnum {
  TEnum() = default;
  TEnum(E) {}
  operator E() const { return E(); }
  FName   Name() const;
  FString String() const;
};

/* A soft pointer to a subclass passes where one to its parent is wanted, as UE's converts. Conv.h
   adds Kismet's conversions: from the object (the class) it names and from a soft path, to its path
   as a string, and back with an explicit operator, `(AItem *)Soft`, null unless it is loaded. */
template <class T> struct TSoftObjectPtr {
  WChar *Path;
  TSoftObjectPtr() = default;
  TSoftObjectPtr(const char *) {}
  TSoftObjectPtr(const WChar *) {}
  template <class U> TSoftObjectPtr(const TSoftObjectPtr<U> &) {}
  UE_CONV_TSoftObjectPtr

#include "Methods_TSoftObjectPtr.inc"
};

template <class T> struct TSoftClassPtr {
  WChar *Path;
  TSoftClassPtr() = default;
  TSoftClassPtr(const char *) {}
  TSoftClassPtr(const WChar *) {}
  template <class U> TSoftClassPtr(const TSoftClassPtr<U> &) {}
  UE_CONV_TSoftClassPtr

#include "Methods_TSoftClassPtr.inc"
};

template <class T, class U> T *Cast(U *) { return nullptr; }

/* An object seen through one of its interfaces: InterfaceProperty. Made from an object
   (EX_ObjToInterfaceCast) or another interface (EX_CrossInterfaceCast); GetObject() is
   EX_InterfaceToObjCast, and `I->Fn()` runs Fn in EX_InterfaceContext(I). */
template <class T> struct TScriptInterface {
  UObject *Object;
  T       *Interface;
  TScriptInterface() = default;
  TScriptInterface(UObject *) {}
  template <class U> TScriptInterface(const TScriptInterface<U> &) {}
  UObject *GetObject() const;
  T       *operator->() const;
  /* `if (I)`, `!I`, `I == nullptr`: whether an object is behind it (CST_InterfaceToBool). */
  explicit operator bool() const;
  bool     operator==(decltype(nullptr)) const;
  bool     operator!=(decltype(nullptr)) const;
};

/* Delegates. A delegate value is `{this, &Class::Handler}`, self's function bound by name
   (EX_InstanceDelegate). A dispatcher takes Add / Remove / Clear, and Broadcast when AssetGen knows
   its signature function, which it does for one declared with UE_DISPATCHER. */
template <class Sig> struct TDelegate;
template <class R, class... A> struct TDelegate<R(A...)> {
  uint8 Opaque[16];
  TDelegate() = default;
  template <class O, class C> TDelegate(O *, R (C::*)(A...)) {}
};

template <class Sig> struct TMulticastInlineDelegate;
template <class R, class... A> struct TMulticastInlineDelegate<R(A...)> {
  uint8                            Opaque[16];
  template <class O, class C> void Add(O *, R (C::*)(A...));
  template <class O, class C> void Remove(O *, R (C::*)(A...));
  void                             Clear();
  void                             Broadcast(A...);
};

template <class Sig> using TMulticastSparseDelegate = TMulticastInlineDelegate<Sig>;

/* Containers. Containers.h declares every Kismet Array_ / Set_ / Map_ function as a method of the
   template it takes; AssetGen compiles `Items.Add(X)` back to Array_Add(Items, X). Num() is
   Length(). `A[i]` is EX_ArrayGetByRef. */
#include "Containers.h"

/* A container's braced list. As a member default AssetGen writes it into the CDO / asset:
   `TArray<int32> Primes = { 2, 3, 5 };`, `TMap<FName, int32> Cost = { { "Gold", 5 } };`. In a function, or as
   the value of a `static inline const` class variable (UeMeta.h), it is a Make Array / Set / Map where it is used. */
#include <initializer_list>

template <class T> struct TArray {
  T    *Data;
  int32 Num_;
  int32 Max_;

  TArray() = default;
  TArray(std::initializer_list<T>) {}
  UE_CONTAINER_TArray int32 Num() const;
  T                        &operator[](int32 I) { return Data[I]; }
  const T                  &operator[](int32 I) const { return Data[I]; }
  /* Range-for: AssetGen walks the array in place by index; a T& loop variable writes through. */
  T       *begin();
  T       *end();
  const T *begin() const;
  const T *end() const;
};

template <class T> struct TSet {
  uint8 Opaque[80];

  TSet() = default;
  TSet(std::initializer_list<T>) {}
  UE_CONTAINER_TSet int32 Num() const;
  /* Range-for: over a ToArray() copy, so elements are read-only. */
  const T *begin() const;
  const T *end() const;
};

/* A TMap element, for `for (auto [Key, Value] : Map)`. */
template <class K, class V> struct TPair {
  K Key;
  V Value;
};

template <class K, class V> struct TMap {
  uint8 Opaque[80];

  TMap() = default;
  TMap(std::initializer_list<TPair<K, V>>) {}
  UE_CONTAINER_TMap int32 Num() const;
  /* A read is Find(), the value type's default for a missing key; `Map[Key] = V` is Add(). A container method on
     `Map[Key]` runs on a copy, stored back when it writes. */
  V       &operator[](const K &Key);
  const V &operator[](const K &Key) const;
  /* Range-for: `auto& [Key, Value]` walks the map's own slots, so Value is the value where it lives. A body that adds
     to, removes from or sorts a map of this type, or a by-value `auto [Key, Value]`, walks a copy of Keys() instead. */
  TPair<const K, V>       *begin();
  TPair<const K, V>       *end();
  const TPair<const K, V> *begin() const;
  const TPair<const K, V> *end() const;
};
