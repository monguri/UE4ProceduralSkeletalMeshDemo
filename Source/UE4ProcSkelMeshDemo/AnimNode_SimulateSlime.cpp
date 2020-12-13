#include "AnimNode_SimulateSlime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_SimulateSlime::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()���Q�l�ɂ��Ă���
	USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();

	// �z������Z�b�g���A�܂�����Evaluate�ō�蒼��
	Spheres.Empty();

	OwnerActor = nullptr;

	// �I�[�i�[�̃A�N�^�����[�g�ɓo���čs���ĒT��
	UObject* CurrentObject = SkeletalMeshComp;
	while (CurrentObject)
	{
		CurrentObject = CurrentObject->GetOuter();
		OwnerActor = Cast<AActor>(CurrentObject);
		if (OwnerActor != nullptr)
		{
			break;
		}
	}
}

void FAnimNode_SimulateSlime::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	RootBone.Initialize(RequiredBones);
}

void FAnimNode_SimulateSlime::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);

	DeltaTime = Context.GetDeltaTime();
}

bool FAnimNode_SimulateSlime::IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones)
{
	return (UsePhysicsAsset != nullptr && RootBone.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_SimulateSlime::InitSpheres(FComponentSpacePoseContext& Input)
{
	check(UsePhysicsAsset != nullptr);

	const FBoneContainer& BoneContainer = Input.Pose.GetPose().GetBoneContainer();
	const USkeletalMeshComponent* SkeletalMeshComp = Input.AnimInstanceProxy->GetSkelMeshComponent();

	for (int32 i = 0; i < UsePhysicsAsset->SkeletalBodySetups.Num(); ++i)
	{
		if (UsePhysicsAsset->SkeletalBodySetups[i]->BoneName.IsNone())
		{
			continue;
		}

		if (UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems.Num() == 0)
		{
			continue;
		}

		// ��{�̍����ƂɈ�̃X�t�B�A�����z�肵�Ȃ��B�܂��A�X�t�B�A�ȊO�͍l���Ȃ��B�����A�Z�b�g�̃f�[�^�����I�ɏ�������邱�Ƃ��z�肵�Ȃ��B
		const FKSphereElem& SphereElem = UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems[0]; 

		FPhysicsAssetSphere Sphere;
		Sphere.Radius = SphereElem.Radius;

		// ���I�Ƀ��b�V�����ς�邱�Ƃ͑z�肵�Ȃ�
		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(UsePhysicsAsset->SkeletalBodySetups[i]->BoneName);
		Sphere.BoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);

		FTransform BoneTM = Input.Pose.GetComponentSpaceTransform(Sphere.BoneIndex);
		float Scale = BoneTM.GetScale3D().GetAbsMax();
		FVector VectorScale(Scale);
		BoneTM.RemoveScaling();

		FTransform SphereLocalTM = SphereElem.GetTransform();
		SphereLocalTM.ScaleTranslation(VectorScale);

		const FTransform& SphereCompTM = SphereLocalTM * BoneTM;

		// ���̓|�[�Y�̔��f�͂��̏��������̈�񂵂��s��Ȃ�
		Sphere.WorkLocation = SphereCompTM.GetLocation();
		Sphere.WorkPrevLocation = Sphere.WorkLocation;

		Spheres.Add(Sphere);
	}
}

namespace
{
	// KismetTraceUtils.cpp��DebugDrawSweptSphere()���Q�l�ɂ��Ă���
	void DebugDrawSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
	{
		FVector const TraceVec = End - Start;
		float const Dist = TraceVec.Size();

		FVector const Center = Start + TraceVec * 0.5f;
		float const HalfHeight = (Dist * 0.5f) + Radius;

		FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
		::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	// KismetTraceUtils.cpp��DrawDebugSphereTraceSingle()���Q�l�ɂ��Ă���
	void DebugDrawSphereTraceSingle(const UWorld* World, const FVector& Start, const FVector& End, float Radius, bool bHit, const FHitResult& OutHit, FLinearColor TraceColor, FLinearColor TraceHitColor)
	{
		bool bPersistent = false;
		float LifeTime = 0.0f;

		if (bHit && OutHit.bBlockingHit)
		{
			::DebugDrawSweptSphere(World, Start, OutHit.Location, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DebugDrawSweptSphere(World, OutHit.Location, End, Radius, TraceHitColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
			::DrawDebugPoint(World, OutHit.ImpactPoint, 16.0f, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
		else
		{
			::DebugDrawSweptSphere(World, Start, End, Radius, TraceColor.ToFColor(true), bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
		}
	}
}

void FAnimNode_SimulateSlime::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const FTransform& CSToWS = SkeletalMeshComp->GetComponentToWorld();

	// Spheres����Ȃ̂������������Ƃ��ď�����
	if (Spheres.Num() == 0)
	{
		InitSpheres(Output);
		
		// �����A�Z�b�g�ɗL���ȏ�񂪂Ȃ��Ĕz�񂪍���Ȃ���Ώ������Ȃ�
		if (Spheres.Num() == 0)
		{
			return;
		}

		PreCSToWS = CSToWS;
	}

	// IsValidToEvaluate()�ɂ͂��̏����͓�����Ȃ��BIsValidToEvaluate()��UpdateInternal()�Ăяo���̏����ł�����̂ŁB
	if (DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()��ElemSelectedColor�̐F���f�o�b�K�Œl�𒲂ׂ�
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	check(DeltaTime > KINDA_SMALL_NUMBER);

	// Root�Ƃ̋��������a�̋������ێ�����ʒu�␳������B
	// �R���|�[�l���g���W�ɑ΂��ČŒ肷��͂��Ȃ���Root����ǂꂾ���ł����R�ɗ���Ă��܂��̂ŁB
	// �΂˂Ȃ��B
	// �����͐��񂵂Ȃ��B�R���X�g���C���g�Ƃ������͓��̓��[�V�����ɂ���ʒu���߂Ȃ̂ŁA�x�����ϕ�����ɂ���B
	FCompactPoseBoneIndex RootBoneIndex = RootBone.GetCompactPoseIndex(BoneContainer);
	const FVector& RootPosePos = Output.Pose.GetComponentSpaceTransform(RootBoneIndex).GetLocation();
	for (FPhysicsAssetSphere& Sphere : Spheres)
	{
		const FVector& Diff = Sphere.WorkLocation - RootPosePos;
		Sphere.WorkLocation += -(Diff.Size() - Sphere.Radius) * Diff.GetSafeNormal();
	}

	// ���[���h���W�ł̈ړ��́A�R���|�[�l���g���W�ւ̔��f���̌v�Z
	const FVector& ComponentMove = CSToWS.InverseTransformPosition(PreCSToWS.GetLocation());
	PreCSToWS = CSToWS;

	// �R���|�[�l���g�X�y�[�X�ł̏d��*���ʂ̃x�N�g��
	const FVector& GravityCS = CSToWS.InverseTransformVector(Gravity);

	// �x�����ϕ�
	for (FPhysicsAssetSphere& Sphere : Spheres)
	{
		FVector Velocity = (Sphere.WorkLocation - Sphere.WorkPrevLocation) / DeltaTime;
		Velocity *= (1.0f - Damping);
		Sphere.WorkPrevLocation = Sphere.WorkLocation;
		Sphere.WorkLocation += Velocity * DeltaTime;
		Sphere.WorkLocation += ComponentMove * (1.0f - WorldDampingLocation);
		Sphere.WorkLocation += 0.5 * GravityCS * DeltaTime * DeltaTime;
	}

	// �X�t�B�A���m�̑��ݍ�p�B�����ڐG���鋗�����m�ɂȂ�悤�Ɉʒu�␳����B
	// �΂˂���B
	for (int32 i = 0; i < Spheres.Num(); ++i)
	{
		FPhysicsAssetSphere& Sphere = Spheres[i];

		for (int32 j = 0; j < Spheres.Num(); ++j)
		{
			if (i == j)
			{
				continue;
			}

			FPhysicsAssetSphere& AnotherSphere = Spheres[j];

			float TargetRadius = Sphere.Radius + AnotherSphere.Radius;
			const FVector& Diff = Sphere.WorkLocation - AnotherSphere.WorkLocation;

			Sphere.WorkLocation += -(Diff.Size() - TargetRadius) * Diff.GetSafeNormal() * Stiffness;
		}
	}

	// ���̃I�u�W�F�N�g�Ƃ̃R���W����
	// Radius * PenetrateDepthRadiusRatio�̋����ɂȂ�悤�Ɉʒu�␳����B
	// �΂˂���B
	if (bEnableCollision)
	{
		for (FPhysicsAssetSphere& Sphere : Spheres)
		{
			const FVector& PosWS = CSToWS.TransformPosition(Sphere.WorkLocation);
			bool bTraceComplex = false;

			FHitResult HitResult;

			// KismetTraceUtils.cpp��ConfigureCollisionParams()���Q�l�ɂ��Ă���
			static const FName SphereTraceSingleName(TEXT("SphereTraceSingle"));
			FCollisionQueryParams Params(SphereTraceSingleName, TStatId(), bTraceComplex);
			Params.bReturnPhysicalMaterial = true;
			Params.bReturnFaceIndex = false;
			Params.AddIgnoredComponent(SkeletalMeshComp);
			if (OwnerActor != nullptr)
			{
				Params.AddIgnoredActor(OwnerActor);
			}

			// �N�G���̓X�t�B�A�̂��邻�̏ꏊ�ŁA���߂�Hit�ЂƂ̂݊m�F����
			bool bHit = World->SweepSingleByChannel(HitResult, PosWS, PosWS, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeSphere(Sphere.Radius), Params);
			if (bHit)
			{
				const FVector& ImpactPosCS = CSToWS.InverseTransformPosition(HitResult.ImpactPoint);
				const FVector& ImpactNormalCS = CSToWS.InverseTransformVector(HitResult.ImpactNormal).GetSafeNormal();

				// UKismetSystemLibrary::SphereTraceSingle()�ł����Ă��X�t�B�A�ƃ|���S���Ƃ̃R���W�����ł́A
				// �X�t�B�A�̒��S�Ƃ̍ŋߐړ_��ImpactPoint�Ƃ����悤���B�܂�X�t�B�A���S�_����|���S����
				// �Ђ��������̌�_
				float OverPenetrateDistance = Sphere.Radius * PenetrateDepthRadiusRatio - ((Sphere.WorkLocation - ImpactPosCS) | ImpactNormalCS); // DotProduct�ɂ��邱�ƂŁA���S�������߂肱�񂾃P�[�X�ł��Ή�����
				if (OverPenetrateDistance > 0.0f)
				{
					// TODO: Stiffness�ŉ����o��������ƁA�����o�������������đ��̋����ɂ��ʒu�ړ��̕����ړ��ʂ��傫���ĕ����Ă��܂�
					// TODO: �����A�傫��Stiffness�ŉ����o�����Ă��A���̂��͂��ނ悤�ȓ����ŁA���܂�S���̂��ۂ��͂��݂ɂȂ��ĂȂ��̂ŒP���ȉ����o���ŏ\�����Ǝv��
					//Sphere.WorkLocation += OverPenetrateDistance * ImpactNormalCS * Stiffness;
					Sphere.WorkLocation += OverPenetrateDistance * ImpactNormalCS;
				}
			}

			if (bDebugDrawCollisionQuery)
			{
				float Radius = Sphere.Radius;

				FFunctionGraphTask::CreateAndDispatchWhenReady(
					[World, PosWS, Radius, bHit, HitResult]()
					{
						bool bPersistent = false;
						float LifeTime = 0.0f;
						const FColor& ShapeColor = FColor::Blue;
						const FLinearColor& TraceColor = FLinearColor::Red;
						const FLinearColor& TraceHitColor = FLinearColor::Green;

						DebugDrawSphereTraceSingle(World, PosWS, PosWS, Radius, bHit, HitResult, TraceColor, TraceHitColor);
					},
					TStatId(), nullptr, ENamedThreads::GameThread
				);
			}
		}
	}

	for (const FPhysicsAssetSphere& Sphere : Spheres)
	{
		OutBoneTransforms.Add(FBoneTransform(Sphere.BoneIndex, FTransform(Sphere.WorkLocation)));
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());

	// �f�o�b�O�`��
	if (bDebugDrawPhysicsAsset)
	{
		for (const FPhysicsAssetSphere& Sphere : Spheres)
		{
			float Radius = Sphere.Radius;
			FVector LocationWS = CSToWS.TransformPosition(Sphere.WorkLocation);

			// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()���Q�l�ɂ��Ă���
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[World, LocationWS, Radius, ElemSelectedColor]()
				{
					bool bPersistent = false;
					float LifeTime = 0.0f;
					const FColor& ShapeColor = FColor::Blue;

					::DrawDebugSphere(World, LocationWS, Radius, 16, ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
	}
}

