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

public class TiltFive : ModuleRules
{
	public TiltFive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivatePCHHeaderFile = "Public/TiltFive.h";
#if UE_4_24_OR_LATER
		DefaultBuildSettings = BuildSettingsVersion.V2;
#endif

        PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"TiltFiveLibrary",
				"Projects",
				"HeadMountedDisplay",
#if UE_5_3_OR_LATER
				"XRBase",
#endif
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"RHI",
				"RenderCore",
				"EngineSettings",
				"Slate",
				"SlateCore",
            }
		);

        if (Target.Version.MinorVersion < 22 && Target.Version.MajorVersion == 4)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ShaderCore",
				});
		}

		if (Target.Version.MinorVersion < 24 && Target.Version.MajorVersion == 4)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UtilityShaders",
				});
		}

		if (Target.Version.MinorVersion >= 26 || Target.Version.MajorVersion == 5)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"DeveloperSettings",
				});
		}

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"MessageLog",
                "UnrealEd",
                "PropertyEditor",
                "EditorStyle",
            });
		}
	}
}