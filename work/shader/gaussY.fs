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

@fragment
fn main(
	@location(0) eye : vec3<f32>,
	@location(1) direction : vec3<f32>,
	@location(2) light : vec3<f32>,
	@location(3) lightPos : vec3<f32>,
	@location(4) fragPos : vec3<f32>,
	@location(5) tex_coords : vec2<f32> 
) 
-> @location(0) vec4<f32> 
{
	var maxSize = max(param.param0.x, param.param0.y);
	var amount = 20.0 / 1280.0 * maxSize;
	var size = vec2<f32>(param.param0.x, param.param0.y);
	
	var step = 1.0 / (size.y);

	var texel = vec4<f32> (0.0);
	var color = vec4<f32> (0.0);
	var w = 0.0;
	var sum = 0.0;
	
	var coord = tex_coords;
	coord.x = tex_coords.y;
	coord.y = tex_coords.x;
 
	if (amount == 0.0)
	{
		color = textureSample(tex, texSampler, coord);
		//color = texture2D(tex0, gl_TexCoord[0].st);
		sum = 1.0;
	}
	else
	{
		var iAmount = i32(amount + 1.0);
		for (var i = -iAmount; i <= iAmount; i = i+1)
		{
			//vec2 sc = gl_TexCoord[0].st + vec2(i * step, 0.0);
			var sc = coord + vec2<f32>(0.0, f32(i) * step);
			if (sc.x < 0.0 || sc.x > 1.0 || sc.y < 0.0 || sc.y > 1.0)
			{
				continue;
			}
			//texel = texture2D(tex0, sc);	
			//texel = textureSample(tex, texSampler, sc);
			texel = textureSampleLevel(tex, texSampler, sc, 0.0);
			w = exp(-pow(f32(i) / amount * 1.5, 2.0));
			//w = 1.0;	
			color = color + texel * w;	
			sum = sum + w;
		}
	}

	return color / sum;
}


