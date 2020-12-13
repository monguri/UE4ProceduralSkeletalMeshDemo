#include "ThirdPersonSlimeGameMode.h"
#include "ThirdPersonCpp425Character.h"
#include "UObject/ConstructorHelpers.h"

AThirdPersonSlimeGameMode::AThirdPersonSlimeGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/BP_ThirdPersonSlime"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
