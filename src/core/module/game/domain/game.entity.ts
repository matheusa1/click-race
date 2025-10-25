export enum EGameStatus {
  AWAITING = 1,
  IN_PROGRESS = 2,
  FINISHED = 3,
}

export type TPlayer = {
  id: number;
  clicks: number;
};

export type TGameState = {
  status: EGameStatus;
  players: TPlayer[];
  winner?: TPlayer;
};
