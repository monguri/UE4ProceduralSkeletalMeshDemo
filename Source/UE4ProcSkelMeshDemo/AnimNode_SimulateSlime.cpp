#include "AnimNode_SimulateSlime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "ReferenceSkeleton.h"
#include "DrawDebugHelpers.h"

void FAnimNode_SimulateSlime::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	// FAnimNode_RigidBody::OnInitializeAnimInstance()を参考にしている
	USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();

	// 配列をリセットし、また初回Evaluateで作り直す
	Spheres.Empty();

	OwnerActor = nullptr;

	// オーナーのアクタをルートに登って行って探す
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

namespace
{
	// KismetTraceUtils.cppのDebugDrawSweptSphere()を参考にしている
	void DebugDrawSweptSphere(const UWorld* InWorld, FVector const& Start, FVector const& End, float Radius, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
	{
		FVector const TraceVec = End - Start;
		float const Dist = TraceVec.Size();

		FVector const Center = Start + TraceVec * 0.5f;
		float const HalfHeight = (Dist * 0.5f) + Radius;

		FQuat const CapsuleRot = FRotationMatrix::MakeFromZ(TraceVec).ToQuat();
		::DrawDebugCapsule(InWorld, Center, HalfHeight, Radius, CapsuleRot, Color, bPersistentLines, LifeTime, DepthPriority);
	}

	// KismetTraceUtils.cppのDrawDebugSphereTraceSingle()を参考にしている
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
	// ばねなし。
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

	// コンポーネントスペースでの重力*質量のベクトル
	const FVector& GravityCS = CSToWS.InverseTransformVector(Gravity);

	// ベルレ積分
	for (FPhysicsAssetSphere& Sphere : Spheres)
	{
		FVector Velocity = (Sphere.WorkLocation - Sphere.WorkPrevLocation) / DeltaTime;
		Velocity *= (1.0f - Damping);
		Sphere.WorkPrevLocation = Sphere.WorkLocation;
		Sphere.WorkLocation += Velocity * DeltaTime;
		Sphere.WorkLocation += ComponentMove * (1.0f - WorldDampingLocation);
		Sphere.WorkLocation += 0.5 * GravityCS * DeltaTime * DeltaTime;
	}

	// スフィア同士の相互作用。球が接触する距離同士になるように位置補正する。
	// ばねあり。
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

	// 他のオブジェクトとのコリジョン
	// Radius * PenetrateDepthRadiusRatioの距離になるように位置補正する。
	// ばねあり。
	if (bEnableCollision)
	{
		for (FPhysicsAssetSphere& Sphere : Spheres)
		{
			const FVector& PosWS = CSToWS.TransformPosition(Sphere.WorkLocation);
			bool bTraceComplex = false;

			FHitResult HitResult;

			// KismetTraceUtils.cppのConfigureCollisionParams()を参考にしている
			static const FName SphereTraceSingleName(TEXT("SphereTraceSingle"));
			FCollisionQueryParams Params(SphereTraceSingleName, TStatId(), bTraceComplex);
			Params.bReturnPhysicalMaterial = true;
			Params.bReturnFaceIndex = false;
			Params.AddIgnoredComponent(SkeletalMeshComp);
			if (OwnerActor != nullptr)
			{
				Params.AddIgnoredActor(OwnerActor);
			}

			// クエリはスフィアのあるその場所で、直近のHitひとつのみ確認する
			bool bHit = World->SweepSingleByChannel(HitResult, PosWS, PosWS, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeSphere(Sphere.Radius), Params);
			if (bHit)
			{
				const FVector& ImpactPosCS = CSToWS.InverseTransformPosition(HitResult.ImpactPoint);
				const FVector& ImpactNormalCS = CSToWS.InverseTransformVector(HitResult.ImpactNormal).GetSafeNormal();

				// UKismetSystemLibrary::SphereTraceSingle()であってもスフィアとポリゴンとのコリジョンでは、
				// スフィアの中心との最近接点がImpactPointとされるようだ。つまりスフィア中心点からポリゴンに
				// ひいた垂線の交点
				float OverPenetrateDistance = Sphere.Radius * PenetrateDepthRadiusRatio - ((Sphere.WorkLocation - ImpactPosCS) | ImpactNormalCS); // DotProductにすることで、中心さえもめりこんだケースでも対応する
				if (OverPenetrateDistance > 0.0f)
				{
					// TODO: Stiffnessで押し出しをすると、押し出しが小さすぎて他の挙動による位置移動の方が移動量が大きくて負けてしまう
					// TODO: ただ、大きいStiffnessで押し出ししても、剛体がはずむような動きで、あまり粘性体っぽいはずみになってないので単純な押し出しで十分だと思う
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

