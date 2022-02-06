#include "shaders.h"

namespace OGL_Renderer
{

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// shaded point spheres
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// point depth vertex shader
const char *vertexPointDepthShader = STRINGIFY(

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels

void main()
{
    // calculate window-space point size
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
	gl_PointSize = pointScale * (pointRadius / gl_Position.w);

	gl_TexCoord[0] = gl_MultiTexCoord0;    
	gl_TexCoord[1] = gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0);
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
// point depth fragment shader
const char *fragmentPointDepthShader = STRINGIFY(

uniform float pointRadius;  // point size in world space

void main()
{
    // calculate normal from texture coordinates
    vec3 normal;
    normal.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
   	normal.z = sqrt(1.0-mag);

	vec3 eyePos = gl_TexCoord[1].xyz + normal*pointRadius*2.0;
	vec4 ndcPos = gl_ProjectionMatrix * vec4(eyePos, 1.0);
	ndcPos.z /= ndcPos.w;

	gl_FragColor = vec4(eyePos.z, 1.0, 1.0, 1.0);
	gl_FragDepth = ndcPos.z*0.5 + 0.5;
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
// pixel shader for rendering points density
const char *fragmentPointThicknessShader = STRINGIFY(

void main()
{
    // calculate normal from texture coordinates
    vec3 normal;
    normal.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
   	normal.z = sqrt(1.0-mag);

	gl_FragColor = vec4(normal.z*0.005);	
}
);

}