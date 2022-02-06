//
// Isosurface extraction using marching tetrahedra
//
// Authors: Simon Green, Yury Urlasky, and Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <GL/glew.h>
#include <GL/glut.h>

#include <nvMath.h>
#include <nvGlutManipulators.h>
#include <nvSDKPath.h>

#include <Cg/cg.h>
#include <Cg/cgGL.h>

#define DEMO_PATH "src/cg_isosurf/"

using namespace nv;
using std::map;

////////////////////////////////////////////////////////////////////////////////
//
// Globals
//
////////////////////////////////////////////////////////////////////////////////

// Grid sizes (in vertices)
#define X_SIZE_LOG2		6
#define Y_SIZE_LOG2		6
#define Z_SIZE_LOG2		6

#define TOTAL_POINTS	(1<<(X_SIZE_LOG2 + Y_SIZE_LOG2 + Z_SIZE_LOG2))
#define CELLS_COUNT		(((1<<X_SIZE_LOG2) - 1) * ((1<<Y_SIZE_LOG2) - 1) * ((1<<Z_SIZE_LOG2) - 1))

#define SWIZZLE	1

GLuint position_vbo, index_vbo;

GlutExamine manipulator;

nv::SDKPath sdkPath;

enum UIOption {
    OPTION_DISPLAY_WIREFRAME,
    OPTION_USE_VERTEX_SHADER,
    OPTION_USE_GEOMETRY_SHADER,
    OPTION_USE_FRAGMENT_SHADER,
    OPTION_USE_POINTS,
    OPTION_DRAW_LAYERS,
    OPTION_ANIMATE,
    OPTION_COUNT
};
bool options[OPTION_COUNT];
map<char, UIOption> optionKeyMap;

CGcontext cg_context; 
CGprofile cg_vprofile, cg_gprofile, cg_fprofile;

CGprogram vprog[3], gprog[3], fprog;
int current_prog = 0;

float isovalue = 1.0;
float anim = 0.0;

const unsigned char edge_table[] = {
	0, 0, 0, 0,
	3, 0, 3, 1,
	2, 1, 2, 0,
	2, 0, 3, 0,
	1, 2, 1, 3,
	1, 0, 1, 2,
	1, 0, 2, 0,
	3, 0, 1, 0,
	0, 2, 0, 1,
	0, 1, 3, 1,
	0, 1, 0, 3,
	3, 1, 2, 1,
	0, 2, 1, 2,
	1, 2, 3, 2,
	0, 3, 2, 3,

	0, 0, 0, 1,
	3, 2, 0, 0,
	2, 3, 0, 0,
	2, 1, 3, 1,
	1, 0, 0, 0,
	3, 0, 3, 2,
	1, 3, 2, 3,
	2, 0, 0, 0,
	0, 3, 0, 0,
	0, 2, 3, 2,
	2, 1, 2, 3,
	0, 1, 0, 0,
	0, 3, 1, 3,
	0, 2, 0, 0,
	1, 3, 0, 0,
};

GLuint edge_tex;
GLuint volume_tex, noise_tex;

GLuint prim = GL_LINES_ADJACENCY_EXT;
int nprims = CELLS_COUNT*24;

GLuint query;

GLuint loadRawVolume( const char *filename, int w, int h, int d);
GLuint create_noise_texture(GLenum format, int w, int h, int d);

//--------------------------------------------------------------------------------------
// Fills pPos with x, y, z de-swizzled from index with bitsizes in sizeLog2
//
//  Traversing the grid in a swizzled fashion improves locality of reference,
// and this is very beneficial when sampling a texture.
//--------------------------------------------------------------------------------------
void UnSwizzle(GLuint index, GLuint sizeLog2[3], GLuint * pPos)
{

    // force index traversal to occur in 2x2x2 blocks by giving each of x, y, and z one
    // of the bottom 3 bits
	pPos[0] = index & 1;
    index >>= 1;
    pPos[1] = index & 1;
    index >>= 1;
    pPos[2] = index & 1;
    index >>= 1;

    // Treat the rest of the index like a row, collumn, depth array
    // Each dimension needs to grab sizeLog2 - 1 bits
    // This will make the blocks traverse the grid in a raster style order
    index <<= 1;
    pPos[0] = pPos[0] | (index &  ( (1 << sizeLog2[0]) - 2));
    index >>=  sizeLog2[0] - 1;
    pPos[1] = pPos[1] | ( index &  ( (1 << sizeLog2[1]) - 2));
    index >>= sizeLog2[1] - 1;
    pPos[2] = pPos[2] | ( index &  ( (1 << sizeLog2[2]) - 2));

}



#define MAKE_INDEX(x, y, z, sizeLog2)	(GLint)((x) | ((y) << sizeLog2[0]) | ((z) << (sizeLog2[0] + sizeLog2[1])))

//--------------------------------------------------------------------------------------
// Generate indices for the sampling grid in swizzled order
//--------------------------------------------------------------------------------------
void GenerateIndices(GLuint *indices, GLuint sizeLog2[3])
{
	GLuint nTotalBits = sizeLog2[0] + sizeLog2[1] + sizeLog2[2];
	GLuint nPointsTotal = 1 << nTotalBits;

	for (GLuint i = 0; i<nPointsTotal; i++) {

		GLuint pos[3];
#if SWIZZLE
		UnSwizzle(i, sizeLog2, pos);	// un-swizzle current index to get x, y, z for the current sampling point
#else
		pos[0] = i & ((1<<X_SIZE_LOG2)-1);
		pos[1] = (i >> X_SIZE_LOG2) & ((1<<Y_SIZE_LOG2)-1);
		pos[2] = (i >> (X_SIZE_LOG2+Y_SIZE_LOG2)) & ((1<<Z_SIZE_LOG2)-1);
#endif
		if (pos[0] == (1 << sizeLog2[0]) - 1 || pos[1] == (1 << sizeLog2[1]) - 1 || pos[2] == (1 << sizeLog2[2]) - 1)
			continue;	// skip extra cells


		// NOTE: order of vertices matters! important for backface culling

		// T0
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);

		// T1
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1] + 1, pos[2], sizeLog2);

		// T2
		*indices++ = MAKE_INDEX(pos[0], pos[1] + 1, pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1] + 1, pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);

		// T3
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1] + 1, pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);

		// T4
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1], pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);

		// T5
		*indices++ = MAKE_INDEX(pos[0], pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1], pos[2], sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1], pos[2] + 1, sizeLog2);
		*indices++ = MAKE_INDEX(pos[0] + 1, pos[1] + 1, pos[2] + 1, sizeLog2);
	}
}

//
// Generate vertex positions
//
//////////////////////////////////////////////////////////////////////
void GeneratePositions(float *positions, GLuint sizeLog2[3]) {
	GLuint nTotalBits = sizeLog2[0] + sizeLog2[1] + sizeLog2[2];
	GLuint nPointsTotal = 1 << nTotalBits;
	for(GLuint i=0; i<nPointsTotal; i++) {
		GLuint pos[3];
		pos[0] = i & ((1<<X_SIZE_LOG2)-1);
		pos[1] = (i >> X_SIZE_LOG2) & ((1<<Y_SIZE_LOG2)-1);
		pos[2] = (i >> (X_SIZE_LOG2+Y_SIZE_LOG2)) & ((1<<Z_SIZE_LOG2)-1);

		*positions++ = (float(pos[0]) / float(1<<X_SIZE_LOG2))*2.0-1.0;
		*positions++ = (float(pos[1]) / float(1<<Y_SIZE_LOG2))*2.0-1.0;
		*positions++ = (float(pos[2]) / float(1<<Z_SIZE_LOG2))*2.0-1.0;
		*positions++ = 1.0f;
	}
}

//
//
//
//////////////////////////////////////////////////////////////////////
void cgErrorCallback() {
    CGerror lastError = cgGetError();
    if(lastError)
    {
        printf("%s\n", cgGetErrorString(lastError));
        printf("%s\n", cgGetLastListing(cg_context));
        exit(1);
    }
}

void setParameter3i(CGprogram prog, char *name, int x, int y, int z)
{
	CGparameter param = cgGetNamedParameter(prog, name);
	if (param) {
		cgSetParameter3i(param, x, y, z);
	}
}

void setSizeParameters(CGprogram prog)
{
	setParameter3i(prog, "SizeMask", (1<<X_SIZE_LOG2)-1, (1<<Y_SIZE_LOG2)-1, (1<<Z_SIZE_LOG2)-1);
	setParameter3i(prog, "SizeShift", 0, X_SIZE_LOG2, X_SIZE_LOG2+Y_SIZE_LOG2);
}

//
//
//////////////////////////////////////////////////////////////////////
void init_opengl() {
    glewInit();

    if (!glewIsSupported(
        "GL_VERSION_2_0 "
		"GL_ARB_vertex_program "
		"GL_ARB_fragment_program "
        "GL_ARB_texture_float "
		"GL_NV_gpu_program4 " // include GL_NV_geometry_program4
        "GL_ARB_texture_rectangle "
		))
    {
        printf("Unable to load extension()s:\n  GL_ARB_vertex_program\n  GL_ARB_fragment_program\n"
               "  GL_ARB_texture_float\n  GL_NV_gpu_program4\n  GL_ARB_texture_rectangle\n  OpenGL Version 2.0\nExiting...\n");
        exit(-1);
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.2, 0.2, 1.0); //gray

    // Create Cg Context
    cg_context = cgCreateContext();
    cgSetErrorCallback(cgErrorCallback);

    // load Cg programs
    cg_vprofile = cgGLGetLatestProfile(CG_GL_VERTEX);
    cg_gprofile = cgGLGetLatestProfile(CG_GL_GEOMETRY);
	cg_fprofile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

    std::string resolved_path;
    if (sdkPath.getFilePath( DEMO_PATH "isosurf.cg", resolved_path)) {
        vprog[0] = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_vprofile, "SampleFieldVS", 0);
	    cgGLLoadProgram(vprog[0]);
	    setSizeParameters(vprog[0]);

        vprog[1] = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_vprofile, "SampleVolumeTexVS", 0);
	    cgGLLoadProgram(vprog[1]);
	    setSizeParameters(vprog[1]);

        vprog[2] = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_vprofile, "SampleProceduralVS", 0);
	    cgGLLoadProgram(vprog[2]);
	    setSizeParameters(vprog[2]);

	    gprog[0] = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_gprofile, "TessellateTetrahedraGS", 0);
	    cgGLLoadProgram(gprog[0]);

	    gprog[1] = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_gprofile, "TessellateTetrahedraNormGS", 0);
	    cgGLLoadProgram(gprog[1]);

	    gprog[2] = gprog[1];

	    fprog = cgCreateProgramFromFile(cg_context, CG_SOURCE, resolved_path.c_str(), cg_fprofile, "MetaballPS", 0);
	    cgGLLoadProgram(fprog);
    }
    else {
        fprintf( stderr, "Unable to locate shader file '%s'\n", resolved_path.c_str());
    }

	// create vertex buffers
	GLuint sizeLog2[3] = { X_SIZE_LOG2, Y_SIZE_LOG2, Z_SIZE_LOG2 };

	glGenBuffers(1, &position_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
    glBufferData(GL_ARRAY_BUFFER, TOTAL_POINTS*4*sizeof(GLfloat), 0, GL_STATIC_DRAW);
	float *positions = (float *) glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	GeneratePositions(positions, sizeLog2);
	glUnmapBuffer(GL_ARRAY_BUFFER);

    glGenBuffers(1, &index_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, CELLS_COUNT*24*sizeof(GLuint), 0, GL_STATIC_DRAW);

	GLuint *indices = (GLuint *) glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	GenerateIndices(indices, sizeLog2);
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	// create edge table texture
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &edge_tex);
	glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_tex);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, 15, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, edge_table);
//	glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB8UI_EXT, 15, 2, 0, GL_RGBA_INTEGER_EXT, GL_UNSIGNED_BYTE, edge_table);

	// load other textures

    if ( sdkPath.getFilePath( DEMO_PATH "Bucky.raw", resolved_path)) {
	    volume_tex = loadRawVolume( resolved_path.c_str(), 32, 32, 32);
    }
    else {
        fprintf( stderr, "Unable to locate file '%s'\n", DEMO_PATH "Bucky.raw");
    }
	noise_tex = create_noise_texture(GL_RGBA16F_ARB, 64, 64, 64);

    // create query object
    glGenQueries(1, &query);
}

//
//
//////////////////////////////////////////////////////////////////////
GLuint loadRawVolume( const char *filename, int w, int h, int d) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) return 0;

	GLubyte *data = new GLubyte [w*h*d];
	fread(data, sizeof(GLubyte), w*h*d, fp);
	fclose(fp);

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_LUMINANCE8, w, h, d, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
	delete [] data;

	return tex;
}

//
//
//////////////////////////////////////////////////////////////////////
float frand() {
    return rand() / (float) RAND_MAX;
}

//
//
//////////////////////////////////////////////////////////////////////
GLuint create_noise_texture(GLenum format, int w, int h, int d) {
	float *data = new float [w*h*d*4];
	float *ptr = data;
	for(int i=0; i<w*h*d; i++) {
		*ptr++ = frand()*2.0-1.0;
		*ptr++ = frand()*2.0-1.0;
		*ptr++ = frand()*2.0-1.0;
		*ptr++ = frand()*2.0-1.0;
	}

	GLuint texid;
	glGenTextures(1, &texid);
	GLenum target = GL_TEXTURE_3D;
	glBindTexture(target, texid);

	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_REPEAT);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage3D(target, 0, format, w, h, d, 0, GL_RGBA, GL_FLOAT, data);
	delete [] data;

	return texid;
}

//
// Not used for some reason
//////////////////////////////////////////////////////////////////////
void drawQuad() {
	glBegin(GL_QUADS);
	glTexCoord3f(0.0, 0.0, 0.0); glVertex2f(-1.0, -1.0);
	glTexCoord3f(1.0, 0.0, 0.0); glVertex2f(1.0, -1.0);
	glTexCoord3f(1.0, 1.0, 0.0); glVertex2f(1.0, 1.0);
	glTexCoord3f(0.0, 1.0, 0.0); glVertex2f(-1.0, 1.0);
	glEnd();
}

//
//
//////////////////////////////////////////////////////////////////////
void drawTetrahedra() {
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointerARB(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArrayARB(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
	glDrawElements(prim, nprims, GL_UNSIGNED_INT, 0);
	glDisableVertexAttribArrayARB(0);
}

//
//
//////////////////////////////////////////////////////////////////////
void drawLayers() {
    const int layers = 5;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_DEPTH_TEST);

	glColor4f(1.0, 1.0, 1.0, 0.1); //fog
	
    for(int i=0; i<layers; i++) {
		float t = i / (float)(layers+1);
		float iso = isovalue + t*0.5;
		cgSetParameter1f(cgGetNamedParameter(vprog[current_prog], "IsoValue"), iso);
		cgSetParameter1f(cgGetNamedParameter(gprog[current_prog], "IsoValue"), iso);
		drawTetrahedra();
	}

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

//
//
//////////////////////////////////////////////////////////////////////
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    manipulator.applyTransform();

    glPolygonMode( GL_FRONT_AND_BACK, options[OPTION_DISPLAY_WIREFRAME] ? GL_LINE : GL_FILL);

    //setup vertex program
    if ( options[OPTION_USE_VERTEX_SHADER]) {
		glActiveTexture(GL_TEXTURE1);
		if (current_prog==1) {
			glBindTexture(GL_TEXTURE_3D, volume_tex);
		} else if (current_prog==2) {
			glBindTexture(GL_TEXTURE_3D, noise_tex);
			cgSetParameter1f(cgGetNamedParameter(vprog[current_prog], "time"), anim);
		}
		cgSetParameter1f(cgGetNamedParameter(vprog[current_prog], "IsoValue"), isovalue);
		cgSetParameter4f(cgGetNamedParameter(vprog[current_prog], "Metaballs[1]"), 0.1 + sinf(anim)*0.5, cosf(anim)*0.5, 0.0, 0.1);
		cgGLBindProgram(vprog[current_prog]);
        cgGLEnableProfile(cg_vprofile);
	}

    //setup the GS program and state
	if ( options[OPTION_USE_GEOMETRY_SHADER]) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, edge_tex);
		cgSetParameter1f(cgGetNamedParameter(gprog[current_prog], "IsoValue"), isovalue);
		cgGLBindProgram(gprog[current_prog]);
		cgGLEnableProfile(cg_gprofile);
		prim = GL_LINES_ADJACENCY_EXT;
	}
    else {
        prim = options[OPTION_USE_POINTS] ? GL_POINTS : GL_LINES_ADJACENCY_EXT;
    }

    //setup the fragment program and state
	if ( options[OPTION_USE_FRAGMENT_SHADER]) {
		cgGLBindProgram(fprog);
		cgGLEnableProfile(cg_fprofile);
	}

	glColor3f(1.0, 1.0, 1.0);
	glBeginQuery(GL_PRIMITIVES_GENERATED_NV, query);

	if ( options[OPTION_DRAW_LAYERS]) {
		drawLayers();
	} else {
		drawTetrahedra();
	}
    glEndQuery(GL_PRIMITIVES_GENERATED_NV);

	GLuint prims_generated;
	glGetQueryObjectuiv(query, GL_QUERY_RESULT, &prims_generated);
//	printf("%d\n", prims_generated);

    //turn things off
	cgGLDisableProfile(cg_vprofile);
	cgGLDisableProfile(cg_gprofile);
	cgGLDisableProfile(cg_fprofile);
	
    glutSwapBuffers();
	glutReportErrors();
}

//
//
//////////////////////////////////////////////////////////////////////
void idle() {
	if ( options[OPTION_ANIMATE]) {
        manipulator.idle();
		anim += 0.01;
	}

    glutPostRedisplay();
}

//
//
//////////////////////////////////////////////////////////////////////
void key(unsigned char k, int x, int y) {
    k = (unsigned char)tolower(k);

	//if this is an options key, update the associated option
    if (optionKeyMap.find(k) != optionKeyMap.end())
        options[optionKeyMap[k]] = !options[optionKeyMap[k]];

	switch(k) {
	case 27:
	case 'q':
		exit(0);
		break;
	case '1':
		isovalue = 1.0;
		current_prog = k - '1';
		break;
	case '2':
		isovalue = 0.5;
		current_prog = k - '1';
		break;
	case '3':
		isovalue = 0.5;
		current_prog = k - '1';
		break;
	case '=':
	case '+':
		isovalue += 0.01;
		break;
	case '-':
		isovalue -= 0.01;
		break;
	case ']':
		nprims += 1024;
		if (nprims > CELLS_COUNT*24) nprims = CELLS_COUNT*24;
		break;
	case '[':
		nprims -= 1024;
		if (nprims < 0) nprims = 0;
		break;
	}

	printf("isovalue = %f\n", isovalue);
    
	glutPostRedisplay();
}

//
//
//////////////////////////////////////////////////////////////////////
void resize(int w, int h) {
    glViewport(0, 0, w, h);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    gluPerspective(60.0, (GLfloat)w/(GLfloat)h, 0.1, 10.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    manipulator.reshape(w, h);
}

//
//
//////////////////////////////////////////////////////////////////////
void mouse(int button, int state, int x, int y) {
    manipulator.mouse(button, state, x, y);
}

//
//
//////////////////////////////////////////////////////////////////////
void motion(int x, int y) {
    manipulator.motion(x, y);
}

//
//
//////////////////////////////////////////////////////////////////////
void main_menu(int i) {
  key((unsigned char) i, 0, 0);
}

//
//
//////////////////////////////////////////////////////////////////////
void init_menus() {
    glutCreateMenu(main_menu);
	glutAddMenuEntry("Metaballs [1]", '1');
	glutAddMenuEntry("Volume texture [2]", '2');
	glutAddMenuEntry("Procedural [3]", '3');
	glutAddMenuEntry("Increment isovalue[+]", '+');
	glutAddMenuEntry("Decrement isovalue [-]", '-');
	glutAddMenuEntry("Toggle geometry program [g]", 'g');
	glutAddMenuEntry("Toggle fragment program [f]", 'f');
	glutAddMenuEntry("Toggle points [p]", 'p');
	glutAddMenuEntry("Quit [esc]", '\033');
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

//
//
//////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(512, 512);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
	glutCreateWindow("cg_isosurf");

	init_opengl();
	init_menus();

    manipulator.setDollyActivate( GLUT_LEFT_BUTTON, GLUT_ACTIVE_CTRL);
    manipulator.setPanActivate( GLUT_LEFT_BUTTON, GLUT_ACTIVE_SHIFT);
    manipulator.setDollyPosition( -3.0f);

	glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutIdleFunc(idle);
    glutKeyboardFunc(key);
    glutReshapeFunc(resize);

 
    // set the animate option to use the space key, and enable it by default
    options[OPTION_ANIMATE] = true;
    optionKeyMap[' '] = OPTION_ANIMATE;

    // set the wireframe option to use the 'w' key, and disable it by default
    options[OPTION_DISPLAY_WIREFRAME] = false;
    optionKeyMap['w'] = OPTION_DISPLAY_WIREFRAME;

    // set the us vertex program option to use the 'v' key, and enable it by default
    options[OPTION_USE_VERTEX_SHADER] = true;
    optionKeyMap['v'] = OPTION_USE_VERTEX_SHADER;

    // set the us geometry program option to use the 'g' key, and enable it by default
    options[OPTION_USE_GEOMETRY_SHADER] = true;
    optionKeyMap['g'] = OPTION_USE_GEOMETRY_SHADER;

    // set the us fragment program option to use the 'f' key, and enable it by default
    options[OPTION_USE_FRAGMENT_SHADER] = true;
    optionKeyMap['f'] = OPTION_USE_FRAGMENT_SHADER;

    // set the option to use points to use the 'p' key, and disable it by default
    options[OPTION_USE_POINTS] = false;
    optionKeyMap['p'] = OPTION_USE_POINTS;

    // set the option to use layers to use the 'l' key, and disable it by default
    options[OPTION_DRAW_LAYERS] = false;
    optionKeyMap['l'] = OPTION_DRAW_LAYERS;


    //print the help info
    printf( "cg_isosurf - sample showing the extraction of an isosurface using marching tetrahedra\n");
    printf( "  Commands:\n");
    printf( "    q / [ESC] - Quit the application\n");
    printf( "    [SPACE]   - Toggle continuous animation\n");
    printf( "    w         - Toggle displaying wireframe\n");
    printf( "    v         - Toggle using a vertex shader\n");
    printf( "    g         - Toggle using a geometry shader\n");
    printf( "    f         - Toggle using a fragment shader\n");
    printf( "    p         - Toggle using points instead of lines with adjacency\n");
    printf( "    l         - Toggle using layers instead of tetrahedra\n");
    printf( "    +         - Increase isovalue\n");
    printf( "    -         - Decrease isovalue\n");
    printf( "    ]         - Increase number of primitives\n");
    printf( "    [         - Decrease number of primitives\n");
    printf( "    1         - Switch to the metaballs program\n");
    printf( "    2         - Switch to the \n");
    printf( "    3         - Toggle using layers instead of tetrahedra\n\n");

	glutMainLoop();

	return 0;
}
