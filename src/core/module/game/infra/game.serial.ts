import { ReadlineParser, SerialPort } from "serialport";
import type { TGameState } from "../domain/game.entity";
import type { IGameStateProvider } from "../domain/game.ports";

export class SerialPortDataSource implements IGameStateProvider {
  private port: SerialPort;
  private parser: ReadlineParser;
  private onDataCallback: (data: TGameState) => void = () => {};

  /**
   * Construtor do nosso adaptador de porta serial.
   * @param portPath - O caminho para a porta serial (ex: 'COM3' no Windows, '/dev/ttyACM0' no Linux).
   */
  constructor(portPath: string) {
    this.port = new SerialPort({
      path: portPath,
      baudRate: 9600,
      autoOpen: false,
    });

    this.parser = this.port.pipe(new ReadlineParser({ delimiter: "\n" }));
  }

  public start(onData: (data: TGameState) => void): void {
    this.onDataCallback = onData;

    this.port.open((err) => {
      if (err) {
        console.error(
          `Erro ao abrir a porta serial ${this.port.path}:`,
          err.message,
        );
        console.error(
          "Verifique se a placa está conectada e o caminho da porta está correto.",
        );
        return;
      }
      console.log(`Porta serial ${this.port.path} aberta com sucesso.`);
    });

    this.parser.on("data", (line: string) => {
      try {
        const gameState: TGameState = JSON.parse(line);
        this.onDataCallback(gameState);
      } catch {
        console.error(
          "Erro ao processar dados da porta serial. Dado recebido:",
          line,
        );
      }
    });

    this.port.on("error", (error) => {
      console.error("Erro na porta serial:", error);
    });
  }

  public sendCommand(command: string): void {
    const commandToSend = `${command}\n`;
    this.port.write(commandToSend, (err) => {
      if (err) {
        return console.error("Erro ao escrever na porta serial:", err.message);
      }
      console.log(`Comando "${command}" enviado para a placa.`);
    });
  }
}
