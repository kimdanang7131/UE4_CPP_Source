#include "BehaviorTree/CBTService_Melee.h"

#include "Global.h"
#include "Characters/CPlayer.h"
#include "Characters/CEnemy.h"
#include "CAIController.h"

#include "Components/CBehaviorComponent.h"
#include "Components/CStateComponent.h"


UCBTService_Melee::UCBTService_Melee()
{
	NodeName = "Melee";
}

void UCBTService_Melee::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ACAIController* Controller = Cast<ACAIController>(OwnerComp.GetOwner());
	UCBehaviorComponent* BehaviorComp = Cast<UCBehaviorComponent>(Controller->GetComponentByClass(UCBehaviorComponent::StaticClass()));

	ACEnemy* Enemy = Cast<ACEnemy>(Controller->GetPawn());
	UCStateComponent* State = Cast<UCStateComponent>(Enemy->GetComponentByClass(UCStateComponent::StaticClass()));

	// #1. ��Ʈ ���ϸ� AIController->BehaviorComp�� Enum ���ߴٰ� �˷��ֱ�
	if (State->IsHittedMode())
	{
		BehaviorComp->SetHittedMode();

		return;
	}

	// #2. AIController->BehaviorComp���� �÷��̾� �����ͼ� ���ٸ� Idle , �ִٸ� �׾����� üũ
	ACPlayer* Target = BehaviorComp->GetTargetPlayer();
	if (Target == NULL)
	{
		// ��Ʈ�Ѹ��

		BehaviorComp->SetIdleMode();
		return;
	}
	else
	{
		UCStateComponent* TargetState = Cast<UCStateComponent>(Target->GetComponentByClass(UCStateComponent::StaticClass()));

		if (TargetState->IsDeadMode())
		{
			BehaviorComp->SetIdleMode();

			return;
		}
	}


	// #3. �÷��̾ �ְ� ������ ���� ���¶�� �����ȿ� �ִ��� üũ�ϰ� Attack
	float distance = Enemy->GetDistanceTo(Target);
	if (distance < Controller->GetMeleeActionRange())
	{
		State->SetActionMode();
		BehaviorComp->SetActionMode();
	}

	// #4. ���� ������ �ƴϰ� �þ� �ȿ� �ִ� ���¶�� ����
	if (distance < Controller->GetSightRadius())
	{
		BehaviorComp->SetApproachMode();
	}

}