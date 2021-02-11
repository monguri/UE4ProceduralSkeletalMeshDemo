//======================================================================================
//  Copyright (c) 2019 SPARK CREATIVE Inc.
//  All rights reserved.
//======================================================================================
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SPCRJointDynamicsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SPCRJOINTDYNAMICS_API USPCRJointDynamicsComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USPCRJointDynamicsComponent();

	//���Z�b�g���s���t���O���I���ɂ���
	//UFUNCTION(BlueprintCallable)
	void OnStartReset();

	//AnimBP��JointDynamics�m�[�h�Ń��Z�b�g���s�����Ƃ��ɌĂ΂��
	void OnFinishReset();

	//���Z�b�g�t���O
	UPROPERTY(BlueprintReadOnly)
	bool isReset;

protected:
	//���Z�b�g�����I���m�F�̃t���O
	bool isFinishReset;

	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void OnInit();
	void OnUpdateReset();

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
