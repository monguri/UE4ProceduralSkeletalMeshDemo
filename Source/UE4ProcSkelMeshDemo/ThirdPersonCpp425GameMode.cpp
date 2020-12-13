// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonCpp425GameMode.h"
#include "ThirdPersonCpp425Character.h"
#include "UObject/ConstructorHelpers.h"

AThirdPersonCpp425GameMode::AThirdPersonCpp425GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
