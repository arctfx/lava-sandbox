#pragma once

#include "Program.h"
#include <string>

using namespace std;

struct Texture
{
	string name;
};

//texture + shader data wrapper
class Material
{
public:
	Material();
	~Material();
};

