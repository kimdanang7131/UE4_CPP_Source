#include "Characters/CPlayer.h"
#include "Global.h"
 //////////////////////
#include "Components/UIComponents/CSkillComponent.h"
#include "Components/UIComponents/CInventoryComponent.h"
#include "Components/CWeaponStateComponent.h"
#include "Components/CTargetComponent.h"
#include "Components/CStatusComponent.h"
#include "Components/CStateComponent.h"
///////////////////////
#include "Managers/UIManager.h"
#include "CPlayerController.h"
#include "Characters/CCombatantCharacter.h"
#include "Widgets/CUserWidget_MainUI.h"
#include "Widgets/CUserWidget_InvenWindow.h"
#include "Widgets/CUserWidget_Stamina.h"
#include "Widgets/CUserWidget_Health.h"
//////////////////////
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Actors/CItem.h"

ACPlayer::ACPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	///
	CHelpers::CreateComponent<USpringArmComponent>(this, &SpringArm, "SpringArm", GetRootComponent());
	CHelpers::CreateComponent<UCameraComponent>(this, &Camera, "Camera", SpringArm);
	CHelpers::CreateActorComponent<UCTargetComponent>(this, &Target, "Target");

	// 기본 SpringArm & Camera 셋팅
	{
		SpringArm->SetRelativeLocation(FVector(0, 0, 40));
		SpringArm->SetRelativeRotation(FRotator(0, 90, 0));
		SpringArm->TargetArmLength = 500.f;
		SpringArm->SocketOffset = FVector(0, 0, 60.f);
		SpringArm->bDoCollisionTest = false;
		SpringArm->bUsePawnControlRotation = true;
		SpringArm->bEnableCameraLag = true;
	}
}

void ACPlayer::BeginPlay()
{
	Super::BeginPlay();


	// #1. 모든 Trader갖고와서 영역 안에 있으면 교환하기 위해서 델리게이트를 등록
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACTrader::StaticClass(), TradersArr);

	for (AActor* trader : TradersArr)
	{
		ACTrader* castedTrader = Cast<ACTrader>(trader);
		if (!!castedTrader)
		{
			castedTrader->TradeBeginDelegate.AddDynamic(this, &ACPlayer::OpenTradeWindow);
			castedTrader->TradeEndDelegate.AddDynamic(this, &ACPlayer::CloseTradeWindow);
		}
	}

	// #2. InvenWindow의 bIsPlayerInven을 적용하기 위해
	MyController = Cast<ACPlayerController>(GetController());
	MyController->GetPlayerMainUI()->SetPlayerInvenWindowBool();

	// #3. Update에서 사용하기 위해 Widget 저장하고 Status-> HealthWidget을 Timer로 컨트롤하기 위해 등록해줌
	HealthWidget  = MyController->GetPlayerMainUI()->GetHealthBar();
	Status->SetHealthWidget(HealthWidget);

	StaminaWidget = MyController->GetPlayerMainUI()->GetStaminaBar();
	MaxStamina = Status->GetMaxStamina();

	// Test용 나중에 지울 것
	PotionItem = CHelpers::MySpawnActor<ACItem>(PotionItemClass, this, GetActorTransform());
}

void ACPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// #1. Spinting중일때 지속적으로 Stamina Update
	if (bIsSprinting)
	{
		Status->SubStamina(STAMINA_USAGE * DeltaTime);
		StaminaWidget->Update(Status->GetStamina(), MaxStamina);

		if (Status->GetStamina() <= 0.1f)
			OffSprint();
	}
	else
	{
		Status->AddStamina(STAMINA_RECOVERAGE * DeltaTime);
		StaminaWidget->Update(Status->GetStamina(), MaxStamina);
	}
}

void ACPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveFB", this, &ACPlayer::OnMoveFB);
	PlayerInputComponent->BindAxis("MoveLR", this, &ACPlayer::OnMoveLR);
	PlayerInputComponent->BindAxis("HorizontalLook", this, &ACPlayer::OnHorizontalLook);
	PlayerInputComponent->BindAxis("VerticalLook", this, &ACPlayer::OnVerticalLook);

	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Pressed , this, &ACPlayer::OnSprint);
	PlayerInputComponent->BindAction("Sprint", EInputEvent::IE_Released, this, &ACPlayer::OffSprint);

	PlayerInputComponent->BindAction("Dodge", EInputEvent::IE_Pressed, this, &ACPlayer::Dodging);
	PlayerInputComponent->BindAction("ActionL", EInputEvent::IE_Pressed, this, &ACPlayer::DoActionL);
	//PlayerInputComponent->BindAction("ActionR", EInputEvent::IE_Pressed, this, &ACPlayer::OffSprint);
	//PlayerInputComponent->BindAction("ActionM", EInputEvent::IE_Pressed, this, &ACPlayer::OffSprint);

	PlayerInputComponent->BindAction("SkillZ", EInputEvent::IE_Pressed, this, &ACPlayer::SkillZ);
	PlayerInputComponent->BindAction("SkillZ", EInputEvent::IE_Released, this, &ACPlayer::End_Hold);

	PlayerInputComponent->BindAction("SkillX", EInputEvent::IE_Pressed, this, &ACPlayer::SkillX);
	PlayerInputComponent->BindAction("SkillX", EInputEvent::IE_Released, this, &ACPlayer::End_Hold);

	PlayerInputComponent->BindAction("SkillC", EInputEvent::IE_Pressed, this, &ACPlayer::SkillC);
	PlayerInputComponent->BindAction("SkillC", EInputEvent::IE_Released, this, &ACPlayer::End_Hold);

	PlayerInputComponent->BindAction("SkillV", EInputEvent::IE_Pressed, this, &ACPlayer::SkillV);
	PlayerInputComponent->BindAction("SkillV", EInputEvent::IE_Released, this, &ACPlayer::End_Hold);


	PlayerInputComponent->BindAction("WeaponA", EInputEvent::IE_Pressed, this, &ACPlayer::ToggleWeaponA);
	PlayerInputComponent->BindAction("WeaponB", EInputEvent::IE_Pressed, this, &ACPlayer::ToggleWeaponB);

	PlayerInputComponent->BindAction("ToggleSkillWindow", EInputEvent::IE_Pressed, this, &ACPlayer::ToggleSkillWindow);
	PlayerInputComponent->BindAction("ToggleInventoryWindow", EInputEvent::IE_Pressed, this, &ACPlayer::ToggleInventoryWindow);

	PlayerInputComponent->BindAction("Close", EInputEvent::IE_Pressed, this, &ACPlayer::ClearUI);

	PlayerInputComponent->BindAction("Test1", EInputEvent::IE_Pressed, this, &ACPlayer::Test1);
	PlayerInputComponent->BindAction("Test2", EInputEvent::IE_Pressed, this, &ACPlayer::Test2);

	PlayerInputComponent->BindAction("UseItemA", EInputEvent::IE_Pressed, this, &ACPlayer::UseItemA);
	PlayerInputComponent->BindAction("UseItemB", EInputEvent::IE_Pressed, this, &ACPlayer::UseItemB);
	PlayerInputComponent->BindAction("UseItemC", EInputEvent::IE_Pressed, this, &ACPlayer::UseItemC);
	PlayerInputComponent->BindAction("UseItemD", EInputEvent::IE_Pressed, this, &ACPlayer::UseItemD);
}

/** 인터페이스를 통한 팀 설정 */
FGenericTeamId ACPlayer::GetGenericTeamId() const
{
	return FGenericTeamId(TeamId);
}

/** Player가 마을에 들어왔을때 BoxTrigger를 통하여 Trader의 이름을 OnOff하여 보여주기 */
void ACPlayer::ToggleVillage()
{
	bNPCNameVisible = !bNPCNameVisible;

	for (AActor* trader : TradersArr)
	{
		ACTrader* castedTrader = Cast<ACTrader>(trader);
		if (!!castedTrader)
		{
			castedTrader->SetNameVisibility(bNPCNameVisible);
		}
	}
}

/** 앞뒤 움직임 */
void ACPlayer::OnMoveFB(float InAxis)
{
	FALSE_RETURN(Status->CheckCanMove())

	FRotator rotator = FRotator(0, GetControlRotation().Yaw, 0);
	FVector direction = FQuat(rotator).GetForwardVector();

	AddMovementInput(direction, InAxis);
}

/** 좌우 움직임 */
void ACPlayer::OnMoveLR(float InAxis)
{
	FALSE_RETURN(Status->CheckCanMove())

	FRotator rotator = FRotator(0, GetControlRotation().Yaw, 0);
	FVector direction = FQuat(rotator).GetRightVector();

	AddMovementInput(direction, InAxis);
}

/** 마우스 좌우 움직임 */
void ACPlayer::OnHorizontalLook(float InAxis)
{
	FALSE_RETURN(Status->CheckCanControl())
	float rate = Status->GetHorizontalLookRate();

	AddControllerYawInput(rate * InAxis * GetWorld()->GetDeltaSeconds());
}

/** 마우스 위아래 움직임 */
void ACPlayer::OnVerticalLook(float InAxis)
{
	FALSE_RETURN(Status->CheckCanControl())
	float rate = Status->GetVerticalLookRate();

	AddControllerPitchInput(rate * InAxis * GetWorld()->GetDeltaSeconds());
}

/** Shift 달리기 On Off */
void ACPlayer::OnSprint()
{
	FALSE_RETURN(Status->CheckCanControl());

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = 800.f;
}
void ACPlayer::OffSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
}


/** 좌클릭 공격 */
void ACPlayer::DoActionL()
{
	WeaponState->DoAction();
}

/** Q 무기 장착,해제 */
void ACPlayer::ToggleWeaponA()
{
	WeaponState->ToggleWeaponA();
}
/** E 무기 장착,해제 */
void ACPlayer::ToggleWeaponB()
{
	WeaponState->ToggleWeaponB();
}


/** ZXCV 메인 스킬차에 스킬 있을 때 사용 */
void ACPlayer::SkillZ()
{
	Skill->SkillZ();
}
void ACPlayer::SkillX()
{
	Skill->SkillX();
}
void ACPlayer::SkillC()
{
	Skill->SkillC();
}
void ACPlayer::SkillV()
{
	Skill->SkillV();
}
/** ZXCV Release 했을 때 홀드 스킬 사용 */
void ACPlayer::End_Hold()
{
	Skill->End_Hold();
}


/** K 눌렀을때 스킬창 UI 오픈 */
void ACPlayer::ToggleSkillWindow()
{
	Skill->ToggleSkillWindow();
}
/** I 눌렀을때 인벤토리 UI 오픈*/
void ACPlayer::ToggleInventoryWindow()
{
	Inventory->ToggleInventoryWindow();
}
/** 현재 5번 눌렀을때 UIManager를 통하여 모든 UI 끄기 */
void ACPlayer::ClearUI()
{
	UIManager::SetGameModeOnly();
}


/** Trader영역에 들어오면 Delegate를 통해 TraderInventory 오픈 */
void ACPlayer::OpenTradeWindow(const TArray<FItemDataTableBase>& InTraderFItems, const int32& InMoney)
{
	Inventory->OpenTraderWindow(InTraderFItems , InMoney);
}

/** Trader영역을 나가면 Delegate를 통해 Trader의 Inventory 돈,아이템 업데이트 */
void ACPlayer::CloseTradeWindow(ACTrader* InTrader)
{
	// 서순 제발..
	int32 tempMoney                      = Inventory->GetTraderMoney();
	TArray<FItemDataTableBase> tempInven = Inventory->GetTraderInvenDatas();
	InTrader->UpdateTraderInvenDatas(tempInven, tempMoney);

	// 위에꺼 다 옮기고 지워야함 2시간 날린듯
	Inventory->CloseTraderWindow();
}

void ACPlayer::Test1()
{
	Inventory->AddFItemToUI();
	//Inventory->TestPrint();
}

void ACPlayer::Test2()
{
	//Inventory->AddFItemToUI2();
	Status->SubHealth(30.f);

	//PotionItem->AcivateItem(FItem);

	//ACPlayerController* controller = Cast<ACPlayerController>(GetController());

	//if (!!controller)
	//{
	//	controller->GetPlayerMainUI()->GetInvenWindow()->TestAdd(0);
	//}
}

void ACPlayer::UseItemA()
{
	if (Inventory)
		Inventory->UseInvenSlotItem(0);
}

void ACPlayer::UseItemB()
{
	if (Inventory)
		Inventory->UseInvenSlotItem(1);
}

void ACPlayer::UseItemC()
{
	if (Inventory)
		Inventory->UseInvenSlotItem(2);
}

void ACPlayer::UseItemD()
{
	if (Inventory)
		Inventory->UseInvenSlotItem(3);
}
