# Razigra setup

The C++ project, gameplay framework, local two-instance networking fallback, menu, and EOS session integration are implemented. The following steps require credentials or visual/editor decisions and therefore must be completed manually.

## Toolchain

1. Install Visual Studio 2022 with **Game development with C++**, MSVC v14.4x, Windows 11 SDK, and the Visual Studio Tools for Unreal Engine components.
2. Right-click `GamejamRazigra.uproject` and choose **Generate Visual Studio project files** if the generated solution is ever removed.
3. Open `GamejamRazigra.sln`, select **Development Editor / Win64**, and build `GamejamRazigraEditor`.

## Epic Online Services

1. In the [Epic Developer Portal](https://dev.epicgames.com/portal), create or select a product, sandbox, deployment, and client policy/client credentials.
2. Enable Epic Account Services and EOS Connect for the product. For development, permit the client policy actions needed for authentication, presence, lobbies, and sessions.
3. Configure the product's Epic Account Services application, allowed countries/permissions, and redirect URLs as required by Epic. Add both tester Epic accounts to the product organization or deployment access list.
4. Copy `Config/EOS.ini.example` to `Config/EOS.ini` on every machine, replace all six `REPLACE_ME` values, and use a 64-character hexadecimal `ClientEncryptionKey`. Transfer this local file privately; Git intentionally ignores it.
5. The default login mode is `accountportal`. For EOS Dev Auth Tool, run it on port 6300, create two profiles, and change the three values under `[GamejamRazigra.EOS]` as shown in the example. Each client needs its own profile/token. Command-line `-AUTH_TYPE`, `-AUTH_LOGIN`, and `-AUTH_PASSWORD` can instead be used with `AutoLogin` by leaving `LoginCredentialType` blank.
6. Package or launch two standalone clients. PIE is useful with `OnlineSubsystemNull`; EOS account portal/overlay testing is more reliable in Standalone or packaged builds.
7. For a packaged build, copy `EOS.ini` beside the staged project config files at `<PackagedRoot>/GamejamRazigra/Config/EOS.ini`. EOS client credentials ship with game clients, so restrict the client policy to only the minimum actions the game needs.

## Editor/content hookup

1. Open `/Game/Data/GlobalGameData`. It is the only global gameplay data asset; tune the gameplay map, menu class, HUD class, hero Blueprint, weapon, zombie, and session values there. You may create a Blueprint class derived from `UGlobalGameData`, but the runtime instance must remain named and located at `/Game/Data/GlobalGameData`.
2. The hero's appearance is owned by a Blueprint, not by the data asset. `GlobalGameData.HeroBlueprint` points at `/Game/Blueprints/BP_SharedHero` (created for you by the `CreateRazigraData` commandlet with the sample mannequin already assigned). Open that Blueprint to set the skeletal mesh, animation Blueprint, weapon attachments, muzzle sockets, and the `On Gun Fired` / `On Hero Died` events. Point `HeroBlueprint` at a different Blueprint to swap the survivor wholesale; if it is empty the game falls back to the plain C++ class, which has no mesh.
3. Open the map selected by `GlobalGameData.GameplayMap`, place and size one or more `NavMeshBoundsVolume` actors over every zombie-walkable area, and press **P** to verify the green navigation coverage. Navigation bounds are deliberately map-owned and are not created at runtime.
4. Place Blueprint children or instances of `AZombieSpawner` where zombies should appear. Configure **Spawner Active**, **Spawn Rate**, **Spawn Radius**, **Max Alive Zombies**, and **Zombie Class** on each placed spawner. No spawner is created automatically.
5. Optional presentation pass: derive Blueprint classes from `UGlobalGameData`, `AZombieCharacter`, `AZombieSpawner`, `ACoopGameMode`, `ACoopPlayerController`, `UCoopMenuWidget`, or `UCoopHudWidget`. Set the desired class references in `GlobalGameData` and implement the exposed fire/attack/death events for montages, muzzle flashes, sounds, decals, and UI.
6. If an imported animation Blueprint assumes variables from its original demo character and reports errors, derive a new animation Blueprint and drive it with `IsCharacterMoving`, `IsCharacterCrouching`, `IsCharacterJumping`, `IsCharacterFalling`, `IsCharacterFiring`, `IsAttacking`, and `GetMovementSpeed`.

## Waiting for both players

**Host Game** creates the session and immediately opens `GlobalGameData.GameplayMap` as a listen server, exactly as before — the map must be open before anyone can connect, and travelling again once a client is attached tears the connection down, so the host never travels twice.

The wait happens on that map instead: the menu stays up as a waiting room showing `x of 2 connected`, and the shared hero is not spawned until every required player has joined, so nobody plays alone. The joining client travels straight into the same map and picks up the hero as soon as it appears. Set `GlobalGameData.bWaitForAllPlayersBeforeStart` to false to spawn the hero the moment the map loads.

All connection and error messages are reported in the menu; nothing is drawn as on-screen debug text.

## In-game HUD

`UCoopHudWidget` is created automatically for each local player when the shared hero appears. It lists every consensus action (W / A / S / D / Space / Ctrl / LMB) and colours each card by who is holding it: red for player one, blue for player two, green once both agree. Colours, blend speed, and the panel tint are exposed under **Zombie Zero|HUD Style**; set `GlobalGameData.GameplayHudWidgetClass` to a Blueprint child to restyle it.

## Testing the consensus mechanic

Without `Config/EOS.ini`, the project intentionally uses OnlineSubsystemNull for LAN/local testing. Start one standalone instance and choose **Host Game**, then start a second and choose **Find & Join Game**. The shared character only moves, looks, crouches, jumps, or fires while the matching input is active on both clients.

For quick iteration, press Play from any editor map and choose **Single Player (Test)**. The game reloads the map referenced by `GlobalGameData.GameplayMap`, and one local player's input satisfies the consensus checks. Host and join retain the two-player requirement.
