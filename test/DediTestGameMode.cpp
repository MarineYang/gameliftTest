// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// Include game project files. "GameLiftFPS" is a sample game name, replace with file names from your own game project
#include "DediTestGameMode.h"
#include "DediTest.h"
#include "Engine.h"
#include "EngineGlobals.h"
#include "DediTestCharacter.h"

ADediTestGameMode::ADediTestGameMode()
	: Super()
{
	m_GameSessionID.Empty();
	m_byEnterUserCount = 0;

	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	UE_LOG(LogTemp, Log, TEXT("GameMode IN"));
	//Let's run this code only if GAMELIFT is enabled. Only with Server targets!
#if WITH_GAMELIFT
	UE_LOG(LogTemp, Log, TEXT("GameMode GameLift IN"));
	//Getting the module first.
	FGameLiftServerSDKModule* gameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));
	UE_LOG(LogTemp, Log, TEXT("GameMode GameLift IN 2 "));
	//InitSDK establishes a local connection with GameLift's agent to enable communication.
	gameLiftSdkModule->InitSDK();
	UE_LOG(LogTemp, Log, TEXT("GameMode ============InitSDK============"));



	
	auto onGameSession = [=](Aws::GameLift::Server::Model::GameSession gameSession)
	{
		gameLiftSdkModule->ActivateGameSession();

		m_iRoomNumber = atoi(gameSession.GetGameSessionData());
		m_GameSessionID = gameSession.GetGameSessionData();
		m_Subject = gameSession.GetName();
	};


	FProcessParameters* params = new FProcessParameters();
	params->OnStartGameSession.BindLambda(onGameSession);


	params->OnTerminate.BindLambda([=]() {gameLiftSdkModule->ProcessEnding(); });


	params->OnHealthCheck.BindLambda([]() {return true; });


	params->port = 7777;


	TArray<FString> logfiles;
	logfiles.Add(TEXT("aLogFile.txt"));
	params->logParameters = logfiles;

	//Call ProcessReady to tell GameLift this game server is ready to receive game sessions!
	FGameLiftGenericOutcome outCome = gameLiftSdkModule->ProcessReady(*params);
	if (!outCome.IsSuccess()) {
		UE_LOG(LogTemp, Log, TEXT("GameMode ============ Fail ProcessReady============"));
	}
	UE_LOG(LogTemp, Log, TEXT("GameMode ============ProcessReady============"));


	
	//Accept()
	Connection& connection;

	FGameLiftGenericOutcome outcome = gameLiftSdkModule->AcceptPlayerSession(PlayerSessionID);
	if (outcome.IsSuccess() == TRUE) {
		//성공.
		m_ConnectUserList.Emplace(connection, PlayerSessionID);
		TEXT("================Success Accept to server================");
	}

	else {
		//실패.
		TEXT("================Faild Accept to server================");
		gameLiftSdkModule->RemovePlayerSession(PlayerSessionID);
	}
	

#endif
}

void ADediTestGameMode::GameLiftStartProcess()
{
#if WITH_GAMELIFT
	
	WebReq_GameStart();

	if (gameLiftSdkModule.IsValid())
	{
		FGameLiftGenericOutcome outCome = gameLiftSdkModule->UpdatePlayerSessionCreationPolicy(EPlayerSessionCreationPolicy::DENY_ALL);

		if (!outCome.IsSuccess())
		{
			UE_LOG(LogTemp, Error, TEXT("### UpdatePlayerSessionCreationPolicy Fail %s"), *outCome.GetError().m_errorMessage);
		}
	}
	
#endif

}

void ADediTestGameMode::WebReq_GameStart()
{

#if WITH_GAMELIFT
	FHttpModule* http;
	http = &FHttpModule::Get();
	TSharedRef<IHttpRequest> Request = http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &ADediTestGameMode::WebRsp_GameStart);

	FString GameSessionID = "0";
	GameSessionID = m_GameSessionID;
	FString PostData = FString::Printf(TEXT("serverID=%s"), *GameSessionID);

	Request->SetVerb("POST");
	Request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", TEXT("application/x-www-form-urlencoded"));
	Request->SetContentAsString(PostData);
	Request->ProcessRequest();
#endif

}

void ADediTestGameMode::WebRsp_GameStart(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	
#if WITH_GAMELIFT
	TSharedPtr<FJsonObject> JsonObject;
	//How to Parser ? => TJsonREader Obj Type change the TCHAR .
	TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Response->GetContentAsString());

	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (!JsonObject->GetIntegerField("result"))
		{
			UE_LOG(LogTemp, Log, TEXT("WebRes_GameStart Fail"));
		}
	}
#endif
	
}
