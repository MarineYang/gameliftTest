"use strict";


/**
 * @swagger
 * tags:
 *   name: ResultType
 *   description: 
 */


/**
 * @swagger
 * /RETURN CODE:
 *   post:
 *     summary: 패킷 응답 result 값 enum list
 *     tags: [ResultType]
 *     description: |
 *       # [ResultType]
 *       ```
 *       FAIL
 *       ,SUCCESS                                    // (서버로 요청내용) 성공
 *       ,INVALID_VERSION                            // 서버 클라 버전 미스매치
 *       ,WRONG_PASSWORD                             // 비밀번호가 틀림
 *       ,NOT_FOUND_USER                             // 유저 정보 없음
 *       ,ALREADY_IN_CONNECT                         // 이미 로그인 중
 *       ,AUTH_TOKEN_INVALID                         // 토큰 오류
 *       ,GAMELIFT_NO_AVAILABLE_PROCESS              // 사용 가능한 데디케이트 서버가 없음. ( 만들수 있는 방이 없음 )
 *       ,GAMELIFT_GAMESESSION_FULL                  // 입장 하려는 방이 이미 풀 상태
 *       ,GAMELIFT_GAMESESSION_INVALID_STATUS        // 방 입장이 불가 상태
 *       ,GAMELIFT_GAMESESSION_ACTIVATING_RETRY      // 방 입장 가능한 상태 준비중으로 1초 후 다시 입장 시도 처리
 *       ,GAMELIFT_PLAYERSESSION_TERMINATED          // 이미 종료된 방
 *       ,GAMELIFT_PLAYERSESSION_ALREADY_CONNECTED   // 이미 방에 입장한 상태
 *       ,GAMELIFT_PLAYERSESSION_DISCONNECT          // 입장 중 연결이 끊김
 *       ,GAMELFIT_PLAYERSESSION_TIMEOUT             // 입장 처리가 시간 지연으로 실패
 *       ,GAMELFIT_PLAYERSESSION_ALREADY_GAMESTART   // 게임이 시작되어 입장할 수 없음
 *       ,GAMELIFT_ETC_ERROR                         // 기타 gamelift 에러
 *       ,UNKNOWN_DB_ERROR=1001                      // 알수 없는 DB 오류
 *       ,UNKNOWN_SERVER_ERROR                       // 알수 없는 서버 오류
 *       ,WRONG_PARAMETER                            // 잘못된 파라메터 오류
 *       ,NO_VERSION_PARAMETER                       // 버전 정보 파라메터 없음
 *       ,NO_GAME_BAISC_SET_DATA                     // 게임 기본 세팅 정보 없음
 *       ,DIFFERENT_GAME_VERSION                     // 게임 클라이언트와 서버 버전이 다름
 *       ,REDIS_DB_SELECT_ERROR=1010                 // 레디스 DB select 오류
 *       ,REDIS_DB_RANK_ERROR                        // 레디스 DB rank 오류
 *       ,REDIS_DB_RANGE_ERROR                       // 레디스 DB range 오류
 *       ,REDIS_DB_ADD_ERROR                         // 레디스 DB add 오류
 *       ,REDIS_DB_COUNT_ERROR                       // 레디스 DB count 오류
 *       ,ERROR_CHARACTER_NAME_USED=1050             // 이미 존재하는 케릭터명입니다.(이름이)
 *       ,ERROR_CHARACTER_CREATE_ALREADY             // 이미 존재하는 케릭터명입니다.(해당 슬롯에)
 *       ,FAILED_HTTP_HANDLER=1101                   // HTTP 호출 요청 실패
 *       ,TEMP_INSPECTION=2000                       // 임시 점검 중입니다.
 *       ,INSPECTION                                 // 점검 중입니다.
 *       ```
 */

export enum ERROR_TYPE {
	 FAIL
    ,SUCCESS                                    // (서버로 요청내용) 성공
    ,INVALID_VERSION                            // 서버 클라 버전 미스매치
    ,WRONG_PASSWORD                             // 비밀번호가 틀림
    ,NOT_FOUND_USER                             // 유저 정보 없음
    ,ALREADY_IN_CONNECT                         // 이미 로그인 중
    ,AUTH_TOKEN_INVALID                         // 토큰 오류
    ,GAMELIFT_NO_AVAILABLE_PROCESS		        // 사용 가능한 데디케이트 서버가 없음. ( 만들수 있는 방이 없음 )
    ,GAMELIFT_GAMESESSION_FULL                  // 입장 하려는 방이 이미 풀 상태
    ,GAMELIFT_GAMESESSION_INVALID_STATUS        // 방 입장이 불가 상태
    ,GAMELIFT_GAMESESSION_ACTIVATING_RETRY      // 방 입장 가능한 상태 준비중으로 1초 후 다시 입장 시도 처리
    ,GAMELIFT_PLAYERSESSION_TERMINATED          // 이미 종료된 방
    ,GAMELIFT_PLAYERSESSION_ALREADY_CONNECTED   // 이미 방에 입장한 상태
    ,GAMELIFT_PLAYERSESSION_DISCONNECT          // 입장 중 연결이 끊김
    ,GAMELFIT_PLAYERSESSION_TIMEOUT             // 입장 처리가 시간 지연으로 실패
    ,GAMELFIT_PLAYERSESSION_ALREADY_GAMESTART   // 게임이 시작되어 입장할 수 없음
    ,GAMELIFT_ETC_ERROR					        // 기타 gamelift 에러
    ,UNKNOWN_DB_ERROR=1001                      // 알수 없는 DB 오류
    ,UNKNOWN_SERVER_ERROR                       // 알수 없는 서버 오류
    ,WRONG_PARAMETER                            // 잘못된 파라메터 오류
    ,NO_VERSION_PARAMETER                       // 버전 정보 파라메터 없음
    ,NO_GAME_BAISC_SET_DATA                     // 게임 기본 세팅 정보 없음
    ,DIFFERENT_GAME_VERSION                     // 게임 클라이언트와 서버 버전이 다름
    ,REDIS_DB_SELECT_ERROR=1010                 // 레디스 DB select 오류
    ,REDIS_DB_RANK_ERROR                        // 레디스 DB rank 오류
    ,REDIS_DB_RANGE_ERROR                       // 레디스 DB range 오류
    ,REDIS_DB_ADD_ERROR                         // 레디스 DB add 오류
    ,REDIS_DB_COUNT_ERROR                       // 레디스 DB count 오류
    ,ERROR_CHARACTER_NAME_USED=1050             // 이미 존재하는 케릭터명입니다.(이름이)
    ,ERROR_CHARACTER_CREATE_ALREADY             // 이미 존재하는 케릭터명입니다.(해당 슬롯에)
    ,FAILED_HTTP_HANDLER=1101                   // HTTP 호출 요청 실패
    ,TEMP_INSPECTION=2000                       // 임시 점검 중입니다.
    ,INSPECTION                                 // 점검 중입니다.
}