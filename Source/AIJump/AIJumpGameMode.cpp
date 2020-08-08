// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIJumpGameMode.h"
#include "AIJumpHUD.h"
#include "AIJumpCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAIJumpGameMode::AAIJumpGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AAIJumpHUD::StaticClass();
}
