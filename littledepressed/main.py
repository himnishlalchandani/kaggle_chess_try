
import os
import atexit
import subprocess


class ChessEngine:
    def __init__(self, engine_path):
        # Start the engine process
        self.engine = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        self.ponder_move = None
        self._initialize_engine()

    def _initialize_engine(self):
        # Initialize the engine with UCI protocol
        self._send_command("uci")
        while True:
            output = self._read_output()
            if output == "uciok":
                break

        # Set engine options to minimize memory usage
        self._send_command("setoption name Threads value 1")
        
        self._send_command("setoption name Hash value 1")

        self._send_command("setoption name Ponder value true")

        # Waiting for the engine to finish initializing
        self._send_command("isready")
        while True:
            output = self._read_output()
            if output == "readyok":
                # Let the engine know if starting a new game
                self._send_command("ucinewgame")
                break

    def _send_command(self, command):
        """Send a command to the engine."""
        self.engine.stdin.write(command + "\n")
        self.engine.stdin.flush()

    def _read_output(self):
        """Read a single line of output from the engine."""
        output = self.engine.stdout.readline().strip()
        return output

    def get_best_move(self, fen, wtime, btime, last_move):
        
        if self.ponder_move != last_move:
            
            self._send_command("stop")
            
            """Get the best move for a given position."""
            # Set the position
            self._send_command(f"position fen {fen}")

            # Waiting for the engine to finish initializing
            self._send_command("isready")
            while True:
                output = self._read_output()
                if output == "readyok":
                    break

            # Start the search
            self._send_command(f"go wtime {wtime} btime {btime}")
            
        else:
            self._send_command("ponderhit")

        # Wait for the best move
        best_move = None
        self.ponder_move = None
        while True:
            output = self._read_output()
            if output.startswith("bestmove"):
                best_move = output.split()[1]
                if len(output.split()) > 2:
                    self.ponder_move = output.split()[3]
                break

        if self.ponder_move is not None:
            self._send_command(f"position fen {fen} moves {best_move} {self.ponder_move}")
            self._send_command(f"go ponder wtime {wtime} btime {btime}")
        
        return best_move

        

    def stop(self):
        """Stop the engine process."""
        self._send_command("stop")
        self._send_command("quit")
        self.engine.terminate()
        self.engine.wait()


# Define a global variable to store the ChessEngine instance
ultima = None

def chess_bot(obs):
    
    global ultima  # Declare ultima as global to modify it
    
    fen = obs['board']
    
    if obs['mark'] == 'white':
        wtime = obs['remainingOverageTime'] * 1000
        btime = obs['opponentRemainingOverageTime'] * 1000
    else:
        wtime = obs['opponentRemainingOverageTime'] * 1000
        btime = obs['remainingOverageTime'] * 1000
        
    last_move = obs['lastMove']



     

    if os.path.exists("/kaggle_simulations"):
        engine_path = "/kaggle_simulations/agent/lazer"
    else:
        engine_path = "/kaggle/working/lazer"
    if ultima is None:
        ultima = ChessEngine(engine_path)
        atexit.register(ultima.stop)  # Register to stop engine when the process ends

    # Get the best move from the engine
    best_move = ultima.get_best_move(fen, wtime, btime, last_move)

    return best_move
