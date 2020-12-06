#include "AnimNode_DrawPhysicsAsset.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_DrawPhysicsAsset::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()を参考にしている
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : SkeletalMeshComp->GetPhysicsAsset();
}

bool FAnimNode_DrawPhysicsAsset::IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones)
{
	return (UsePhysicsAsset != nullptr);
}

namespace
{
	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveTransform()を参考にしている
	FTransform GetPrimitiveTransform(UBodySetup* SharedBodySetup, FTransform& BoneTM, int32 BodyIndex, EAggCollisionShape::Type PrimType, int32 PrimIndex, float Scale)
	{
		FVector Scale3D(Scale);

		if (PrimType == EAggCollisionShape::Sphere)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.SphereElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Box)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.BoxElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Sphyl)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.SphylElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::Convex)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.ConvexElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}
		else if (PrimType == EAggCollisionShape::TaperedCapsule)
		{
			FTransform PrimTM = SharedBodySetup->AggGeom.TaperedCapsuleElems[PrimIndex].GetTransform();
			PrimTM.ScaleTranslation(Scale3D);
			return PrimTM * BoneTM;
		}

		// Should never reach here
		check(0);
		return FTransform::Identity;
	}
}

void FAnimNode_DrawPhysicsAsset::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()のElemSelectedColorの色をデバッガで値を調べた
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()を参考にしている

	// Draw bodies
	for (int32 i = 0; i <UsePhysicsAsset->SkeletalBodySetups.Num(); ++i)
	{
		if (!ensure(UsePhysicsAsset->SkeletalBodySetups[i]))
		{
			continue;
		}
		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(UsePhysicsAsset->SkeletalBodySetups[i]->BoneName);

		// If we found a bone for it, draw the collision.
		// The logic is as follows; always render in the ViewMode requested when not in hit mode - but if we are in hit mode and the right editing mode, render as solid
		if (BoneIndex != INDEX_NONE)
		{
			FTransform BoneTM = SkeletalMeshComp->GetBoneTransform(BoneIndex);
			float Scale = BoneTM.GetScale3D().GetAbsMax();
			FVector VectorScale(Scale);
			BoneTM.RemoveScaling();

			FKAggregateGeom* AggGeom = &UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom;

			for (int32 j = 0; j <AggGeom->SphereElems.Num(); ++j)
			{
				FTransform ElemTM = AggGeom->SphereElems[j].GetTransform();
				ElemTM.ScaleTranslation(VectorScale);
				ElemTM *= BoneTM;

				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[World, ElemTM, AggGeom, j, Scale, ElemSelectedColor]() {
						bool bPersistent = false;
						float LifeTime = 0.0f;
						const FColor& ShapeColor = FColor::Blue;

						::DrawDebugSphere(World, ElemTM.GetLocation(), AggGeom->SphereElems[j].Radius * Scale, 16, ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
					},
					TStatId(), nullptr, ENamedThreads::GameThread
				);
			}

			for (int32 j = 0; j <AggGeom->BoxElems.Num(); ++j)
			{
				FTransform ElemTM = AggGeom->BoxElems[j].GetTransform();
				ElemTM.ScaleTranslation(VectorScale);
				ElemTM *= BoneTM;

				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[World, ElemTM, AggGeom, j, Scale, ElemSelectedColor]() {
						bool bPersistent = false;
						float LifeTime = 0.0f;

						::DrawDebugBox(World, ElemTM.GetLocation(), Scale * 0.5f * FVector(AggGeom->BoxElems[j].X, AggGeom->BoxElems[j].Y, AggGeom->BoxElems[j].Z), ElemTM.GetRotation(), ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
					},
					TStatId(), nullptr, ENamedThreads::GameThread
				);
			}

			for (int32 j = 0; j <AggGeom->SphylElems.Num(); ++j)
			{
				FTransform ElemTM = AggGeom->SphylElems[j].GetTransform();
				ElemTM.ScaleTranslation(VectorScale);
				ElemTM *= BoneTM;

				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[World, ElemTM, AggGeom, j, Scale, ElemSelectedColor]() {
						bool bPersistent = false;
						float LifeTime = 0.0f;
						const FColor& ShapeColor = FColor::Blue;

						::DrawDebugCapsule(World, ElemTM.GetLocation(), AggGeom->SphylElems[j].Length * Scale, AggGeom->SphylElems[j].Radius * Scale, ElemTM.GetRotation(), ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
					},
					TStatId(), nullptr, ENamedThreads::GameThread
				);
			}

			for (int32 j = 0; j <AggGeom->ConvexElems.Num(); ++j)
			{
				FTransform ElemTM = AggGeom->ConvexElems[j].GetTransform();
				ElemTM.ScaleTranslation(VectorScale);
				ElemTM *= BoneTM;

#if 0 // TODO:とりあえず省略
				//convex doesn't have solid draw so render lines if we're in hitTestAndBodyMode
				AggGeom->ConvexElems[j].DrawElemWire(PDI, ElemTM, Scale, ElemSelectedColor);
#endif
			}

			for (int32 j = 0; j <AggGeom->TaperedCapsuleElems.Num(); ++j)
			{
				FTransform ElemTM = AggGeom->TaperedCapsuleElems[j].GetTransform();
				ElemTM.ScaleTranslation(VectorScale);
				ElemTM *= BoneTM;

#if 0 // TODO:とりあえず省略
				AggGeom->TaperedCapsuleElems[j].DrawElemSolid(PDI, ElemTM, VectorScale, ElemSelectedMaterial->GetRenderProxy());
				AggGeom->TaperedCapsuleElems[j].DrawElemWire(PDI, ElemTM, VectorScale, ElemSelectedColor);
#endif
			}
		}
	}
}

