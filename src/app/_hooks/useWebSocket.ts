import { useEffect, useRef, useState } from "react";
import {
  EGameStatus,
  type TGameState,
} from "@/core/module/game/domain/game.entity";

// Reutilizamos a mesma interface do backend para garantir a consistência
interface ServerMessage {
  type: "GAME_STATE_UPDATE";
  payload: TGameState;
}

interface WebSocketHook {
  gameState: TGameState | null;
  isConnected: boolean;
  sendMessage: (message: string) => void;
}

const useWebSocket = (url: string): WebSocketHook => {
  const [gameState, setGameState] = useState<TGameState | null>(null);
  const [isConnected, setIsConnected] = useState<boolean>(false);
  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    if (typeof window === "undefined") return;

    ws.current = new WebSocket(url);
    ws.current.onopen = () => setIsConnected(true);
    ws.current.onclose = () => setIsConnected(false);
    ws.current.onerror = (error) => console.error("WebSocket Error:", error);

    ws.current.onmessage = (event: MessageEvent) => {
      const message: ServerMessage = JSON.parse(event.data);
      if (message.type === "GAME_STATE_UPDATE") {
        setGameState(message.payload);
      }
    };

    return () => ws.current?.close();
  }, [url]);

  const sendMessage = (message: string) => {
    if (ws.current && ws.current.readyState === WebSocket.OPEN) {
      ws.current.send(message);
    }
  };

  return { gameState, isConnected, sendMessage };
};

export default useWebSocket;
