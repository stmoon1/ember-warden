#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;
layout(location=3) in float material;
uniform mat4 model,viewProjection,lightProjection;
uniform int materialOverride;
out vec3 worldPosition,worldNormal;
out vec2 textureUV;
out vec4 shadowPosition;
flat out int tile;
void main(){vec4 p=model*vec4(position,1);worldPosition=p.xyz;worldNormal=normalize(transpose(inverse(mat3(model)))*normal);textureUV=uv;tile=materialOverride<0?int(material+.5):materialOverride;shadowPosition=lightProjection*p;gl_Position=viewProjection*p;}
