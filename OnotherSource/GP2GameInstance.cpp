// Fill out your copyright notice in the Description page of Project Settings.

#include "GP2GameInstance.h"
#include "GP2Project.h"
#include "UI/UIManager.h"
#include <Kismet/GameplayStatics.h>
#include <GameFramework/GameMode.h>
#include "Runtime/NetworkReplayStreaming/NullNetworkReplayStreaming/Public/NullNetworkReplayStreaming.h"
#include "NetworkVersion.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Core/Public/HAL/FileManager.h"
#include "Runtime/Core/Public/Serialization/MemoryReader.h"
#include "Runtime/Slate/Public/Framework/Application/SlateApplication.h"
#include "Engine.h"

#if WITH_GAMELIFT
int8 UGP2GameInstance::byInitGameLiftCount = 0;
#endif
UGP2GameInstance::UGP2GameInstance()
{
	GP2LoadGlobals();
	GP2AppInit();

	InitGameData();

	m_iRoomNumber = 0;
	m_Subject.Empty();
	m_WebServerIP = "127.0.0.1:3001";

	GetServerAddress( 0 );

#if WITH_GAMELIFT
	m_bActiveGameLiftSession = false;
	m_bSettingMap = false;
	m_Map = NULL;
	m_eGameMode = EGAME_MODE::PVP;

	FParse::Bool(FCommandLine::Get(), TEXT("local="), m_bLocal);
	FParse::Bool(FCommandLine::Get(), TEXT("lobby="), m_bLobby);

	//if (m_bLocal)
	//{
	//	bool bPVP_MODE = true;

	//	FParse::Bool(FCommandLine::Get(), _T("pvp_mode="), bPVP_MODE);

	//	if (bPVP_MODE)
	//	{
	//		m_eGameMode = EGAME_MODE::PVP;
	//	}
	//	else
	//	{
	//		m_eGameMode = EGAME_MODE::PVE;
	//	}

	//	return;
	//}

	m_bCheckEmptyGameSession = false;

	if (byInitGameLiftCount < 2)
	{
		byInitGameLiftCount++;
		return;
	}

	if (!m_bLobby)
	{
		gameLiftSdkModule = MakeShareable(new FGameLiftServerSDKModule);

		//InitSDK establishes a local connection with GameLift's agent to enable further communication.
		gameLiftSdkModule->InitSDK();
	}

	//When a game session is created, GameLift sends an activation request to the game server and passes along the game session object containing game properties and other settings.
	//Here is where a game server should take action based on the game session object.
	//Once the game server is ready to receive incoming player connections, it should invoke GameLiftServerAPI.ActivateGameSession()

	if (!m_bLobby)
	{
		if (!this->GetTimerManager().IsTimerActive(m_timer))
			this->GetTimerManager().SetTimer(m_timer, this, &UGP2GameInstance::EmptyGameSessionCheckTick, 1, true);

		auto onGameSession = [=](Aws::GameLift::Server::Model::GameSession gameSession)
		{
			//UE_LOG(LogTemp, Log, TEXT("GetGameSessionData %s"), ANSI_TO_TCHAR(gameSession.GetGameSessionData()));

			int count = 0;

			auto getProperty = gameSession.GetGameProperties(count);

			UWorld* world = this->GetWorld();

			for (int i = 0; i < count; i++)
			{
				if (strcmp(getProperty[i].GetKey(), "mode") == 0)
				{
					if (strcmp(getProperty[i].GetValue(), "PVP") == 0)
					{
						UE_LOG(LogTemp, Log, TEXT("PVP MODE"));

						m_eGameMode = EGAME_MODE::PVP;
					}
					else if (strcmp(getProperty[i].GetValue(), "TUTORIAL") == 0)
					{
						UE_LOG(LogTemp, Log, TEXT("TUTORIAL MODE"));

						m_eGameMode = EGAME_MODE::TUTORIAL;
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("PVE MODE"));

						m_eGameMode = EGAME_MODE::PVE;
					}
				}

				if (strcmp(getProperty[i].GetKey(), "serverip") == 0)
				{
					m_WebServerIP = getProperty[i].GetValue();
				}

				if (strcmp(getProperty[i].GetKey(), "Map") == 0)
				{
					m_Map = getProperty[i].GetValue();
				}

				//UE_LOG(LogTemp, Log, TEXT("Property key : %s , value : %s"), ANSI_TO_TCHAR(getProperty[i].GetKey()), getProperty[i].GetValue());
			};

			InitGameData();

			m_iRoomNumber = atoi(gameSession.GetGameSessionData());
			m_GameSessionID = gameSession.GetGameSessionId();
			m_Subject = gameSession.GetName();
			m_bCheckEmptyGameSession = true;
			m_bActiveGameLiftSession = true;
			m_bSettingMap = false;
			//gameLiftSdkModule->ActivateGameSession();
		};

		FProcessParameters* params = new FProcessParameters();
		params->OnStartGameSession.BindLambda(onGameSession);

		////OnProcessTerminate callback. GameLift invokes this callback before shutting down an instance hosting this game server.
		////It gives this game server a chance to save its state, communicate with services, etc., before being shut down.
		////In this case, we simply tell GameLift we are indeed going to shut down.
		params->OnTerminate.BindLambda([=]() {
			gameLiftSdkModule->ProcessEnding();
		});

		////This is the HealthCheck callback.
		////GameLift invokes this callback every 60 seconds or so.
		////Here, a game server might want to check the health of dependencies and such.
		////Simply return true if healthy, false otherwise.
		////The game server has 60 seconds to respond with its health status. GameLift defaults to 'false' if the game server doesn't respond in time.
		////In this case, we're always healthy!
		params->OnHealthCheck.BindLambda([]() {
			return true;
		});

		////This game server tells GameLift that it listens on port 7777 for incoming player connections.

		int port = 0;

		if (!FParse::Value(FCommandLine::Get(), TEXT("port="), port))
		{
			port = 7777;
		}

		params->port = port;
		m_port = port;

		////Here, the game server tells GameLift what set of files to upload when the game session ends.
		////GameLift uploads everything specified here for the developers to fetch later.
		TArray<FString> logfiles;
		logfiles.Add(TEXT("aLogFile.txt"));
		params->logParameters = logfiles;

		////Calling ProcessReady tells GameLift this game server is ready to receive incoming game sessions!
		FGameLiftGenericOutcome ReadyOutcome = gameLiftSdkModule->ProcessReady(*params);

		if (!ReadyOutcome.IsSuccess())
		{
			UE_LOG(LogTemp, Error, TEXT("ProcessReady Fail : %s"), *ReadyOutcome.GetError().m_errorMessage);
		}
	}
	else
	{
		InitGameData();

		//	m_iRoomNumber = atoi(gameSession.GetGameSessionData());
		//m_GameSessionID = gameSession.GetGameSessionId();
		//m_Subject = gameSession.GetName();

		m_bSettingMap = true;
		m_bActiveGameLiftSession = true;

		int port = 0;

		if (!FParse::Value(FCommandLine::Get(), TEXT("port="), port))
		{
			port = 7777;
		}

		m_port = port;
	}

#endif

}

void UGP2GameInstance::HandleNetworkFailure(UWorld *World, UNetDriver *NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogTemp, Error, TEXT("LevelEnterFail : %s"), *ErrorString);

	LevelEnterFail(ErrorString);
}

void UGP2GameInstance::HandleNetworkError(UWorld * World, UNetDriver * NetDriver, ENetworkFailure::Type FailureType, const FString & ErrorString)
{
	const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ENetworkFailure"), true);
 
	if (EnumPtr == nullptr)
	{
		return;
	}

	FString FailureTypeString = (EnumPtr->GetNameByValue((int64)FailureType)).ToString();

	UE_LOG(LogTemp, Error, TEXT("GP2 NetworkFailure FailureType : %s"), *FailureTypeString);

	UE_LOG(LogTemp, Error, TEXT("GP2 NetworkFailure ErrorString : %s"), *ErrorString);
}

UGP2GameInstance::~UGP2GameInstance()
{
	GP2AppExit();
}

void UGP2GameInstance::Init()
{
	Super::Init();

	UIManager = NewObject< UUIManager >();
	UIManager->AddToRoot();

	EnumerateStreamsPtr = FNetworkReplayStreaming::Get().GetFactory().CreateReplayStreamer();

	OnEnumerateStreamsCompleteDelegate = FOnEnumerateStreamsComplete::CreateUObject(this, &UGP2GameInstance::OnEnumerateStreamsComplete);

	OnDeleteFinishedStreamCompleteDelegate = FOnDeleteFinishedStreamComplete::CreateUObject(this, &UGP2GameInstance::OnDeleteFinishedStreamComplete);

	GoToTimeCompleteDelegate = FOnGotoTimeDelegate::CreateUObject(this, &UGP2GameInstance::OnGotoTimeFinished);

	if (GEngine != nullptr)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UGP2GameInstance::HandleNetworkError);

	}

}

void UGP2GameInstance::Shutdown()
{
	Super::Shutdown();
}

void UGP2GameInstance::InitGameData()
{
#if WITH_GAMELIFT

	//if(!m_bLocal)
		m_GameSessionID.Empty();

	m_byEnterUserCount = 0;
	m_bGameStart = false;
	m_bCheckEmptyGameSession = false;
	m_EmptyGameSessionCheckCount = 0;
#endif
}

APlayerController* UGP2GameInstance::GetPlayerController() const
{
	UWorld* world = GetWorld();
	if ( !world )
		return nullptr;

	return UGameplayStatics::GetPlayerController( world , 0 );
}

ACharacterPC* UGP2GameInstance::GetPlayerCharacterPC()
{
	UWorld* world = GetWorld();
	if ( !world )
		return nullptr;

	ACharacter* character = UGameplayStatics::GetPlayerCharacter( world , 0 );

	if ( ACharacterPC* pc = Cast< ACharacterPC >( character ) )
		return pc;

	return nullptr;
}

ACharacter* UGP2GameInstance::GetPlayerCharacter()
{
	UWorld* world = GetWorld();
	if ( !world )
		return nullptr;

	return UGameplayStatics::GetPlayerCharacter( world , 0 );
}

AGameMode* UGP2GameInstance::GetGP2GameMode()
{
	UWorld* world = GetWorld();
	if ( !world )
		return nullptr;

	return Cast< AGameMode >( UGameplayStatics::GetGameMode( world ) );
}

////////////////////////////////////////////Replay function //////////////////////////////////////////////

void UGP2GameInstance::StartRecordingReplayFromBP(FString ReplayName, FString FriendlyName)
{
	
	StartRecordingReplay(ReplayName, FriendlyName);

}

void UGP2GameInstance::StopRecordingReplayFromBP()
{
	StopRecordingReplay();

	FindReplays();
}

void UGP2GameInstance::PlayReplayFromBP(FString ReplayName)
{
	PlayReplay(ReplayName);
}

void UGP2GameInstance::FindReplays()
{
	if (EnumerateStreamsPtr.Get() != nullptr)
	{
		EnumerateStreamsPtr.Get()->EnumerateStreams(FNetworkReplayVersion(), FString(), FString(), OnEnumerateStreamsCompleteDelegate);
	}
}

void UGP2GameInstance::RenameReplay(const FString & ReplayName, const FString & NewFriendlyReplayName)
{
	// Get File Info
	FNullReplayInfo Info;

	const FString DemoPath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/"));
	const FString StreamDirectory = FPaths::Combine(*DemoPath, *ReplayName);
	const FString StreamFullBaseFilename = FPaths::Combine(*StreamDirectory, *ReplayName);
	const FString InfoFilename = StreamFullBaseFilename + TEXT(".replayinfo");

	TUniquePtr<FArchive> InfoFileArchive(IFileManager::Get().CreateFileReader(*InfoFilename));

	if (InfoFileArchive.IsValid() && InfoFileArchive->TotalSize() != 0)
	{
		FString JsonString;
		*InfoFileArchive << JsonString;

		Info.FromJson(JsonString);
		Info.bIsValid = true;

		InfoFileArchive->Close();
	}

	// Set FriendlyName
	Info.FriendlyName = NewFriendlyReplayName;

	// Write File Info
	TUniquePtr<FArchive> ReplayInfoFileAr(IFileManager::Get().CreateFileWriter(*InfoFilename));

	if (ReplayInfoFileAr.IsValid())
	{
		FString JsonString = Info.ToJson();
		*ReplayInfoFileAr << JsonString;

		ReplayInfoFileAr->Close();
	}

}

void UGP2GameInstance::DeleteReplay(const FString & ReplayName)
{
	if (EnumerateStreamsPtr.Get())
	{
		EnumerateStreamsPtr.Get()->DeleteFinishedStream(ReplayName, OnDeleteFinishedStreamCompleteDelegate);
	}

}

void UGP2GameInstance::SaveGameInfo(const FString & ReplayName , const FString& PlayerCharacterName, const FString& PlayedMap)
{

	const FString DemoPath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/"));
	const FString StreamDirectory = FPaths::Combine(*DemoPath, *ReplayName);

	const FString SaveFileName = TEXT("ReplayInfo.sav");

	bool AllowOverwriting = false;

    TArray<FString> SaveInfo;

	SaveInfo.Add(PlayerCharacterName);
	SaveInfo.Add(PlayedMap);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		// Get absolute file path
		FString AbsoluteFilePath = StreamDirectory + "/" + SaveFileName;

		// Allow overwriting or file doesn't already exist
		if (AllowOverwriting || !PlatformFile.FileExists(*AbsoluteFilePath))
		{
			FFileHelper::SaveStringArrayToFile(SaveInfo, *AbsoluteFilePath);
		}

}

bool UGP2GameInstance::IsReplayPlaying()
{
	if (GetWorld()->DemoNetDriver != nullptr)
	{
		return GetWorld()->DemoNetDriver->IsPlaying();

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DemoNetDriver Is Null"));
	}
	
	return false;
}

bool UGP2GameInstance::IsRecording()
{


	if (GetWorld()->DemoNetDriver != nullptr)
	{
		return GetWorld()->DemoNetDriver->IsRecording();

		
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DemoNetDriver Is Null"));
	}

	return false;
}

bool UGP2GameInstance::IsGamePadconnected()
{
	auto genericApplication = FSlateApplication::Get().GetPlatformApplication();
	if (genericApplication.Get() != nullptr && genericApplication->IsGamepadAttached())
	{
		return true;
	}
	return false;
}

void UGP2GameInstance::FindReplayGameInfo(const FString & ReplayName)
{
	const FString DemoPath = FPaths::Combine(*FPaths::ProjectSavedDir(), TEXT("Demos/"));
	const FString StreamDirectory = FPaths::Combine(*DemoPath, *ReplayName);
	const FString SaveFileName = TEXT("ReplayInfo.sav");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	FString AbsoluteFilePath = StreamDirectory + "/" + SaveFileName;

	//파일있는지 검증 없으면 info 배열에 임시 내용을 넣는다
	if (PlatformFile.FileExists(*AbsoluteFilePath))
	{
		FFileHelper::LoadFileToStringArray(FindingGameSaveInfo, *AbsoluteFilePath);
	}
	else
	{
		FindingGameSaveInfo.Add(FString("EmptyCharacterName"));
		FindingGameSaveInfo.Add(FString("EmptyMapName"));
	}

}

void UGP2GameInstance::OnEnumerateStreamsComplete(const TArray<FNetworkReplayStreamInfo>& StreamInfos)
{
	
	TArray<FGP2_ReplayInfo> AllReplays;


	if (StreamInfos.Num() == 0)
	{
		return;
	}

	if (NumberOfSaveableReplay != 0)
	{
		if (StreamInfos.Num() > NumberOfSaveableReplay)
		{

			DeleteReplay(StreamInfos[0].Name);

		}
	}

		for (int32 Index = StreamInfos.Num() - 1; Index >= 0; Index--)
		{
			if (!StreamInfos[Index].bIsLive)
			{
				FindReplayGameInfo(StreamInfos[Index].Name);

				AllReplays.Add(FGP2_ReplayInfo(StreamInfos[Index].Name, StreamInfos[Index].FriendlyName, FString::FromInt(StreamInfos[Index].SizeInBytes),
					FindingGameSaveInfo[0], FindingGameSaveInfo[1], StreamInfos[Index].Timestamp, StreamInfos[Index].LengthInMS));

			}

		}
		
		BP_OnFindReplaysComplete(AllReplays);

}

void UGP2GameInstance::OnDeleteFinishedStreamComplete(const bool bDeleteSucceeded)
{
	//FindReplays();
}

void UGP2GameInstance::OnGotoTimeFinished(const bool FinishRoad)
{
	BP_OnGotoTimeFinished(FinishRoad);
}

void UGP2GameInstance::OnStart()
{
	if (!IsDedicatedServerInstance())
	{
		UE_LOG(LogTemp, Error, TEXT("UGP2GameInstance::OnStart HandleNetworkFailure Add"));
		GEngine->OnNetworkFailure().AddUObject(this, &UGP2GameInstance::HandleNetworkFailure);
	}

// 내부 데디케이트 서버리스트 등록을 위해 socket.io 구현 했던 코드 임시로 남겨 둔다. sjg 18.08.09
#if 0
	if (!NativeClient.IsValid() && this->IsDedicatedServerInstance())
		NativeClient = MakeShareable(new FSocketIONative);

	if (NativeClient.IsValid() && this->IsDedicatedServerInstance())
	{

		NativeClient->Connect(FString("http://172.20.30.106:3002"), nullptr, nullptr);

		int port = 0;

		if (!FParse::Value(FCommandLine::Get(), _T("port"), port))
		{
			UE_LOG(LogTemp, Log, TEXT("Port Not Found Default Setting 7777"));
			port = 7777;
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Port Found Default Setting %d"), port);
		}

		auto message = sio::int_message::create(port);

		NativeClient->EmitRaw(FString("ServerReg"), message);

		NativeClient->OnRawEvent(FString(TEXT("ServerReg")), [&](const FString& Name, const sio::message::ptr& Message)
		{
			//if you expected an object e.g. {}
			if (Message->get_flag() != sio::message::flag_object)
			{
				UE_LOG(LogTemp, Warning, TEXT("Warning! event did not receive expected Object."));
				return;
			}

			auto MessageMap = Message->get_map();

			//use the map to decode an object key e.g. type string
			auto typeMessage = MessageMap["result"];

			if (typeMessage->get_flag() == typeMessage->flag_boolean)
			{
				if (typeMessage->get_bool())
				{
					UE_LOG(LogTemp, Log, TEXT("ServerReg ok"));
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("ServerReg fail"));
				}
			}

		});
	}
#endif

	//WebReq_ServerReg();

	Super::OnStart();
}

void UGP2GameInstance::WebReq_ServerReg()
{
#if WITH_GAMELIFT
	if (!m_bLocal)
		return;

	if (m_bLobby) return;

	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_ServerReg);

	int port = 0;

	if (!FParse::Value(FCommandLine::Get(), TEXT("port="), port))
	{
		UE_LOG(LogTemp, Log, TEXT("Port Not Found Default Setting 7777"));
		port = 7777;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Port Found Default Setting %d"), port);
	}

	FString GameMode;

	if (m_eGameMode == EGAME_MODE::PVP)
		GameMode = "PVP";
	else
		GameMode = "PVE";

	FString PostData = FString::Printf(TEXT("port=%d&gameMode=%s"), port, *GameMode);

	Request->SetURL("http://localhost:3001/testServer/serverReg");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_ServerReg(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_ServerReg"));
			m_GameSessionID = JsonObject->GetStringField("GameSessionID");
			UE_LOG(LogTemp, Log, TEXT("GameSessionID : %s"), *m_GameSessionID);
		}
	}
#endif
}

void UGP2GameInstance::WebReq_ServerReuse()
{
#if WITH_GAMELIFT
	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_ServerReuse);

	FString GameSessinID = "0";

	GameSessinID = m_GameSessionID;

	FString PostData = FString::Printf(TEXT("serverID=%s"), *GameSessinID);

	Request->SetURL("http://localhost:3001/testServer/serverReuse");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_ServerReuse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_ServerReuse"));
			InitGameData();
		}
	}
#endif
}

void UGP2GameInstance::WebReq_RoomEnterUser()
{
#if WITH_GAMELIFT
	if (!m_bLocal)
		return;

	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_RoomEnterUser);

	FString GameSessinID = "0";

	GameSessinID = m_GameSessionID;

	FString PostData = FString::Printf(TEXT("serverID=%s"), *GameSessinID);

	Request->SetURL("http://localhost:3001/testServer/roomenter");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_RoomEnterUser(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (!JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_RoomEnterUser Fail"));
		}
	}
#endif
}

void UGP2GameInstance::WebReq_RoomOutUser()
{
#if WITH_GAMELIFT
	if (!m_bLocal)
		return;

	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_RoomOutUser);

	FString GameSessinID = "0";

	GameSessinID = m_GameSessionID;

	FString PostData = FString::Printf(TEXT("serverID=%s"), *GameSessinID);

	Request->SetURL("http://localhost:3001/testServer/roomout");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_RoomOutUser(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (!JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_RoomOutUser Fail"));
		}
	}
#endif
}

void UGP2GameInstance::WebReq_GameStart()
{
#if WITH_GAMELIFT
	if (!m_bLocal)
		return;

	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_GameStart);

	FString GameSessinID = "0";

	GameSessinID = m_GameSessionID;

	FString PostData = FString::Printf(TEXT("serverID=%s"), *GameSessinID);

	Request->SetURL("http://localhost:3001/testServer/gamestart");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_GameStart(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (!JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_GameStart Fail"));
		}
	}
#endif
}

void UGP2GameInstance::WebReq_GameEnd()
{
#if WITH_GAMELIFT
	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UGP2GameInstance::WebRes_GameEnd);

	int port = 0;

	if (!FParse::Value(FCommandLine::Get(), TEXT("port="), port))
	{
		UE_LOG(LogTemp, Log, TEXT("Port Not Found Default Setting 7777"));
		port = 7777;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Port Found Default Setting %d"), port);
	}

	FString PostData = FString::Printf(TEXT("port=%d"), port);

	Request->SetURL("http://localhost:3001/testServer/serverReg");
	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif
}

void UGP2GameInstance::WebRes_GameEnd(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{

}

bool UGP2GameInstance::IsWithEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

EWebResultEnum UGP2GameInstance::ToWebResultEnum(UObject* WorldContextObject, float fValue)
{
	int iTemp = static_cast<int>(fValue);

	return static_cast<EWebResultEnum>(iTemp);
}


void UGP2GameInstance::LoginSessionCheck(const int &PlayerID, const FString &PlayerSessionID, const int &UserSN, bool &CheckResult, int &errorType)
{
#if WITH_GAMELIFT
	CheckResult = true;


	if (m_eGameMode == EGAME_MODE::PVE && m_byEnterUserCount >= 4)
	{
		UE_LOG(LogTemp, Log, TEXT("PVE User Full"));
		CheckResult = false;
	}
	else if (m_eGameMode == EGAME_MODE::PVP && m_byEnterUserCount >= 8)
	{
		UE_LOG(LogTemp, Log, TEXT("PVP User Full"));
		CheckResult = false;
	}

	if (m_bGameStart)
	{
		UE_LOG(LogTemp, Log, TEXT("Already GameStart"));
		CheckResult = false;
	}

	if (m_bLocal)
	{
	//	CheckResult = true;
	//	m_ConnectUserList.Emplace(PlayerID, FConnectInfo(UserSN, PlayerSessionID));
		WebReq_RoomEnterUser();
	//	return;
	}

	FGameLiftGenericOutcome result = gameLiftSdkModule->AcceptPlayerSession(PlayerSessionID);

	UE_LOG(LogTemp, Error, TEXT("LoginSessionCheck m_byEnterUserCount : %d"), m_byEnterUserCount);

	m_bCheckEmptyGameSession = false;

	m_byEnterUserCount++;

	if ( result.IsSuccess() )
	{
		m_ConnectUserList.Emplace(PlayerID, FConnectInfo(UserSN, PlayerSessionID));
	}
	else
	{
		CheckResult = false;

		gameLiftSdkModule->RemovePlayerSession(PlayerSessionID);
		UE_LOG(LogTemp, Error, TEXT("### AcceptPlayerSession Fail : %s , UserSN : %d"), result.GetError().m_errorMessage.GetCharArray().GetData(), UserSN);
	}
#endif
}


void UGP2GameInstance::LogoutProcess(const int &PlayerID)
{
#if WITH_GAMELIFT

	//if (m_ConnectUserList.Contains(PlayerID))

	if(m_byEnterUserCount > 0)
		m_byEnterUserCount--;
	
	UE_LOG(LogTemp, Log, TEXT("getGameInstance LogoutProcess m_byEnterUserCount : %d"), m_byEnterUserCount);

	//if (m_byEnterUserCount <= 0)
	//{
	//	if (GetWorld())
	//	{
	//		if (m_eGameMode == EGAME_MODE::PVE)
	//		{
	//			UE_LOG(LogTemp, Log, TEXT("getGameInstance->getGameMode() == EGAME_MODE::PVE"));
	//			GetWorld()->ServerTravel("/Game/Maps/Room_PVE_Lobby");
	//		}
	//		else
	//		{
	//			UE_LOG(LogTemp, Log, TEXT("getGameInstance->getGameMode() == EGAME_MODE::PVP"));
	//			GetWorld()->ServerTravel("/Game/Maps/Room_PVP_Lobby");


	//		}
	//	}
	//}

	if (m_bLocal)
	{
		//if (m_byEnterUserCount <= 0)
		//{
		//	m_bGameStart = false;
		//	m_ConnectUserList.Empty();
		//}
		//else
		//{
		//	if (m_ConnectUserList.Contains(PlayerID))
		//	{
		//		m_ConnectUserList.Remove(PlayerID);
		//	}
		//}

		//if (m_byEnterUserCount <= 0)
		//	WebReq_ServerReuse();
		//else
		if(m_ConnectUserList.Contains(PlayerID))
			WebReq_RoomOutUser();

		//return;
	}

	if (!m_bLobby)
	{
		if (gameLiftSdkModule.IsValid() && m_ConnectUserList.Contains(PlayerID) && m_ConnectUserList[PlayerID].PlayerSessionID.Len() > 0)
		{
			FGameLiftGenericOutcome OutCom = gameLiftSdkModule->RemovePlayerSession(m_ConnectUserList[PlayerID].PlayerSessionID);

			if (!OutCom.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("### RemovePlayerSession Fail UserSN : %d SessionID %s"), m_ConnectUserList[PlayerID].UserSN, *(m_ConnectUserList[PlayerID].PlayerSessionID));
			}
		}
	}

	if (m_byEnterUserCount <= 0)
	{
		m_bGameStart = false;
		m_ConnectUserList.Empty();
	}
	else
	{
		if (m_ConnectUserList.Contains(PlayerID))
		{
			m_ConnectUserList.Remove(PlayerID);
		}
	}

	if(!m_bLobby)
	{
		if (m_byEnterUserCount <= 0 && m_bActiveGameLiftSession)
		{
			if (m_eGameMode == EGAME_MODE::PVP)
				UGameplayStatics::OpenLevel(GetWorld(), "MatchingRoom_PVP");
			else
				UGameplayStatics::OpenLevel(GetWorld(), "MatchingRoom_PVE");

			FGameLiftGenericOutcome OutCom = gameLiftSdkModule->TerminateGameSession();

			m_bActiveGameLiftSession = false;

			if (OutCom.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("### TerminateGameSession success"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("### TerminateGameSession fail %s"), *OutCom.GetError().m_errorMessage);
			}
		}
	}
#endif
}

void UGP2GameInstance::GameLiftStartProcess()
{
	if (m_bGameStart)
	{
		return;
	}

	m_bGameStart = true;

#if WITH_GAMELIFT

	if (m_bLocal)
	{
		WebReq_GameStart();
	//	return;
	}

	if(!m_bLobby)
	{
		if (gameLiftSdkModule.IsValid())
		{
			FGameLiftGenericOutcome outcome = gameLiftSdkModule->UpdatePlayerSessionCreationPolicy(EPlayerSessionCreationPolicy::DENY_ALL);

			if (!outcome.IsSuccess())
			{
				UE_LOG(LogTemp, Error, TEXT("### UpdatePlayerSessionCreationPolicy Fail %s"), *outcome.GetError().m_errorMessage);
			}
		}
	}
#endif
}

EGAME_MODE UGP2GameInstance::getGameMode()
{
	return m_eGameMode;
}

bool UGP2GameInstance::getGameStart()
{
	return m_bGameStart;
}

#if WITH_GAMELIFT
void UGP2GameInstance::EmptyGameSessionCheckTick()
{
//	if (m_bLocal)
//		return;

	if (!m_bCheckEmptyGameSession)
		return;

	if (m_bActiveGameLiftSession)
	{
		if (!m_bSettingMap)
		{
//			if (m_Map == NULL)
			{
				if (m_eGameMode == EGAME_MODE::PVP)
					UGameplayStatics::OpenLevel(GetWorld(), "MatchingRoom_PVP");
				else if (m_eGameMode == EGAME_MODE::TUTORIAL)
					UGameplayStatics::OpenLevel(GetWorld(), "G2_Tutorial");
				else
					UGameplayStatics::OpenLevel(GetWorld(), "MatchingRoom_PVE");
			}
//			else
//				UGameplayStatics::OpenLevel(GetWorld(), m_Map);

			m_bSettingMap = true;

			UE_LOG(LogTemp, Error, TEXT("### EmptyGameSessionCheckTick ActivateGameSession"));

			gameLiftSdkModule->ActivateGameSession();
		}
	}

	if ( m_byEnterUserCount <= 0 )
	{
		m_EmptyGameSessionCheckCount++;

		if ( m_EmptyGameSessionCheckCount >= 30 )
		{
			if (!m_bLobby)
			{
				FGameLiftGenericOutcome OutCom = gameLiftSdkModule->TerminateGameSession();

				if (OutCom.IsSuccess())
				{
					UE_LOG(LogTemp, Error, TEXT("### TerminateGameSession success"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("### TerminateGameSession fail %s"), *OutCom.GetError().m_errorMessage);
				}
			}

			m_EmptyGameSessionCheckCount = 0;
			m_bCheckEmptyGameSession = false;
			m_bActiveGameLiftSession = false;
		}
	}
	else
	{
		m_EmptyGameSessionCheckCount = 0;
	}
}
#endif

int8 UGP2GameInstance::getEnterUserCount()
{
	return m_byEnterUserCount;
}

int32 UGP2GameInstance::getRoomNumber()
{
	return m_iRoomNumber;
}

const FString &  UGP2GameInstance::getRoomSubject()
{
	return m_Subject;
}

const FString & UGP2GameInstance::getGameSession()
{
	return m_GameSessionID;
}

const FString & UGP2GameInstance::getServerSystemInfo()
{
	m_ServerSystemInfo = FString::Printf( TEXT( "CPU Usage : %0.1f%% Memory Usage : %skb" ) , FPlatformTime::GetCPUTime().CPUTimePct , *FString::FormatAsNumber( FPlatformMemory::GetStats().UsedPhysical / 1024 ) );

	return m_ServerSystemInfo;
}

bool UGP2GameInstance::getLocal()
{
	return m_bLocal;
}

bool UGP2GameInstance::getGameLiftActive()
{
	if(m_bLobby) return true;

#if WITH_GAMELIFT
	return m_bActiveGameLiftSession;
#else
	return false;
#endif
}

const FString & UGP2GameInstance::getWebURL()
{

	return m_WebServerIP;
}

int32 UGP2GameInstance::getCurrnetTickCount()
{
	return FPlatformTime::Seconds();
}


//void UGP2GameInstance::AsyncCharacterLoad(int CharacterIndex, FCharacterLoadCompleted CompletedFunc)
//{
//	
//}

const bool UGP2GameInstance::GetAutoLogin()
{
	return g_AutoLogin;
}

const FString& UGP2GameInstance::GetAutoLoginId()
{
	return g_AutoLoginId;
}

const bool UGP2GameInstance::GetLocalMode()
{
	return g_LocalMode;
}

const int UGP2GameInstance::GetDefaultCharacter()
{
	return g_DefaultCharacter;
}

const bool UGP2GameInstance::GetSkipTutorial()
{
	return g_SkipTutorial;
}

const bool UGP2GameInstance::GetSkipAdvent()
{
	return g_SkipAdvent;
}

const bool UGP2GameInstance::GetShowDebugArrow()
{
	return g_ShowDebug;
}

const bool UGP2GameInstance::GetJsonSecurity( int InIndex )
{
	switch ( InIndex )
	{
	case 0:  return g_JsonSecurity;
	case 1:  return g_JsonSecurityJpn;
	case 2:  return g_JsonSecurityUsa;
	}

	return g_JsonSecurity;
}

FString UGP2GameInstance::GetServerAddress( int InIndex )
{
	FString FullUrl = TEXT( "" );

	/// FULL URL 형식으로 변경 - 포트 제거.
	switch (InIndex)
	{
	case 0:  FullUrl = g_LocalServerAddr; break;
	case 1:  FullUrl = g_JapanServerAddr; break;
	case 2:  FullUrl = g_UsaServerAddr;   break;
	default: FullUrl = g_LocalServerAddr; break;
	}

	return FullUrl;
}

