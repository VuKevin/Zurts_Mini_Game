// zurts.cpp

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cassert>
using namespace std;

///////////////////////////////////////////////////////////////////////////
// Manifest constants
///////////////////////////////////////////////////////////////////////////

const int MAXROWS = 20;            // max number of rows in the arena
const int MAXCOLS = 20;            // max number of columns in the arena
const int MAXZURTS = 100;          // max number of zurts allowed
const int MAXCOLORS = 3;           // max number of colors
const int ZURT_HEALTH = 3;         // initial zurt health
const double WALL_DENSITY = 0.13;  // density of walls

const int NORTH = 0;
const int EAST = 1;
const int SOUTH = 2;
const int WEST = 3;
const int NUMDIRS = 4;

const int EMPTY = 0;
const int WALL = 1;

///////////////////////////////////////////////////////////////////////////
// Type definitions
///////////////////////////////////////////////////////////////////////////

class Arena;  // This is needed to let the compiler know that Arena is a
			  // type name, since it's mentioned in the Zurt declaration.

class Zurt
{
public:
	// Constructor
	Zurt(Arena* ap, int r, int c, char color);

	// Accessors
	int  row() const;
	int  col() const;
	char color() const;
	bool isDead() const;

	// Mutators
	void forceMove(int dir);
	void move();

private:
	Arena* m_arena;
	int    m_row;
	int    m_col;
	char   m_color;
	int    m_health;
};

class Player
{
public:
	// Constructor
	Player(Arena *ap, int r, int c);

	// Accessors
	int  row() const;
	int  col() const;
	bool isDead() const;

	// Mutators
	string stand();
	string move(int dir);
	void   setDead();

private:
	Arena* m_arena;
	int    m_row;
	int    m_col;
	bool   m_dead;
};

class Arena
{
public:
	// Constructor/destructor
	Arena(int nRows, int nCols);
	~Arena();

	// Accessors
	int     rows() const;
	int     cols() const;
	Player* player() const;
	int     zurtCount() const;
	int     getCellStatus(int r, int c) const;
	int     numberOfZurtsAt(int r, int c) const;
	void    display(string msg) const;

	// Mutators
	void   setCellStatus(int r, int c, int status);
	bool   addZurt(int r, int c, char color);
	bool   addPlayer(int r, int c);
	string moveZurts(char color, int dir);

private:
	int     m_grid[MAXROWS][MAXCOLS];
	int     m_rows;
	int     m_cols;
	Player* m_player;
	Zurt*   m_zurts[MAXZURTS];
	int     m_nZurts;

	// Helper functions
	void checkPos(int r, int c) const;
	bool isPosInBounds(int r, int c) const;
};

class Game
{
public:
	// Constructor/destructor
	Game(int rows, int cols, int nZurts);
	~Game();

	// Mutators
	void play();

private:
	Arena* m_arena;

	// Helper functions
	string takePlayerTurn();
	string takeZurtsTurn();
};

///////////////////////////////////////////////////////////////////////////
//  Auxiliary function declarations
///////////////////////////////////////////////////////////////////////////

int randInt(int lowest, int highest);
bool charToDir(char ch, int& dir);
bool attemptMove(const Arena& a, int dir, int& r, int& c);
bool recommendMove(const Arena& a, int r, int c, int& bestDir);
int computeDanger(const Arena& a, int r, int c);
void clearScreen();

///////////////////////////////////////////////////////////////////////////
//  Zurt implementation
///////////////////////////////////////////////////////////////////////////

Zurt::Zurt(Arena* ap, int r, int c, char color)
{
	if (ap == nullptr)
	{
		cout << "***** A zurt must be created in some Arena!" << endl;
		exit(1);
	}
	if (r < 1 || r > ap->rows() || c < 1 || c > ap->cols())
	{
		cout << "***** Zurt created with invalid coordinates (" << r << ","
			<< c << ")!" << endl;
		exit(1);
	}
	if (color != 'R'  &&  color != 'Y'  &&  color != 'B')
	{
		cout << "***** Zurt created with invalid color " << color << endl;
		exit(1);
	}
	m_arena = ap;
	m_row = r;
	m_col = c;
	m_color = color;
	m_health = ZURT_HEALTH;
}

int Zurt::row() const
{
	return m_row;
}

int Zurt::col() const
{
	return m_col;
}

char Zurt::color() const
{
	return m_color;
}

bool Zurt::isDead() const
{
	return m_health == 0;
}

void Zurt::forceMove(int dir)
{
	// Suffer one unit of damage if moving fails

	if (!isDead() && !attemptMove(*m_arena, dir, m_row, m_col))
		m_health--;
}

void Zurt::move()
{
	// Attempt to move in a random direction; if we can't move, don't move
	if (!isDead())
		attemptMove(*m_arena, randInt(0, NUMDIRS - 1), m_row, m_col);
}

///////////////////////////////////////////////////////////////////////////
//  Player implementation
///////////////////////////////////////////////////////////////////////////

Player::Player(Arena* ap, int r, int c)
{
	if (ap == nullptr)
	{
		cout << "***** The player must be created in some Arena!" << endl;
		exit(1);
	}
	if (r < 1 || r > ap->rows() || c < 1 || c > ap->cols())
	{
		cout << "**** Player created with invalid coordinates (" << r
			<< "," << c << ")!" << endl;
		exit(1);
	}
	m_arena = ap;
	m_row = r;
	m_col = c;
	m_dead = false;
}

int Player::row() const
{
	return m_row;
}

int Player::col() const
{
	return m_col;
}

string Player::stand()
{
	return "Player stands.";
}

string Player::move(int dir)
{
	if (attemptMove(*m_arena, dir, m_row, m_col))
	{
		if (m_arena->numberOfZurtsAt(m_row, m_col) > 0)
		{
			setDead();
			return "Player walked into a zurt and died.";
		}
		string msg = "Player moved ";
		switch (dir)
		{
		case NORTH: msg += "north"; break;
		case EAST:  msg += "east";  break;
		case SOUTH: msg += "south"; break;
		case WEST:  msg += "west";  break;
		}
		msg += ".";
		return msg;
	}
	else
		return "Player couldn't move; player stands.";
}

bool Player::isDead() const
{
	return m_dead;
}

void Player::setDead()
{
	m_dead = true;
}

///////////////////////////////////////////////////////////////////////////
//  Arena implementation
///////////////////////////////////////////////////////////////////////////

Arena::Arena(int nRows, int nCols)
{
	if (nRows <= 0 || nCols <= 0 || nRows > MAXROWS || nCols > MAXCOLS)
	{
		cout << "***** Arena created with invalid size " << nRows << " by "
			<< nCols << "!" << endl;
		exit(1);
	}
	m_rows = nRows;
	m_cols = nCols;
	m_player = nullptr;
	m_nZurts = 0;
	for (int r = 1; r <= m_rows; r++)
		for (int c = 1; c <= m_cols; c++)
			setCellStatus(r, c, EMPTY);
}

Arena::~Arena()
{
	for (int k = 0; k < m_nZurts; k++)
		delete m_zurts[k];
	delete m_player;
}

int Arena::rows() const
{
	return m_rows;
}

int Arena::cols() const
{
	return m_cols;
}

Player* Arena::player() const
{
	return m_player;
}

int Arena::zurtCount() const
{
	return m_nZurts;
}

int Arena::getCellStatus(int r, int c) const
{
	checkPos(r, c);
	return m_grid[r - 1][c - 1];
}

int Arena::numberOfZurtsAt(int r, int c) const
{
	int count = 0;
	for (int k = 0; k < m_nZurts; k++)
	{
		Zurt* zp = m_zurts[k];
		if (zp->row() == r  &&  zp->col() == c)
			count++;
	}
	return count;
}

void Arena::display(string msg) const
{
	char displayGrid[MAXROWS][MAXCOLS];
	int r, c;

	// Fill displayGrid with dots (empty) and stars (wall)
	for (r = 1; r <= rows(); r++)
		for (c = 1; c <= cols(); c++)
			displayGrid[r - 1][c - 1] = (getCellStatus(r, c) == EMPTY ? '.' : '*');

	// Indicate zurt positions by their colors.  If more than one zurt
	// occupies a cell, show just one (any one will do).
	for (int k = 0; k < m_nZurts; k++)
	{
		Zurt* zp = m_zurts[k];
		displayGrid[zp->row() - 1][zp->col() - 1] = zp->color();
	}

	// Indicate player's position
	if (m_player != nullptr)
		displayGrid[m_player->row() - 1][m_player->col() - 1] = (m_player->isDead() ? 'X' : '@');

	// Draw the grid
	clearScreen();
	for (r = 1; r <= rows(); r++)
	{
		for (c = 1; c <= cols(); c++)
			cout << displayGrid[r - 1][c - 1];
		cout << endl;
	}
	cout << endl;

	// Write message, zurt, and player info
	if (msg != "")
		cout << msg << endl;
	cout << "There are " << zurtCount() << " zurts remaining." << endl;
	if (m_player == nullptr)
		cout << "There is no player!" << endl;
	else if (m_player->isDead())
		cout << "The player is dead." << endl;
}

void Arena::setCellStatus(int r, int c, int status)
{
	checkPos(r, c);
	m_grid[r - 1][c - 1] = status;
}

bool Arena::addZurt(int r, int c, char color)
{
	if (!isPosInBounds(r, c) || getCellStatus(r, c) != EMPTY)
		return false;
	if (m_player != nullptr  &&  m_player->row() == r  &&  m_player->col() == c)
		return false;
	if (color != 'R'  &&  color != 'Y'  &&  color != 'B')
		return false;
	if (m_nZurts == MAXZURTS)
		return false;
	m_zurts[m_nZurts] = new Zurt(this, r, c, color);
	m_nZurts++;
	return true;
}

bool Arena::addPlayer(int r, int c)
{
	if (m_player != nullptr || !isPosInBounds(r, c) || getCellStatus(r, c) != EMPTY)
		return false;
	if (numberOfZurtsAt(r, c) > 0)
		return false;
	m_player = new Player(this, r, c);
	return true;
}

string Arena::moveZurts(char color, int dir)
{
	// Zurts of the indicated color will follow that color with probability 1/2
	bool willFollow = (randInt(0, 1) == 0);

	// Move all zurts
	int nZurtsOriginally = m_nZurts;
	for (int k = m_nZurts - 1; k >= 0; k--)
	{
		Zurt* zp = m_zurts[k];
		if (willFollow  &&  zp->color() == color)
			zp->forceMove(dir);
		else
			zp->move();

		if (m_player != nullptr  &&
			zp->row() == m_player->row() && zp->col() == m_player->col())
			m_player->setDead();

		if (zp->isDead())
		{
			delete zp;

			// The order of Zurt pointers in the m_zurts array is
			// irrelevant, so it's easiest to move the last pointer to
			// replace the one pointing to the now-deleted zurt.  Since
			// we are traversing the array from last to first, we know this
			// last pointer does not point to a dead zurt.

			m_zurts[k] = m_zurts[m_nZurts - 1];
			m_nZurts--;
		}
	}

	if (m_nZurts < nZurtsOriginally)
		return "Some zurts have been destroyed.";
	else
		return "No zurts were destroyed.";
}

bool Arena::isPosInBounds(int r, int c) const
{
	return (r >= 1 && r <= m_rows  &&  c >= 1 && c <= m_cols);
}

void Arena::checkPos(int r, int c) const
{
	if (!isPosInBounds(r, c))
	{
		cout << "***** " << "Invalid arena position (" << r << ","
			<< c << ")" << endl;
		exit(1);
	}
}

///////////////////////////////////////////////////////////////////////////
//  Game implementation
///////////////////////////////////////////////////////////////////////////

Game::Game(int rows, int cols, int nZurts)
{
	if (nZurts < 0 || nZurts > MAXZURTS)
	{
		cout << "***** Game created with invalid number of zurts:  "
			<< nZurts << endl;
		exit(1);
	}
	int nEmpty = rows * cols - nZurts - 1;  // 1 for Player
	if (nEmpty < 0)
	{
		cout << "***** Game created with a " << rows << " by "
			<< cols << " arena, which is too small too hold a player and "
			<< nZurts << " zurts!" << endl;
		exit(1);
	}

	// Create arena
	m_arena = new Arena(rows, cols);

	// Add some walls in WALL_DENSITY of the empty spots
	assert(WALL_DENSITY >= 0 && WALL_DENSITY <= 1);
	int nWalls = static_cast<int>(WALL_DENSITY * nEmpty);
	while (nWalls > 0)
	{
		int r = randInt(1, rows);
		int c = randInt(1, cols);
		if (m_arena->getCellStatus(r, c) == WALL)
			continue;
		m_arena->setCellStatus(r, c, WALL);
		nWalls--;
	}

	// Add player
	int rPlayer;
	int cPlayer;
	do
	{
		rPlayer = randInt(1, rows);
		cPlayer = randInt(1, cols);
	} while (m_arena->getCellStatus(rPlayer, cPlayer) != EMPTY);
	m_arena->addPlayer(rPlayer, cPlayer);

	// Populate with zurts
	while (nZurts > 0)
	{
		int r = randInt(1, rows);
		int c = randInt(1, cols);
		if (m_arena->getCellStatus(r, c) != EMPTY || (r == rPlayer && c == cPlayer))
			continue;
		const char colors[MAXCOLORS] = { 'R', 'Y', 'B' };
		m_arena->addZurt(r, c, colors[randInt(1, MAXCOLORS) - 1]);
		nZurts--;
	}
}

Game::~Game()
{
	delete m_arena;
}

string Game::takePlayerTurn()
{
	for (;;)
	{
		cout << "Your move (n/e/s/w/x or nothing): ";
		string playerMove;
		getline(cin, playerMove);

		Player* player = m_arena->player();
		int dir;

		if (playerMove.size() == 0)
		{
			if (recommendMove(*m_arena, player->row(), player->col(), dir))
				return player->move(dir);
			else
				return player->stand();
		}
		else if (playerMove.size() == 1)
		{
			if (tolower(playerMove[0]) == 'x')
				return player->stand();
			else if (charToDir(playerMove[0], dir))
				return player->move(dir);
		}
		cout << "Player move must be nothing, or 1 character n/e/s/w/x." << endl;
	}
}

string Game::takeZurtsTurn()
{
	for (;;)
	{
		cout << "Color thrown and direction (e.g., Rn or bw): ";
		string colorAndDir;
		getline(cin, colorAndDir);
		if (colorAndDir.size() != 2)
		{
			cout << "You must specify a color followed by a direction." << endl;
			continue;
		}
		char color = toupper(colorAndDir[0]);
		if (color != 'R'  &&  color != 'Y'  &&  color != 'B')
		{
			cout << "Color must be upper or lower R, Y, or B." << endl;
			continue;
		}
		int dir;
		if (charToDir(colorAndDir[1], dir))
			return m_arena->moveZurts(color, dir);
		else
			cout << "Direction must be n, e, s, or w." << endl;
	}
}

void Game::play()
{
	m_arena->display("");
	Player* player = m_arena->player();
	while (!player->isDead() && m_arena->zurtCount() > 0)
	{
		string msg = takePlayerTurn();
		m_arena->display(msg);
		if (player->isDead())
			break;
		msg = takeZurtsTurn();
		m_arena->display(msg);
	}
	if (player->isDead())
		cout << "You lose." << endl;
	else
		cout << "You win." << endl;
}

///////////////////////////////////////////////////////////////////////////
//  Auxiliary function implementations (except clearScreen)
///////////////////////////////////////////////////////////////////////////

// Return a uniformly distributed random int from lowest to highest, inclusive
int randInt(int lowest, int highest)
{
	if (highest < lowest)
		swap(highest, lowest);
	return lowest + (rand() % (highest - lowest + 1));
}

bool charToDir(char ch, int& dir)
{
	switch (tolower(ch))
	{
	default:  return false;
	case 'n': dir = NORTH; break;
	case 'e': dir = EAST;  break;
	case 's': dir = SOUTH; break;
	case 'w': dir = WEST;  break;
	}
	return true;
}

// Return false without changing anything if moving one step from (r,c)
// in the indicated direction would hit a wall or run off the edge of the
// arena.  Otherwise, update r and c to the position resulting from the
// move and return true.
bool attemptMove(const Arena& a, int dir, int& r, int& c)
{
	int rnew = r;
	int cnew = c;
	switch (dir)
	{
	case NORTH:  if (r <= 1)        return false; else rnew--; break;
	case EAST:   if (c >= a.cols()) return false; else cnew++; break;
	case SOUTH:  if (r >= a.rows()) return false; else rnew++; break;
	case WEST:   if (c <= 1)        return false; else cnew--; break;
	}
	if (a.getCellStatus(rnew, cnew) == WALL)
		return false;
	r = rnew;
	c = cnew;
	return true;
}

// Recommend a move for a player at (r,c):  A false return means the
// recommendation is that the player should stand; otherwise, bestDir is
// set to the recommended direction to move.
bool recommendMove(const Arena& a, int r, int c, int& bestDir)
{
	// How dangerous is it to stand?
	int standDanger = computeDanger(a, r, c);

	// if it's not safe, see if moving is safer
	if (standDanger > 0)
	{
		int bestMoveDanger = standDanger;
		int bestMoveDir = NORTH;  // arbitrary initialization

								  // check the four directions to see if any move is
								  // better than standing, and if so, record the best
		for (int dir = 0; dir < NUMDIRS; dir++)
		{
			int rnew = r;
			int cnew = c;
			if (attemptMove(a, dir, rnew, cnew))
			{
				int danger = computeDanger(a, rnew, cnew);
				if (danger < bestMoveDanger)
				{
					bestMoveDanger = danger;
					bestMoveDir = dir;
				}
			}
		}

		// if moving is better than standing, recommend move
		if (bestMoveDanger < standDanger)
		{
			bestDir = bestMoveDir;
			return true;
		}
	}
	return false;  // recommend standing
}

int computeDanger(const Arena& a, int r, int c)
{
	// Our measure of danger will be the number of zurts that might move
	// to position r,c.  If a zurt is at that position, it is fatal,
	// so a large value is returned.

	if (a.numberOfZurtsAt(r, c) > 0)
		return MAXZURTS + 1;

	int danger = 0;
	if (r > 1)
		danger += a.numberOfZurtsAt(r - 1, c);
	if (r < a.rows())
		danger += a.numberOfZurtsAt(r + 1, c);
	if (c > 1)
		danger += a.numberOfZurtsAt(r, c - 1);
	if (c < a.cols())
		danger += a.numberOfZurtsAt(r, c + 1);

	return danger;
}

///////////////////////////////////////////////////////////////////////////
// main()
///////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <ctime>
using namespace std;

int main()
{
	// Initialize the random number generator
	srand(static_cast<unsigned int>(time(0)));

	// Create a game
	// Use this instead to create a mini-game:   Game g(3, 5, 2);
	Game g(10, 12, 50);

	// Play the game
	g.play();
}

///////////////////////////////////////////////////////////////////////////
//  clearScreen implementations
///////////////////////////////////////////////////////////////////////////

// DO NOT MODIFY OR REMOVE ANY CODE BETWEEN HERE AND THE END OF THE FILE!!!
// THE CODE IS SUITABLE FOR VISUAL C++, XCODE, AND g++ UNDER LINUX.

// Note to Xcode users:  clearScreen() will just write a newline instead
// of clearing the window if you launch your program from within Xcode.
// That's acceptable.

#ifdef _MSC_VER  //  Microsoft Visual C++

#include <windows.h>

void clearScreen()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
	COORD upperLeft = { 0, 0 };
	DWORD dwCharsWritten;
	FillConsoleOutputCharacter(hConsole, TCHAR(' '), dwConSize, upperLeft,
		&dwCharsWritten);
	SetConsoleCursorPosition(hConsole, upperLeft);
}

#else  // not Microsoft Visual C++, so assume UNIX interface

#include <cstring>

void clearScreen()  // will just write a newline in an Xcode output window
{
	static const char* term = getenv("TERM");
	if (term == nullptr || strcmp(term, "dumb") == 0)
		cout << endl;
	else
	{
		static const char* ESC_SEQ = "\x1B[";  // ANSI Terminal esc seq:  ESC [
		cout << ESC_SEQ << "2J" << ESC_SEQ << "H" << flush;
	}
}

#endif
