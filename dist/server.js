"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
// server.ts
const http_1 = require("http");
const next_1 = __importDefault(require("next"));
const url_1 = require("url");
const ws_1 = require("ws");
const game_serial_1 = require("@/core/module/game/infra/game.serial");
const game_use_case_1 = require("./core/module/game/application/game.use.case");
const game_memory_1 = require("./core/module/game/infra/game.memory");
const dev = process.env.NODE_ENV !== "production";
const hostname = "0.0.0.0";
const port = 3000;
const app = (0, next_1.default)({ dev, hostname, port });
const handle = app.getRequestHandler();
app.prepare().then(() => {
    const SERIAL_PORT_PATH = process.env.SERIAL_PORT || "/dev/ttyACM0";
    let dataSource;
    if (process.env.NODE_ENV === "test") {
        dataSource = new game_memory_1.GameMemory();
    }
    else {
        dataSource = new game_serial_1.SerialPortDataSource(SERIAL_PORT_PATH);
    }
    const gameUseCase = new game_use_case_1.GameUseCase(dataSource);
    const server = (0, http_1.createServer)(async (req, res) => {
        try {
            if (!req.url)
                return;
            const parsedUrl = (0, url_1.parse)(req.url, true);
            await handle(req, res, parsedUrl);
        }
        catch (err) {
            console.error("Error handling request", err);
            res.statusCode = 500;
            res.end("internal server error");
        }
    });
    const wss = new ws_1.WebSocketServer({ noServer: true });
    server.on("upgrade", (request, socket, head) => {
        if (!request.url)
            return;
        const { pathname } = (0, url_1.parse)(request.url, true);
        if (pathname === "/api/websocket") {
            wss.handleUpgrade(request, socket, head, (ws) => {
                wss.emit("connection", ws, request);
            });
        }
        else {
            socket.destroy();
        }
    });
    // Função para transmitir o estado para todos os clientes conectados
    const broadcastGameState = (gameState) => {
        const message = {
            type: "GAME_STATE_UPDATE",
            payload: gameState,
        };
        const messageString = JSON.stringify(message);
        wss.clients.forEach((client) => {
            if (client.readyState === ws_1.WebSocket.OPEN) {
                client.send(messageString);
            }
        });
    };
    // Conecta o serviço de jogo ao nosso broadcaster
    gameUseCase.onStateChange(broadcastGameState);
    // Lógica do WebSocket
    wss.on("connection", (ws) => {
        console.log("Cliente conectado!");
        // Envia o estado atual assim que o cliente se conecta
        const currentState = gameUseCase.getCurrentState();
        if (currentState) {
            const message = {
                type: "GAME_STATE_UPDATE",
                payload: currentState,
            };
            ws.send(JSON.stringify(message));
        }
        ws.on("message", (rawMessage) => {
            try {
                const message = String(rawMessage);
                console.log({ message });
                if (message === "START_GAME") {
                    console.log("Recebido comando para iniciar o jogo.");
                    gameUseCase.startGame();
                }
                if (message === "RESTART_GAME") {
                    console.log("Recebido comando para reiniciar o jogo.");
                    gameUseCase.restartGame();
                }
                if (message === "FINISH_GAME") {
                    console.log("Recebido comando para encerrar o jogo.");
                    gameUseCase.endGame();
                }
            }
            catch (error) {
                console.error("Erro ao processar mensagem do cliente:", error);
            }
        });
        ws.on("close", () => console.log("Cliente desconectado."));
        ws.on("error", (error) => console.error("Erro no WebSocket:", error));
    });
    // Inicie o servidor
    server.listen(port, () => {
        console.log(`> Ready on http://${hostname}:${port}`);
    });
});
