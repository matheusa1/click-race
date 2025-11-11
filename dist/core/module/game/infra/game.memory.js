"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GameMemory = void 0;
const game_entity_1 = require("../domain/game.entity");
class GameMemory {
    constructor() {
        this.onDataCallback = () => { };
        this.gameInterval = null;
        this.gameTicks = 0;
        this.gameState = {
            status: game_entity_1.EGameStatus.AWAITING,
            players: [
                { id: 1, clicks: 0 },
                { id: 2, clicks: 0 },
            ],
        };
    }
    start(onData) {
        this.onDataCallback = onData;
        this.onDataCallback(this.gameState);
    }
    sendCommand(command) {
        if (command === "START_GAME" &&
            this.gameState.status === game_entity_1.EGameStatus.AWAITING) {
            this.startGame();
        }
        console.log({ command, status: this.gameState.status });
        if (command === "RESTART_GAME" &&
            this.gameState.status === game_entity_1.EGameStatus.FINISHED) {
            this.restartGame();
        }
    }
    restartGame() {
        console.log("Restarting game...");
        this.gameState.status = game_entity_1.EGameStatus.AWAITING;
        this.gameState.players.forEach((player) => {
            player.clicks = 0;
        });
        this.onDataCallback(this.gameState);
        this.startGame();
    }
    startGame() {
        this.gameState.status = game_entity_1.EGameStatus.IN_PROGRESS;
        this.gameTicks = 0;
        this.onDataCallback(this.gameState);
        this.gameInterval = setInterval(() => {
            this.gameTicks++;
            this.gameState.players.forEach((player) => {
                if (Math.random() > 0.5) {
                    player.clicks += 1;
                }
            });
            this.onDataCallback(Object.assign({}, this.gameState));
            if (this.gameTicks >= 20) {
                this.endGame();
            }
        }, 200);
    }
    endGame() {
        if (this.gameInterval) {
            clearInterval(this.gameInterval);
            this.gameInterval = null;
        }
        this.gameState.status = game_entity_1.EGameStatus.FINISHED;
        const winner = [...this.gameState.players].sort((a, b) => b.clicks - a.clicks)[0];
        this.gameState.winner = winner;
        this.onDataCallback(this.gameState);
    }
}
exports.GameMemory = GameMemory;
