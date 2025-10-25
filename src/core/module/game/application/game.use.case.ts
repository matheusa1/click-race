import type { TGameState } from "../domain/game.entity";
import type { IGameStateProvider } from "../domain/game.ports";

export class GameUseCase {
  private dataSource: IGameStateProvider;
  private currentGameState: TGameState | null = null;
  private onStateChangeCallback: (newState: TGameState) => void = () => {};

  constructor(dataSource: IGameStateProvider) {
    this.dataSource = dataSource;

    this.dataSource.start(this.handleDataSourceUpdate.bind(this));
  }

  private handleDataSourceUpdate(newState: TGameState): void {
    this.currentGameState = newState;

    this.onStateChangeCallback(this.currentGameState);
  }

  /**
   * Registra um callback para ser notificado sobre mudanças de estado.
   */
  public onStateChange(callback: (newState: TGameState) => void): void {
    this.onStateChangeCallback = callback;
  }

  /**
   * Inicia o jogo enviando o comando para a fonte de dados.
   */
  public startGame(): void {
    this.dataSource.sendCommand("START_GAME");
  }

  /**
   * Retorna o último estado conhecido do jogo.
   */
  public getCurrentState(): TGameState | null {
    return this.currentGameState;
  }

  /**
   * Reinicia o jogo enviando o comando para a fonte de dados
   */
  public restartGame(): void {
    this.dataSource.sendCommand("RESTART_GAME");
  }
}
