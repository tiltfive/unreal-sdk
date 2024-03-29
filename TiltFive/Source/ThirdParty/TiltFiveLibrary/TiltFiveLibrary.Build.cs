// Copyright 2022 Tilt Five, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System.IO;
using UnrealBuildTool;

public class TiltFiveLibrary : ModuleRules
{
	public TiltFiveLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "include"));

			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "TiltFiveNative.dll.if.lib"));

            // Delay-load the DLL, so we can load it from the right place first
            PublicDelayLoadDLLs.Add("TiltFiveNative.dll");

			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add("$(PluginDir)/Binaries/ThirdParty/TiltFiveLibrary/Win64/TiltFiveNative.dll");
        }
	}
}
