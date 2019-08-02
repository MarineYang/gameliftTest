import { ERROR_TYPE } from "./ResultType";
import * as fs from "fs";
import {send} from "./packet";

export const setErrObj = function (err: any, targetErrorCode?: ERROR_TYPE, targetLogger?: any) {

    if(targetLogger)
    {
        // const loggerInstance = (targetLogger) ? targetLogger : logger;
        // loggerInstance.error(err.stack);
        targetLogger.error(err.stack);
    }

    if (!targetErrorCode) {
        targetErrorCode = ERROR_TYPE.FAIL;
    }

    const arr = err.message.split("#");
    const response = {
        result: (1 < arr.length) ? parseInt(arr[arr.length - 1]) : targetErrorCode,
        errMsg: arr[0].trim()
    };

    return response;
};


export const genErrMsg = function (targetErrorCode: any , extraMessage: any) {

    let errMsg;

    if (extraMessage && typeof extraMessage === "string") {
        errMsg = `${targetErrorCode}, ${extraMessage.trim()} #${targetErrorCode}`;
    }
    else {
        errMsg = `${targetErrorCode} #${targetErrorCode}`;
    }

    return errMsg;
};


export const genErrMsgPair = function (errorCode: any, errMsg: any, optionString: any) {
    const str = genErrMsg(errorCode, `ERROR : ${errMsg}`);
    return [str, `${str}, ${optionString}`];
};


export const genErrMsgLog = function (errorCode: any, extraMessage: any, logMessage: any, targetLogger: any) {
    const genMsgArr = genErrMsgPair(errorCode, extraMessage, logMessage);
    const errMsg = genMsgArr[0];
    const logMsg = genMsgArr[1];

    if(targetLogger)
    {
        // const loggerInstance = (targetLogger) ? targetLogger : logger;
        // loggerInstance.error(logMsg);
        targetLogger.error(logMsg);
    }

    return errMsg;
};

export function toResultJson (result: any, Msg: string) {
    const resultObj = {"result": (isNaN(result) ? ERROR_TYPE.UNKNOWN_SERVER_ERROR  : result) , "errMsg":"" };

    if(Msg != undefined && typeof Msg == "string")
        resultObj.errMsg = Msg;

    return resultObj;
}

export function sendResult (res: any, result: any, Msg?: string) {
    const resultObj = {"result": (isNaN(result) ? 10 : result), "errMsg":"" };

    if(Msg != undefined && typeof Msg == "string")
        resultObj.errMsg = Msg;

    send(res, resultObj);
}

export function isString (val: string): boolean{
    return ("string" == typeof val);
}

export function isNumber (val: number): boolean {
    return ("number" == typeof val);
}

export function isArray (arr: any , type?: any ) {
    if ( 1 == arguments.length ) {
        return Array.isArray(arr);
    } else if ( 2 == arguments.length ) {
        if ( "string" == (typeof type) ) {
            for ( const i in arr ) {
                if ( "number" === type ) {
                    if ( isNaN(arr[i]) ) {
                        return false;
                    }
                } else if ( type != typeof arr[i] ) {
                    return false;
                }
            }
            return true;
        } else {
            for ( const i in arr ) {
                if ( !(arr[i] instanceof type) ) {
                    return false;
                }
            }
            return true;
        }
    } else {
        return false;
    }
}

export function isObject (val: any) {
    return ( "object" == typeof(val) && 0 < Object.keys(val).length);
}

export function GetDateToKo(callback: any) {
    try {
        const dt = new Date();
        const date = new Date().toISOString();
        const time = dt.getHours() + ":" + dt.getMinutes() + ":" + dt.getSeconds();
        const dateTime = date.substring(0, 10) + " " + time;

        callback(dateTime);
    } catch (ex) {
        console.log("ex error : " + ex);
    }
}

let logFolder = "";

export function SetLogFolder(folder: string)
{
    logFolder = folder;
}


//
// ���� �α� ���
//
export function SetLogToFile(path: string, str: string , TypeSelect?: any) {
    try {
        GetDateToKo(function (resultTime: any) {
            path = logFolder + path;
            if (path == undefined || path == "")
                path = "./FileLog";

            fs.exists(path, function (exists: any) {
                function WriteLog(str: any, callback: any) {
                    const fileName = path + "/Log" + resultTime.split(" ")[0] + ".txt";
                    const bufString = resultTime + " : " + String(str) + "\n\n";// ��
                    const buf = new Buffer(bufString);

                    fs.open(fileName, "a", function (err: any, fd: any) {
                        if (err) {
                            console.log(err);
                            return;
                        }
                        fs.write(fd, buf, 0, buf.length, undefined, function (err: any, written: any, buffer: any) {
                            if (err) {
                                console.log(err);
                                return;
                            }
                            // console.log(err, written, buffer);
                            fs.close(fd, function () {
                                // console.log('Done');
                            });
                        });
                    });
                }

                if ((exists != true) && (TypeSelect == 1)) {             // TypeSelect = 1 : ���� �϶��� üũ�Ͽ� ���� ����
                    fs.mkdir(path, "0777", function (err: any) {
                        if (err)
                            console.log(err);
                        WriteLog(str, function () { });
                    });
                } else if (exists == true) {
                    WriteLog(str, function () { });                     // ���� ���� ���Ѱ����� ���� ������ ����, ��� ��������.
                }
            });
        });
    } catch (ex) {
        console.log("ex error : " + ex);
    }
}