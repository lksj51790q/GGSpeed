#include <windows.h>
#include <GL/glew.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <FreeImage.h>
#include <cstring>

#define ALPHABET_BITMAP_WIDTH		468
#define ALPHABET_BITMAP_HEIGHT		45
#define ALPHABET_LETTER_WIDTH		8
#define ALPHABET_LETTER_HEIGHT		19
#define ALPHABET_MARGIN_WIDTH		2
#define ALPHABET_MARGIN_HEIGHT		7

#define MAX_COLOR_NUM				256

class Printer
{
public:
	enum Align
	{
		eCENTER, 
		eLEFT,
		eRIGHT,
		eTOP,
		eBOTTOM
	};
	Printer();
	~Printer();
	bool init(void);
	bool setColor(int id, int R, int G, int B);
	void print(int colorID, 
		int window_width, 
		int window_height, 
		int x, 
		int y, 
		const char* content, 
		Printer::Align horizontal = Printer::eLEFT, 
		Printer::Align vertical = Printer::eBOTTOM, 
		unsigned int scale = 1, 
		unsigned int margin = 2);
private:
	FIBITMAP*	mFiBitmap;
	GLubyte*	mAlphabet;
	GLuint		mAlphabetTexture[MAX_COLOR_NUM];
	bool		mValidID[MAX_COLOR_NUM];
};