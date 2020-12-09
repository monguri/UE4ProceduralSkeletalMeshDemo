#include "AnimNode_SimulateSlime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_SimulateSlime::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()を参考にしている
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();

	// 配列をリセットし、また初回Evaluateで作り直す
	Spheres.Empty();
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

		// 一本の骨ごとに一個のスフィアしか想定しない。また、スフィア以外は考えない。物理アセットのデータが動的に書き換わることも想定しない。
		const FKSphereElem& SphereElem = UsePhysicsAsset->SkeletalBodySetups[i]->AggGeom.SphereElems[0]; 

		FPhysicsAssetSphere Sphere;
		Sphere.Radius = SphereElem.Radius;

		// 動的にメッシュが変わることは想定しない
		int32 BoneIndex = SkeletalMeshComp->GetBoneIndex(UsePhysicsAsset->SkeletalBodySetups[i]->BoneName);
		Sphere.BoneIndex = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(BoneIndex);

		FTransform BoneTM = Input.Pose.GetComponentSpaceTransform(Sphere.BoneIndex);
		float Scale = BoneTM.GetScale3D().GetAbsMax();
		FVector VectorScale(Scale);
		BoneTM.RemoveScaling();

		FTransform SphereLocalTM = SphereElem.GetTransform();
		SphereLocalTM.ScaleTranslation(VectorScale);

		const FTransform& SphereCompTM = SphereLocalTM * BoneTM;

		// 入力ポーズの反映はこの初期化時の一回しか行わない
		Sphere.WorkLocation = SphereCompTM.GetLocation();
		Sphere.WorkPrevLocation = Sphere.WorkLocation;

		Spheres.Add(Sphere);
	}
}

void FAnimNode_SimulateSlime::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	const USkeletalMeshComponent* SkeletalMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	const FTransform& CSToWS = SkeletalMeshComp->GetComponentToWorld();

	// Spheresが空なのを初期化条件として初期化
	if (Spheres.Num() == 0)
	{
		InitSpheres(Output);
		
		// 物理アセットに有効な情報がなくて配列が作られなければ処理しない
		if (Spheres.Num() == 0)
		{
			return;
		}

		PreCSToWS = CSToWS;
	}

	// IsValidToEvaluate()にはこの条件は入れられない。IsValidToEvaluate()はUpdateInternal()呼び出しの条件でもあるので。
	if (DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	UWorld* World = GEngine->GetWorldFromContextObject(SkeletalMeshComp, EGetWorldErrorMode::LogAndReturnNull);

	// UPhysicsAssetEditorSkeletalMeshComponent::GetPrimitiveColor()のElemSelectedColorの色をデバッガで値を調べた
	const FColor ElemSelectedColor = FColor(222, 163, 9);

	check(DeltaTime > KINDA_SMALL_NUMBER);

	// Rootとの距離が半径の距離を維持する位置補正をする。
	// コンポーネント座標に対して固定する力がないとRootからどれだけでも自由に離れてしまうので。
	// 方向は制約しない。コンストレイントというよりは入力モーションによる基準位置決めなので、ベルレ積分より先にする。
	FCompactPoseBoneIndex RootBoneIndex = RootBone.GetCompactPoseIndex(BoneContainer);
	const FVector& RootPosePos = Output.Pose.GetComponentSpaceTransform(RootBoneIndex).GetLocation();
	for (FPhysicsAssetSphere& Sphere : Spheres)
	{
		const FVector& Diff = Sphere.WorkLocation - RootPosePos;
		Sphere.WorkLocation += -(Diff.Size() - Sphere.Radius) * Diff.GetSafeNormal();
	}

	// ワールド座標での移動の、コンポーネント座標への反映項の計算
	const FVector& ComponentMove = CSToWS.InverseTransformPosition(PreCSToWS.GetLocation());
	PreCSToWS = CSToWS;

	// ベルレ積分
	for (FPhysicsAssetSphere& Sphere : Spheres)
	{
		FVector Velocity = (Sphere.WorkLocation - Sphere.WorkPrevLocation) / DeltaTime;
		Velocity *= (1.0f - Damping);
		Sphere.WorkPrevLocation = Sphere.WorkLocation;
		Sphere.WorkLocation += Velocity * DeltaTime;
		Sphere.WorkLocation += ComponentMove * (1.0f - WorldDampingLocation);
	}

	// スフィア同士の相互作用。球が接触する距離同士になるように位置補正する
	// TODO:現在は入力ポーズが影響するのがシミュレーション開始時の位置の初期化のみなので、あとで、常に影響するようにして
	// 個別に骨をマウスで動かしてみて挙動を確認したいな
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

	for (const FPhysicsAssetSphere& Sphere : Spheres)
	{
		OutBoneTransforms.Add(FBoneTransform(Sphere.BoneIndex, FTransform(Sphere.WorkLocation)));
	}

	OutBoneTransforms.Sort(FCompareBoneTransformIndex());

	// デバッグ描画
	if (bDebugDrawPhysicsAsset)
	{
		for (const FPhysicsAssetSphere& Sphere : Spheres)
		{
			float Radius = Sphere.Radius;
			FVector LocationWS = CSToWS.TransformPosition(Sphere.WorkLocation);

			// UPhysicsAssetEditorSkeletalMeshComponent::RenderAssetTools()を参考にしている
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

