# Mirror's Edge LAN (P2P Multiplayer)

Mirror's Edge LAN is a fork of [btbd/mmultiplayer](https://github.com/btbd/mmultiplayer), Only possible thanks to it. It is not affiliated with or endorsed by EA DICE or Electronic Arts. This was tested and made for a Steam copy (`MirrorsEdge.exe` v1.0.1.0).

A project to get Mirror's Edge Multiplayer working **in P2P**, no more server needed.

If you want to give a thanks: https://ko-fi.com/birden. No perks or help will be given to this, Ko-fi is actually for different non-AI projects I do.

---

## Getting started

1. Download the release files from HERE

2. Place `mmultiplayer.asi` and `mmultiplayer.cfg` inside your game's `Binaries` folder right next to `MirrorsEdge.exe`.

3. Download [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) (32-bit / Win32 `dsound.dll` for example). Then drop `dsound.dll` next to `MirrorsEdge.exe`.

---

## Using in-game

1. Launch `MirrorsEdge.exe`.
2. **Multiplayer connects automatically!** As soon as the game starts, it binds an available UDP port (`5222`–`5237`) and broadcasts your presence to your local network (`255.255.255.255`) and `localhost` (`127.0.0.1`).
3. Press **`Insert`** (default keybind) at any time to open the **MMultiplayer** menu:
   - **Multiplayer tab**: Change your player name, character model (Faith, Kate, Celeste, Jacknife, Miller, Merc, etc.), room name, chat keybind, or set a Direct Peer IP (useful for Radmin VPN / ZeroTier / Hamachi if broadcast is blocked).
   - **Engine tab**: Run Unreal console commands, toggle framerate smoothing, adjust gamma, or change the menu keybind.
   - **World tab**: Adjust Time Dilation, Gravity, and inspect streaming levels.
   - **Player tab**: Toggle real-time position/velocity/movement state HUD (`X`, `Y`, `Z`, `V`, `RX`, `RY`, `S`).
   - **Trainer tab**: Save/Load position & movement states, God mode, Fly mode, Kickglitch (KG), Beamer, and Strang helpers.
   - **Dolly tab**: Keyframe camera paths and record/replay player ghost runs.

---

## Multiplayer Settings

All settings are in the `mmultiplayer.cfg` file right next to the `.asi`/`.dll`. If the file doesn't exist, the mod creates it automatically with default values. Any changes you make in the in-game menu are saved back to this file.

Example `mmultiplayer.cfg`:

```ini
[display]
fullscreen=1
res=0x0

[client]
character=3
chatKeybind=0
disabled=false
name=player
peerIp=
room=lobby
showChatOverlay=true
showNameTags=true

[menu]
showKeybind=45

[player]
showInfo=false

[trainer]
beamerKeybind=84
enabled=false
flyDownKeybind=16
flyFasterKeybind=69
flyKeybind=50
flySlowerKeybind=81
flyUpKeybind=32
godKeybind=49
kgKeybind=69
loadKeybind=53
overlay=true
saveKeybind=52
strangKeybind=82
```

## How to compile from source

### Requirements
- Windows with **Visual Studio 2019 / 2022 / 2026** (Desktop development with C++ workload and MSVC v142/v143/v145 x86 build tools + Windows 10 SDK).

### Option 1: Build via PowerShell / MSBuild (Command Line)
Open PowerShell in the repository root and run:

```powershell
# 1. Build the Client DLL / ASI (Release | Win32)
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "Client\Client.vcxproj" /p:Configuration=Release /p:Platform=Win32 /m
```
*(Adjust the `MSBuild.exe` path if you are using VS 2019/2022 Professional or Enterprise).*

### Option 2: Build via Visual Studio IDE
1. Open `Client\Client.vcxproj` in Visual Studio.
2. Set the configuration to **`Release`** and platform to **`x86` (`Win32`)**.
3. Right-click the **`Client`** project and click **Build**.
4. Grab `Client\Release\Client.dll` and rename it to `mmultiplayer.asi` (or copy it from `output\mmultiplayer.asi`).