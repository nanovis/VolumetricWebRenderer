[[block]] struct Param 
{
	paramF1 : f32,
	paramF2 : f32,
	paramF3 : f32,
	screenSize : vec2<f32>,
	paramV1 : vec3<f32>,
	paramV2 : vec3<f32>,
	paramV3 : vec3<f32>,
};

[[group(0), binding(0)]] var texSampler : sampler;
[[group(0), binding(1)]] var tex : texture_2d<f32>;
[[group(0), binding(2)]] var<uniform> param : Param;

[[stage(fragment)]]
fn main(
[[location(0)]] eye : vec3<f32>,
[[location(1)]] direction : vec3<f32>,
[[location(2)]] light : vec3<f32>,
[[location(3)]] lightPos : vec3<f32>,
[[location(4)]] fragPos : vec3<f32>,
[[location(5)]] tex_coords : vec2<f32> 
) 
-> [[location(0)]] vec4<f32> 
{
	var fragColor = vec4<f32>(tex_coords.y, tex_coords.x, 0.0 , 1.0);
	
	var coord = tex_coords;
	coord.x = tex_coords.y;
	coord.y = tex_coords.x;
	
	var color = vec3<f32> (0.0, 0.0, 0.0);
	var offsetScale = 0.01;
	var count = 0.0;
	
	for (var xOffset: f32 = -1.0; xOffset < 2.0; xOffset = xOffset + 1.0) 
	{
		for (var yOffset: f32 = -1.0; yOffset < 2.0; yOffset = yOffset + 1.0) 
		{
			var coordOffset = coord;
			coordOffset.x = coordOffset.x + xOffset * offsetScale;
			coordOffset.y = coordOffset.y + yOffset * offsetScale;
			//color = color + textureSample(tex, texSampler, coordOffset).xyz;
			color = color + textureSampleLevel(tex, texSampler, coordOffset, 0.0).xyz;
			count = count + 1.0;
		}
	}
	color = color / count;
	//fragColor = textureSample(tex, texSampler, coord);
	fragColor = textureSampleLevel(tex, texSampler, coord, 0.0);
	fragColor = vec4<f32>(color.x, color.y, color.z, 1.0);
	
	return fragColor;
}