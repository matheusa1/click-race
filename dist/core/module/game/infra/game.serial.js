"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.SerialPortDataSource = void 0;
const serialport_1 = require("serialport");
class SerialPortDataSource {
    /**
     * Construtor do nosso adaptador de porta serial.
     * @param portPath - O caminho para a porta serial (ex: 'COM3' no Windows, '/dev/ttyACM0' no Linux).
     */
    constructor(portPath) {
        this.onDataCallback = () => { };
        this.port = new serialport_1.SerialPort({
            path: portPath,
            baudRate: 115200,
            autoOpen: false,
        });
        this.parser = this.port.pipe(new serialport_1.ReadlineParser({ delimiter: "\n" }));
    }
    start(onData) {
        this.onDataCallback = onData;
        this.port.open((err) => {
            if (err) {
                console.error(`Erro ao abrir a porta serial ${this.port.path}:`, err.message);
                console.error("Verifique se a placa está conectada e o caminho da porta está correto.");
                return;
            }
            console.log(`Porta serial ${this.port.path} aberta com sucesso.`);
        });
        this.parser.on("data", (line) => {
            try {
                const gameState = JSON.parse(line);
                this.onDataCallback(gameState);
            }
            catch (_a) { }
            console.log(line);
        });
        this.port.on("error", (error) => {
            console.error("Erro na porta serial:", error);
        });
    }
    sendCommand(command) {
        const commandToSend = `${command}\n`;
        this.port.write(commandToSend, (err) => {
            if (err) {
                return console.error("Erro ao escrever na porta serial:", err.message);
            }
            console.log(`Comando "${command}" enviado para a placa.`);
        });
    }
}
exports.SerialPortDataSource = SerialPortDataSource;
