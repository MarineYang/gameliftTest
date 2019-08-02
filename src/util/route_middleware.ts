import { Request, Response, NextFunction, RequestHandler } from "express";

export function DO_ASYNC(funcObj: RequestHandler) {
    return async function (req: Request, res: Response, next: NextFunction) {
        try {
            await funcObj(req, res, next);
        } catch (err) {
            next(err);
        }
    };
}