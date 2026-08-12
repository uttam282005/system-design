#include <iostream>
#include <memory>
#include <vector>

/* non-functional requirements-
 * player and game control should be centralized
 */

enum Symbol { Empty, Ring, Cross };

class Player {
protected:
  Symbol symbol;

public:
  Player(Symbol s) : symbol(s) {};
  virtual Symbol getSymbol() const = 0;
  virtual ~Player() = default;
};

class Ring : public Player {
public:
  Ring() : Player(Symbol::Ring) {};
  Symbol getSymbol() const override { return Symbol::Ring; }
};

class Cross : public Player {
public:
  Cross() : Player(Symbol::Cross) {};
  Symbol getSymbol() const override { return Symbol::Cross; }
};

class TicTacToe {
  int rows, cols;
  int current_player_index, filled;
  Player *winner;
  std::vector<std::vector<Symbol>> board;
  std::vector<std::unique_ptr<Player>> players;

  bool checkLine(int row, int col, int rowDelta, int colDelta, Symbol symbol) {
    for (int i = 0; i < rows; i++) {
      if (board[row][col] != symbol)
        return false;

      row += rowDelta;
      col += colDelta;
    }

    return true;
  }

  void checkForWinner(int row, int col) {
    Symbol symbol = getCurrentPlayer()->getSymbol();

    if (checkLine(row, 0, 0, 1, symbol) || checkLine(0, col, 1, 0, symbol) ||
        (row == col && checkLine(0, 0, 1, 1, symbol)) ||
        (row + col == cols - 1 && checkLine(0, cols - 1, 1, -1, symbol))) {

      winner = getCurrentPlayer();
    }
  }

  void nextPlayer() {
    current_player_index = (current_player_index + 1) % players.size();
  }

public:
  TicTacToe(int n)
      : filled(0), current_player_index(0), winner(nullptr), rows(n), cols(n),
        board(rows, std::vector<Symbol>(cols, Symbol::Empty)) {};

  void registerPlayer(std::unique_ptr<Player> player) {
    players.push_back(std::move(player));
  }

  Player *getCurrentPlayer() { return players[current_player_index].get(); }

  void makeMove(int x, int y) {
    if (x >= rows or x < 0 or y >= cols or y < 0 or
        board[x][y] != Symbol::Empty)
      return;

    board[x][y] = getCurrentPlayer()->getSymbol();
    filled++;

    checkForWinner(x, y);

    if (winner == nullptr)
      nextPlayer();
  }

  bool isGameOver() { return (winner != nullptr || filled == rows * cols); }

  void showResult() {
    if (winner == nullptr) {
      std::cout << "TIE\n";
      return;
    }

    std::cout << winner->getSymbol() << " is the winner\n";
  }
};

int main() {
  TicTacToe t(3);

  t.registerPlayer(std::make_unique<class Ring>());
  t.registerPlayer(std::make_unique<class Cross>());

  while (!t.isGameOver()) {
    int r, c;
    std::cout << t.getCurrentPlayer()->getSymbol() << ": your move\n";
    std::cin >> r >> c;
    t.makeMove(r, c);
  }

  t.showResult();
}
