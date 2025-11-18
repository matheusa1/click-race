"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.GameUseCase = void 0;
class GameUseCase {
    constructor(dataSource) {
        this.currentGameState = null;
        this.onStateChangeCallback = () => { };
        this.dataSource = dataSource;
        this.dataSource.start(this.handleDataSourceUpdate.bind(this));
    }
    handleDataSourceUpdate(newState) {
        this.currentGameState = newState;
        this.onStateChangeCallback(this.currentGameState);
    }
    /**
     * Registra um callback para ser notificado sobre mudanças de estado.
     */
    onStateChange(callback) {
        this.onStateChangeCallback = callback;
    }
    /**
     * Inicia o jogo enviando o comando para a fonte de dados.
     */
    startGame() {
        this.dataSource.sendCommand("2");
    }
    /**
     * Retorna o último estado conhecido do jogo.
     */
    getCurrentState() {
        return this.currentGameState;
    }
    /**
     * Reinicia o jogo enviando o comando para a fonte de dados
     */
    restartGame() {
        this.dataSource.sendCommand("1");
    }
    /**
     * Encerra o jogo enviando o comando para a fonte de dados
     */
    endGame() {
        this.dataSource.sendCommand("3");
    }
}
exports.GameUseCase = GameUseCase;
