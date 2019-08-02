import { Router, Request, Response } from "express";
import * as UTILS from "../util/utils";
import {send} from "../util/packet";

let localIP = "127.0.0.1";

const router: Router = Router();

import AWS = require("aws-sdk");
import GameLiftSDK = require("aws-sdk/clients/gamelift");

let arrGameSession: any[] = [];

const AWSGameLiftCfg: AWS.GameLift.Types.ClientConfiguration = {
    accessKeyId:process.env.ACCESSKEYID,
    secretAccessKey:process.env.SECRETACCESSKEY,
    region:process.env.REGION,
    endpoint: process.env.AWSENDPOINT
};

const gameLift = new GameLiftSDK(AWSGameLiftCfg);

router.post("/create", function(req: Request, rsp: Response) {
    if ( !UTILS.isNumber(req.body.usersn) || !UTILS.isString(req.body.gamesessionid) )
    {
        //11 => Worn
        //1 => success

        UTILS.sendResult(rsp, 11);
        console.log("GameLog", "Create Error : WRONG_PARAMETER : " + JSON.stringify(req.body));
        //UTILS.SetLogToFile("GameLog", "Create Error : WRONG_PARAMETER : " + JSON.stringify(req.body));
        return;
    }

    let EnterUserMax = 5;
    const params: GameLiftSDK.CreateGameSessionInput = {
        MaximumPlayerSessionCount: req.body.usermax, /* required */
        CreatorId: req.body.usersn.toString(),
  
        GameProperties: [
            {
                Key: "mode", /* required */
                Value: req.body.gamemode /* required */
            },
            {
                Key: "usermax",
                Value: EnterUserMax.toString()
            },
            {
                Key: "serverip",
                Value: process.env.SERVERIP
            }
        ],

        Name: JSON.stringify(req.body.subject)
    };
    params.FleetId = process.env.FLEETID;


    gameLift.createGameSession(params, function(err: AWS.AWSError, data: AWS.GameLift.CreateGameSessionOutput) {
        if (err)
        {
            //-1 => Error
            console.log("GameLog", "createGameSession Error : WRONG_PARAMETER : " + JSON.stringify(req.body));
            UTILS.sendResult(rsp, -1, err.code);
        }
        else
        {
            if (data.GameSession.Status != "ACTIVE" && data.GameSession.Status != "ACTIVATING") {
                UTILS.sendResult(rsp, -1, data.GameSession.Status);
                UTILS.SetLogToFile("GameLog", "Room create Error createGameSession ACTIVE OR ACTIVATING : " + data.GameSession.Status);
            }
            //send 하기 전에 room index 생성 ?
            //11 => Worn
            //1 => success
            send(rsp,
                {
                    "result" : 1,
                    "GameSessionId":data.GameSession.GameSessionId,
                    "Subject" : req.body.subject,
                    "MaxPlayer" : EnterUserMax
                }
            );
        }
    });
});

router.post("/join", function(req: Request, rsp: Response) {
    if ( !UTILS.isNumber(req.body.usersn) || !UTILS.isString(req.body.gamesessionid) )
    {
        //11 => Worn
        //1 => success

        UTILS.sendResult(rsp, 11);
        console.log("GameLog", "join Error : WRONG_PARAMETER : " + JSON.stringify(req.body));
        return;
    }
    const params: AWS.GameLift.CreatePlayerSessionInput = {
        GameSessionId: req.body.gamesessionid, /* required */
        PlayerId: req.body.userid, /* required */
    };
    let ipAddress: string|undefined = "";
    gameLift.createPlayerSession(params, function(err: AWS.AWSError, data: AWS.GameLift.CreatePlayerSessionOutput) {
        if (err)
        {
            //let return_value = 0;
            console.log(err, err.stack); // an error occurred
            console.log("GameLog", "Room join Error : createPlayerSession - " + JSON.stringify(err.code));
            send(rsp,
                {
                    "result" : -1,
                    "errMsg" : err.code
                });
        } else {

            const outputResult: any = data.PlayerSession;
            console.log("AWS.GameLift.PlayerSession : " + outputResult);

            ipAddress = outputResult.IpAddress+":"+outputResult.Port;
            let game: any = arrGameSession.find(x=>x.GameSessionID==req.body.gamesessionid);
            send( rsp,
                {
                    "result" : 1,
                    "IpAddress" : ipAddress,
                    "PlayerSessionID" : outputResult.PlayerSessionId,
                    "No" : game.No,
                    "GameSessionId": game.GameSessionID,
                    "Subject" : game.subject,
                    "Usercount" : game.Usercount,
                    "Usermax" : game.Usermax,
                    "EnterState" : game.EnterState,
                });
        }
    });
});

export = router;