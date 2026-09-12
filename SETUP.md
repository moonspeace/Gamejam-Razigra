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
4. Copy `Config/EOS.ini.example` to `Config/EOS.ini` and replace all six `REPLACE_ME` values. `EncryptionKey` must be 64 hexadecimal characters. The real file is intentionally ignored by Git.
5. The default login mode is `accountportal`. For EOS Dev Auth Tool, run it on port 6300, create two profiles, and change the three values under `[GamejamRazigra.EOS]` as shown in the example. Each client needs its own profile/token. Command-line `-AUTH_TYPE`, `-AUTH_LOGIN`, and `-AUTH_PASSWORD` can instead be used with `AutoLogin` by leaving `LoginCredentialType` blank.
6. Package or launch two standalone clients. PIE is useful with `OnlineSubsystemNull`; EOS account portal/overlay testing is more reliable in Standalone or packaged builds.
7. For a packaged build, keep `EOS.ini` external: copy it beside the staged project config files at `<PackagedRoot>/GamejamRazigra/Config/EOS.ini`. Do not put production client secrets in source control; use a client policy restricted to the minimum EOS actions.

## Editor/content hookup

1. Open `/Game/Data/GlobalGameData`. It is the only global gameplay data asset; tune the gameplay map, menu class, hero, weapon, zombie, mesh, animation, and session values there. You may create a Blueprint class derived from `UGlobalGameData`, but the runtime instance must remain named and located at `/Game/Data/GlobalGameData`.
2. Open the map selected by `GlobalGameData.GameplayMap`, place and size one or more `NavMeshBoundsVolume` actors over every zombie-walkable area, and press **P** to verify the green navigation coverage. Navigation bounds are deliberately map-owned and are not created at runtime.
3. Place Blueprint children or instances of `AZombieSpawner` where zombies should appear. Configure **Spawner Active**, **Spawn Rate**, **Spawn Radius**, **Max Alive Zombies**, and **Zombie Class** on each placed spawner. No spawner is created automatically.
4. Optional presentation pass: derive Blueprint classes from `UGlobalGameData`, `ASharedHeroCharacter`, `AZombieCharacter`, `AZombieSpawner`, `ACoopGameMode`, `ACoopPlayerController`, or `UCoopMenuWidget`. Set the desired class references in `GlobalGameData` and implement the exposed fire/attack/death events for montages, muzzle flashes, sounds, decals, and UI.
5. If an imported animation Blueprint assumes variables from its original demo character and reports errors, derive a new animation Blueprint and drive it with `IsCharacterMoving`, `IsCharacterCrouching`, `IsCharacterJumping`, `IsCharacterFalling`, `IsCharacterFiring`, `IsAttacking`, and `GetMovementSpeed`.

## Testing the consensus mechanic

Without `Config/EOS.ini`, the project intentionally uses OnlineSubsystemNull for LAN/local testing. Start one standalone instance and choose **Host Game**, then start a second and choose **Find & Join Game**. The shared character only moves, looks, crouches, jumps, or fires while the matching input is active on both clients.

For quick iteration, press Play from any editor map and choose **Single Player (Test)**. The game reloads the map referenced by `GlobalGameData.GameplayMap`, and one local player's input satisfies the consensus checks. Host and join retain the two-player requirement.
