#ifndef RENDER_H
#define RENDER_H

#include "../../external/glad/include/glad/glad.h"
#include "../../external/SDL2-2.0.4/include/SDL.h"
#include <string>
#include <iostream>

#include <SOIL.h>

namespace OGL_Renderer {

	//namespace {
	static int g_msaaSamples;
	static GLuint g_msaaFbo;
	static GLuint g_msaaColorBuf;
	static GLuint g_msaaDepthBuf;

	static int g_screenWidth;
	static int g_screenHeight;

	static SDL_Window* g_window;

	static float g_spotMin = 0.5f;
	static float g_spotMax = 1.0f;
	static float g_shadowBias = 0.05f;
	//} // anonymous namespace
}

//CUSTOM!!!
static void saveScreenshotToFile(std::string filename, int _width, int _height)
{
	//vector<unsigned char> buf;
	//.resize(_width * _height * 3);

	BYTE* pixels = new BYTE[3 * _width * _height]; //4

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	//glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE, &buf[0]);

	/*int err = SOIL_save_image
	(
		"img.bmp",
		SOIL_SAVE_TYPE_BMP,
		_width, _height, 3,
		pixels
	);*/

	/*//const int numberOfPixels = windowWidth * windowHeight * 3;
	//unsigned char pixels[numberOfPixels];

	BYTE* pixels = new BYTE[4 * _width * _height]; //4

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, OGL_Renderer::g_msaaFbo); //_fbo
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_FRONT); //GL_FRONT
	glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE, pixels); //GL_BGR_EXT

	FILE *outputFile = fopen(filename.c_str(), "w");
	short header[] = { 0, 2, 0, 0, 0, 0, (short)_width, (short)_height, 32 }; //24 bit depth for RGB

	std::fwrite(&header, sizeof(header), 1, outputFile);
	std::fwrite(pixels, 1, 4 * _width*_height, outputFile); //3
	std::fclose(outputFile);

	std::printf("Finished writing to file.\n");
	std::cout << _width << " " << _height;*/
}

#endif