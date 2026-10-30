// ModHubApi: ModHub's six Blueprint interfaces at /Game/_ModHub, with the signatures of ModHub 1.2.5's cooked ones
// (read with bpdecomp). Mods built against ModHub or Unified UI call them by name, so this copy and theirs are
// interchangeable whichever wins the mount. ElytrasModManager embeds this mod's pak (ElytrasModManager/HUB.md).
#pragma once
#include "../../UeApi/Types.h"
#include "../../UeApi/UeMeta.h"
#include "../../UeApi/Engine.h"
#include "../../UeApi/UMG.h"
#include "../../UeApi/Slate.h"

class IHub;
class IHubContext;
class IHubDataSource;
class IHubMod;
class IHubPageWidget;

// A mod's saved settings, as ModHub's settings pages edit them.
class IHubDataSource {
public:
    UE_INTERFACE;
    void SetFloatValue(FGuid SaveID, float Value);
    void Save();
};

// What a hub hands each mod (OnHubContext) and its pages: the hub, the mod and its data source.
class IHubContext {
public:
    UE_INTERFACE;
    void WriteLog(FString Title, FString LogContent);
    void SaveDataSource();
    TScriptInterface<IHubDataSource> GetDataSource();
    void GetMod(TScriptInterface<IHubMod>& Mod);
    void GetHub(TScriptInterface<IHub>& Hub);
};

// The host: what a mod may ask of the hub that registered it.
class IHub {
public:
    UE_INTERFACE;
    void EnableGameplayFeatures();
    int32 GetMaxPlayerCount();
    void WriteLog(FString Title, FString LogContent);
    void GetKeybinding(FName ActionName, FInputChord& Key);
    void RegisterKeybinding(TScriptInterface<IHubMod> Mod, FName ActionName, FText DisplayName, FInputChord DefaultKey);
    void ShowNotification(FString Title, FString Message, int32 Duration);
    void SetOpacity(float Opacity);
    void RefreshPages();
    void SelectPage(TScriptInterface<IHubPageWidget> HubPage);
    void CloseHub();
    void OpenHub();
};

// A mod the hub lists: an actor, found with GetAllActorsWithInterface.
class IHubMod {
public:
    UE_INTERFACE;
    void KeybindingChanged(FName ActionName, FInputChord NewKey);
    void MaxPlayersChanged(int32 LobbySize);
    void ModHubClosed();
    void ModHubOpened();
    void ModRegistered(TScriptInterface<IHub> Hub);
    void OnHubContext(const TScriptInterface<IHubContext>& HubContext);
    void GetDataSource(TScriptInterface<IHubDataSource>& DataSource);
    void GetModPages(TArray<TScriptInterface<IHubPageWidget>>& HubPages);
    void GetModInfo(FText& ModName, FText& ModAuthor, FText& ModVersion);
};

// One page of a mod: a UUserWidget the hub shows in its content area.
class IHubPageWidget {
public:
    UE_INTERFACE;
    void HubPageClosed();
    void HubPageOpened();
    void BeginHubPage(const TScriptInterface<IHubContext>& HubContext);
    void GetContainerMode(bool& bStretchToContainer);
    void GetPageInfo(FText& PageName);
};

// A widget inside a page that wants the context.
class IHubWidget {
public:
    UE_INTERFACE;
    void LoadWidget(const TScriptInterface<IHubContext>& HubContext);
};
