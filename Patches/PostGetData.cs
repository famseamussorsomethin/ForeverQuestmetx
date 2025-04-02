﻿using GorillaNetworking;
using GorillaNetworking.Store;
using HarmonyLib;
using System;
using System.Collections.Generic;
using System.Text;

namespace ForeverCosmetx.Patches
{
    [HarmonyPatch(typeof(StoreUpdater))]
    [HarmonyPatch("Initialize", MethodType.Normal)]
    public class PostGetData
    {
        private static void Postfix()
        {
            Plugin.instance.UnlockCosmetics();
        }
    }
}
