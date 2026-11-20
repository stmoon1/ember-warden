#version 330 core
in vec3 worldPosition, worldNormal;
in vec2 textureUV;
in vec4 shadowPosition;
flat in int tile;
out vec4 color;
uniform sampler2D atlas, shadowMap;
uniform vec3 tint, eye;
uniform float glow, flash, materialDetail;
uniform bool lighting;
uniform vec3 lampPosition[12], lampColor[12];

float shadow(vec3 normal) {
  vec3 point=shadowPosition.xyz/shadowPosition.w*.5+.5;
  if(point.z>1 || point.x<0 || point.y<0 || point.x>1 || point.y>1) return 0;
  float bias=max(.0008,.003*(1-dot(normal,normalize(vec3(-.5,.9,-.3)))));
  vec2 texel=1.0/textureSize(shadowMap,0);
  float covered=0;
  for(int x=-1;x<=1;x++) for(int y=-1;y<=1;y++)
    covered+=point.z-bias>texture(shadowMap,point.xy+vec2(x,y)*texel).r?1:0;
  return covered/9.;
}
void main() {
  const vec3 palette[16]=vec3[16](
    vec3(.25,.33,.22),vec3(.29,.23,.19),vec3(.38,.41,.42),vec3(.29,.35,.39),
    vec3(.36,.26,.18),vec3(.35,.30,.24),vec3(.74,.39,.14),vec3(.29,.34,.38),
    vec3(.15,.28,.24),vec3(.23,.29,.35),vec3(.22,.67,.71),vec3(.40,.24,.20),
    vec3(.33,.40,.42),vec3(.61,.57,.46),vec3(.49,.25,.57),vec3(.16,.34,.39));
  float grid=(tile==0 || tile==1)?16.:32.;
  vec2 clustered=(floor(clamp(textureUV,0.,.999)*grid)+.5)/grid;
  vec2 atlasUV=(vec2(tile%4,tile/4)+.04+clustered*.92)/4.;
  vec3 textured=textureGrad(atlas,atlasUV,dFdx(textureUV)*.23,dFdy(textureUV)*.23).rgb;
  float distanceToEye=length(worldPosition-eye);
  float detail=materialDetail*mix(1.,.2,smoothstep(4.,28.,distanceToEye));
  vec3 base=mix(palette[clamp(tile,0,15)],textured,detail)*tint;
  vec3 normal=normalize(worldNormal);
  float diffuse=max(0.,dot(normal,normalize(vec3(-.5,.9,-.3))));
  diffuse=floor(diffuse*5.)/5.;
  float coverage=lighting?shadow(normal):0.;
  vec3 illumination=vec3(.34,.41,.47)+vec3(.70,.68,.60)*diffuse*(1.-coverage*.65);
  if(lighting) for(int i=0;i<12;i++) {
    vec3 delta=lampPosition[i]-worldPosition;
    float attenuation=max(0.,1.-length(delta)/7.);
    illumination+=lampColor[i]*attenuation*attenuation*(.3+.7*max(0.,dot(normal,normalize(delta))));
  }
  float materialGlow=(tile==6||tile==10||tile==14)?.4:tile==11?.2:0.;
  vec3 result=base*illumination+base*(glow+materialGlow);
  result=mix(result,vec3(1.35),flash*.6);
  float fog=1.-exp(-pow(distanceToEye*.017,2.));
  color=vec4(mix(result,vec3(.065,.10,.14),clamp(fog,0.,.8)),1);
}
