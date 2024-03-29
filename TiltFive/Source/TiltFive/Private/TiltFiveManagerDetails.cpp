// Fill out your copyright notice in the Description page of Project Settings.

#if WITH_EDITOR
#include "TiltFiveManagerDetails.h"
#include "TiltFiveManager.h"
#include "GameFramework/Pawn.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorCategoryUtils.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "TiltFiveManagerDetails"

TSharedRef<IDetailCustomization> TiltFiveManagerDetails::MakeInstance()
{
	return MakeShareable(new TiltFiveManagerDetails);
}

void TiltFiveManagerDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Settings|TiltFive", LOCTEXT("CatName", "Tilt Five Settings"), ECategoryPriority::Important);

	Category.AddCustomRow(LOCTEXT("Player1", "Local Player 1 Settings"));

	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, localPlayer1Pawn));
	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, player1FOV));

	Category.AddCustomRow(LOCTEXT("Player2", "Local Player 2 Settings"));

	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, localPlayer2Pawn));
	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, player2FOV));

	Category.AddCustomRow(LOCTEXT("Player3", "Local Player 3 Settings"));

	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, localPlayer3Pawn));
	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, player3FOV));

	Category.AddCustomRow(LOCTEXT("Player4", "Local Player 4 Settings"));

	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, localPlayer4Pawn));
	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, player4FOV));

	Category.AddCustomRow(LOCTEXT("Keyword", "Global Settings"));

	Category.AddProperty(GET_MEMBER_NAME_CHECKED(ATiltFiveManager, spectatedPlayer));
}
#endif