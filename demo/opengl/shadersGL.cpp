/*#include "shadersGL.h"

void saveScreenshotToFile(std::string filename, int _width, int _height) //CUSTOM!!!
{
	//const int numberOfPixels = windowWidth * windowHeight * 3;
	//unsigned char pixels[numberOfPixels];

	BYTE* pixels = new BYTE[4 * _width * _height]; //3

	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, g_msaaFbo); //_fbo
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadBuffer(GL_BACK); //GL_FRONT
	glReadPixels(0, 0, _width, _height, GL_BGRA, GL_UNSIGNED_BYTE, pixels); //GL_BGR_EXT

	FILE *outputFile = fopen(filename.c_str(), "w");
	short header[] = { 0, 2, 0, 0, 0, 0, (short)_width, (short)_height, 32 }; //24 bit depth for RGB

	std::fwrite(&header, sizeof(header), 1, outputFile);
	std::fwrite(pixels, 4 * _width*_height, 1, outputFile); //3
	std::fclose(outputFile);

	std::printf("Finished writing to file.\n");
}*/