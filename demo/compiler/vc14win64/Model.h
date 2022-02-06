#pragma once

#include "Material.h"
#include "core/mesh.h"

//object data wrapper; incl. flex properties
class Model
{
public:
	Model(Mesh mesh, Material mat);
	~Model() = default;

private:
	Material m_material;
	Mesh m_mesh;
};

