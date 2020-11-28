// Copyright monguri. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProceduralSkeletalMeshFunctionLibrary.generated.h"

UCLASS()
class UE4PROCSKELMESHDEMO_API UProceduralSkeletalMeshFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

#if WITH_EDITOR
	/** 
	 * Create skeletal mesh of a triangle and skeleton asset.
	 * @return True if the operation succeeds, check the log for errors if it didn't succeed.
	 */
	UFUNCTION(BlueprintCallable)
	static bool CreateTriangleSkeletalMesh();

	/** 
	 * Create skeletal mesh of spheres and skeleton asset.
	 * @return True if the operation succeeds, check the log for errors if it didn't succeed.
	 */
	UFUNCTION(BlueprintCallable)
	static bool CreateSpheresSkeletalMesh();

	/** 
	 * Create skeletal mesh of a box and skeleton asset.
	 * @return True if the operation succeeds, check the log for errors if it didn't succeed.
	 */
	UFUNCTION(BlueprintCallable)
	static bool CreateBoxSkeletalMesh();
#endif
};

