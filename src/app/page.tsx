"use client";

import Image from "next/image";
import { useCallback, useEffect, useRef, useState } from "react";
import { EGameStatus } from "@/core/module/game/domain/game.entity";
import useWebSocket from "./_hooks/useWebSocket";
import { getGameStatus } from "./_utils/getGameStatus";

const currentIp = "192.168.18.90";

export default function Home() {
  const { gameState, isConnected, sendMessage } = useWebSocket(
    `ws://${currentIp}:3000/api/websocket`,
  );
  const maxTime = 10;
  const [gameTime, setGameTime] = useState<number>(0);

  useEffect(() => {
    if (gameState?.status === EGameStatus.IN_PROGRESS) {
      const interval = setInterval(() => {
        setGameTime((prevTime) => prevTime + 1);
      }, 1000);

      return () => {
        setGameTime(0);
        clearInterval(interval);
      };
    }
  }, [gameState?.status]);

  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    const resizeCanvas = () => {
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;
    };

    resizeCanvas();
    window.addEventListener("resize", resizeCanvas);

    const stars: Array<{
      x: number;
      y: number;
      size: number;
      brightness: number;
      speed: number;
      pulseDirection: number;
    }> = [];

    for (let i = 0; i < 200; i++) {
      stars.push({
        x: Math.random() * canvas.width,
        y: Math.random() * canvas.height,
        size: Math.random() * 1.5 + 0.5,
        brightness: Math.random() * 0.5 + 0.5,
        speed: Math.random() * 0.05 + 0.02,
        pulseDirection: Math.random() > 0.5 ? 1 : -1,
      });
    }

    const animate = () => {
      ctx.fillStyle = "#0f172a";
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      stars.forEach((star) => {
        star.brightness += star.speed * star.pulseDirection;
        if (star.brightness > 1 || star.brightness < 0.3) {
          star.pulseDirection *= -1;
          star.brightness = Math.max(0.3, Math.min(1, star.brightness));
        }

        ctx.beginPath();
        ctx.arc(star.x, star.y, star.size, 0, Math.PI * 2);
        ctx.fillStyle = `rgba(255, 255, 255, ${star.brightness})`;
        ctx.fill();

        if (star.size > 1) {
          ctx.beginPath();
          ctx.arc(star.x, star.y, star.size * 2, 0, Math.PI * 2);
          const gradient = ctx.createRadialGradient(
            star.x,
            star.y,
            0,
            star.x,
            star.y,
            star.size * 2,
          );
          gradient.addColorStop(
            0,
            `rgba(255, 255, 255, ${star.brightness * 0.3})`,
          );
          gradient.addColorStop(1, "rgba(255, 255, 255, 0)");
          ctx.fillStyle = gradient;
          ctx.fill();
        }
      });

      if (Math.random() < 0.002) {
        const shootingStar = {
          x: Math.random() * canvas.width,
          y: 0,
          length: Math.random() * 50 + 30,
          speed: Math.random() * 5 + 3,
          angle: Math.random() * 0.5 + 0.2,
        };

        const drawShootingStar = () => {
          ctx.beginPath();
          ctx.moveTo(shootingStar.x, shootingStar.y);
          ctx.lineTo(
            shootingStar.x - shootingStar.length * Math.cos(shootingStar.angle),
            shootingStar.y + shootingStar.length * Math.sin(shootingStar.angle),
          );
          ctx.strokeStyle = `rgba(255, 255, 255, 0.8)`;
          ctx.lineWidth = 2;
          ctx.stroke();

          const gradient = ctx.createLinearGradient(
            shootingStar.x,
            shootingStar.y,
            shootingStar.x - shootingStar.length * Math.cos(shootingStar.angle),
            shootingStar.y + shootingStar.length * Math.sin(shootingStar.angle),
          );
          gradient.addColorStop(0, "rgba(255, 255, 255, 0.8)");
          gradient.addColorStop(1, "rgba(255, 255, 255, 0)");
          ctx.strokeStyle = gradient;
          ctx.lineWidth = 3;
          ctx.stroke();

          shootingStar.x -= shootingStar.speed * Math.cos(shootingStar.angle);
          shootingStar.y += shootingStar.speed * Math.sin(shootingStar.angle);

          if (shootingStar.y < canvas.height && shootingStar.x > 0) {
            requestAnimationFrame(drawShootingStar);
          }
        };

        drawShootingStar();
      }

      requestAnimationFrame(animate);
    };

    animate();

    return () => {
      window.removeEventListener("resize", resizeCanvas);
    };
  }, []);

  const handleStartGame = () => {
    sendMessage("START_GAME");
  };

  const handleRestartGame = () => {
    sendMessage("RESTART_GAME");
  };

  const handleFinishGame = useCallback(() => {
    sendMessage("FINISH_GAME");
  }, [sendMessage]);

  const gameStatus = getGameStatus(gameState);

  useEffect(() => {
    if (gameTime >= maxTime) {
      handleFinishGame();
    }
  }, [gameTime, handleFinishGame]);

  const showStartButton =
    gameState?.status === EGameStatus.AWAITING ||
    gameState.status === EGameStatus.LOADING ||
    gameState.status === EGameStatus.ERROR;

  return (
    <main className="p-8 font-sans max-w-6xl mx-auto min-h-screen relative">
      {/* Canvas do céu estrelado */}
      <canvas
        ref={canvasRef}
        className="fixed top-0 left-0 w-full h-full -z-10"
      />
      <p>{gameTime}</p>

      {/* Conteúdo principal */}
      <div className="relative z-10">
        {/* Card Principal */}
        <div className="bg-slate-900/80 backdrop-blur-sm rounded-xl p-8 shadow-2xl border border-white/10 mb-8">
          {/* Logo Click Race */}
          <div className="flex justify-center items-center mb-8">
            <div className="relative group">
              <Image
                src="/images/click-race.png"
                alt="Click Race"
                width={350}
                height={100}
                className="object-contain drop-shadow-lg filter brightness-110 contrast-110 group-hover:scale-105 group-hover:drop-shadow-2xl transition-all duration-300"
                priority
              />
              {/* Efeito de brilho sutil */}
              <div className="absolute inset-0 bg-gradient-to-r from-transparent via-white/5 to-transparent opacity-0 group-hover:opacity-100 transition-opacity duration-500 rounded-lg" />
            </div>
          </div>

          {/* Status da Conexão */}
          <div className="flex justify-center items-center gap-4 mb-6">
            <div
              className={`w-3 h-3 rounded-full ${isConnected ? "bg-green-500 animate-pulse shadow-green-500/50" : "bg-red-500 shadow-red-500/50"} shadow-lg`}
            />
            <p className="text-slate-200 text-lg m-0">
              Status da Conexão:{" "}
              <span
                className={`font-semibold ${isConnected ? "text-green-400" : "text-red-400"}`}
              >
                {isConnected ? "Conectado" : "Desconectado"}
              </span>
            </p>
          </div>

          <hr className="border-none h-px bg-white/20 my-6" />

          {!gameState ? (
            <div className="text-center py-12 text-slate-300">
              <div className="w-12 h-12 border-4 border-white/20 border-t-blue-500 rounded-full animate-spin mx-auto mb-4" />
              <p className="text-xl m-0">Aguardando dados do servidor...</p>
            </div>
          ) : (
            <div>
              {/* Header com Status e Botões */}
              <div className="flex justify-between items-center mb-8 flex-wrap gap-4">
                <h2 className="text-slate-200 text-2xl m-0">
                  Status da Partida:{" "}
                  <span className="font-bold bg-gradient-to-r from-blue-400 to-purple-400 bg-clip-text text-transparent">
                    {gameStatus}
                  </span>
                </h2>

                {showStartButton && (
                  <button
                    disabled={gameState.status === EGameStatus.LOADING}
                    onClick={handleStartGame}
                    data-disabled={gameState.status === EGameStatus.LOADING}
                    className="data-[disabled=true]:opacity-50 data-[disabled=true]:cursor-not-allowed px-8 py-3 text-lg font-semibold cursor-pointer border-none rounded-full bg-gradient-to-r from-purple-600 to-blue-600 text-white shadow-lg shadow-purple-500/40 hover:shadow-purple-500/60 hover:-translate-y-0.5 transition-all duration-300 relative overflow-hidden group"
                  >
                    <span className="relative z-10">🎮 Iniciar Jogo</span>
                    <div className="absolute top-0 -left-full w-full h-full bg-gradient-to-r from-transparent via-white/20 to-transparent group-hover:left-full transition-all duration-500" />
                  </button>
                )}

                {gameState.status === EGameStatus.FINISHED && (
                  <button
                    onClick={handleRestartGame}
                    className="px-8 py-3 text-lg font-semibold cursor-pointer border-none rounded-full bg-gradient-to-r from-pink-500 to-red-500 text-white shadow-lg shadow-pink-500/40 hover:shadow-pink-500/60 hover:-translate-y-0.5 transition-all duration-300"
                  >
                    🔄 Reiniciar Jogo
                  </button>
                )}
              </div>

              {/* Placar dos Jogadores */}
              {gameState.players.length > 0 && (
                <div className="mb-8">
                  <h3 className="text-slate-200 text-xl mb-4 text-center">
                    👥 Jogadores Conectados: {gameState.players.length}
                  </h3>
                  <div className="flex justify-center gap-8 flex-wrap">
                    {gameState.players.map((player, index) => (
                      <div
                        key={player.id}
                        className="flex flex-col items-center p-3 bg-slate-800/60 backdrop-blur-sm rounded-lg border border-white/10 min-w-[120px] hover:bg-slate-700/60 transition-colors"
                      >
                        <div className="text-slate-300 text-sm mb-1">
                          Player {index + 1}
                        </div>
                        <div className="text-blue-300 font-semibold text-base mb-1">
                          {player.id}
                        </div>
                        <div className="text-white font-bold text-lg">
                          {player.clicks}
                        </div>
                        <div className="text-slate-400 text-xs">cliques</div>
                      </div>
                    ))}
                  </div>
                </div>
              )}

              {/* Anúncio do Vencedor */}
              {gameState.status === EGameStatus.FINISHED &&
                gameState.winner && (
                  <div className="border-2 border-green-500 p-6 mt-4 rounded-xl bg-gradient-to-r from-green-500/10 to-emerald-500/20 text-center backdrop-blur-sm">
                    <h3 className="text-green-400 text-2xl mb-4 m-0">
                      🏆 Partida Finalizada!
                    </h3>
                    <p className="text-green-400 text-xl m-0 font-bold">
                      Vencedor: {gameState.winner} com{" "}
                      {gameState.winner === gameState.players[0].id
                        ? gameState.players[0].clicks
                        : gameState.players[1].clicks}{" "}
                      cliques!
                    </p>
                  </div>
                )}
            </div>
          )}
        </div>

        {/* Área de Visualização da Corrida */}
        {gameState && gameState.players.length > 0 && (
          <div className="bg-slate-900/80 backdrop-blur-sm rounded-xl p-8 shadow-2xl border border-white/10">
            <h3 className="text-slate-200 text-xl mb-6 text-center">
              🏁 Progresso da Corrida
            </h3>
            <div className="flex gap-8 justify-center items-end min-h-[200px] p-4 bg-gradient-to-br from-slate-900/60 to-slate-800/80 rounded-lg border-2 border-white/10 relative overflow-hidden">
              {/* Linha de Chegada */}
              <div className="absolute top-0 right-12 bottom-0 w-1 bg-gradient-to-b from-transparent via-green-500 to-transparent animate-pulse" />

              {gameState.players.map((player) => (
                <div
                  key={player.id}
                  className="flex flex-col items-center gap-2"
                >
                  <div
                    className="w-16 h-16 rounded-xl bg-gradient-to-r from-purple-600 to-blue-600 flex items-center justify-center text-white font-semibold text-sm text-center shadow-lg shadow-purple-500/40 border-2 border-white/20 transition-all duration-500"
                    style={{
                      transform: `translateY(-${Math.min(player.clicks * 3, 100)}px)`,
                    }}
                  >
                    {player.id}
                  </div>
                  <div className="text-slate-200 font-semibold text-sm bg-slate-900/80 px-2 py-1 rounded">
                    {player.clicks} clicks
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}
      </div>
    </main>
  );
}
