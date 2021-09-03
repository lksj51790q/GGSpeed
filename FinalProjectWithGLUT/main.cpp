#include <Windows.h>

#include "printer.h"
#include "game.h"

#include <GL/freeglut.h>
#include <GL/glut.h>
#include <FreeImage.h>

#include <ctime>
#include <iostream>
#include <cstdio>
#include <cstring>

//Menu
#define MENU_OPTIONS_NUM 3
#define MENU_OPTION_ONE_PLAYER_GAME 0
#define MENU_OPTION_SETTING 1
#define MENU_OPTION_EXIT 2

//Setting
#define SETTING_OPTIONS_NUM 3
#define SETTING_OPTION_FULLSCREEN 0
#define SETTING_OPTION_FPS 1
#define SETTING_OPTION_CONFIRM_CANCEL 2


#define COLOR_DEFAULT 0
#define COLOR_MENU 1
#define COLOR_MENU_SELECTED 2
#define COLOR_SPEED 5
#define DELETE(x) if(x) {delete x; x = nullptr;};

using namespace std;



enum GameStatus
{
	MENU = 0,
	SETTING,
	GAME_LOADING,
	ONE_PLAYER_GAME,
	TWO_PLAYER_GAME
};

//	Global Variables
static int			gWindowWidth;
static int			gWindowHeight;
static int			gWindowWidthRecover;
static int			gWindowHeightRecover;
static bool			gFullscreen;
static int			gMaxFPS;

static bool			gTempFullscreen;
static int			gTempMaxFPS;
static bool			gTempComfirm;
static GameStatus	gGameStatus			= MENU;
static Game			*gGame				= nullptr;
static char			gMenuContent[4][30] = {"Play",
										"Setting",
										"Exit"};
static int			gMenuSelectIndex	= 0;
static GLuint		gMenuBackgroundTexture;
static int			gSettingSelectIndex = 0;
static GLuint		gSettingBackgroundTexture;

static Printer		*gPrinter			= nullptr;
static clock_t		gSavedTime			= 0;
static double		gTimePass			= 0.0;
static double		gCurrentFPS			= 0.0;
static int			gFPSRefreshCount;

void loadSetting(void)
{
	//Default Setting
	gWindowWidth = 900;
	gWindowHeight = 600;
	gFullscreen = false;
	gMaxFPS = 60;

	//Load Custom Setting
	FILE* fptr = nullptr;
	fopen_s(&fptr, "setting", "r");
	char line_buffer[100];
	char *token1, *token2, *next_token;
	PxVec3 vec_buffer;
	while (fgets(line_buffer, sizeof(line_buffer), fptr) != NULL)
	{
		token1 = strtok_s(line_buffer, "= \t\n", &next_token);
		token2 = strtok_s(NULL, "= \t\n", &next_token);
		if (strcmp(token1, "fullscreen") == 0)
		{
			if (strcmp(token2, "true") == 0)
				gFullscreen = true;
			else if (strcmp(token2, "false") == 0)
				gFullscreen = false;
		}
		else if (strcmp(token1, "fps") == 0)
		{
			if (strcmp(token2, "30") == 0)
				gMaxFPS = 30;
			else if (strcmp(token2, "60") == 0)
				gMaxFPS = 60;
		}
	}
	fclose(fptr);

	gWindowWidthRecover = gWindowWidth;
	gWindowHeightRecover = gWindowHeight;
	gFPSRefreshCount = gMaxFPS / 3;
	return;
}
void saveSetting(void)
{
	FILE* fptr = nullptr;
	fopen_s(&fptr, "setting", "w");
	char buffer[100];
	if(gFullscreen)
		sprintf_s(buffer, 100, "fullscreen=true\n");
	else
		sprintf_s(buffer, 100, "fullscreen=false\n");
	fputs(buffer, fptr);
	sprintf_s(buffer, 100, "fps=%d\n", gMaxFPS);
	fputs(buffer, fptr);
	fclose(fptr);
	return;
}
void drawBackground(void)
{
	GLfloat vertices[12] = {
		0.0f, 0.0f, -0.01f, 
		0.0f, (GLfloat)gWindowHeight, -0.01f,
		(GLfloat)gWindowWidth, (GLfloat)gWindowHeight, -0.01f,
		(GLfloat)gWindowWidth, 0.0f, -0.01f,
	};
	GLfloat texture_vertices[8] = {
		0.0f, 0.0f,
		0.0f, 1.0f, 
		1.0f, 1.0f, 
		1.0f, 0.0f
	};
	GLuint indices[4] = { 0, 1, 2, 3 };
	GLuint vertex_VBO;
	GLuint texture_VBO;
	GLuint indice_VBO;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_LIGHTING_BIT);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gMenuBackgroundTexture);
			//Generate Buffers
			glGenBuffers(1, &vertex_VBO);
			glGenBuffers(1, &texture_VBO);
			glGenBuffers(1, &indice_VBO);

			//Set Vertex Buffer
			glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
			glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
			glVertexPointer(3, GL_FLOAT, 0, 0);
			glEnableClientState(GL_VERTEX_ARRAY);

			//Set Texture Buffer
			glBindBuffer(GL_ARRAY_BUFFER, texture_VBO);
			glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), texture_vertices, GL_STATIC_DRAW);
			glTexCoordPointer(2, GL_FLOAT, 0, 0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);

			//Set Indice Buffer
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indice_VBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indices, GL_STATIC_DRAW);

			//Draw
			glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, (void*)(0));

			//Unbind Buffers
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			//Delete Buffers
			glDeleteBuffers(1, &vertex_VBO);
			glDeleteBuffers(1, &texture_VBO);
			glDeleteBuffers(1, &indice_VBO);
		glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopClientAttrib();
	return;
}
void drawSetting(void)
{
	GLfloat vertices[12] = {
		(GLfloat)gWindowWidth * 0.1, (GLfloat)gWindowHeight * 0.1, -0.005f,
		(GLfloat)gWindowWidth * 0.1, (GLfloat)gWindowHeight * 0.9, -0.005f,
		(GLfloat)gWindowWidth * 0.9, (GLfloat)gWindowHeight * 0.9, -0.005f,
		(GLfloat)gWindowWidth * 0.9, (GLfloat)gWindowHeight * 0.1, -0.005f,
	};
	GLfloat texture_vertices[8] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f
	};
	GLuint indices[4] = { 0, 1, 2, 3 };
	GLuint vertex_VBO;
	GLuint texture_VBO;
	GLuint indice_VBO;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_LIGHTING_BIT);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, gSettingBackgroundTexture);
		//Generate Buffers
		glGenBuffers(1, &vertex_VBO);
		glGenBuffers(1, &texture_VBO);
		glGenBuffers(1, &indice_VBO);

		//Set Vertex Buffer
		glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
		glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glEnableClientState(GL_VERTEX_ARRAY);

		//Set Texture Buffer
		glBindBuffer(GL_ARRAY_BUFFER, texture_VBO);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(GLfloat), texture_vertices, GL_STATIC_DRAW);
		glTexCoordPointer(2, GL_FLOAT, 0, 0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		//Set Indice Buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indice_VBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indices, GL_STATIC_DRAW);

		//Draw
		glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, (void*)(0));

		//Unbind Buffers
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		//Delete Buffers
		glDeleteBuffers(1, &vertex_VBO);
		glDeleteBuffers(1, &texture_VBO);
		glDeleteBuffers(1, &indice_VBO);
		glBindTexture(GL_TEXTURE_2D, 0);
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopClientAttrib();
	return;
}
void init(void)
{
	glewInit();

	//Load Menu Background Image
	{
		FreeImage_Initialise();
		FIBITMAP* lBitmap;
		lBitmap = FreeImage_Load(FIF_PNG, "./picture/background.png", PNG_DEFAULT);
		int width = FreeImage_GetWidth(lBitmap);
		int height = FreeImage_GetHeight(lBitmap);
		int bpp = FreeImage_GetBPP(lBitmap);
		int scan_width = FreeImage_GetPitch(lBitmap);
		GLubyte* imageRawData = new GLubyte[scan_width * height];
		FreeImage_ConvertToRawBits((BYTE*)imageRawData, lBitmap, scan_width, bpp, FreeImage_GetRedMask(lBitmap), FreeImage_GetGreenMask(lBitmap), FreeImage_GetBlueMask(lBitmap), false);
		FreeImage_Unload(lBitmap);
		glGenTextures(1, &gMenuBackgroundTexture);
		glBindTexture(GL_TEXTURE_2D, gMenuBackgroundTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, imageRawData);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete imageRawData;

		lBitmap = FreeImage_Load(FIF_PNG, "./picture/setting.png", PNG_DEFAULT);
		width = FreeImage_GetWidth(lBitmap);
		height = FreeImage_GetHeight(lBitmap);
		bpp = FreeImage_GetBPP(lBitmap);
		scan_width = FreeImage_GetPitch(lBitmap);
		imageRawData = new GLubyte[scan_width * height];
		FreeImage_ConvertToRawBits((BYTE*)imageRawData, lBitmap, scan_width, bpp, FreeImage_GetRedMask(lBitmap), FreeImage_GetGreenMask(lBitmap), FreeImage_GetBlueMask(lBitmap), false);
		FreeImage_Unload(lBitmap);
		glGenTextures(1, &gSettingBackgroundTexture);
		glBindTexture(GL_TEXTURE_2D, gSettingBackgroundTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, imageRawData);
		glBindTexture(GL_TEXTURE_2D, 0);
		delete imageRawData;
		FreeImage_DeInitialise();
	}

	//Create Printer
	gPrinter = new Printer();
	gPrinter->init();
	gPrinter->setColor(COLOR_MENU, 230, 230, 0);
	gPrinter->setColor(COLOR_MENU_SELECTED, 237, 63, 63);
	gPrinter->setColor(COLOR_SPEED, 55, 166, 28);

	//Initiallize OpenGL
	glClearColor(0.0f, 0.0f, 0.0f, 1.0);
	glClearDepth(1.0f);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	return;
}
void display(void)
{
	char* strBuffer;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	switch (gGameStatus)
	{
	case MENU:
	{
		glOrtho(0.0f, (GLfloat)gWindowWidth, 0.0f, (GLfloat)gWindowHeight, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		drawBackground();

		for (int i = 0; i < MENU_OPTIONS_NUM; i++)
		{
			if (i == gMenuSelectIndex)
			{
				strBuffer = new char[100];
				sprintf_s(strBuffer, 100, "- %s -", gMenuContent[i]);
				gPrinter->print(COLOR_MENU_SELECTED, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * (8 - i) / 10, strBuffer, Printer::eCENTER, Printer::eCENTER, 2);
				delete strBuffer;
			}
			else
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * (8 - i) / 10, gMenuContent[i], Printer::eCENTER, Printer::eCENTER, 2);
		}
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 50, 10, "Use Arrow And Enter Keys To Select Options");
		break;
	}
	case SETTING:
	{
		glOrtho(0.0f, (GLfloat)gWindowWidth, 0.0f, (GLfloat)gWindowHeight, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		drawBackground();
		drawSetting();
		if (gSettingSelectIndex == SETTING_OPTION_FULLSCREEN)
		{
			gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.2, gWindowHeight * 0.6, "Fullscreen", Printer::eLEFT, Printer::eCENTER, 2);
			if (gTempFullscreen)
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.6, "< O N  ", Printer::eCENTER, Printer::eCENTER, 2);
			else
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.6, "  OFF >", Printer::eCENTER, Printer::eCENTER, 2);
		}
		else
		{
			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.2, gWindowHeight * 0.6, "Fullscreen", Printer::eLEFT, Printer::eCENTER, 2);
			if(gTempFullscreen)
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.6, "  O N  ", Printer::eCENTER, Printer::eCENTER, 2);
			else
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.6, "  OFF  ", Printer::eCENTER, Printer::eCENTER, 2);
		}

		if (gSettingSelectIndex == SETTING_OPTION_FPS)
		{
			gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.2, gWindowHeight * 0.4, "FPS", Printer::eLEFT, Printer::eCENTER, 2);
			if (gTempMaxFPS == 60)
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.4, "< 6 0  ", Printer::eCENTER, Printer::eCENTER, 2);
			else
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.4, "  3 0 >", Printer::eCENTER, Printer::eCENTER, 2);
		}
		else
		{
			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.2, gWindowHeight * 0.4, "FPS", Printer::eLEFT, Printer::eCENTER, 2);
			if (gTempMaxFPS == 60)
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.4, "  6 0  ", Printer::eCENTER, Printer::eCENTER, 2);
			else
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.7, gWindowHeight * 0.4, "  3 0  ", Printer::eCENTER, Printer::eCENTER, 2);
		}

		if (gSettingSelectIndex == SETTING_OPTION_CONFIRM_CANCEL)
		{
			if (gTempComfirm)
			{
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.35, gWindowHeight * 0.2, "<Confirm>", Printer::eCENTER, Printer::eCENTER, 2);
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.65, gWindowHeight * 0.2, " Cancel ", Printer::eCENTER, Printer::eCENTER, 2);
			}
			else
			{
				gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.35, gWindowHeight * 0.2, " Confirm ", Printer::eCENTER, Printer::eCENTER, 2);
				gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth * 0.65, gWindowHeight * 0.2, "<Cancel>", Printer::eCENTER, Printer::eCENTER, 2);
			}
		}
		else
		{
			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.35, gWindowHeight * 0.2, " Confirm ", Printer::eCENTER, Printer::eCENTER, 2);
			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth * 0.65, gWindowHeight * 0.2, " Cancel ", Printer::eCENTER, Printer::eCENTER, 2);
		}
				
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 50, 10, "Use Arrow And Enter Keys To Select Options");
		break;
	}
	case GAME_LOADING:
		glOrtho(0.0f, (GLfloat)gWindowWidth, 0.0f, (GLfloat)gWindowHeight, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth / 2, gWindowHeight / 2, "Loading...", Printer::eCENTER, Printer::eCENTER, 3, 4);
		break;
	case ONE_PLAYER_GAME:
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(60.0f, (GLfloat)gWindowWidth / (GLfloat)gWindowHeight, 0.1, 10000.0);

		gGame->render();

		//Print Count Down or Time
		if (gGame->getStatus() == Game::eREADY)
		{
			strBuffer = new char[100];
			sprintf_s(strBuffer, 100, "%d", gGame->getCountDown());
			gPrinter->print(COLOR_MENU, gWindowWidth, gWindowHeight, gWindowWidth / 2, gWindowHeight / 2, strBuffer, Printer::eCENTER, Printer::eCENTER, 7);
			delete strBuffer;
		}
		else if (gGame->getStatus() == Game::eGAMING)
		{
			strBuffer = new char[100];
			int timeBuffer = gGame->getGameTime();
			sprintf_s(strBuffer, 100, "%02d:%02d:%02d", timeBuffer / 60000, (timeBuffer / 1000) % 60, (timeBuffer % 1000) / 10);
			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth - 20, gWindowHeight - 10, strBuffer, Printer::eRIGHT, Printer::eTOP);
			delete strBuffer;
		}
		else if (gGame->getStatus() == Game::eOVER)
		{
			strBuffer = new char[100];
			int timeBuffer = gGame->getRecordTime();
			sprintf_s(strBuffer, 100, "%02d:%02d:%02d", timeBuffer / 60000, (timeBuffer / 1000) % 60, (timeBuffer % 1000) / 10);
			gPrinter->print(COLOR_MENU_SELECTED, gWindowWidth, gWindowHeight, gWindowWidth / 2, gWindowHeight / 2, strBuffer, Printer::eCENTER, Printer::eCENTER, 5);
			delete strBuffer;

			gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth / 2, gWindowHeight * 0.2, "Press Enter To Leave", Printer::eCENTER, Printer::eTOP, 1);
		}


		//Print Front Speed
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "%d km/h", (int)(gGame->getRaceCarForwardSpeed() * 3.0));
		gPrinter->print(COLOR_SPEED, gWindowWidth, gWindowHeight, gWindowWidth / 2, 20, strBuffer, Printer::eCENTER, Printer::eBOTTOM, 2);
		delete strBuffer;

		//Print Sideways Speed
		/*strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Sideways Speed : %d", (int)gGame->getRaceCarSidewaysSpeed());
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth - 20, 20, strBuffer, Printer::eRIGHT);
		delete strBuffer;*/

		//Print Round
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Round : %d / %d", gGame->getCurrentRound(), gGame->getGoalRound());
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, gWindowWidth - 20, gWindowHeight - 30, strBuffer, Printer::eRIGHT, Printer::eTOP);
		delete strBuffer;

		//Refresh FPS Per 1/3 Second
		if (!(gFPSRefreshCount--))
		{
			gCurrentFPS = 1000.0 / gTimePass;
			gFPSRefreshCount = gMaxFPS / 3;
		}
		//Print FPS
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "FPS : %.2f", gCurrentFPS);
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 20, 20, strBuffer);
		delete strBuffer;

		//Print Help
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Press WASD To Control The Car");
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 10, gWindowHeight - 10, strBuffer, Printer::eLEFT, Printer::eTOP);
		delete strBuffer;
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Press L For Handbrake");
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 10, gWindowHeight - 30, strBuffer, Printer::eLEFT, Printer::eTOP);
		delete strBuffer;
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Press R To Go Back To Saved Point");
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 10, gWindowHeight - 50, strBuffer, Printer::eLEFT, Printer::eTOP);
		delete strBuffer;
		strBuffer = new char[100];
		sprintf_s(strBuffer, 100, "Press JK To Cheat");
		gPrinter->print(COLOR_DEFAULT, gWindowWidth, gWindowHeight, 10, gWindowHeight - 70, strBuffer, Printer::eLEFT, Printer::eTOP);
		delete strBuffer;
		break;
	}
	default:
		return;
	}
	glutSwapBuffers();
	return;
}
void reshape(int w, int h)
{
	gWindowWidth = w;
	gWindowHeight = h;
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	/*glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, (GLfloat)w, 0.0f, (GLfloat)h, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();*/
	return;
}
void keyboard(unsigned char key, int x, int y)
{
	if (key == 27)
		exit(0);

	switch (gGameStatus)
	{
	case MENU:
		switch (key)
		{
		case 13:
			switch (gMenuSelectIndex)
			{
			case MENU_OPTION_ONE_PLAYER_GAME:
				gGameStatus = GAME_LOADING;
				gGame = new Game(1, 2);
				break;
			case MENU_OPTION_SETTING:
				gGameStatus = SETTING;
				gSettingSelectIndex = 0;
				gTempFullscreen = gFullscreen;
				gTempMaxFPS = gMaxFPS;
				gTempComfirm = true;
				break;
			case MENU_OPTION_EXIT:
				exit(0);
				break;
			default:
				;
			}
			break;
		default:
			;
		}
		break;
	case SETTING:
		switch (key)
		{
		case 13:
			if(gSettingSelectIndex == SETTING_OPTION_CONFIRM_CANCEL)
			{
				if (gTempComfirm)
				{
					gFullscreen = gTempFullscreen;
					gMaxFPS = gTempMaxFPS;
					gFPSRefreshCount = gMaxFPS / 3;
					saveSetting();
					if (gFullscreen)
					{
						gWindowWidthRecover = gWindowWidth;
						gWindowHeightRecover = gWindowHeight;
						glutFullScreen();
					}
					else
					{
						glutReshapeWindow(gWindowWidthRecover, gWindowHeightRecover);
						gWindowWidth = gWindowWidthRecover;
						gWindowHeight = gWindowWidthRecover;
						glutPositionWindow(100, 50);
					}
				}
				gGameStatus = MENU;
			}
			break;
		default:
			;
		}
		break;
	case ONE_PLAYER_GAME:
		if(key == 13 && gGame->getStatus() == Game::eOVER)
		{
			DELETE(gGame);
			gGameStatus = MENU;
		}
		else
			gGame->keyboard(key);
		break;
	default:
		return;
	}
	return;
}
void specialKey(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		switch (gGameStatus)
		{
		case MENU:
			gMenuSelectIndex -= gMenuSelectIndex == 0 ? 1 - MENU_OPTIONS_NUM : 1;
			break;
		case SETTING:
			gSettingSelectIndex -= gSettingSelectIndex == 0 ? 1 - SETTING_OPTIONS_NUM : 1;
			break;
		default:
			;
		}
		break;
	case GLUT_KEY_DOWN:
		switch (gGameStatus)
		{
		case MENU:
			gMenuSelectIndex += gMenuSelectIndex == MENU_OPTIONS_NUM - 1 ? 1 - MENU_OPTIONS_NUM : 1;
			break;
		case SETTING:
			gSettingSelectIndex += gSettingSelectIndex == SETTING_OPTIONS_NUM - 1 ? 1 - SETTING_OPTIONS_NUM : 1;
		default:
			;
		}
		break;
	case GLUT_KEY_LEFT:
		switch (gGameStatus)
		{
		case SETTING:
			switch (gSettingSelectIndex)
			{
			case SETTING_OPTION_FULLSCREEN:
				if (gTempFullscreen)
					gTempFullscreen = false;
				break;
			case SETTING_OPTION_FPS:
				if (gTempMaxFPS == 60)
					gTempMaxFPS = 30;
				break;
			case SETTING_OPTION_CONFIRM_CANCEL:
				if (!gTempComfirm)
					gTempComfirm = true;
				break;
			default:
				;
			}
		default:
			;
		}
		break;
	case GLUT_KEY_RIGHT:
		switch (gGameStatus)
		{
		case SETTING:
			switch (gSettingSelectIndex)
			{
			case SETTING_OPTION_FULLSCREEN:
				if (!gTempFullscreen)
					gTempFullscreen = true;
				break;
			case SETTING_OPTION_FPS:
				if (gTempMaxFPS == 30)
					gTempMaxFPS = 60;
				break;
			case SETTING_OPTION_CONFIRM_CANCEL:
				if (gTempComfirm)
					gTempComfirm = false;
				break;
			default:
				;
			}
		default:
			;
		}
		break;
	default:
		return;
	}
	return;
}
void keyboardup(unsigned char key, int x, int y)
{
	switch (gGameStatus)
	{
	case ONE_PLAYER_GAME:
		gGame->keyboardUp(key);
		break;
	default:
		return;
	}
}
void timer(int id)
{
	//static clock_t lSavedClock;
	static clock_t lNextTimerClock = 1000 / gMaxFPS;
	switch (id)
	{
	case 1:
		gTimePass = clock() - gSavedTime;
		gSavedTime = clock();
		switch (gGameStatus)
		{
		case GAME_LOADING:
			display();
			gGame->init();
			gGameStatus = gGame->getNbPlayers() == 1 ? ONE_PLAYER_GAME : TWO_PLAYER_GAME;
			gSavedTime = clock();
			break;
		case ONE_PLAYER_GAME:
			gGame->stepPhysics(1.0f / (float)gMaxFPS);
			break;
		default:
			;
		}
		glutPostRedisplay();
		lNextTimerClock = (1000 / gMaxFPS) - (clock() - gSavedTime);
		if (lNextTimerClock <= 0)
			lNextTimerClock = 1;
		glutTimerFunc(lNextTimerClock, timer, 1);
		break;
	default:
		return;
	}
	return;
}
void cleanup(void)
{
	DELETE(gGame);
	DELETE(gPrinter);
	return;
}

int main(int argc, char** argv)
{
	loadSetting();

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(gWindowWidth, gWindowHeight);
	glutInitWindowPosition(100, 50);
	int mainHandle = glutCreateWindow("GG­¸¨®");
	glutSetWindow(mainHandle);
	if(gFullscreen)
		glutFullScreen();

	init();
	atexit(cleanup);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardup);
	glutSpecialFunc(specialKey);
	glutSetCursor(GLUT_CURSOR_NONE);
	glutTimerFunc(33, timer, 1);
	glutMainLoop();
	return 0;
}