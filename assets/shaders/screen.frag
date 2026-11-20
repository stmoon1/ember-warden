#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D scene;
uniform float hurt,bloom;
void main() {
  vec2 step=1./textureSize(scene,0);
  vec3 original=texture(scene,uv).rgb;
  vec3 light=vec3(0);
  for(int x=-2;x<=2;x++) for(int y=-2;y<=2;y++) {
    vec3 neighbour=texture(scene,uv+vec2(x,y)*step*2.).rgb;
    light+=max(vec3(0),neighbour-1.0)/25.;
  }
  vec3 mapped=original+light*bloom;
  mapped=pow(mapped/(mapped+vec3(.75)),vec3(.9));
  float vignette=1.-smoothstep(.15,.8,distance(uv,vec2(.5)));
  mapped*=.82+.18*vignette;
  mapped=mix(mapped,vec3(.65,.09,.06),hurt*(1.-vignette)*.3);
  color=vec4(mapped,1);
}
