// multiple of 16 bytes
struct Param 
{	
	param0 : vec4<f32>,
	param1 : vec4<f32>,
	param2 : vec4<f32>,
	param3 : vec4<f32>,
	param4 : vec4<f32>,
}

@group(0) @binding(0) var texSampler : sampler;
@group(0) @binding(1) var tex : texture_2d<f32>;
@group(0) @binding(2) var tex0 : texture_2d<f32>;
@group(0) @binding(3) var<uniform> param : Param;


fn toneMap(color : vec4<f32>) -> vec4<f32>
{
	var low = 0.4;
	var high = 1.0;
	var mappedColor = vec4<f32>(0.0, 0.0, 0.0, color.w);
	mappedColor.x = clamp((color.x - low) / (high - low), 0.0, 1.0);
	mappedColor.y = clamp((color.y - low) / (high - low), 0.0, 1.0);
	mappedColor.z = clamp((color.z - low) / (high - low), 0.0, 1.0);
	
	return mappedColor;
}

@fragment
fn main(
	@location(0) eye : vec3<f32>,
	@location(1) direction : vec3<f32>,
	@location(2) light : vec3<f32>,
	@location(3) lightPos : vec3<f32>,
	@location(4) fragPos : vec3<f32>,
	@location(5) tex_coords : vec2<f32>,
	@builtin(position) position : vec4<f32>
) 
-> @location(0) vec4<f32> 
{
	var coord = tex_coords;
	coord.x = tex_coords.y;
	coord.y = tex_coords.x;
	
	var color = textureSample(tex, texSampler, coord);

	coord.y = 1.0 - coord.y;
	
	var interaction = (param.param0.z == 1.0);
	
	if (interaction)
	{
		if (i32(position.x) % 2 == 0)
		{
			coord.x = coord.x + 1.0 / param.param0.x; 
		}
		if (i32(position.y) % 2 == 0)
		{
			coord.y = coord.y + 1.0 / param.param0.y; 
		}
	}
	
	var color0 = textureSample(tex0, texSampler, coord);
		
	color = color * 1.25;
	
	return color0;
	
//	return vec4<f32>(color.x + color0.x, color.y + color0.y, color.z + color0.z, color.w);
}