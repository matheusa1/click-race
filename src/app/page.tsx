"use client";

import { EGameStatus } from "@/core/module/game/domain/game.entity";
import useWebSocket from "./_hooks/useWebSocket";
import { getGameStatus } from "./_utils/getGameStatus";

export default function Home() {
  const { gameState, isConnected, sendMessage } = useWebSocket(
    "ws://localhost:3000/api/websocket",
  );

  const handleStartGame = () => {
    sendMessage("START_GAME");
  };

  const handleRestartGame = () => {
    sendMessage("RESTART_GAME");
  };

  const gameStatus = getGameStatus(gameState);

  return (
    <main
      style={{
        padding: "2rem",
        fontFamily: "sans-serif",
        maxWidth: "800px",
        margin: "auto",
      }}
    >
      <h1>Corrida de Cliques com Microcontroladores</h1>
      <p>
        Status da Conexão:{" "}
        {isConnected ? (
          <span style={{ color: "green" }}>Conectado</span>
        ) : (
          <span style={{ color: "red" }}>Desconectado</span>
        )}
      </p>

      <hr />

      {!gameState ? (
        <p>Aguardando dados do servidor...</p>
      ) : (
        <div>
          <h2>
            Status da Partida:{" "}
            <span style={{ color: "#0070f3" }}>{gameStatus}</span>
          </h2>

          {gameState.status === EGameStatus.AWAITING && (
            <button
              onClick={handleStartGame}
              type="button"
              style={{
                padding: "10px 20px",
                fontSize: "1.2rem",
                cursor: "pointer",
              }}
            >
              Iniciar Jogo
            </button>
          )}

          {gameState.status === EGameStatus.FINISHED && (
            <button
              onClick={handleRestartGame}
              type="button"
              style={{
                padding: "10px 20px",
                fontSize: "1.2rem",
                cursor: "pointer",
              }}
            >
              Reiniciar Jogo
            </button>
          )}

          {gameState.status === EGameStatus.FINISHED && gameState.winner && (
            <div
              style={{
                border: "2px solid green",
                padding: "1rem",
                marginTop: "1rem",
              }}
            >
              <h3>Partida Finalizada!</h3>
              <p>
                <strong>
                  Vencedor: {gameState.winner.id} com {gameState.winner.clicks}{" "}
                  cliques!
                </strong>
              </p>
            </div>
          )}

          <div style={{ marginTop: "2rem" }}>
            <h3>Jogadores</h3>
            {gameState.players.map((player) => (
              <div
                key={player.id}
                style={{
                  border: "1px solid #ccc",
                  padding: "1rem",
                  marginBottom: "0.5rem",
                }}
              >
                <strong>Placa: {player.id}</strong>
                <p>Cliques: {player.clicks}</p>
              </div>
            ))}
          </div>
        </div>
      )}

      <div style={{ display: "flex", gap: 32 }}>
        {gameState?.players.map((player) => (
          <div
            className={`w-50 h-70 bg-linear-to-r from-green-500 to-pink-500`}
            style={{
              marginTop: player.clicks * 40,
            }}
            key={player.id}
          >
            {player.id}
          </div>
        ))}
      </div>
    </main>
  );
}
