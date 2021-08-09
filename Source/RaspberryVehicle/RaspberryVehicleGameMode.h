#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"
#include "GameFramework/GameModeBase.h"
#include "RaspberryVehicleGameMode.generated.h"

class FSocket;
class UTexture2D;
class ISocketSubsystem;

UCLASS(Blueprintable)
class ARaspberryVehicleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ARaspberryVehicleGameMode(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(BlueprintReadWrite)
	FString IP;

	UPROPERTY(BlueprintReadOnly)
	FDateTime UDPLastTime;

	UPROPERTY(BlueprintReadOnly)
	bool bCLP;

	UPROPERTY(BlueprintReadOnly)
	bool bNear;

	UPROPERTY(BlueprintReadOnly)
	bool bIS1;

	UPROPERTY(BlueprintReadOnly)
	bool bIS2;

	UPROPERTY(BlueprintReadOnly)
	bool bIS3;

	UPROPERTY(BlueprintReadOnly)
	bool bIS4;

	UPROPERTY(BlueprintReadOnly)
	bool bIS5;

	UPROPERTY(BlueprintReadOnly)
	float SpeedL = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float SpeedR = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float Distance = 0.0f;
	
	UPROPERTY(BlueprintReadWrite)
	int32 MotorMode = -1;

	UPROPERTY(BlueprintReadWrite)
	int32 MotorSpeed = 100;

	UPROPERTY(BlueprintReadWrite)
	UTexture2D* CameraTexture;

	ISocketSubsystem* SocketSubsystem;

	FSocket* UDPSocket;

	TArray<uint8> UDPBuffer;

	TArray<uint8> CameraBuffer;

	FDateTime UDPSendTime;

	//~ Begin AActor Interface.
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End AActor Interface.

};
