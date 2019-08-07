import express from "express";
import compression from "compression";  // compresses requests
import bodyParser from "body-parser";
import * as packet from "./util/packet";


const app = express();
console.log("RunTest app \n");

app.set("port", 9080);

app.use(compression()); //압축
app.use(bodyParser.json());

app.use(bodyParser.urlencoded({ extended: true }));
app.use("/game/gamelift", require("./routes/gamelift"));

app.use( (req, res, next) => {
    const err = new Error("Not Found");
    res.status(404);
    packet.send(res,
        {result: -1, msg: "NOT FOUND URL"}
    );
});

export default app;
