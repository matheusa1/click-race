import { EGameStatus, type TGameState } from "../domain/game.entity";
import type { IGameStateProvider } from "../domain/game.ports";

export class GameMemory implements IGameStateProvider {
  private gameState: TGameState;
  private onDataCallback: (data: TGameState) => void = () => {};
  private gameInterval: NodeJS.Timeout | null = null;
  private gameTicks = 0;

  constructor() {
    this.gameState = {
      status: EGameStatus.AWAITING,
      players: [
        { id: 1, clicks: 0 },
        { id: 2, clicks: 0 },
      ],
    };
  }

  public start(onData: (data: TGameState) => void): void {
    this.onDataCallback = onData;
    this.onDataCallback(this.gameState);
  }

  public sendCommand(command: string): void {
    if (
      command === "START_GAME" &&
      this.gameState.status === EGameStatus.AWAITING
    ) {
      this.startGame();
    }

    console.log({ command, status: this.gameState.status });

    if (
      command === "RESTART_GAME" &&
      this.gameState.status === EGameStatus.FINISHED
    ) {
      this.restartGame();
    }
  }

  private restartGame(): void {
    console.log("Restarting game...");
    this.gameState.status = EGameStatus.AWAITING;
    this.gameState.players.forEach((player) => {
      player.clicks = 0;
    });
    this.onDataCallback(this.gameState);

    this.startGame();
  }

  private startGame(): void {
    this.gameState.status = EGameStatus.IN_PROGRESS;
    this.gameTicks = 0;
    this.onDataCallback(this.gameState);

    this.gameInterval = setInterval(() => {
      this.gameTicks++;

      this.gameState.players.forEach((player) => {
        if (Math.random() > 0.5) {
          player.clicks += 1;
        }
      });

      this.onDataCallback({ ...this.gameState });

      if (this.gameTicks >= 20) {
        this.endGame();
      }
    }, 200);
  }

  private endGame(): void {
    if (this.gameInterval) {
      clearInterval(this.gameInterval);
      this.gameInterval = null;
    }

    this.gameState.status = EGameStatus.FINISHED;

    const winner = [...this.gameState.players].sort(
      (a, b) => b.clicks - a.clicks,
    )[0];
    this.gameState.winner = winner;

    this.onDataCallback(this.gameState);
  }
}
