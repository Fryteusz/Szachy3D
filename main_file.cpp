/**
 * @file main.cpp
 * @brief Główny plik silnika szachowego 3D zrealizowanego w OpenGL.
 * * Program implementuje renderowanie sceny 3D (szachownicy i figur),
 * system swobodnej kamery, logikę szachową weryfikującą legalność ruchów,
 * płynne animacje oraz parser plików PGN pozwalający na odtwarzanie partii.
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <fstream>
#include <string>
#include <cctype>
#include <algorithm>

#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "Mloader.h"

 /** * @brief Stan gry na szachownicy (8x8).
  * * Wartości:
  * - 0 = puste pole
  * - 1 do 6 = Białe figury (1=Pionek, 2=Wieża, 3=Skoczek, 4=Goniec, 5=Królowa, 6=Król)
  * - 11 do 16 = Czarne figury (+10 do wartości podstawowej)
  */
int boardState[8][8] = {
	{12, 13, 14, 15, 16, 14, 13, 12},
	{11, 11, 11, 11, 11, 11, 11, 11},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{1,  1,  1, 1,  1,  1,  1,  1},
	{ 2,  3,  4,  5,  6,  4,  3,  2}
};

/** @brief Flaga określająca, czy aktualnie trwa animacja ruchu figury. */
bool isAnimating = false;
/** @brief Postęp animacji w przedziale [0.0, 1.0]. */
float animProgress = 0.0f;
/** @brief Prędkość wykonywania animacji ruchu. */
float animSpeed = 2.0f;
/** @brief Koordynata X pola startowego animowanej figury. */
int animFromX;
/** @brief Koordynata Z pola startowego animowanej figury. */
int animFromZ;
/** @brief Koordynata X pola docelowego animowanej figury. */
int animToX;
/** @brief Koordynata Z pola docelowego animowanej figury. */
int animToZ;
/** @brief Kod animowanej figury (wartość z boardState). */
int animatedPiece = 0;

/** @brief Prędkość obrotu kamery w osi X. */
float speed_x = 0;
/** @brief Prędkość obrotu kamery w osi Y. */
float speed_y = 0;
/** @brief Aktualny kąt obrotu kamery w poziomie. */
float cameraAngleX = 0.0f;
/** @brief Aktualny kąt obrotu kamery w pionie. */
float cameraAngleY = PI / 4.0f;
/** @brief Odległość kamery od środka planszy. */
float cameraDistance = 15.0f;
/** @brief Proporcje ekranu (szerokość / wysokość). */
float aspectRatio = 1;

/** @brief Flaga sprawdzająca, czy użytkownik przesuwa mysz z wciśniętym przyciskiem. */
bool isDragging = false;
/** @brief Ostatnia zapisana pozycja kursora w osi X. */
double lastMouseX = 0.0;
/** @brief Ostatnia zapisana pozycja kursora w osi Y. */
double lastMouseY = 0.0;

/** @brief Licznik wykonanych ruchów. */
int moveCount = 0;
/** @brief Flaga tury. True jeśli ruch wykonują białe, false dla czarnych. */
bool isWhiteTurn = true;

/**
 * @brief Struktura opisująca pojedynczy ruch na planszy.
 */
struct Move {
	int fromX; ///< Pozycja X pola startowego.
	int fromZ; ///< Pozycja Z pola startowego.
	int toX;   ///< Pozycja X pola docelowego.
	int toZ;   ///< Pozycja Z pola docelowego.
};

/** @brief Flaga określająca tryb gry (z pliku PGN lub ruchy losowe). */
bool useLoadedMoves = false;
/** @brief Kontener przechowujący ruchy wczytane z pliku PGN. */
std::vector<Move> loadedMoves;
/** @brief Indeks aktualnie odtwarzanego ruchu z kontenera loadedMoves. */
size_t loadedMoveIndex = 0;

/** @brief Wskaźnik na używany program cieniujący (shader). */
ShaderProgram* sp;
/** @brief Model 3D Pionka. */
ChessModel pawnModel;
/** @brief Model 3D Wieży. */
ChessModel rookModel;
/** @brief Model 3D Skoczka. */
ChessModel knightModel;
/** @brief Model 3D Gońca. */
ChessModel bishopModel;
/** @brief Model 3D Królowej (Hetmana). */
ChessModel queenModel;
/** @brief Model 3D Króla. */
ChessModel kingModel;

/** @brief Tablica wierzchołków dla pojedynczego kafelka szachownicy. */
float tileVertices[] = {
	0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f
};
/** @brief Tablica wektorów normalnych dla pojedynczego kafelka szachownicy. */
float tileNormals[] = {
	0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f
};
/** @brief Tablica współrzędnych tekstur (UV) dla pojedynczego kafelka szachownicy. */
float tileTexCoords[] = {
	0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,
	0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f
};

/** @brief Uchwyt tekstury metalu (diffuse) dla białych figur. */
GLuint tex0;
/** @brief Uchwyt tekstury metalu (specular). */
GLuint tex1;
/** @brief Uchwyt wygenerowanej w pamięci tekstury dla jasnych pól. */
GLuint texLight;
/** @brief Uchwyt wygenerowanej w pamięci tekstury dla ciemnych pól oraz czarnych figur. */
GLuint texDark;

// --- FUNKCJE LOGICZNE SZACHÓW ---

/**
 * @brief Sprawdza, czy linia ruchu pomiędzy dwoma polami jest wolna od przeszkód.
 * * Funkcja używana dla Wieży, Gońca i Królowej do upewnienia się,
 * że po drodze do celu nie stoją żadne inne figury.
 * * @param fromX Współrzędna X pola startowego.
 * @param fromZ Współrzędna Z pola startowego.
 * @param toX Współrzędna X pola docelowego.
 * @param toZ Współrzędna Z pola docelowego.
 * @return true jeśli trasa jest czysta, false jeśli wystąpiła kolizja.
 */
bool isPathClear(int fromX, int fromZ, int toX, int toZ) {
	int stepX = 0;
	if (toX - fromX > 0) stepX = 1;
	else if (toX - fromX < 0) stepX = -1;

	int stepZ = 0;
	if (toZ - fromZ > 0) stepZ = 1;
	else if (toZ - fromZ < 0) stepZ = -1;

	int currentX = fromX + stepX;
	int currentZ = fromZ + stepZ;

	while (currentX != toX || currentZ != toZ) {
		if (boardState[currentX][currentZ] != 0) {
			return false;
		}
		currentX += stepX;
		currentZ += stepZ;
	}
	return true;
}

/**
 * @brief Główny walidator zasad szachowych.
 * * Weryfikuje, czy figura znajdująca się na polu startowym może zgodnie
 * z zasadami szachowymi przemieścić się na pole docelowe (w tym bicie).
 * * @param fromX Współrzędna X pola startowego.
 * @param fromZ Współrzędna Z pola startowego.
 * @param toX Współrzędna X pola docelowego.
 * @param toZ Współrzędna Z pola docelowego.
 * @return true jeśli ruch jest zgodny z zasadami gry, false w przeciwnym wypadku.
 */
bool isMoveLegal(int fromX, int fromZ, int toX, int toZ) {
	int piece = boardState[fromX][fromZ];
	int target = boardState[toX][toZ];

	if (target != 0) {
		bool isWhitePiece = (piece < 10);
		bool isWhiteTarget = (target < 10);
		if (isWhitePiece == isWhiteTarget) return false;
	}

	int type = piece % 10;
	int dx = toX - fromX;
	int dz = toZ - fromZ;

	switch (type) {
	case 1: // PIONEK
		if (piece < 10) {
			if (dx == -1 && dz == 0 && target == 0) return true;
			if (fromX == 6 && dx == -2 && dz == 0 && target == 0 && boardState[fromX - 1][fromZ] == 0) return true;
			if (dx == -1 && abs(dz) == 1 && target != 0) return true;
		}
		else {
			if (dx == 1 && dz == 0 && target == 0) return true;
			if (fromX == 1 && dx == 2 && dz == 0 && target == 0 && boardState[fromX + 1][fromZ] == 0) return true;
			if (dx == 1 && abs(dz) == 1 && target != 0) return true;
		}
		return false;
	case 2: // WIEŻA
		if ((dx == 0 && dz != 0) || (dx != 0 && dz == 0)) {
			return isPathClear(fromX, fromZ, toX, toZ);
		}
		return false;
	case 3: // SKOCZEK
		if ((abs(dx) == 2 && abs(dz) == 1) || (abs(dx) == 1 && abs(dz) == 2)) {
			return true;
		}
		return false;
	case 4: // GONIEC
		if (abs(dx) == abs(dz)) {
			return isPathClear(fromX, fromZ, toX, toZ);
		}
		return false;
	case 5: // HETMAN (KRÓLOWA)
		if ((dx == 0 && dz != 0) || (dx != 0 && dz == 0) || (abs(dx) == abs(dz))) {
			return isPathClear(fromX, fromZ, toX, toZ);
		}
		return false;
	case 6: // KRÓL
		if (abs(dx) <= 1 && abs(dz) <= 1) {
			return true;
		}
		return false;
	}
	return false;
}

/**
 * @brief Resetuje stan gry do układu początkowego.
 * * Przywraca figury na pozycje startowe, resetuje liczniki tury i animacji,
 * oraz czyści indeks wczytanych ruchów.
 */
void resetBoard() {
	int initialBoard[8][8] = {
		{12, 13, 14, 15, 16, 14, 13, 12},
		{11, 11, 11, 11, 11, 11, 11, 11},
		{ 0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0},
		{ 0,  0,  0,  0,  0,  0,  0,  0},
		{ 1,  1,  1,  1,  1,  1,  1,  1},
		{ 2,  3,  4,  5,  6,  4,  3,  2}
	};
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			boardState[i][j] = initialBoard[i][j];
		}
	}
	moveCount = 0;
	isWhiteTurn = true;
	isAnimating = false;
	loadedMoveIndex = 0;
}

/**
 * @brief Parser plików PGN (Portable Game Notation).
 * * Wczytuje plik tekstowy w formacie PGN, ignoruje nagłówki i komentarze,
 * a następnie dekoduje notację algebraiczną SAN na fizyczne ruchy na tablicy.
 * W trakcie działania przeprowadza wirtualną symulację partii, by powiązać
 * notację z konkretnymi figurami na podstawie zasad gry.
 * * @param filename Ścieżka do pliku z zapisaną partią szachową.
 */
void loadPGN(const char* filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		printf("Nie udalo sie otworzyc pliku: %s\n", filename);
		return;
	}

	resetBoard();
	loadedMoves.clear();
	std::string token;

	while (file >> token) {
		if (token.empty()) continue;

		if (token[0] == '[') {
			std::string remainder;
			std::getline(file, remainder);
			continue;
		}
		if (isdigit(token[0]) && token.find('.') != std::string::npos) {
			continue;
		}
		if (token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*") {
			continue;
		}

		token.erase(std::remove(token.begin(), token.end(), '+'), token.end());
		token.erase(std::remove(token.begin(), token.end(), '#'), token.end());
		token.erase(std::remove(token.begin(), token.end(), '!'), token.end());
		token.erase(std::remove(token.begin(), token.end(), '?'), token.end());

		if (token.empty()) continue;

		Move m;
		m.fromX = -1; m.fromZ = -1; m.toX = -1; m.toZ = -1;
		bool found = false;

		if (token == "O-O") {
			int r = isWhiteTurn ? 7 : 0;
			m.fromX = r; m.fromZ = 4; m.toX = r; m.toZ = 6;
			found = true;
		}
		else if (token == "O-O-O") {
			int r = isWhiteTurn ? 7 : 0;
			m.fromX = r; m.fromZ = 4; m.toX = r; m.toZ = 2;
			found = true;
		}
		else {

			int type = 1;
			char pieceChar = token[0];

			if (pieceChar == 'K') { type = 6; token = token.substr(1); }
			else if (pieceChar == 'Q') { type = 5; token = token.substr(1); }
			else if (pieceChar == 'B') { type = 4; token = token.substr(1); }
			else if (pieceChar == 'N') { type = 3; token = token.substr(1); }
			else if (pieceChar == 'R') { type = 2; token = token.substr(1); }

			token.erase(std::remove(token.begin(), token.end(), 'x'), token.end());

			if (token.length() >= 2) {
				char fileTo = token[token.length() - 2];
				char rankTo = token[token.length() - 1];
				m.toZ = fileTo - 'a';
				m.toX = 8 - (rankTo - '0');

				int hintFile = -1;
				int hintRank = -1;
				if (token.length() > 2) {
					char hint = token[0];
					if (hint >= 'a' && hint <= 'h') hintFile = hint - 'a';
					else if (hint >= '1' && hint <= '8') hintRank = 8 - (hint - '0');
				}

				for (int x = 0; x < 8; x++) {
					for (int z = 0; z < 8; z++) {
						int p = boardState[x][z];
						if (p == 0) continue;
						if (p % 10 != type) continue;
						if ((isWhiteTurn && p > 10) || (!isWhiteTurn && p < 10)) continue;

						if (hintFile != -1 && z != hintFile) continue;
						if (hintRank != -1 && x != hintRank) continue;

						if (isMoveLegal(x, z, m.toX, m.toZ)) {
							m.fromX = x;
							m.fromZ = z;
							found = true;
							break;
						}
					}
					if (found) break;
				}
			}
		}

		if (found) {
			std::string originalToken = token;
			loadedMoves.push_back(m);
			if (originalToken == "O-O" || originalToken == "O-O-O") {  // użyj oryginału
				int r = isWhiteTurn ? 7 : 0;
				if (originalToken == "O-O") {
					boardState[r][6] = boardState[r][4]; boardState[r][4] = 0;
					boardState[r][5] = boardState[r][7]; boardState[r][7] = 0;
				}
				else {
					boardState[r][2] = boardState[r][4]; boardState[r][4] = 0;
					boardState[r][3] = boardState[r][0]; boardState[r][0] = 0;
				}
			}
			else {
				boardState[m.toX][m.toZ] = boardState[m.fromX][m.fromZ];
				boardState[m.fromX][m.fromZ] = 0;
			}
			isWhiteTurn = !isWhiteTurn;
			moveCount++;
		}
		else {
			printf("Nie zinterpretowano ruchu: %s\n", token.c_str());
		}
	}

	file.close();
	printf("Pomyslnie zaladowano %d ruchow z pliku PGN.\n", (int)loadedMoves.size());
	resetBoard();
	useLoadedMoves = true;
}

/**
 * @brief Kontroler wykonujący następny ruch na planszy.
 * * Jeśli tryb PGN (useLoadedMoves) jest włączony, funkcja wykonuje
 * następny ruch z wektora loadedMoves. W przeciwnym razie przeszukuje
 * planszę, kompiluje listę wszystkich możliwych, legalnych ruchów
 * i wykonuje w pełni losowy krok. Wyzwala również proces animacji (Ducha).
 */
void executeNextMove() {
	if (isAnimating) return;

	Move chosenMove;
	bool moveFound = false;

	if (useLoadedMoves) {
		if (loadedMoveIndex >= loadedMoves.size()) {
			printf("Koniec partii zapisanej w pliku PGN!\n");
			return;
		}
		chosenMove = loadedMoves[loadedMoveIndex++];
		moveFound = true;
	}
	else {
		std::vector<Move> legalMoves;
		for (int x = 0; x < 8; x++) {
			for (int z = 0; z < 8; z++) {
				int piece = boardState[x][z];
				if (piece == 0) continue;

				bool isWhitePiece = (piece < 10);
				if (isWhitePiece != isWhiteTurn) continue;

				for (int tx = 0; tx < 8; tx++) {
					for (int tz = 0; tz < 8; tz++) {
						if (x == tx && z == tz) continue;

						if (isMoveLegal(x, z, tx, tz)) {
							legalMoves.push_back({ x, z, tx, tz });
						}
					}
				}
			}
		}

		if (!legalMoves.empty()) {
			int randomIndex = rand() % legalMoves.size();
			chosenMove = legalMoves[randomIndex];
			moveFound = true;
		}
		else {
			printf("Brak mozliwych ruchow!\n");
		}
	}

	if (moveFound) {
		animatedPiece = boardState[chosenMove.fromX][chosenMove.fromZ];
		animFromX = chosenMove.fromX;
		animFromZ = chosenMove.fromZ;
		animToX = chosenMove.toX;
		animToZ = chosenMove.toZ;
		animProgress = 0.0f;
		isAnimating = true;

		if ((animatedPiece % 10 == 6) && abs(chosenMove.fromZ - chosenMove.toZ) == 2) {
			int r = chosenMove.fromX;
			if (chosenMove.toZ == 6) {
				boardState[r][5] = boardState[r][7]; boardState[r][7] = 0;
			}
			else if (chosenMove.toZ == 2) {
				boardState[r][3] = boardState[r][0]; boardState[r][0] = 0;
			}
		}

		boardState[chosenMove.toX][chosenMove.toZ] = animatedPiece;
		boardState[chosenMove.fromX][chosenMove.fromZ] = 0;

		printf("Ruch %d: z (%d,%d) na (%d,%d)\n", moveCount + 1, animFromX, animFromZ, animToX, animToZ);
		isWhiteTurn = !isWhiteTurn;
		moveCount++;
	}
}

// --- CALLBACKI SYSTEMOWE GLFW ---

/**
 * @brief Obsługa błędów biblioteki GLFW.
 * * @param error Kod błędu.
 * @param description Treść błędu.
 */
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

/**
 * @brief Obsługa klawiatury fizycznej.
 * * Nasłuchuje na klawisz spacji (wykonanie ruchu) oraz klawisz L (wczytanie PGN).
 * * @param window Wskaźnik na okno GLFW.
 * @param key Kod naciśniętego klawisza.
 * @param scancode Zależny od platformy kod skanowania klawisza.
 * @param action Akcja klawiatury (PRESS, RELEASE, REPEAT).
 * @param mods Modyfikatory (Shift, Ctrl, Alt).
 */
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_SPACE) {
			executeNextMove();
		}
		if (key == GLFW_KEY_L) {
			loadPGN("game.pgn");
		}
	}
}

/**
 * @brief Obsługa rolki (scrolla) myszki. Służy do zoomowania kamery.
 * * @param window Wskaźnik na okno GLFW.
 * @param xoffset Przesunięcie rolki w osi X (zazwyczaj niewykorzystywane).
 * @param yoffset Przesunięcie rolki w osi Y (góra/dół).
 */
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	cameraDistance -= (float)yoffset * 1.5f;
	if (cameraDistance < 2.0f) cameraDistance = 2.0f;
	if (cameraDistance > 40.0f) cameraDistance = 40.0f;
}

/**
 * @brief Obsługa kliknięć przycisków myszy.
 * * Aktywuje i deaktywuje tryb obracania kamerą po wciśnięciu lewego przycisku.
 * * @param window Wskaźnik na okno GLFW.
 * @param button Kod przycisku myszy.
 * @param action Akcja (PRESS, RELEASE).
 * @param mods Modyfikatory (Shift, Ctrl, Alt).
 */
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			isDragging = true;
			glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
		}
		else if (action == GLFW_RELEASE) {
			isDragging = false;
		}
	}
}

/**
 * @brief Zdarzenie ruchu myszką po ekranie.
 * * Oblicza deltę (zmianę) przesunięcia kursora w celu zaktualizowania
 * kątów widzenia kamery (orbity).
 * * @param window Wskaźnik na okno GLFW.
 * @param xpos Obecna pozycja kursora w osi X.
 * @param ypos Obecna pozycja kursora w osi Y.
 */
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	if (isDragging) {
		double deltaX = xpos - lastMouseX;
		double deltaY = ypos - lastMouseY;

		cameraAngleX -= (float)deltaX * 0.01f;
		cameraAngleY -= (float)deltaY * 0.01f;

		if (cameraAngleY > PI / 2.0f - 0.1f) cameraAngleY = PI / 2.0f - 0.1f;
		if (cameraAngleY < 0.1f) cameraAngleY = 0.1f;

		lastMouseX = xpos;
		lastMouseY = ypos;
	}
}

/**
 * @brief Aktualizacja obszaru renderowania w przypadku zmiany rozmiaru okna.
 * * @param window Wskaźnik na okno GLFW.
 * @param width Nowa szerokość okna.
 * @param height Nowa wysokość okna.
 */
void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}

/**
 * @brief Wczytuje plik graficzny jako teksturę w pamięci karty graficznej.
 * * Wykorzystuje bibliotekę lodepng do zdekodowania pliku.
 * * @param filename Ścieżka do pliku graficznego (np. PNG).
 * @return Uchwyt (identyfikator) wygenerowanej tekstury w OpenGL.
 */
GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);

	std::vector<unsigned char> image;
	unsigned width, height;
	unsigned error = lodepng::decode(image, width, height, filename);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return tex;
}

/**
 * @brief Inicjalizuje stan silnika OpenGL i ładuje asety projektu.
 * * Funkcja wywoływana jednorazowo przed rozpoczęciem pętli głównej programu.
 * Ładuje programy cieniujące (shadery), modele figur z plików .obj
 * oraz tworzy tekstury potrzebne do poprawnego wyrenderowania sceny.
 * * @param window Wskaźnik na okno GLFW.
 */
void initOpenGLProgram(GLFWwindow* window) {
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);

	pawnModel = loadModel("model/Pawn.obj", "model/");
	rookModel = loadModel("model/Rook.obj", "model/");
	knightModel = loadModel("model/Knight.obj", "model/");
	bishopModel = loadModel("model/Bishop.obj", "model/");
	queenModel = loadModel("model/Queen.obj", "model/");
	kingModel = loadModel("model/King.obj", "model/");

	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	

	unsigned char lightPixel[] = { 230, 230, 204, 255 };
	glGenTextures(1, &texDark);
	glBindTexture(GL_TEXTURE_2D, texDark);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, lightPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned char darkPixel[] = { 102, 76, 51, 255 };
	glGenTextures(1, &texLight);
	glBindTexture(GL_TEXTURE_2D, texLight);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, darkPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/**
 * @brief Zwalnia zasoby zajęte przez program przed jego zamknięciem.
 * * @param window Wskaźnik na okno GLFW.
 */
void freeOpenGLProgram(GLFWwindow* window) {
	delete sp;
}

// --- RENDERING SCENY ---

/**
 * @brief Główna procedura rysująca scenę 3D.
 * * Funkcja wywoływana w każdej klatce programu (fps).
 * Odpowiada za czyszczenie buforów, wyliczanie macierzy Widoku (V) i Projekcji (P),
 * narysowanie statycznej szachownicy, nałożenie na nią figur ze zaktualizowanej
 * struktury boardState oraz, w razie potrzeby, narysowanie klatki animacji "ducha"
 * poruszającej się figury wykorzystując interpolację (glm::mix).
 * * @param window Wskaźnik na okno GLFW.
 * @param angle_x Aktualny kąt rotacji obiektu (nieużywany dla silnika z własną kamerą).
 * @param angle_y Aktualny kąt rotacji obiektu (nieużywany dla silnika z własną kamerą).
 */
void drawScene(GLFWwindow* window, float angle_x, float angle_y) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	float camX = cameraDistance * cos(cameraAngleY) * sin(cameraAngleX);
	float camY = cameraDistance * sin(cameraAngleY);
	float camZ = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);

	glm::mat4 V = glm::lookAt(
		glm::vec3(camX, camY, camZ),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	glm::mat4 P = glm::perspective(50.0f * PI / 180.0f, aspectRatio, 0.01f, 100.0f);

	sp->use();
	glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));

	glEnableVertexAttribArray(sp->a("vertex"));
	glEnableVertexAttribArray(sp->a("normal"));
	glEnableVertexAttribArray(sp->a("texCoord0"));
	glEnableVertexAttribArray(sp->a("color"));
	glUniform1i(sp->u("textureMap0"), 0);
	glActiveTexture(GL_TEXTURE0);

	glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, tileVertices);
	glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, tileNormals);
	glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, tileTexCoords);

	glm::mat4 boardTransform = glm::mat4(1.0f);
	boardTransform = glm::translate(boardTransform, glm::vec3(-4.0f, -2.0f, -4.0f));

	for (int x = 0; x < 8; x++) {
		for (int z = 0; z < 8; z++) {
			glm::mat4 M_tile = glm::translate(boardTransform, glm::vec3(x, 0.0f, z));
			glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M_tile));

			if ((x + z) % 2 == 0) {
				glBindTexture(GL_TEXTURE_2D, texDark);
			}
			else {
				glBindTexture(GL_TEXTURE_2D, texLight);
			}
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
	}

	for (int x = 0; x < 8; x++) {
		for (int z = 0; z < 8; z++) {
			int piece = boardState[x][z];

			if (isAnimating && x == animToX && z == animToZ) {
				continue;
			}

			if (piece != 0) {
				ChessModel* currentModel = NULL;
				int type = piece % 10;

				switch (type) {
				case 1: currentModel = &pawnModel; break;
				case 2: currentModel = &rookModel; break;
				case 3: currentModel = &knightModel; break;
				case 4: currentModel = &bishopModel; break;
				case 5: currentModel = &queenModel; break;
				case 6: currentModel = &kingModel; break;
				}

				if (currentModel != NULL) {
					if (piece < 10) glBindTexture(GL_TEXTURE_2D, texDark);
					else glBindTexture(GL_TEXTURE_2D, texLight);

					glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, currentModel->vertices.data());
					glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, currentModel->normals.data());
					glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, currentModel->texCoords.data());

					glm::mat4 M_piece = glm::mat4(1.0f);
					M_piece = glm::translate(M_piece, glm::vec3(-4.0f + x + 0.5f, -2.0f, -4.0f + z + 0.5f));
					M_piece = glm::rotate(M_piece, -PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
					M_piece = glm::scale(M_piece, glm::vec3(0.01f, 0.01f, 0.01f));

					glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M_piece));
					glDrawArrays(GL_TRIANGLES, 0, currentModel->vertexCount);
				}
			}
		}
	}

	if (isAnimating) {
		ChessModel* animModel = NULL;
		int type = animatedPiece % 10;
		switch (type) {
		case 1: animModel = &pawnModel; break;
		case 2: animModel = &rookModel; break;
		case 3: animModel = &knightModel; break;
		case 4: animModel = &bishopModel; break;
		case 5: animModel = &queenModel; break;
		case 6: animModel = &kingModel; break;
		}

		if (animModel != NULL) {
			if (animatedPiece < 10) glBindTexture(GL_TEXTURE_2D, texDark);
			else glBindTexture(GL_TEXTURE_2D, texLight);

			glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, animModel->vertices.data());
			glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, animModel->normals.data());
			glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, animModel->texCoords.data());

			float currentX = glm::mix((float)animFromX, (float)animToX, animProgress);
			float currentZ = glm::mix((float)animFromZ, (float)animToZ, animProgress);

			glm::mat4 M_anim = glm::mat4(1.0f);
			M_anim = glm::translate(M_anim, glm::vec3(-4.0f + currentX + 0.5f, -2.0f, -4.0f + currentZ + 0.5f));
			M_anim = glm::rotate(M_anim, -PI / 2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
			M_anim = glm::scale(M_anim, glm::vec3(0.01f, 0.01f, 0.01f));

			glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(M_anim));
			glDrawArrays(GL_TRIANGLES, 0, animModel->vertexCount);
		}
	}

	glDisableVertexAttribArray(sp->a("vertex"));
	glDisableVertexAttribArray(sp->a("normal"));
	glDisableVertexAttribArray(sp->a("texCoord0"));
	glDisableVertexAttribArray(sp->a("color"));

	glfwSwapBuffers(window);
}

/**
 * @brief Punkt wejściowy programu.
 * * Inicjuje środowisko OpenGL (GLFW, GLEW), tworzy kontekst okna,
 * przygotowuje zasoby oraz uruchamia główną pętlę renderującą scenę
 * wraz ze stałym nasłuchiwaniem na zdarzenia wejścia od użytkownika.
 * * @return 0 po poprawnym wyjściu z aplikacji.
 */
int main(void)
{
	srand(time(NULL));
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "Chess 3D", NULL, NULL);

	if (!window) {
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	float angle_x = 0;
	float angle_y = 0;

	// Główna pętla programu
	glfwSetTime(0);
	while (!glfwWindowShouldClose(window))
	{
		float deltaTime = glfwGetTime();
		glfwSetTime(0);

		angle_x += speed_x * deltaTime;
		angle_y += speed_y * deltaTime;

		if (isAnimating) {
			animProgress += animSpeed * deltaTime;
			if (animProgress >= 1.0f) {
				animProgress = 1.0f;
				isAnimating = false;
			}
		}

		drawScene(window, angle_x, angle_y);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}