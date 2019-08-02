import app from "./app";
console.log("RunTest server1 \n");

function GameLiftServerStart() {
    console.log("RunTest Start 1\n");
    const server = app.listen(app.get("port"), function () {
        console.log(
            "Http Server has started on port %d in %s mode",
            app.get("port"),
            app.get("env")
        );
    });
    console.log("Press CTRL-C to stop\n");
}
function ConnectLog() {
    console.log("Connect Complete");
    setTimeout(ConnectLog, 1500);
}
console.log("RunTest Start 2\n");
setTimeout(ConnectLog, 1500);

GameLiftServerStart();

