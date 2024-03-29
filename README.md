# Tilt Five Unreal Engine SDK

SDK Version 1.6.0 / Runtime 1.4.1 "Jousting Jackalope"

## Dependencies

### Unreal Engine support

- Unreal Engine 5.3
- DirectX 11 DHI bridge

### Runtime

- [Tilt Five Drivers](https://docs.tiltfive.com/latest_release.html) version 1.4.1 or later

## Hardware and Targets supported

- Host with USB 3.0 high speed or faster port (some may require a USB 3.0 powered hub)
- Windows 10+ x86_64 platforms
- Multiple Glasses supported locally off one host, up to four, though graphics rendering complexity may be a limiting factor
  - Graphics core capability should be minimally equivalent to rendering at least 1440p/60fps per each Glasses
  - Two approximately 720p frames are rendered per Glasses processed into a single texture for all Glasses
  - This may reach practical limits of maximum texture size on some systems

## Documentation

A brief [Getting Started Guide](Documentation/Getting_Started.pdf) is provided in the Documentation directory.

New Tilt Five Components added:

- Tilt Five Camera Component
- Tilt Five Wand Component
- Tilt Five Manager

First time project setup:

- A new Tilt Five Manager Actor will need to be added to your scene. This controls adding and removing player pawns from the scene as glasses connect and disconnect
- The default Local Player class will need to be changed to either TiltFiveLocalPlayer.cpp or a class that inherits from the Tilt Five Local Player.
	- This setting can be found under Edit -> Project Settings
- You will need to create a new pawn to represent your Tilt Five view within the game. Some basic pawns have been provided, TiltFivePawn_Empty and TiltFivePawn_Wand. 
	- If you are creating your own pawn, the important information is:
		- If you want wand tracking for that pawn, be sure to add a Tilt Five Wand Component to the pawn. Also ensure the Motion Source settings for the Motion Controller component are set to "Right" for your primary wand, or "Left" for a secondary wand. Controller ID is managed automatically.
		- To enable camera tracking, you need to add a Tilt Five Camera Component to the Actor.
		- Both the Wand and the Camera component will use their object parent as the tracking origin. If you want to pan around the scene, it's recommended you move this scene component.
- For each desired player, set their Local Player Pawn class in the Tilt Five Manager to the pawn you wish to have spawned for them.
- To change which player's view is being shown on the Spectator Screen, adjust the Spectated Player value on the Tilt Five Manager.

Important notes:

- All headsets are indexed by their Device ID, 0, 1, 2, and 3, representing the first, second, third, and fourth glasses in the Tilt Five Control Panel. Newly connected glasses will always fill the lowest available slot.
- When trying to access a specific Player Controller, use UGameplayStatics::GetPlayerControllerFromID()
- It's recommended to use GEngine->XRSystem to locate the Tilt Five XR Tracking System, and GEngine->XRSystem->GetHMDDevice(int32 DeviceId) to access a specific HMD controller.
- HMD controllers will exist regardless of glasses connectivity, use GEngine->XRSystem->GetHMDDevice(int32 DeviceId)->IsHMDEnabled() to determine if there is a valid pair of glasses connected for a specific player.

## More Information

Please visit the [developers section](https://www.tiltfive.com/make/home) of Tilt Five's website for more information and resources on this and other development tools for the
Tilt Five system and our developer program. You can also find [news](https://www.tiltfive.com/news) and [support](https://www.tiltfive.com/support) for the Tilt Five system there.

## Legal Notices

Copyright 2024 Tilt Five, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
