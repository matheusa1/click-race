import {
  EGameStatus,
  type TGameState,
} from "@/core/module/game/domain/game.entity";

export const getGameStatus = (gameState: TGameState | null): string => {
  if (!gameState) {
    return "Jogo não criado";
  }

  if (gameState.status === EGameStatus.FINISHED) {
    return "Finalizado";
  }

  if (gameState.status === EGameStatus.IN_PROGRESS) {
    return "Em jogo";
  }

  if (gameState.status === EGameStatus.AWAITING) {
    return "Aguardando";
  }

  return "Erro";
};
