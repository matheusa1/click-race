// server.ts
import { createServer, type IncomingMessage, type ServerResponse } from "http";
import type { Socket } from "net";
import next from "next";
import { parse } from "url";
import { WebSocket, WebSocketServer } from "ws";
import { SerialPortDataSource } from "@/core/module/game/infra/game.serial";
import { GameUseCase } from "./core/module/game/application/game.use.case";
import type { TGameState } from "./core/module/game/domain/game.entity";
import type { IGameStateProvider } from "./core/module/game/domain/game.ports";
import { GameMemory } from "./core/module/game/infra/game.memory";

const dev = process.env.NODE_ENV !== "production";
const hostname = "localhost";
const port = 3000;

const app = next({ dev, hostname, port });
const handle = app.getRequestHandler();

interface ServerMessage {
  type: "GAME_STATE_UPDATE";
  payload: TGameState;
}

app.prepare().then(() => {
  const SERIAL_PORT_PATH = process.env.SERIAL_PORT || "/dev/ttyACM0";

  let dataSource: IGameStateProvider;

  if (process.env.NODE_ENV === "test") {
    dataSource = new GameMemory();
  } else {
    dataSource = new SerialPortDataSource(SERIAL_PORT_PATH);
  }

  const gameUseCase = new GameUseCase(dataSource);

  const server = createServer(
    async (req: IncomingMessage, res: ServerResponse) => {
      try {
        if (!req.url) return;
        const parsedUrl = parse(req.url, true);
        await handle(req, res, parsedUrl);
      } catch (err) {
        console.error("Error handling request", err);
        res.statusCode = 500;
        res.end("internal server error");
      }
    },
  );

  const wss = new WebSocketServer({ noServer: true });

  server.on(
    "upgrade",
    (request: IncomingMessage, socket: Socket, head: Buffer) => {
      if (!request.url) return;
      const { pathname } = parse(request.url, true);

      if (pathname === "/api/websocket") {
        wss.handleUpgrade(request, socket, head, (ws: WebSocket) => {
          wss.emit("connection", ws, request);
        });
      } else {
        socket.destroy();
      }
    },
  );

  // Função para transmitir o estado para todos os clientes conectados
  const broadcastGameState = (gameState: TGameState) => {
    const message: ServerMessage = {
      type: "GAME_STATE_UPDATE",
      payload: gameState,
    };
    const messageString = JSON.stringify(message);

    wss.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(messageString);
      }
    });
  };

  // Conecta o serviço de jogo ao nosso broadcaster
  gameUseCase.onStateChange(broadcastGameState);

  // Lógica do WebSocket
  wss.on("connection", (ws: WebSocket) => {
    console.log("Cliente conectado!");

    // Envia o estado atual assim que o cliente se conecta
    const currentState = gameUseCase.getCurrentState();
    if (currentState) {
      const message: ServerMessage = {
        type: "GAME_STATE_UPDATE",
        payload: currentState,
      };
      ws.send(JSON.stringify(message));
    }

    ws.on("message", (rawMessage: string) => {
      try {
        const message: string = String(rawMessage);

        console.log({ message });

        if (message === "START_GAME") {
          console.log("Recebido comando para iniciar o jogo.");
          gameUseCase.startGame();
        }

        if (message === "RESTART_GAME") {
          console.log("Recebido comando para reiniciar o jogo.");
          gameUseCase.restartGame();
        }
      } catch (error) {
        console.error("Erro ao processar mensagem do cliente:", error);
      }
    });

    ws.on("close", () => console.log("Cliente desconectado."));
    ws.on("error", (error: Error) =>
      console.error("Erro no WebSocket:", error),
    );
  });

  // Inicie o servidor
  server.listen(port, () => {
    console.log(`> Ready on http://${hostname}:${port}`);
  });
});
