#include "AnimNode_SimulateSlime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_SimulateSlime::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()���Q�l�ɂ��Ă���
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();

	// �z������Z�b�g���A�܂�����Evaluate�ō�蒼��
	Spheres.Empty();
}

void FAnimNode_SimulateSlime::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);

	DeltaTime = Context.GetDeltaTime();
}

bool FAnimNode_SimulateSlime::IsValidToEvaluate(const class USkeleton* Skeleton, const struct FBoneContainer& RequiredBones)
{
	return (UsePhysicsAsset != nullptr);
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

void FAnimNode_SimulateSlime::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (Spheres.Num() == 0)
	{
		InitSpheres(Output);
	}

	// IsValidToEvaluate()�ɂ͂��̏����͓�����Ȃ��BIsValidToEvaluate()��UpdateInternal()�Ăяo���̏����ł�����̂ŁB
	if (DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()��ElemSelectedColor�̐F���f�o�b�K�Œl�𒲂ׂ�
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	check(DeltaTime > KINDA_SMALL_NUMBER);

	// �x�����ϕ�
	for (int32 i = 0; i < Spheres.Num(); ++i)
	{
		FPhysicsAssetSphere& Sphere = Spheres[i];

		FVector Velocity = (Sphere.WorkLocation - Sphere.WorkPrevLocation) / DeltaTime;
		Velocity *= (1.0f - Damping);
		Sphere.WorkPrevLocation = Sphere.WorkLocation;
		Sphere.WorkLocation += Velocity * DeltaTime;
	}

	// ���ݍ�p
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
			// TODO:����
		}
	}

	// �f�o�b�O�`��
	if (bDebugDrawPhysicsAsset)
	{
		for (int32 i = 0; i < Spheres.Num(); ++i)
		{
			FPhysicsAssetSphere& Sphere = Spheres[i];
			float Radius = Sphere.Radius;
			FVector Location = Sphere.WorkLocation;

			// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()���Q�l�ɂ��Ă���
			FFunctionGraphTask::CreateAndDispatchWhenReady(
				[World, Location, Radius, ElemSelectedColor]()
				{
					bool bPersistent = false;
					float LifeTime = 0.0f;
					const FColor& ShapeColor = FColor::Blue;

					::DrawDebugSphere(World, Location, Radius, 16, ElemSelectedColor, bPersistent, LifeTime, ESceneDepthPriorityGroup::SDPG_Foreground);
				},
				TStatId(), nullptr, ENamedThreads::GameThread
			);
		}
	}

	for (const FPhysicsAssetSphere& Sphere : Spheres)
	{
		OutBoneTransforms.Add(FBoneTransform(Sphere.BoneIndex, FTransform(Sphere.WorkLocation)));
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

