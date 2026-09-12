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
2. The hero is owned by a Blueprint, not by the data asset. Right-click `SharedHeroCharacter` in the Content Browser and pick **Create Blueprint class based on** it (or run the `CreateRazigraData` commandlet, which generates `/Game/Blueprints/BP_SharedHero` with the sample mannequin already assigned), then point `GlobalGameData.HeroBlueprint` at it. In that Blueprint set the skeletal mesh, animation Blueprint, weapon meshes and sockets, and implement **On Gun Fired** for muzzle flashes, tracers, sounds, camera shake and impact decals — it fires on every machine and hands you the muzzle location, the impact point, whether anything was hit, and the hit actor. **On Hero Died** is available the same way. If `HeroBlueprint` is empty the game falls back to the plain C++ class and warns in the log.
3. Open the map selected by `GlobalGameData.GameplayMap`, place and size one or more `NavMeshBoundsVolume` actors over every zombie-walkable area, and press **P** to verify the green navigation coverage. Navigation bounds are deliberately map-owned and are not created at runtime.
4. Drag `AZombieSpawner` (or a Blueprint child of it) into the level wherever zombies should come from. Each one shows a wireframe sphere in the viewport previewing its **Spawn Radius**; zombies appear at a random navigable point inside it, or exactly at the actor when the radius is zero. Configure **Spawner Active**, **Spawn Rate**, **Spawn Radius**, **Max Alive Zombies**, and **Zombie Class** per spawner. No spawner is created automatically.
5. Optional presentation pass: derive Blueprint classes from `UGlobalGameData`, `AZombieCharacter`, `AZombieSpawner`, `ACoopGameMode`, `ACoopPlayerController`, `UCoopMenuWidget`, or `UCoopHudWidget`. Set the desired class references in `GlobalGameData` and implement the exposed fire/attack/death events for montages, muzzle flashes, sounds, decals, and UI.
6. If an imported animation Blueprint assumes variables from its original demo character and reports errors, derive a new animation Blueprint and drive it with `IsCharacterMoving`, `IsCharacterCrouching`, `IsCharacterJumping`, `IsCharacterFalling`, `IsCharacterFiring`, `IsAttacking`, and `GetMovementSpeed`.

## Waiting for both players

**Host Game** creates the session and immediately opens `GlobalGameData.GameplayMap` as a listen server, exactly as before — the map must be open before anyone can connect, and travelling again once a client is attached tears the connection down, so the host never travels twice.

The wait happens on that map instead: the menu stays up as a waiting room showing `x of 2 connected`, and the shared hero is not spawned until every required player has joined, so nobody plays alone. The joining client travels straight into the same map and picks up the hero as soon as it appears. Set `GlobalGameData.bWaitForAllPlayersBeforeStart` to false to spawn the hero the moment the map loads.

All connection and error messages are reported in the menu; nothing is drawn as on-screen debug text.

## In-game HUD

`UCoopHudWidget` is created automatically for each local player when the shared hero appears. Key caps are laid out the way they sit on a keyboard (W above A S D, with Ctrl / Space / LMB along the bottom) and each is coloured by who is holding it: red for player one, blue for player two, green once both agree. The two numbered badges on the left say which player you are — yours is solid, the other is dimmed. The HUD carries no explanatory text by design. Colours and blend speed are exposed on the widget under **Zombie Zero|HUD Style**, and every font the menu and HUD draw with lives in `GlobalGameData` under **UI|Fonts** (`MenuTitleFont`, `MenuButtonFont`, `MenuStatusFont`, `HudKeyFont`, `HudKeyLabelFont`) — point those at a font asset to restyle both at once. Set `GlobalGameData.GameplayHudWidgetClass` to a Blueprint child for anything deeper.

Aiming is a consensus action too. Both players' mouse deltas must arrive within `LookInputGraceSeconds` of each other, and they are then **summed** rather than averaged, so pulling in opposite directions cancels out and pulling together turns twice as fast. Aim has no key cap: it is metered instead, by four bars to the right of the keys — two horizontal (yaw, player one above player two) and two vertical (pitch). Each rests at centre and its fill edge swings either side with that player's mouse delta, so you can watch the two contributions add up or cancel. `GlobalGameData.LookMeterRange` sets the delta that pins a bar to an end.

While a player is waiting for the others, the view is held on black and fades up over `GlobalGameData.GameplayFadeInSeconds` the moment the shared hero appears.

## Combat feedback

The native gameplay HUD draws a centre-screen aim dot and a replicated hero health bar. LMB traces from the camera through that dot; `GlobalGameData.FireDamage`, `FireRange`, and `FireInterval` control the server-authoritative shot. Zombie hits create client-local floating world-space numbers above the target. Their font, colour, size, height, rise speed, and lifetime are under **UI|Damage Numbers** in `GlobalGameData`.

Zombie attacks now start their attack state and animation first, then apply `ZombieAttackDamage` only after `ZombieAttackWindupSeconds`. The hero must still be inside `ZombieAttackRange` when that delay expires, so moving away dodges the hit. `On Zombie Attack` is the Blueprint attack-tell hook, while `On Zombie Attack Resolved` reports whether the delayed strike connected.

Hero damage flashes red screen-edge bands and starts the camera shake selected by `HeroDamageCameraShakeClass`. Tune its colour, duration, and shake scale under **UI|Damage Feedback**. `On Hero Damaged` and `On Zombie Damaged` remain available to Blueprint children for additional audio, animation, particles, or hit reactions.

## Testing the consensus mechanic

Without `Config/EOS.ini`, the project intentionally uses OnlineSubsystemNull for LAN/local testing. Start one standalone instance and choose **Host Game**, then start a second and choose **Find & Join Game**. The shared character only moves, looks, crouches, jumps, or fires while the matching input is active on both clients.

For quick iteration, press Play from any editor map and choose **Single Player (Test)**. The game reloads the map referenced by `GlobalGameData.GameplayMap`, and one local player's input satisfies the consensus checks. Host and join retain the two-player requirement.
