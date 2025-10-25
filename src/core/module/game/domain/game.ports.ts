import type { TGameState } from "./game.entity";

export interface IGameStateProvider {
  /**
   * Inicia a fonte de dados.
   * @param onData - Callback que será chamado toda vez que houver uma atualização de estado.
   */
  start(onData: (data: TGameState) => void): void;
  /**
   * Envia um comando para a fonte de dados.
   * @param command - O comando a ser enviado (ex: "START_GAME").
   */
  sendCommand(command: string): void;
}
