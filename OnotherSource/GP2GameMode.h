// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameMode.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "GP2GameMode.generated.h"
/**
*
*/
UENUM( BlueprintType )
enum class ETeam : uint8
{
	NONE ,
	RED ,
	BLUE
};
/**
 * 
 */
UCLASS()
class GP2PROJECT_API AGP2GameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void Tick( float DeltaSeconds ) override;
	

public:
	AGP2GameMode();

	virtual void PreLogin( const FString& Options , const FString& Address , const FUniqueNetIdRepl& UniqueId , FString& ErrorMessage ) override;

	void OnResponseReceived( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful );

	void InitData();

	UFUNCTION( BlueprintCallable )
		void getTeamInfo( ETeam &getTeam , uint8 &getSlot );

	UFUNCTION( BlueprintCallable )
		void LoginSessionCheck( const int &PlayerID , const FString &PlayerSessionID , const int &UserSN , bool &CheckResult , int &errorType );

	UFUNCTION( BlueprintCallable )
		void LogoutProcess( const int &PlayerID , const int &LogOutUserSN , const ETeam &LogOutTeam , const int &LogOutSlot , int &MasterUserSN );

	uint8 getEmptySlot( ETeam &Team );


private:

	UPROPERTY()
	TMap<int8 , bool> m_RedTeamMap;  

	UPROPERTY()
	TMap<int8 , bool> m_BlueTeamMap; 

	UPROPERTY()
	TArray<int>       m_AcceptList;  

	ETeam m_eLastSelectTeam;
};
