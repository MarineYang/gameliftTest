import { Router, Request, Response } from "express";
import * as UTILS from "../util/utils";
import {send} from "../util/packet";

//let localIP = "127.0.0.1";

const router: Router = Router();

import AWS = require("aws-sdk");
import GameLiftSDK = require("aws-sdk/clients/gamelift");
import { ERROR_TYPE } from "../util/ResultType";
let roomcreated = 0;
let gamesessionBak = "";

const AWSGameLiftCfg: AWS.GameLift.Types.ClientConfiguration = {
    accessKeyId:"AKIAJU3CYKVZYOYFBMQA",
    secretAccessKey:"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",
    region:"ap-northeast-1",
    endpoint: "http://localhost:9080"
};
console.log("GameLog", "AWSGameLiftCfg : " + JSON.stringify(AWSGameLiftCfg));
const gameLift = new GameLiftSDK(AWSGameLiftCfg);

console.log("Connected Route....")

router.post("/create", function(req: Request, rsp: Response) {

    console.log("GameLog", "Create Init ! ");

    const params: GameLiftSDK.CreateGameSessionInput = {
        GameProperties: [
            {
                Key: "serverip",
                Value: "127.0.0.1:8080"
            }
        ],
        MaximumPlayerSessionCount: 100, /* required */
    };
    params.FleetId = "fleet-1a2b3c4d-5e6f-7a8b-9c0d-1e2f3a4b5c6d";
    //params.AliasId = "alias-f31ee654-2614-4fc9-b258-c04da0114ea0";

    if(roomcreated)
    {
        send(rsp,
            {
                "result" : ERROR_TYPE.SUCCESS,
                "GameSessionId":gamesessionBak,
            }
        );

        return;
    }

    gameLift.createGameSession(params, function(err: AWS.AWSError, data: AWS.GameLift.CreateGameSessionOutput) {
        
        if (err)
        {
            if(err.code == 'FleetCapacityExceededException')
		  {
            console.log("GameLog", "createGameSession Error Err Code : " + err.code + " : WRONG_PARAMETER : " + JSON.stringify(req.body));
              UTILS.sendResult(rsp, ERROR_TYPE.GAMELIFT_NO_AVAILABLE_PROCESS, err.code);
          }
        }
        else
        {
            if (data.GameSession.Status != "ACTIVE" && data.GameSession.Status != "ACTIVATING") {
                UTILS.sendResult(rsp, ERROR_TYPE.GAMELIFT_ETC_ERROR, data.GameSession.Status);
                console.log("GameLog", "Room create Error createGameSession ACTIVE OR ACTIVATING : " + data.GameSession.Status);
            }
            console.log("GameLog", "Room create success : GameSessioId : " + JSON.stringify(data.GameSession.GameSessionId));
            roomcreated = 1;
            gamesessionBak = data.GameSession.GameSessionId;

            send(rsp,
                {
                    "result" : ERROR_TYPE.SUCCESS,
                    "GameSessionId":data.GameSession.GameSessionId,
                }
            );
           return;
        }
        
    });

});
let PlayerIdx = 0;


router.post("/join", function(req: Request, rsp: Response) {
    console.log("GameLog", "join Init ! ");
    if ( !UTILS.isString(req.body.gamesessionid) )
    {

        UTILS.sendResult(rsp, ERROR_TYPE.WRONG_PARAMETER);
        console.log("GameLog", "join Error : WRONG_PARAMETER : " + JSON.stringify(req.body));
        return;
    }
    PlayerIdx++;
    const params: AWS.GameLift.CreatePlayerSessionInput = {
        GameSessionId: req.body.gamesessionid, /* required */
        PlayerId: PlayerIdx.toString(), /* required */
    };
    let ipAddress: string|undefined = "";
    gameLift.createPlayerSession(params, function(err: AWS.AWSError, data: AWS.GameLift.CreatePlayerSessionOutput) {
        const outputResult: AWS.GameLift.PlayerSession = <AWS.GameLift.PlayerSession>data.PlayerSession;
        if (err)
        {
            console.log(err, err.stack); // an error occurred
            console.log("GameLog", "Room join Error : createPlayerSession - " + JSON.stringify(err.code));
            
            send(rsp,
                {
                    "result" : ERROR_TYPE.FAIL,
                    "GameSessionId" : params.GameSessionId,
                    "errMsg" : err.code
                });
        } else {
            console.log("AWS.GameLift.PlayerSession : " + outputResult);
            
            ipAddress = "172.20.30.195" + ":"+outputResult.Port;
            console.log("======GameSessionId : " + params.GameSessionId);
            console.log("======PlayerSessionId : " + params.PlayerId);
            send( rsp,
                {
                    "result" : ERROR_TYPE.SUCCESS,
                    "IpAddress" : ipAddress,
                    "PlayerSessionID" : outputResult.PlayerSessionId,
                    "GameSessionId": params.GameSessionId,
                });
        }
    });
});

export = router;