#include <jni.h>
#include "BNMResolve.hpp" // also includes all the bnm includes.

using namespace BNM;

// void methods
void UnlockCosmetics();
//

// enums
Class VRRig;
Class StoreUpdater;
Class CosmeticsController;
Class CosmeticSet;
Class GTPlayer; // to hook awake to set the cosmetics controller instance.
//

// enums
enum CosmeticSlots
{
    Hat,
    Badge,
    Face,
    ArmLeft,
    ArmRight,
    BackLeft,
    BackRight,
    HandLeft,
    HandRight,
    Chest,
    Fur,
    Shirt,
    Pants,
    Back,
    Arms,
    TagEffect,
    Count
};
//

// structs
struct CosmeticItem { // cosmeticitem struct (from cosmeticscontroller) it works when you use this struct with an instance of cosmeticscontroller.
    Mono::String* itemName;
    int itemCategory;
    bool isHoldable;
    bool isThrowable;
    Sprite* itemPicture;
    Mono::String* displayName;
    Mono::String* itemPictureResourceString;
    Mono::String* overrideDisplayName;
    int cost;
    Mono::Array<Mono::String*>* bundledItems;
    bool canTryOn;
    bool bothHandsHoldable;
    bool bLoadsFromResources;
    bool bUsesMeshAtlas;
    Vector3 rotationOffset;
    Vector3 positionOffset;
    Mono::String* meshAtlasResourceString;
    Mono::String* meshResourceString;
    Mono::String* materialResourceString;
    bool isNullItem;
};
//

// methods
Method<CosmeticItem> GetItemFromDict;
Method<Mono::String*> SlotPlayerPreferenceName;
Method<void> UnlockItem;
Method<Mono::List<CosmeticItem>*> allCosmeticsm;
//


// fields
Field<Mono::String*> concatStringCosmeticsAllowedf;
Field<Mono::Array<CosmeticItem>*> itemsf;
//

// instances
Il2CppObject* cosmControllerInstance; // cosmetics controller instance is needed in things that arent in cosmetics controller, so i need to get the instance manually and heres the main variable def for it.
//

// AllowedPatch.cs from https://github.com/iiDk-the-actual/ForeverCosmetx/blob/master/Patches/AllowedPatch.cs
bool (*old_IsItemAllowed)(void*);
bool new_IsItemAllowed(void* instance) {
    return true;
}

// PostGetData.cs from https://github.com/iiDk-the-actual/ForeverCosmetx/blob/master/Patches/PostGetData.cs
void (*old_InitializeStoreUpdater)(void*);
void new_InitializeStoreUpdater(void* instance) {
    UnlockCosmetics();
}

// NoNull.cs from https://github.com/iiDk-the-actual/ForeverCosmetx/blob/master/Patches/NoNull.cs
void (*old_LoadFromPlayerPreferences)(void*, void*);
void new_LoadFromPlayerPreferences(void* instance, void* controller) {
    int num = 16;
    for (int i = 0; i < num; i++) {
        CosmeticSlots slot = (CosmeticSlots)i;
        GetItemFromDict.SetInstance(reinterpret_cast<Il2CppObject*>(controller)); // instance set to controller
        //SlotPlayerPreferenceName.SetInstance(reinterpret_cast<Il2CppObject*>(instance));  NO NEED TO SET INSTANCE ITS STATIC !!!!
        // CosmeticItem item = GetItemFromDict(PlayerPrefs::GetString(SlotPlayerPreferenceName(slot)->str(), "NOTHING")); oops thats not how you do it.
        std::string slotplayerprefname = PlayerPrefs::GetString(SlotPlayerPreferenceName(slot)->str(), "NOTHING");
        CosmeticItem item = GetItemFromDict(CreateMonoString(slotplayerprefname));
        itemsf = CosmeticSet.GetField("items");
        itemsf.SetInstance(reinterpret_cast<Il2CppObject*>(instance)); // diff instance set because its cosmeticset not cosmeticscontroller.
        Mono::Array<CosmeticItem>* items = itemsf();
        items->GetData()[i] = item; // ts took a lil to figure out (its a thing in basicmonostructures)
    }
}



// Just to get instance in c++ (no need in c# with harmony patching)
void (*old_GTPlayerAwake)(void*);
void new_GTPlayerAwake(void* instance) {
    cosmControllerInstance = GameObject::FindObjectOfType(CosmeticsController.GetMonoType()); // sets the instance the earliest it can (since findobjects of type only works when unity fully loads)
    old_GTPlayerAwake(instance);
}

//UnlockCosmetics in Plugin.cs from https://github.com/iiDk-the-actual/ForeverCosmetx/blob/master/Plugin.cs
void UnlockCosmetics() {
    UnlockItem = CosmeticsController.GetMethod("UnlockItem", 2); // set parameters needed (you dont need to)
    UnlockItem.SetInstance(cosmControllerInstance); // instance set
    allCosmeticsm = CosmeticsController.GetMethod("get_allCosmetics");
    allCosmeticsm.SetInstance(cosmControllerInstance); // instance set
    Mono::List<CosmeticItem>* allCosmetics = allCosmeticsm();
    concatStringCosmeticsAllowedf.SetInstance(cosmControllerInstance); // instance set
    //Mono::String* concatStringCosmeticsAllowed = concatStringCosmeticsAllowedf(); no need for concat string, if u wanted to make it mod sided you would modify this to be every cosmetic. in iidks code he checks this to see if u already own the item (this gives bugs in c++ and i found a workaround)
    for (int i = 0; i < allCosmetics->GetSize(); ++i) {
        CosmeticItem* cosmeticItem = allCosmetics->At(i).value; // direct pointer to value
        std::string name = cosmeticItem->itemName->str();
        if (name.find(' ') != std::string::npos) { // alternative to doing whatever the c# code does over on iidks github. i was having issues for some reason so i just made it unlock every cosm (that doesnt have a space in it because that breaks)
            continue;//                           basically it just unlocks all items that dont have a space in it (like bundles).
        }
        UnlockItem(cosmeticItem->itemName, false);
    }
    //CosmeticsController.instance.OnCosmeticsUpdated.Invoke();
    // ^^^ actions without types are cooked in bnm. i tried doing it manually but its a little weird and it auto calls oncosmeticsupdated anyway in game when you authenticate.
}

// i guess similar to awake in Plugin.cs from https://github.com/iiDk-the-actual/ForeverCosmetx/blob/master/Plugin.cs but its more for loading stuff and hooking (similar to applyharmonypatches in c#)
void OnLoaded() {
    VRRig = Class("", "VRRig");
    StoreUpdater = Class("GorillaNetworking.Store", "StoreUpdater");
    CosmeticsController = Class("GorillaNetworking", "CosmeticsController");
    GTPlayer = Class("GorillaLocomotion", "GTPlayer");

    CosmeticSet = CosmeticsController.GetInnerClass("CosmeticSet");

    GetItemFromDict = CosmeticsController.GetMethod("GetItemFromDict");
    SlotPlayerPreferenceName = CosmeticSet.GetMethod("SlotPlayerPreferenceName");

    concatStringCosmeticsAllowedf = CosmeticsController.GetField("concatStringCosmeticsAllowed");

    InvokeHook(GTPlayer.GetMethod("Awake"), new_GTPlayerAwake, old_GTPlayerAwake);

    BasicHook(StoreUpdater.GetMethod("Initialize"), new_InitializeStoreUpdater, old_InitializeStoreUpdater);
    BasicHook(VRRig.GetMethod("IsItemAllowed"), new_IsItemAllowed, old_IsItemAllowed);
    BasicHook(CosmeticSet.GetMethod("LoadFromPlayerPreferences", 1), new_LoadFromPlayerPreferences, old_LoadFromPlayerPreferences);
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, [[maybe_unused]] void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    BNM::Loading::AddOnLoadedEvent(OnLoaded);
    BNM::Loading::TryLoadByJNI(env);
    return JNI_VERSION_1_6;
}