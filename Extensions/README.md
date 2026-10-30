# Extensions

Headers for APIs that are not the game's own: interfaces and classes that other mods ship, which a mod can
call when those mods are installed. They compile against the vanilla `UeApi/` beside this folder.

| Folder    | API                                                                         | Matches       | Installed by                       |
| --------- | --------------------------------------------------------------------------- | ------------- | ---------------------------------- |
| `ModHub/` | ModHub's six Blueprint interfaces at `/Game/_ModHub` (`IHub`, `IHubMod`, …) | ModHub 1.2.5  | ModHub, Unified UI, or EMM         |
| `EMM/`    | Elytras Mod Manager's public API: `EmmApi_C`, its services, its widgets     | EMM 2         | Elytras Mod Manager                |

Include them by path from your mod's folder, with `UeApi/` and `Extensions/` side by side:

```cpp
#include "../Extensions/ModHub/ModHubApi.h"
#include "../Extensions/EMM/EmmApi_C.h"
```

A mod that uses an extension does not contain it. The compiler imports these classes by name, so your pak must not
hold any file under `_ModHub/` or `_ElytrasMods/`. Players need the mod that provides the API installed; without it,
the calls find nothing.

In ue-mods, `BpMods/Extensions` is the source and `BpMods/tools/sdk_extensions.py` keeps both copies in step: `emm`
regenerates `EMM/` from `emmapi_export.py`'s genueapi output, and `sdk <checkout>` copies this folder into the SDK
after compiling CaveLayouts and EmmApiExample against that SDK alone.
