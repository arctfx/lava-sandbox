#include "shader.h"

namespace OGL_Renderer
{

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Lava Flow Shader
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// vertex shader - >
///////////////////////////////////////////////////////////////////////////////////////////////
// fragment shader ////////////////////////////////////////////////////////////////////////////
const char *fragmentLavaFlow = STRINGIFY(

//uniform float time;
//uniform vec2 resolution;
uniform float time;
vec2 resolution = vec2(400, 250);

void main()
{
	// calculate normal from texture coordinates
	vec3 normal;
	normal.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(normal.xy, normal.xy);
	if (mag > 1.0) discard;   // kill pixels outside circle
	normal.z = sqrt(1.0 - mag);

	//GLSLSANDBOX code:
    vec2 position = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
	position*=17.0;
	
    //do not change
	position.y += sin(fract(time*0.1)*6.28)*3.0;
	position.x += cos(fract(time*0.1)*6.28)*3.0;
	
	float sum = 0.;
	
	vec4 param = vec4(1.0,1.0,0.32,4.0);
	
	float tt = time;//mod(time,100.0);
	
	for(float i = 0.; i <= 1.; i += 0.2)
	{
		sum += sin(cos(position.x/(param.x+i)-(tt*i*1.0))+(position.y*(param.y+i)+param.w/(param.z+i)));
		sum+= sin((time*0.4+position.x+position.y)*i);
		//sum+= sin(length(position*1.5))*1.2;
		//sum+=(0.5+sin(2.0*position.x+i*32.0)*0.5);
		position*=(1.05);
		sum+=sin(time*0.3+position.x*length(position*.024));
	}
	
	float v = sum/3.0;
	v = abs(v)+0.5;
	v = pow(v,4.0);
	float A = 0.32;
	vec3 gama = vec3(0.45, 0.23, 0.18);
	vec3 col = vec3(v*0.32,v,1.0);
	//col *= gama;
	
	gl_FragColor = vec4(col,1.0);

}
);

/*
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Point Shading
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// vertex shader //////////////////////////////////////////////////////////////////////////////
const char *vertexPointShader = "#version 130\n" STRINGIFY(

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels

uniform mat4 lightTransform; 
uniform vec3 lightDir;
uniform vec3 lightDirView;

uniform vec4 colors[8];

uniform vec4 transmission;
uniform int mode;

//in int density;
in float density;
in int phase;
in vec4 velocity;

void main()
{
    // calculate window-space point size
	vec4 viewPos = gl_ModelViewMatrix*vec4(gl_Vertex.xyz, 1.0);

	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
	gl_PointSize = -pointScale * (pointRadius / viewPos.z);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = lightTransform*vec4(gl_Vertex.xyz-lightDir*pointRadius*2.0, 1.0);
	gl_TexCoord[2] = gl_ModelViewMatrix*vec4(lightDir, 0.0);

	if (mode == 1)
	{
		// density visualization
		if (density < 0.0f)
			gl_TexCoord[3].xyz = mix(vec3(0.1, 0.1, 1.0), vec3(0.1, 1.0, 1.0), -density);
		else
			gl_TexCoord[3].xyz = mix(vec3(1.0, 1.0, 1.0), vec3(0.1, 0.2, 1.0), density);
	}
	else if (mode == 2)
	{
		gl_PointSize *= clamp(gl_Vertex.w*0.25, 0.0f, 1.0);

		gl_TexCoord[3].xyzw = vec4(clamp(gl_Vertex.w*0.05, 0.0f, 1.0));
	}
	else
	{
		gl_TexCoord[3].xyz = mix(colors[phase % 8].xyz*2.0, vec3(1.0), 0.1);
	}

	gl_TexCoord[4].xyz = gl_Vertex.xyz;
	gl_TexCoord[5].xyz = viewPos.xyz;
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
// fragment shader for rendering points as shaded spheres
const char *fragmentPointShader = STRINGIFY(
		
uniform vec3 lightDir;
uniform vec3 lightPos;
uniform float spotMin;
uniform float spotMax;
uniform int mode;

uniform sampler2DShadow shadowTex;
uniform vec2 shadowTaps[12];
uniform float pointRadius;  // point size in world space

// sample shadow map
float shadowSample()
{
	vec3 pos = vec3(gl_TexCoord[1].xyz/gl_TexCoord[1].w);
	vec3 uvw = (pos.xyz*0.5)+vec3(0.5);
	
	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;
			
	float s = 0.0;
	float radius = 0.002;
			
	for (int i=0; i < 8; i++)
	{
		s += shadow2D(shadowTex, vec3(uvw.xy + shadowTaps[i]*radius, uvw.z)).r;
	}
		
	s /= 8.0;
	return s;
}
			
float sqr(float x) { return x*x; }
		
void main()
{
	// calculate normal from texture coordinates
	vec3 normal;
    normal.xy = gl_TexCoord[0].xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
   	normal.z = sqrt(1.0-mag);
		
	if (mode == 2)
	{
		float alpha  = normal.z*gl_TexCoord[3].w;
		gl_FragColor.xyz = gl_TexCoord[3].xyz*alpha;
		gl_FragColor.w = alpha;
		return;
	}
			
    // calculate lighting
	float shadow = shadowSample();
				
	vec3 lVec = normalize(gl_TexCoord[4].xyz-(lightPos));
	vec3 lPos = vec3(gl_TexCoord[1].xyz/gl_TexCoord[1].w);
	float attenuation = max(smoothstep(spotMax, spotMin, dot(lPos.xy, lPos.xy)), 0.05);
	
	vec3 diffuse = vec3(0.9, 0.9, 0.9);
	vec3 reflectance =  gl_TexCoord[3].xyz;
				
	vec3 Lo = diffuse*reflectance*max(0.0, sqr(-dot(gl_TexCoord[2].xyz, normal)*0.5 + 0.5))*max(0.2,shadow)*attenuation;
			
	gl_FragColor = vec4(pow(Lo, vec3(1.0/2.2)), 1.0);
			
	vec3 eyePos = gl_TexCoord[5].xyz + normal*pointRadius;//*2.0;
	vec4 ndcPos = gl_ProjectionMatrix * vec4(eyePos, 1.0);
	ndcPos.z /= ndcPos.w;
	gl_FragDepth = ndcPos.z*0.5 + 0.5;
}
);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Shadowed Sphere Shading
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// vertex shader
const char *vertexShader = "#version 130\n" STRINGIFY(

uniform mat4 lightTransform; 
uniform vec3 lightDir;
uniform float bias;
uniform vec4 clipPlane;
uniform float expand;

uniform mat4 objectTransform;

void main()
{
	vec3 n = normalize((objectTransform*vec4(gl_Normal, 0.0)).xyz);
	vec3 p = (objectTransform*vec4(gl_Vertex.xyz, 1.0)).xyz;

    // calculate window-space point size
	gl_Position = gl_ModelViewProjectionMatrix * vec4(p + expand*n, 1.0);

	gl_TexCoord[0].xyz = n;
	gl_TexCoord[1] = lightTransform*vec4(p + n*bias, 1.0);
	gl_TexCoord[2] = gl_ModelViewMatrix*vec4(lightDir, 0.0);
	gl_TexCoord[3].xyz = p;
	gl_TexCoord[4] = gl_Color;
	gl_TexCoord[5] = gl_MultiTexCoord0;
	gl_TexCoord[6] = gl_SecondaryColor;
	gl_TexCoord[7] = gl_ModelViewMatrix*vec4(gl_Vertex.xyz, 1.0);

	gl_ClipDistance[0] = dot(clipPlane,vec4(gl_Vertex.xyz, 1.0));
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
// default fragment shader
const char *passThroughShader = STRINGIFY(

void main()
{
	gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
);
///////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////
// fragment shader for rendering points as shaded spheres
const char *fragmentShader = STRINGIFY(

uniform vec3 lightDir;
uniform vec3 lightPos;
uniform float spotMin;
uniform float spotMax;
uniform vec3 color;
uniform vec4 fogColor;

uniform sampler2DShadow shadowTex;
uniform vec2 shadowTaps[12];

uniform sampler2D tex;
uniform bool sky;

uniform bool grid;
uniform bool texture;

float sqr(float x) { return x*x; }

// sample shadow map
float shadowSample()
{
	vec3 pos = vec3(gl_TexCoord[1].xyz/gl_TexCoord[1].w);
	vec3 uvw = (pos.xyz*0.5)+vec3(0.5);

	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;
	
	float s = 0.0;
	float radius = 0.002;

	const int numTaps = 12;

	for (int i=0; i < numTaps; i++)
	{
		s += shadow2D(shadowTex, vec3(uvw.xy + shadowTaps[i]*radius, uvw.z)).r;
	}

	s /= numTaps;
	return s;
}

float filterwidth(vec2 v)
{
  vec2 fw = max(abs(dFdx(v)), abs(dFdy(v)));
  return max(fw.x, fw.y);
}

vec2 bump(vec2 x) 
{
	return (floor((x)/2) + 2.f * max(((x)/2) - floor((x)/2) - .5f, 0.f)); 
}

float checker(vec2 uv)
{
  float width = filterwidth(uv);
  vec2 p0 = uv - 0.5 * width;
  vec2 p1 = uv + 0.5 * width;
  
  vec2 i = (bump(p1) - bump(p0)) / width;
  return i.x * i.y + (1 - i.x) * (1 - i.y);
}

void main()
{
    // calculate lighting
	float shadow = max(shadowSample(), 0.5);

	vec3 lVec = normalize(gl_TexCoord[3].xyz-(lightPos));
	vec3 lPos = vec3(gl_TexCoord[1].xyz/gl_TexCoord[1].w);
	float attenuation = max(smoothstep(spotMax, spotMin, dot(lPos.xy, lPos.xy)), 0.05);
		
	vec3 n = gl_TexCoord[0].xyz;
	vec3 color = gl_TexCoord[4].xyz;

	if (!gl_FrontFacing)
	{
		color = gl_TexCoord[6].xyz;
		n *= -1.0f;
	}

	if (grid && (n.y >0.995))
	{
		color *= 1.0 - 0.25 * checker(vec2(gl_TexCoord[3].x, gl_TexCoord[3].z));
	}
	else if (grid && abs(n.z) > 0.995)
	{
		color *= 1.0 - 0.25 * checker(vec2(gl_TexCoord[3].y, gl_TexCoord[3].x));
	}

	if (texture)
	{
		color = texture2D(tex, gl_TexCoord[5].xy).xyz;
	}
	
	// direct light term
	float wrap = 0.0;
	vec3 diffuse = color*vec3(1.0, 1.0, 1.0)*max(0.0, (-dot(lightDir, n)+wrap)/(1.0+wrap)*shadow)*attenuation;
	
	// wrap ambient term aligned with light dir
	vec3 light = vec3(0.03, 0.025, 0.025)*1.5;
	vec3 dark = vec3(0.025, 0.025, 0.03);
	vec3 ambient = 4.0*color*mix(dark, light, -dot(lightDir, n)*0.5 + 0.5)*attenuation;

	vec3 fog = mix(vec3(fogColor), diffuse + ambient, exp(gl_TexCoord[7].z*fogColor.w));

	gl_FragColor = vec4(pow(fog, vec3(1.0/2.2)), 1.0);				
}
);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// shaded point spheres
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// vertex shader
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
// fragment shader for rendering points as shaded spheres
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Ellipsoid shaders
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const char *vertexEllipsoidDepthShader = "#version 120\n" STRINGIFY(

// rotation matrix in xyz, scale in w
attribute vec4 q1;
attribute vec4 q2;
attribute vec4 q3;

// returns 1.0 for x==0.0 (unlike glsl)
float Sign(float x) { return x < 0.0 ? -1.0: 1.0; }

bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)
{
	if (a == 0.0 && b == 0.0)
	{
		minT = maxT = 0.0;
		return false;
	}

	float discriminant = b*b - 4.0*a*c;

	if (discriminant < 0.0)
	{
		return false;
	}

	float t = -0.5*(b + Sign(b)*sqrt(discriminant));
	minT = t / a;
	maxT = c / t;

	if (minT > maxT)
	{
		float tmp = minT;
		minT = maxT;
		maxT = tmp;
	}

	return true;
}

float DotInvW(vec4 a, vec4 b) {	return a.x*b.x + a.y*b.y + a.z*b.z - a.w*b.w; }

void main()
{	
	vec3 worldPos = gl_Vertex.xyz;// - vec3(0.0, 0.1*0.25, 0.0);	// hack move towards ground to account for anisotropy

	// construct quadric matrix
	mat4 q;
	q[0] = vec4(q1.xyz*q1.w, 0.0);
	q[1] = vec4(q2.xyz*q2.w, 0.0);
	q[2] = vec4(q3.xyz*q3.w, 0.0);
	q[3] = vec4(worldPos, 1.0);

	// transforms a normal to parameter space (inverse transpose of (q*modelview)^-T)
	mat4 invClip = transpose(gl_ModelViewProjectionMatrix*q);

	// solve for the right hand bounds in homogenous clip space
	float a1 = DotInvW(invClip[3], invClip[3]);
	float b1 = -2.0f*DotInvW(invClip[0], invClip[3]);
	float c1 = DotInvW(invClip[0], invClip[0]);

	float xmin;
	float xmax;
 	solveQuadratic(a1, b1, c1, xmin, xmax);	

	// solve for the right hand bounds in homogenous clip space
	float a2 = DotInvW(invClip[3], invClip[3]);
	float b2 = -2.0f*DotInvW(invClip[1], invClip[3]);
	float c2 = DotInvW(invClip[1], invClip[1]); 

	float ymin;
	float ymax;
 	solveQuadratic(a2, b2, c2, ymin, ymax);

	gl_Position = vec4(worldPos.xyz, 1.0);
	gl_TexCoord[0] = vec4(xmin, xmax, ymin, ymax);

	// construct inverse quadric matrix (used for ray-casting in parameter space)
	mat4 invq;
	invq[0] = vec4(q1.xyz/q1.w, 0.0);
	invq[1] = vec4(q2.xyz/q2.w, 0.0);
	invq[2] = vec4(q3.xyz/q3.w, 0.0);
	invq[3] = vec4(0.0, 0.0, 0.0, 1.0);

	invq = transpose(invq);
	invq[3] = -(invq*gl_Position);

	// transform a point from view space to parameter space
	invq = invq*gl_ModelViewMatrixInverse;

	// pass down
	gl_TexCoord[1] = invq[0];
	gl_TexCoord[2] = invq[1];
	gl_TexCoord[3] = invq[2];
	gl_TexCoord[4] = invq[3];

	// compute ndc pos for frustrum culling in GS
	vec4 ndcPos = gl_ModelViewProjectionMatrix * vec4(worldPos.xyz, 1.0);
	gl_TexCoord[5] = ndcPos / ndcPos.w;
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//geometry shader ellipsoids
const char* geometryEllipsoidDepthShader = 
"#version 120\n"
"#extension GL_EXT_geometry_shader4 : enable\n"
STRINGIFY(
void main()
{
	vec3 pos = gl_PositionIn[0].xyz;
	vec4 bounds = gl_TexCoordIn[0][0];
	vec4 ndcPos = gl_TexCoordIn[0][5];

	// frustrum culling
	const float ndcBound = 1.0;
	if (ndcPos.x < -ndcBound) return;
	if (ndcPos.x > ndcBound) return;
	if (ndcPos.y < -ndcBound) return;
	if (ndcPos.y > ndcBound) return;

	float xmin = bounds.x;
	float xmax = bounds.y;
	float ymin = bounds.z;
	float ymax = bounds.w;

	// inv quadric transform
	gl_TexCoord[0] = gl_TexCoordIn[0][1];
	gl_TexCoord[1] = gl_TexCoordIn[0][2];
	gl_TexCoord[2] = gl_TexCoordIn[0][3];
	gl_TexCoord[3] = gl_TexCoordIn[0][4];

	gl_Position = vec4(xmin, ymax, 0.0, 1.0);
	EmitVertex();

	gl_Position = vec4(xmin, ymin, 0.0, 1.0);
	EmitVertex();

	gl_Position = vec4(xmax, ymax, 0.0, 1.0);
	EmitVertex();

	gl_Position = vec4(xmax, ymin, 0.0, 1.0);
	EmitVertex();
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
// pixel shader for rendering points as shaded spheres
const char *fragmentEllipsoidDepthShader = "#version 120\n" STRINGIFY(

uniform vec3 invViewport;
uniform vec3 invProjection;

float Sign(float x) { return x < 0.0 ? -1.0: 1.0; }

bool solveQuadratic(float a, float b, float c, out float minT, out float maxT)
{
	if (a == 0.0 && b == 0.0)
	{
		minT = maxT = 0.0;
		return true;
	}

	float discriminant = b*b - 4.0*a*c;

	if (discriminant < 0.0)
	{
		return false;
	}

	float t = -0.5*(b + Sign(b)*sqrt(discriminant));
	minT = t / a;
	maxT = c / t;

	if (minT > maxT)
	{
		float tmp = minT;
		minT = maxT;
		maxT = tmp;
	}

	return true;
}

float sqr(float x) { return x*x; }

void main()
{
	// transform from view space to parameter space
	mat4 invQuadric;
	invQuadric[0] = gl_TexCoord[0];
	invQuadric[1] = gl_TexCoord[1];
	invQuadric[2] = gl_TexCoord[2];
	invQuadric[3] = gl_TexCoord[3];

	vec4 ndcPos = vec4(gl_FragCoord.xy*invViewport.xy*vec2(2.0, 2.0) - vec2(1.0, 1.0), -1.0, 1.0);
	vec4 viewDir = gl_ProjectionMatrixInverse*ndcPos; 

	// ray to parameter space
	vec4 dir = invQuadric*vec4(viewDir.xyz, 0.0);
	vec4 origin = invQuadric[3];

	// set up quadratric equation
	float a = sqr(dir.x) + sqr(dir.y) + sqr(dir.z);// - sqr(dir.w);
	float b = dir.x*origin.x + dir.y*origin.y + dir.z*origin.z - dir.w*origin.w;
	float c = sqr(origin.x) + sqr(origin.y) + sqr(origin.z) - sqr(origin.w);

	float minT;
	float maxT;

	if (solveQuadratic(a, 2.0*b, c, minT, maxT))
	{
		vec3 eyePos = viewDir.xyz*minT;
		vec4 ndcPos = gl_ProjectionMatrix * vec4(eyePos, 1.0);
		ndcPos.z /= ndcPos.w;

		gl_FragColor = vec4(eyePos.z, 1.0, 1.0, 1.0);
		gl_FragDepth = ndcPos.z*0.5 + 0.5;

		return;
	}
	else
		discard;	

	gl_FragColor = vec4(0.5, 0.0, 0.0, 1.0);
}
);

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Composite shaders
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
// default vertex shader
const char* vertexPassThroughShader = STRINGIFY(

void main()
{
	gl_Position = vec4(gl_Vertex.xyz, 1.0);
	gl_TexCoord[0] = gl_MultiTexCoord0; 
}
);

const char* fragmentBlurDepthShader = 
"#extension GL_ARB_texture_rectangle : enable\n"
STRINGIFY(

uniform sampler2DRect depthTex;
uniform sampler2D thicknessTex;
uniform float blurRadiusWorld;
uniform float blurScale;
uniform float blurFalloff;
uniform vec2 invTexScale;

uniform bool debug;

float sqr(float x) { return x*x; }

void main()
{
    // eye-space depth of center sample
    float depth = texture2DRect(depthTex, gl_FragCoord.xy).x;
	float thickness = texture2D(thicknessTex, gl_TexCoord[0].xy).x;

	// hack: ENABLE_SIMPLE_FLUID
	//thickness = 0.0f;

	if (debug)
	{
		// do not blur
		gl_FragColor.x = depth;
		return;
	}

	// threshold on thickness to create nice smooth silhouettes
	if (depth == 0.0)//|| thickness < 0.02f)
	{
		gl_FragColor.x = 0.0;
		return;
	}

	
	//float dzdx = dFdx(depth);
	//float dzdy = dFdy(depth);

	// handle edge case
	//if (max(abs(dzdx), abs(dzdy)) > 0.05)
	//{
		//dzdx = 0.0;
		//dzdy = 0.0;

		//gl_FragColor.x = depth;
		//return;
	//}


	float blurDepthFalloff = 5.5;//blurFalloff*mix(4.0, 1.0, thickness)/blurRadiusWorld*0.0375;	// these constants are just a re-scaling from some known good values

	float maxBlurRadius = 5.0;
	//float taps = min(maxBlurRadius, blurScale * (blurRadiusWorld / -depth));
	//vec2 blurRadius = min(mix(0.25, 2.0/blurFalloff, thickness) * blurScale * (blurRadiusWorld / -depth) / taps, 0.15)*invTexScale;
	
	//discontinuities between different tap counts are visible. to avoid this we 
	//use fractional contributions between #taps = ceil(radius) and floor(radius) 
	float radius = min(maxBlurRadius, blurScale * (blurRadiusWorld / -depth));
	float radiusInv = 1.0/radius;
	float taps = ceil(radius);
	float frac = taps - radius;

	float sum = 0.0;
    float wsum = 0.0;
	float count = 0.0;

    for(float y=-taps; y <= taps; y += 1.0)
	{
        for(float x=-taps; x <= taps; x += 1.0)
		{
			vec2 offset = vec2(x, y);

            float sample = texture2DRect(depthTex, gl_FragCoord.xy + offset).x;

			if (sample < -10000.0*0.5)
				continue;

            // spatial domain
            float r1 = length(vec2(x, y))*radiusInv;
			float w = exp(-(r1*r1));

			//float expectedDepth = depth + dot(vec2(dzdx, dzdy), offset);

            // range domain (based on depth difference)
            float r2 = (sample - depth) * blurDepthFalloff;
            float g = exp(-(r2*r2));

			//fractional radius contributions
			float wBoundary = step(radius, max(abs(x), abs(y)));
			float wFrac = 1.0 - wBoundary*frac;

			sum += sample * w * g * wFrac;
			wsum += w * g * wFrac;
			count += g * wFrac;
        }
    }

    if (wsum > 0.0) {
        sum /= wsum;
    }

	float blend = count/sqr(2.0*radius+1.0);
	gl_FragColor.x = mix(depth, sum, blend);
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// fragment shader
const char* fragmentCompositeShader = STRINGIFY(

uniform sampler2D tex;
uniform vec2 invTexScale;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform float spotMin;
uniform float spotMax;
uniform vec4 color;
uniform float ior;

uniform vec2 clipPosToEye;

uniform sampler2D reflectTex;
uniform sampler2DShadow shadowTex;
uniform vec2 shadowTaps[12];
uniform mat4 lightTransform;

uniform sampler2D thicknessTex;
uniform sampler2D sceneTex;

uniform bool debug;

// sample shadow map
float shadowSample(vec3 worldPos, out float attenuation)
{
	// hack: ENABLE_SIMPLE_FLUID
	//attenuation = 0.0f;
	//return 0.5;

	vec4 pos = lightTransform*vec4(worldPos+lightDir*0.15, 1.0);
	pos /= pos.w;
	vec3 uvw = (pos.xyz*0.5)+vec3(0.5);

	attenuation = max(smoothstep(spotMax, spotMin, dot(pos.xy, pos.xy)), 0.05);

	// user clip
	if (uvw.x  < 0.0 || uvw.x > 1.0)
		return 1.0;
	if (uvw.y < 0.0 || uvw.y > 1.0)
		return 1.0;
	
	float s = 0.0;
	float radius = 0.002;

	for (int i=0; i < 8; i++)
	{
		s += shadow2D(shadowTex, vec3(uvw.xy + shadowTaps[i]*radius, uvw.z)).r;
	}

	s /= 8.0;
	return s;
}

vec3 viewportToEyeSpace(vec2 coord, float eyeZ)
{
	// find position at z=1 plane
	vec2 uv = (coord*2.0 - vec2(1.0))*clipPosToEye;

	return vec3(-uv*eyeZ, eyeZ);
}

vec3 srgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }
vec3 linearToSrgb(vec3 c) { return pow(c, vec3(1.0/2.2)); }

float sqr(float x) { return x*x; }
float cube(float x) { return x*x*x; }

void main()
{
	float eyeZ = texture2D(tex, gl_TexCoord[0].xy).x;

	if (eyeZ == 0.0)
		discard;

	// reconstruct eye space pos from depth
	vec3 eyePos = viewportToEyeSpace(gl_TexCoord[0].xy, eyeZ);

	// finite difference approx for normals, can't take dFdx because
	// the one-sided difference is incorrect at shape boundaries
	vec3 zl = eyePos - viewportToEyeSpace(gl_TexCoord[0].xy - vec2(invTexScale.x, 0.0), texture2D(tex, gl_TexCoord[0].xy - vec2(invTexScale.x, 0.0)).x);
	vec3 zr = viewportToEyeSpace(gl_TexCoord[0].xy + vec2(invTexScale.x, 0.0), texture2D(tex, gl_TexCoord[0].xy + vec2(invTexScale.x, 0.0)).x) - eyePos;
	vec3 zt = viewportToEyeSpace(gl_TexCoord[0].xy + vec2(0.0, invTexScale.y), texture2D(tex, gl_TexCoord[0].xy + vec2(0.0, invTexScale.y)).x) - eyePos;
	vec3 zb = eyePos - viewportToEyeSpace(gl_TexCoord[0].xy - vec2(0.0, invTexScale.y), texture2D(tex, gl_TexCoord[0].xy - vec2(0.0, invTexScale.y)).x);
	
	vec3 dx = zl;
	vec3 dy = zt;

	if (abs(zr.z) < abs(zl.z))
		dx = zr;

	if (abs(zb.z) < abs(zt.z))
		dy = zb;

	//vec3 dx = dFdx(eyePos.xyz);
	//vec3 dy = dFdy(eyePos.xyz);

	vec4 worldPos = gl_ModelViewMatrixInverse*vec4(eyePos, 1.0);
	
	float attenuation;
	float shadow = shadowSample(worldPos.xyz, attenuation);

	vec3 l = (gl_ModelViewMatrix*vec4(lightDir, 0.0)).xyz;
	vec3 v = -normalize(eyePos);
	
	vec3 n = normalize(cross(dx, dy));
	vec3 h = normalize(v + l);

	vec3 skyColor = vec3(0.1, 0.2, 0.4)*1.2;
	vec3 groundColor = vec3(0.1, 0.1, 0.2);
	
	float fresnel = 0.1 + (1.0 - 0.1)*cube(1.0-max(dot(n, v), 0.0));

	vec3 lVec = normalize(worldPos.xyz-lightPos);

	float ln = dot(l, n)*attenuation;

	vec3 rEye = reflect(-v, n).xyz;
	vec3 rWorld = (gl_ModelViewMatrixInverse*vec4(rEye, 0.0)).xyz;

	vec2 texScale = vec2(0.75, 1.0);	// to account for backbuffer aspect ratio (todo: pass in)

	float refractScale = ior*0.025;
	float reflectScale = ior*0.1;

	// attenuate refraction near ground (hack)
	refractScale *= smoothstep(0.1, 0.4, worldPos.y);
	
	vec2 refractCoord = gl_TexCoord[0].xy + n.xy*refractScale*texScale;	
	//vec2 refractCoord = gl_TexCoord[0].xy + refract(-v, n, 1.0/1.33)*refractScale*texScale;	

	// read thickness from refracted coordinate otherwise we get halos around objectsw
	float thickness = max(texture2D(thicknessTex, refractCoord).x, 0.3);

	//vec3 transmission = exp(-(vec3(1.0)-color.xyz)*thickness);
	vec3 transmission = (1.0-(1.0-color.xyz)*thickness*0.8)*color.w; 
	vec3 refract = texture2D(sceneTex, refractCoord).xyz*transmission;
	
	vec2 sceneReflectCoord = gl_TexCoord[0].xy - rEye.xy*texScale*reflectScale/eyePos.z;
	vec3 sceneReflect = (texture2D(sceneTex, sceneReflectCoord).xyz)*shadow;

	vec3 planarReflect = texture2D(reflectTex, gl_TexCoord[0].xy).xyz;
	planarReflect = vec3(0.0);

	// fade out planar reflections above the ground
	vec3 reflect = mix(planarReflect, sceneReflect, smoothstep(0.05, 0.3, worldPos.y)) + mix(groundColor, skyColor, smoothstep(0.15, 0.25, rWorld.y)*shadow);
	
	// lighting
	vec3 diffuse = color.xyz*mix(vec3(0.29, 0.379, 0.59), vec3(1.0), (ln*0.5 + 0.5)*max(shadow, 0.4))*(1.0-color.w);
	vec3 specular = vec3(1.2*pow(max(dot(h, n), 0.0), 400.0));

	gl_FragColor.xyz = diffuse + (mix(refract, reflect, fresnel) + specular)*color.w;
	gl_FragColor.w = 1.0;

	if (debug)
		gl_FragColor = vec4(n*0.5 + vec3(0.5), 1.0);

	// write valid z
	vec4 clipPos = gl_ProjectionMatrix*vec4(0.0, 0.0, eyeZ, 1.0);
	clipPos.z /= clipPos.w;

	gl_FragDepth = clipPos.z*0.5 + 0.5;
}
);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Diffuse Shading
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// vertex diffuse shader
const char *vertexDiffuseShader = STRINGIFY(

uniform float pointRadius;  // point size in world space
uniform float pointScale;   // scale to calculate size in pixels
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform mat4 lightTransform;
uniform float spotMin;
uniform float spotMax;
uniform vec4 color;


void main()
{
	vec3 worldPos = gl_Vertex.xyz;// - vec3(0.0, 0.1*0.25, 0.0);	// hack move towards ground to account for anisotropy;
	vec4 eyePos = gl_ModelViewMatrix * vec4(worldPos, 1.0);

	gl_Position = gl_ProjectionMatrix * eyePos;
	//gl_Position.z -= 0.0025;	// bias above fluid surface

    // calculate window-space point size
	gl_PointSize = pointRadius * (pointScale / gl_Position.w);

	gl_TexCoord[0] = gl_MultiTexCoord0;    
	gl_TexCoord[1] = vec4(worldPos, gl_Vertex.w);
	gl_TexCoord[2] = eyePos;

	gl_TexCoord[3].xyz = gl_ModelViewMatrix*vec4(gl_MultiTexCoord1.xyz, 0.0);
	gl_TexCoord[4].xyzw = color;

	// hack to color different emitters 
	if (gl_MultiTexCoord1.w == 2.0)
		gl_TexCoord[4].xyzw = vec4(0.85, 0.65, 0.65, color.w);
	else if (gl_MultiTexCoord1.w == 1.0)
		gl_TexCoord[4].xyzw = vec4(0.65, 0.85, 0.65, color.w);

	// compute ndc pos for frustrum culling in GS
	vec4 ndcPos = gl_ModelViewProjectionMatrix * vec4(worldPos.xyz, 1.0);
	gl_TexCoord[5] = ndcPos / ndcPos.w;
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
// geometry shader for diffuse rendering
const char *geometryDiffuseShader = "#version 120\n" "#extension GL_EXT_geometry_shader4 : enable\n" STRINGIFY(

uniform float pointScale;  // point size in world space
uniform float motionBlurScale;
uniform float diffusion;
uniform vec3 lightDir;

void main()
{
	vec4 ndcPos = gl_TexCoordIn[0][5];

	// frustrum culling
	const float ndcBound = 1.0;
	if (ndcPos.x < -ndcBound) return;
	if (ndcPos.x > ndcBound) return;
	if (ndcPos.y < -ndcBound) return;
	if (ndcPos.y > ndcBound) return;

	float velocityScale = 1.0;

	vec3 v = gl_TexCoordIn[0][3].xyz*velocityScale;
	vec3 p = gl_TexCoordIn[0][2].xyz;
		
	// billboard in eye space
	vec3 u = vec3(0.0, pointScale, 0.0);
	vec3 l = vec3(pointScale, 0.0, 0.0);
	
	// increase size based on life
	float lifeFade = mix(1.0f+diffusion, 1.0, min(1.0, gl_TexCoordIn[0][1].w*0.25f));
	u *= lifeFade;
	l *= lifeFade;

	//lifeFade = 1.0;

	float fade = 1.0/(lifeFade*lifeFade);
	float vlen = length(v)*motionBlurScale;

	if (vlen > 0.5)
	{
		float len = max(pointScale, vlen*0.016);
		fade = min(1.0, 2.0/(len/pointScale));

		u = normalize(v)*max(pointScale, vlen*0.016);	// assume 60hz
		l = normalize(cross(u, vec3(0.0, 0.0, -1.0)))*pointScale;
	}	
	
	{
		gl_TexCoord[1] = gl_TexCoordIn[0][1];	// vertex world pos (life in w)
		gl_TexCoord[2] = gl_TexCoordIn[0][2];	// vertex eye pos
		gl_TexCoord[3] = gl_TexCoordIn[0][3];	// vertex velocity in view space
		gl_TexCoord[3].w = fade;
		gl_TexCoord[4] = gl_ModelViewMatrix*vec4(lightDir, 0.0);
		gl_TexCoord[4].w = gl_TexCoordIn[0][3].w; // attenuation
		gl_TexCoord[5].xyzw = gl_TexCoordIn[0][4].xyzw;	// color

		float zbias = 0.0f;//0.00125*2.0;

        gl_TexCoord[0] = vec4(0.0, 1.0, 0.0, 0.0);
        gl_Position = gl_ProjectionMatrix * vec4(p + u - l, 1.0);
		gl_Position.z -= zbias;
        EmitVertex();
		
		gl_TexCoord[0] = vec4(0.0, 0.0, 0.0, 0.0);
        gl_Position = gl_ProjectionMatrix * vec4(p - u - l, 1.0);
		gl_Position.z -= zbias;
        EmitVertex();

		gl_TexCoord[0] = vec4(1.0, 1.0, 0.0, 0.0);
        gl_Position = gl_ProjectionMatrix * vec4(p + u + l, 1.0);
		gl_Position.z -= zbias;
        EmitVertex();

		gl_TexCoord[0] = vec4(1.0, 0.0, 0.0, 0.0);
        gl_Position = gl_ProjectionMatrix * vec4(p - u + l, 1.0);
		gl_Position.z -= zbias;
        EmitVertex();
    }
}
);

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
//fragment diffuse shader
const char *fragmentDiffuseShader = STRINGIFY(

float sqr(float x) { return x*x; }
float cube(float x) { return x*x*x; }

uniform sampler2D depthTex;
uniform sampler2D noiseTex;
uniform vec2 invViewport;
uniform vec4 color;
uniform bool front;
uniform bool shadow;

//uniform sampler2DShadow shadowTex;
uniform sampler2D shadowTex;
uniform vec2 shadowTaps[12];
uniform mat4 lightTransform;
uniform vec3 lightDir;
uniform float inscatterCoefficient;
uniform float outscatterCoefficient;

void main()
{
	float attenuation = gl_TexCoord[4].w;
	float lifeFade = min(1.0, gl_TexCoord[1].w*0.125);

    // calculate normal from texture coordinates
    vec3 normal;
    normal.xy = gl_TexCoord[0].xy*vec2(2.0, 2.0) + vec2(-1.0, -1.0);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0) discard;   // kill pixels outside circle
   	normal.z = 1.0-mag;

	float velocityFade = gl_TexCoord[3].w;
	float alpha = lifeFade*velocityFade*sqr(normal.z);

	gl_FragColor = alpha;
}
);

}

*/


}