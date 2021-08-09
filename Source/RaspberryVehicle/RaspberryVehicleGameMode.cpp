#include "RaspberryVehicleGameMode.h"

#include "Sockets.h"
#include "Networking.h"
#include "Misc/Timespan.h"
#include "SocketSubsystem.h"
#include "Engine/Texture2DDynamic.h"

ARaspberryVehicleGameMode::ARaspberryVehicleGameMode(const FObjectInitializer& ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ARaspberryVehicleGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	UDPSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("Raspberry Socket"), FNetworkProtocolTypes::IPv4);

	UDPBuffer.SetNumUninitialized(1024);

	CameraBuffer.Init(255, 256 * 256 * 4);

	UDPLastTime = FDateTime(0);
	UDPSendTime = FDateTime(0);

	CameraTexture = UTexture2D::CreateTransient(256, 256);
	void* Pixels = CameraTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Pixels, CameraBuffer.GetData(), 256 * 256 * 4);
	CameraTexture->PlatformData->Mips[0].BulkData.Unlock();
	CameraTexture->UpdateResource();
}

void ARaspberryVehicleGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	int32 BytesNum = 0;
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();

	bool bIsValid;
	Addr->SetIp(TEXT("192.168.1.1"), bIsValid);
	Addr->SetIp(*IP, bIsValid);
	Addr->SetPort(6000);
	
	if ((FDateTime::Now() - UDPSendTime) > FTimespan::FromSeconds(0.1))
	{
		uint8* MotorModeBuffer = (uint8*)&MotorMode;
		
		UDPBuffer[0] = MotorModeBuffer[3];
		UDPBuffer[1] = MotorModeBuffer[2];
		UDPBuffer[2] = MotorModeBuffer[1];
		UDPBuffer[3] = MotorModeBuffer[0];

		uint8* MotorSpeedBuffer = (uint8*)&MotorSpeed;

		UDPBuffer[4] = MotorSpeedBuffer[3];
		UDPBuffer[5] = MotorSpeedBuffer[2];
		UDPBuffer[6] = MotorSpeedBuffer[1];
		UDPBuffer[7] = MotorSpeedBuffer[0];

		UDPSocket->SendTo(UDPBuffer.GetData(), 8, BytesNum, *Addr);

		UDPSendTime = FDateTime::Now();
	}

	uint32 PendingDataSize;
	while(UDPSocket->HasPendingData(PendingDataSize))
	{
		UDPSocket->RecvFrom(UDPBuffer.GetData(), UDPBuffer.Num(), BytesNum, *Addr);

		if (BytesNum == 13)
		{
			UDPLastTime = FDateTime::Now();

			bCLP = (bool)UDPBuffer[0];
			bNear = (bool)UDPBuffer[1];

			bIS1 = (bool)UDPBuffer[2];
			bIS2 = (bool)UDPBuffer[3];
			bIS3 = (bool)UDPBuffer[4];
			bIS4 = (bool)UDPBuffer[5];
			bIS5 = (bool)UDPBuffer[6];

			SpeedL = UDPBuffer[7] * 0.5;
			SpeedR = UDPBuffer[8] * 0.5;

			uint8 DistanceBuffer[4];

			DistanceBuffer[3] = UDPBuffer[9];
			DistanceBuffer[2] = UDPBuffer[10];
			DistanceBuffer[1] = UDPBuffer[11];
			DistanceBuffer[0] = UDPBuffer[12];

			FMemory::Memcpy(&Distance, DistanceBuffer, sizeof Distance);
		}
		else if (BytesNum == 770)
		{
			int32 ChunkIndex;
			uint8* ChunkIndexBuffer = (uint8*)&ChunkIndex;

			ChunkIndexBuffer[0] = UDPBuffer[1];
			ChunkIndexBuffer[1] = UDPBuffer[0];
			ChunkIndexBuffer[2] = 0;
			ChunkIndexBuffer[3] = 0;

			uint8* Src = &UDPBuffer[2];

			for (int32 PixelIndex = 0; PixelIndex < 256; ++PixelIndex)
			{
				CameraBuffer[(ChunkIndex * 256 + PixelIndex) * 4 + 0] = Src[PixelIndex * 3 + 0];
				CameraBuffer[(ChunkIndex * 256 + PixelIndex) * 4 + 1] = Src[PixelIndex * 3 + 1];
				CameraBuffer[(ChunkIndex * 256 + PixelIndex) * 4 + 2] = Src[PixelIndex * 3 + 2];
				CameraBuffer[(ChunkIndex * 256 + PixelIndex) * 4 + 3] = 255;
			}
		}
	}
	
	void* Buffer = CameraBuffer.GetData();
	FRHITexture2D* RHICameraTexture = CameraTexture->Resource->GetTexture2DRHI();
	ENQUEUE_RENDER_COMMAND(RaspberryCameraUpdate)(
		[RHICameraTexture, Buffer](FRHICommandListImmediate& RHICmdList)
		{
			uint32 Stride = 0;
			void* Pixels = RHILockTexture2D(RHICameraTexture, 0, RLM_WriteOnly, Stride, false);
			FMemory::Memcpy(Pixels, Buffer, 256 * 256 * 4);
			RHIUnlockTexture2D(RHICameraTexture, 0, false);
		}
	);
}

void ARaspberryVehicleGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SocketSubsystem->DestroySocket(UDPSocket);
	
	Super::EndPlay(EndPlayReason);
}
