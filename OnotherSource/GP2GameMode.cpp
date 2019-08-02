// Fill out your copyright notice in the Description page of Project Settings.

#include "GP2GameMode.h"
#include "GP2Project.h"
#include "Kismet/GameplayStatics.h"
#include "GP2GameInstance.h"
#include "Runtime/Engine/Classes/GameFramework/PlayerStart.h"

AGP2GameMode::AGP2GameMode()
{
	InitData();
}

void AGP2GameMode::Tick( float DeltaSeconds )
{
	Super::Tick( DeltaSeconds );
}

void AGP2GameMode::InitData()
{
	m_RedTeamMap.Add( 0 , false );
	m_RedTeamMap.Add( 1 , false );
	m_RedTeamMap.Add( 2 , false );
	m_RedTeamMap.Add( 3 , false );

	m_BlueTeamMap.Add( 0 , false );
	m_BlueTeamMap.Add( 1 , false );
	m_BlueTeamMap.Add( 2 , false );
	m_BlueTeamMap.Add( 3 , false );

	m_AcceptList.Empty();
}

void AGP2GameMode::getTeamInfo( ETeam &getTeam , uint8 &getSlot )
{
	ETeam selectTeam = ETeam::RED;
	uint8 byRedCount = 0 , byBlueCount = 0;
	UGP2GameInstance * getGameInstance = nullptr;

	if ( GetWorld() )
		getGameInstance = Cast<UGP2GameInstance>( GetWorld()->GetGameInstance() );

	if ( !getGameInstance )
	{
		UE_LOG( LogTemp , Error , TEXT( "%s %d GetInstance Error" ) , __FUNCTION__ , __LINE__ );
		return;
	}

	for ( auto &Elem : m_RedTeamMap )
	{
		if ( Elem.Value )
			byRedCount++;
	}

	for ( auto &Elem : m_BlueTeamMap )
	{
		if ( Elem.Value )
			byBlueCount++;
	}

	if ( ( byRedCount == 0 && byBlueCount == 0 ) || ( getGameInstance->getGameMode() == EGAME_MODE::PVE ) )
	{
		getTeam = ETeam::BLUE;
	}
	else
	{
		if ( byRedCount > byBlueCount )
			getTeam = ETeam::BLUE;
		else if ( byRedCount < byBlueCount )
			getTeam = ETeam::RED;
		else
		{
			if ( m_eLastSelectTeam == ETeam::BLUE )
				getTeam = ETeam::RED;
			else
				getTeam = ETeam::BLUE;
		}
	}

	m_eLastSelectTeam = getTeam;
	getSlot = getEmptySlot( getTeam );
}

void AGP2GameMode::LoginSessionCheck( const int &PlayerID , const FString &PlayerSessionID , const int &UserSN , bool &CheckResult , int &errorType )
{
#if WITH_GAMELIFT
	if ( GetWorld() )
	{
		UGP2GameInstance * getGameInstance = Cast<UGP2GameInstance>( GetWorld()->GetGameInstance() );

		if ( getGameInstance )
		{
			getGameInstance->LoginSessionCheck( PlayerID , PlayerSessionID , UserSN , CheckResult , errorType );

			if ( getGameInstance->getEnterUserCount() == 1 )
				InitData();
		}
	}

	if ( CheckResult )
	{
		m_AcceptList.AddUnique( UserSN );
	}
#endif
}

uint8 AGP2GameMode::getEmptySlot( ETeam &Team )
{
	uint8 bySlotIdx = 0;

	if ( Team == ETeam::BLUE )
	{
		for ( auto Elem : m_BlueTeamMap )
		{
			if ( !Elem.Value )
			{
				m_BlueTeamMap.Add( bySlotIdx , true );
				return bySlotIdx;
			}

			bySlotIdx++;
		}
	}
	else
	{
		for ( auto Elem : m_RedTeamMap )
		{
			if ( !Elem.Value )
			{
				m_RedTeamMap.Add( bySlotIdx , true );
				return bySlotIdx;
			}

			bySlotIdx++;
		}
	}

	return bySlotIdx;
}

void AGP2GameMode::LogoutProcess( const int &PlayerID , const int &LogOutUserSN , const ETeam &LogOutTeam , const int &LogOutSlot , int &MasterUserSN )
{
	UGP2GameInstance * getGameInstance = Cast<UGP2GameInstance>( GetWorld()->GetGameInstance() );

	getGameInstance->LogoutProcess( PlayerID );

	if ( getGameInstance && getGameInstance->getEnterUserCount() == 0 )
	{
		InitData();
	}
	else
	{
		MasterUserSN = 0;

		if ( m_AcceptList.Num() > 1 )
		{
			int32 Index;

			if ( m_AcceptList.Find( LogOutUserSN , Index ) )
			{
				m_AcceptList.RemoveAt( Index );

				if ( Index == 0 )
					MasterUserSN = m_AcceptList[ 0 ];
			}
		}

		if ( LogOutTeam == ETeam::BLUE )
		{
			if ( m_BlueTeamMap.Contains( LogOutSlot ) )
				m_BlueTeamMap[ LogOutSlot ] = false;
		}
		else if ( LogOutTeam == ETeam::RED )
		{
			if ( m_RedTeamMap.Contains( LogOutSlot ) )
				m_RedTeamMap[ LogOutSlot ] = false;
		}
	}
#if 0
	FHttpModule* Http;

	Http = &FHttpModule::Get();

	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject( this , &AGP2GameMode::OnResponseReceived );

	UWorld* World = GetWorld();

	if ( World )
	{
		FString PostData = FString::Printf( TEXT( "gamesessionid=%s" ) , *m_GameSessionID );

		Request->SetURL( "http://localhost:3001/testServer/roomout" );
		Request->SetVerb( "POST" );
		Request->SetHeader( TEXT( "User-Agent" ) , "X-UnrealEngine-Agent" );
		Request->SetHeader( "Content-Type" , TEXT( "application/x-www-form-urlencoded" ) );
		Request->SetContentAsString( PostData );
		Request->ProcessRequest();
	}
#endif

}

void AGP2GameMode::OnResponseReceived( FHttpRequestPtr Request , FHttpResponsePtr Response , bool bWasSuccessful )
{

}

void AGP2GameMode::PreLogin( const FString& Options , const FString& Address , const FUniqueNetIdRepl& UniqueId , FString& ErrorMessage )
{
#if WITH_GAMELIFT
	if ( Role != ROLE_Authority )
		return;

	UGP2GameInstance * getGameInstance = Cast<UGP2GameInstance>( GetWorld()->GetGameInstance() );

	if ( !getGameInstance )
	{
		ErrorMessage = "GameInstance Not Found";
		return;
	}

	if ( !getGameInstance->getGameLiftActive() )
	{
		ErrorMessage = "Not Active GameSession";
		return;
	}
#endif
}
