#include "printer.h"

namespace
{
	GLfloat* generateVertices(int x, int y, int length, Printer::Align horizontal, Printer::Align vertical, unsigned int scale, unsigned int margin)
	{
		margin *= scale;
		int letter_width = ALPHABET_LETTER_WIDTH * scale;
		int letter_height = ALPHABET_LETTER_HEIGHT * scale;
		int numVertices	= length * 4;
		GLfloat *vertices = new GLfloat[numVertices * 2];

		//Transform to Left-Bottom Form
		switch (horizontal)
		{
		case Printer::eLEFT:
			break;
		case Printer::eRIGHT:
			x -= length * letter_width + (length - 1) * margin;
			break;
		case Printer::eCENTER:
			x -= length / 2 * letter_width + length / 2 * margin + (length % 2 == 0 ? -((int)margin / 2) : letter_width / 2);
			break;
		default:
			return nullptr;
		}
		switch (vertical)
		{
		case Printer::eTOP:
			y -= letter_height;
			break;
		case Printer::eBOTTOM:
			break;
		case Printer::eCENTER:
			y -= letter_height / 2;
			break;
		default:
			return nullptr;
		}

		for (int i = 0; i < length; i++)
		{
			//v1
			vertices[i * 8 + 0] = (GLfloat)x;
			vertices[i * 8 + 1] = (GLfloat)y;

			//v2
			vertices[i * 8 + 2] = (GLfloat)x;
			vertices[i * 8 + 3] = (GLfloat)(y + letter_height);

			//v3
			vertices[i * 8 + 4] = (GLfloat)(x + letter_width);
			vertices[i * 8 + 5] = (GLfloat)(y + letter_height);

			//v4
			vertices[i * 8 + 6] = (GLfloat)(x + letter_width);
			vertices[i * 8 + 7] = (GLfloat)y;

			//Next
			x += margin + letter_width;
		}

		return vertices;
	}
	GLfloat* generateTextureVertices(const char* content, int length)
	{
		int numVertices = length * 4;
		GLfloat *texture_vertices = new GLfloat[numVertices * 2];
		int x, y;
		for (int i = 0; i < length; i++)
		{
			if (content[i] < 33 || content[i] > 126)
			{
				for (int j = i * 8; j < i * 8 + 8; j++)
					texture_vertices[j] = 0.0f;
				continue;
			}
			x = ((content[i] - 33) % 47) * (ALPHABET_LETTER_WIDTH + ALPHABET_MARGIN_WIDTH);
			y = (content[i] - 33) > 46 ? 0 : (ALPHABET_LETTER_HEIGHT + ALPHABET_MARGIN_HEIGHT);
			//v1
			texture_vertices[i * 8 + 0] = (GLfloat)x / (GLfloat)ALPHABET_BITMAP_WIDTH;
			texture_vertices[i * 8 + 1] = (GLfloat)y / (GLfloat)ALPHABET_BITMAP_HEIGHT;

			//v2
			texture_vertices[i * 8 + 2] = (GLfloat)x / (GLfloat)ALPHABET_BITMAP_WIDTH;
			texture_vertices[i * 8 + 3] = (GLfloat)(y + ALPHABET_LETTER_HEIGHT) / (GLfloat)ALPHABET_BITMAP_HEIGHT;

			//v3
			texture_vertices[i * 8 + 4] = (GLfloat)(x + ALPHABET_LETTER_WIDTH) / (GLfloat)ALPHABET_BITMAP_WIDTH;
			texture_vertices[i * 8 + 5] = (GLfloat)(y + ALPHABET_LETTER_HEIGHT) / (GLfloat)ALPHABET_BITMAP_HEIGHT;

			//v4
			texture_vertices[i * 8 + 6] = (GLfloat)(x + ALPHABET_LETTER_WIDTH) / (GLfloat)ALPHABET_BITMAP_WIDTH;
			texture_vertices[i * 8 + 7] = (GLfloat)y / (GLfloat)ALPHABET_BITMAP_HEIGHT;
		}
		return texture_vertices;
	}
}

Printer::Printer()
:mFiBitmap(nullptr)
{
	for (int i = 0; i < MAX_COLOR_NUM; i++)
		mValidID[i] = false;
	FreeImage_Initialise();
	return;
}
Printer::~Printer()
{
	if (mAlphabet)
		delete mAlphabet;
	if (mFiBitmap)
		FreeImage_Unload(mFiBitmap);
	FreeImage_DeInitialise();
	return;
}

bool Printer::init(void)
{
	//Load Alphabet Image File
	mFiBitmap = FreeImage_Load(FIF_TARGA, "./picture/alphabet.tga", TARGA_DEFAULT);
	if (!mFiBitmap)
		return false;
	int width = FreeImage_GetWidth(mFiBitmap);
	int height = FreeImage_GetHeight(mFiBitmap);
	int scan_width = FreeImage_GetPitch(mFiBitmap);
	mAlphabet = new GLubyte[scan_width * height];
	FreeImage_ConvertToRawBits((BYTE*)mAlphabet, mFiBitmap, ALPHABET_BITMAP_WIDTH * 4, 32, FreeImage_GetRedMask(mFiBitmap), FreeImage_GetGreenMask(mFiBitmap), FreeImage_GetBlueMask(mFiBitmap), false);
	FreeImage_Unload(mFiBitmap);
	mFiBitmap = nullptr;

	//Generate Texture
	glGenTextures(1, &mAlphabetTexture[0]);
	glBindTexture(GL_TEXTURE_2D, mAlphabetTexture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ALPHABET_BITMAP_WIDTH, ALPHABET_BITMAP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, mAlphabet);
	
	//Unbind Texture
	glBindTexture(GL_TEXTURE_2D, 0);
	mValidID[0] = true;
	return true;
}
bool Printer::setColor(int id, int R, int G, int B)
{
	if (id > (MAX_COLOR_NUM - 1) || id < 0)
		return false;
	if (id == 0)
		return false;
	for (int y = 0; y < ALPHABET_BITMAP_HEIGHT; y++)
	{
		for (int x = 0; x < ALPHABET_BITMAP_WIDTH; x++)
		{
			mAlphabet[y * 4 * ALPHABET_BITMAP_WIDTH + x * 4 + 0] = R;
			mAlphabet[y * 4 * ALPHABET_BITMAP_WIDTH + x * 4 + 1] = G;
			mAlphabet[y * 4 * ALPHABET_BITMAP_WIDTH + x * 4 + 2] = B;
		}
	}

	//Generate Texture
	glGenTextures(1, &mAlphabetTexture[id]);
	glBindTexture(GL_TEXTURE_2D, mAlphabetTexture[id]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ALPHABET_BITMAP_WIDTH, ALPHABET_BITMAP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, mAlphabet);

	//Unbind Texture
	glBindTexture(GL_TEXTURE_2D, 0);
	mValidID[id] = true;
	return true;
}
void Printer::print(int colorID, int window_width, int window_height, int x, int y, const char* content, Printer::Align horizontal, Printer::Align vertical, unsigned int scale, unsigned int margin)
{
	if (colorID > (MAX_COLOR_NUM - 1) || colorID < 0)
		return;
	if (!mValidID[colorID])
		return;
	GLfloat		*vertices = nullptr;
	GLfloat		*texture_vertices = nullptr;
	GLuint		*indices = nullptr;
	GLuint		vertex_VBO;
	GLuint		texture_VBO;
	GLuint		indice_VBO;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glPushAttrib(GL_COLOR_BUFFER_BIT);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_LIGHTING_BIT);
		glPushMatrix();
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0.0, (GLfloat)window_width, 0.0, (GLfloat)window_height);
			glPushMatrix();
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, mAlphabetTexture[colorID]);

				//Compute Vertices Position
				int length = strlen(content);
				vertices = generateVertices(x, y, length, horizontal, vertical, scale, margin);
				texture_vertices = generateTextureVertices(content, length);
				indices = new GLuint[length * 4];
				for (int i = 0; i < length * 4; i++)
					indices[i] = i;

				//Generate Buffers
				glGenBuffers(1, &vertex_VBO);
				glGenBuffers(1, &texture_VBO);
				glGenBuffers(1, &indice_VBO);

				//Set Vertex Buffer
				glBindBuffer(GL_ARRAY_BUFFER, vertex_VBO);
				glBufferData(GL_ARRAY_BUFFER, 2 * 4 * length * sizeof(GLfloat), vertices, GL_STATIC_DRAW);
				glVertexPointer(2, GL_FLOAT, 0, 0);
				glEnableClientState(GL_VERTEX_ARRAY);

				//Set Texture Buffer
				glBindBuffer(GL_ARRAY_BUFFER, texture_VBO);
				glBufferData(GL_ARRAY_BUFFER, 2 * 4 * length * sizeof(GLfloat), texture_vertices, GL_STATIC_DRAW);
				glTexCoordPointer(2, GL_FLOAT, 0, 0);
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);

				//Set Indice Buffer
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indice_VBO);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * length * sizeof(GLuint), indices, GL_STATIC_DRAW);

				//Draw
				for(int i = 0; i < length; i++)
					glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, (void*)(4 * i * sizeof(GLuint)));

				//Unbind Buffers
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				//Delete Buffers
				glDeleteBuffers(1, &vertex_VBO);
				glDeleteBuffers(1, &texture_VBO);
				glDeleteBuffers(1, &indice_VBO);

				//Delete Data
				delete vertices;
				delete texture_vertices;
				delete indices;

				glBindTexture(GL_TEXTURE_2D, 0);
				glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopClientAttrib();
	return;
}