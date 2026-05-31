#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "myTeapot.h"
#include "Mloader.h"
#include <vector>
#include <time.h>

int boardState[8][8] = {
	{12, 13, 14, 15, 16, 14, 13, 12},
	{11, 11, 11, 11, 11, 11, 11, 11},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 0,  0,  0,  0,  0,  0,  0,  0},
	{ 1,  1,  1,  1,  1,  1,  1,  1},
	{ 2,  3,  4,  5,  6,  4,  3,  2}
};

bool isAnimating = false;
float animProgress = 0.0f;     
float animSpeed = 2.0f;        
int animFromX, animFromZ;      
int animToX, animToZ;          
int animatedPiece = 0;         
float speed_x = 0;
float speed_y = 0;
float cameraAngleX = 0.0f;
float cameraAngleY = PI / 4.0f;
float cameraDistance = 15.0f;
float aspectRatio = 1;


ShaderProgram* sp;
ChessModel pawnModel;
ChessModel rookModel;
ChessModel knightModel;
ChessModel bishopModel;
ChessModel queenModel;
ChessModel kingModel;
struct Move {
	int fromX, fromZ;
	int toX, toZ;
};

int moveCount = 0;
bool isWhiteTurn = true;
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
	case 1: //PIONEK
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
	case 2: //WIEŻA
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
	case 5: // HETMAN
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


float tileVertices[] = {
	0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f,   1.0f, 0.0f, 1.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f
};

float tileNormals[] = {
	0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f, 0.0f
};

float tileTexCoords[] = {
	0.0f, 0.0f,   1.0f, 0.0f,   1.0f, 1.0f,
	0.0f, 0.0f,   1.0f, 1.0f,   0.0f, 1.0f
};

float colorLight[] = {
	0.9f, 0.9f, 0.8f, 1.0f,   0.9f, 0.9f, 0.8f, 1.0f,   0.9f, 0.9f, 0.8f, 1.0f,
	0.9f, 0.9f, 0.8f, 1.0f,   0.9f, 0.9f, 0.8f, 1.0f,   0.9f, 0.9f, 0.8f, 1.0f
};

float colorDark[] = {
	0.4f, 0.3f, 0.2f, 1.0f,   0.4f, 0.3f, 0.2f, 1.0f,   0.4f, 0.3f, 0.2f, 1.0f,
	0.4f, 0.3f, 0.2f, 1.0f,   0.4f, 0.3f, 0.2f, 1.0f,   0.4f, 0.3f, 0.2f, 1.0f
};

float* vertices = myCubeVertices;
float* normals = myCubeNormals;
float* texCoords = myCubeTexCoords;
float* colors = myCubeColors;
int vertexCount = myCubeVertexCount;
unsigned char whitePixel[] = { 255, 255, 255, 255 };


GLuint tex0;
GLuint tex1;
GLuint texLight;
GLuint texDark;
void makeRandomMove() {
	/*if (moveCount >= 5) {
		printf("Wykonano juz wymagane 5 ruchow!\n");
		return;
	}
	*/

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
		Move chosenMove = legalMoves[randomIndex];

		animatedPiece = boardState[chosenMove.fromX][chosenMove.fromZ];
		animFromX = chosenMove.fromX;
		animFromZ = chosenMove.fromZ;
		animToX = chosenMove.toX;
		animToZ = chosenMove.toZ;
		animProgress = 0.0f; 
		isAnimating = true;  

		boardState[chosenMove.toX][chosenMove.toZ] = animatedPiece;
		boardState[chosenMove.fromX][chosenMove.fromZ] = 0;

		printf("Ruch %d: z (%d,%d) na (%d,%d)\n", moveCount + 1, animFromX, animFromZ, animToX, animToZ);

		isWhiteTurn = !isWhiteTurn;
		moveCount++;
	}
	else {
		printf("Brak mozliwych ruchow!\n");
	}
}
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {

		if (key == GLFW_KEY_LEFT)  cameraAngleX -= 0.1f;
		if (key == GLFW_KEY_RIGHT) cameraAngleX += 0.1f;

		if (key == GLFW_KEY_UP) {
			cameraAngleY += 0.1f;
			if (cameraAngleY > PI / 2.0f - 0.1f) cameraAngleY = PI / 2.0f - 0.1f;
		}
		if (key == GLFW_KEY_DOWN) {
			cameraAngleY -= 0.1f;
			if (cameraAngleY < 0.1f) cameraAngleY = 0.1f;
		}

		if (key == GLFW_KEY_W) cameraDistance -= 0.5f;
		if (key == GLFW_KEY_S) cameraDistance += 0.5f;
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			makeRandomMove();
		}
	}
}

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	if (height == 0) return;
	aspectRatio = (float)width / (float)height;
	glViewport(0, 0, width, height);
}


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


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	glClearColor(0, 0, 0, 1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window, windowResizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	pawnModel = loadModel("model/Pawn.obj", "model/");
	rookModel = loadModel("model/Rook.obj", "model/");
	knightModel = loadModel("model/Knight.obj", "model/");
	bishopModel = loadModel("model/Bishop.obj", "model/");
	queenModel = loadModel("model/Queen.obj", "model/");
	kingModel = loadModel("model/King.obj", "model/");

	sp = new ShaderProgram("v_simplest.glsl", NULL, "f_simplest.glsl");
	tex0 = readTexture("metal_diffuse.png");
	tex1 = readTexture("metal_specular.png");
	unsigned char lightPixel[] = { 230, 230, 204, 255 };
	glGenTextures(1, &texLight);
	glBindTexture(GL_TEXTURE_2D, texLight);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, lightPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned char darkPixel[] = { 102, 76, 51, 255 };
	glGenTextures(1, &texDark);
	glBindTexture(GL_TEXTURE_2D, texDark);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, darkPixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

}




void freeOpenGLProgram(GLFWwindow* window) {

	delete sp;
}




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

	glBindTexture(GL_TEXTURE_2D, tex0);
	glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, pawnModel.vertices.data());
	glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, pawnModel.normals.data());
	glVertexAttribPointer(sp->a("texCoord0"), 2, GL_FLOAT, false, 0, pawnModel.texCoords.data());

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

					if (piece < 10) {
						glBindTexture(GL_TEXTURE_2D, tex0);
					}
					else {
						glBindTexture(GL_TEXTURE_2D, texDark);
					}

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

	glDisableVertexAttribArray(sp->a("vertex"));
	glDisableVertexAttribArray(sp->a("normal"));
	glDisableVertexAttribArray(sp->a("texCoord0"));
	glDisableVertexAttribArray(sp->a("color"));
	if (isAnimating) {
		glEnableVertexAttribArray(sp->a("vertex"));
		glEnableVertexAttribArray(sp->a("normal"));
		glEnableVertexAttribArray(sp->a("texCoord0"));

		// 1. Ustalenie modelu i koloru
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

		if (animatedPiece < 10) glBindTexture(GL_TEXTURE_2D, tex0);
		else glBindTexture(GL_TEXTURE_2D, texDark);

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

		glDisableVertexAttribArray(sp->a("vertex"));
		glDisableVertexAttribArray(sp->a("normal"));
		glDisableVertexAttribArray(sp->a("texCoord0"));
	}
	glfwSwapBuffers(window);
}


int main(void)
{
	srand(time(NULL));
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);

	if (!window)
	{
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
	glfwSetTime(0);
	while (!glfwWindowShouldClose(window))
	{
		angle_x += speed_x * glfwGetTime();
		angle_y += speed_y * glfwGetTime();
		glfwSetTime(0);
		drawScene(window, angle_x, angle_y);
		glfwPollEvents();
		//Główna pętla
		glfwSetTime(0); //Zeruj timer
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

			drawScene(window, angle_x, angle_y);
			glfwPollEvents();
		}
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
