// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GameLiftServerSDK.h"
#include "DediTestGameMode.generated.h"


UCLASS(minimalapi)
class ADediTestGameMode : public AGameModeBase
{
	GENERATED_BODY()


private:
		FString			m_WebServerIP;
		FString         m_Subject;
		FString         m_GameSessionID;
		int32           m_iRoomNumber;
		int8            m_byEnterUserCount;


#if WITH_GAMELIFT
		TSharedPtr <FGameLiftServerSDKModule> gameLiftSdkModule;
		TMap<int32, FConnectInfo>             m_ConnectUserList;

#endif
public:
	ADediTestGameMode();


public:
	UFUNCTION(BlueprintCallable)
		void GameLiftStartProcess();
public:
	void WebReq_GameStart();
	void WebRsp_GameStart(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	//UFUNCTION(BlueprintCallable)
	//void AcceptGameSession(const FString &PlayerSessionID, const int &UserSN, bool &CheckResult, int &errorType);
};



