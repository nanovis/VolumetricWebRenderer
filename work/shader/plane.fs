struct Param 
{
	param0 : vec4<f32>,
	enableVolumes : vec4<f32>,
	clippingPlaneOrigin : vec4<f32>,
	clippingPlaneNormal : vec4<f32>,
	volumeSize : vec4<f32>,
	screenSize : vec4<f32>,
}

struct TransferFunctionColor
{
	color : array<vec4<f32>>,
}

struct TransferFunctionRamp
{
	ramp : array<f32>,
}

struct VolumeRatios
{
	ratio : array<vec3<f32>>,
}

@group(0) @binding(1) var<uniform> param : Param;
@group(0) @binding(2) var s : sampler;
@group(0) @binding(3) var volume : texture_3d<f32>;
@group(0) @binding(4) var<storage, read> transferFunctionColor: TransferFunctionColor;
@group(0) @binding(5) var<storage, read> transferFunctionRamp1: TransferFunctionRamp;
@group(0) @binding(6) var<storage, read> transferFunctionRamp2: TransferFunctionRamp;
@group(0) @binding(7) var<storage, read> volumeRatios: VolumeRatios;
@group(0) @binding(8) var depthTex : texture_depth_2d;
@group(0) @binding(9) var annotation_tex : texture_3d<f32>;

fn dataRead(pos : vec3<f32>, which : i32) -> vec4<f32>
{
	var mapped : vec4<f32>;
	var sample : vec4<f32>;
	if(which == 0) 
	{
		sample = textureSampleLevel(volume, s, pos, 0.0);	
		if(bool(param.enableVolumes.x)) 
		{
			var low0 = transferFunctionRamp1.ramp[0];
			var high0 = transferFunctionRamp2.ramp[0];
			mapped.x = clamp((sample.x - low0) / (high0 - low0), 0.0, 1.0);	
		}

		if(bool(param.enableVolumes.y)) 
		{	
			var low1 = transferFunctionRamp1.ramp[1];
			var high1 = transferFunctionRamp2.ramp[1];
			mapped.y = clamp((sample.y - low1) / (high1 - low1), 0.0, 1.0);	
		}

		if(bool(param.enableVolumes.z)) 
		{
			var low2 = transferFunctionRamp1.ramp[2];
			var high2 = transferFunctionRamp2.ramp[2];
			mapped.z = clamp((sample.z - low2) / (high2 - low2), 0.0, 1.0);	
		}

		if(bool(param.enableVolumes.w)) 
		{
			var low3 = transferFunctionRamp1.ramp[3];
			var high3 = transferFunctionRamp2.ramp[3];
			//mapped.w = clamp((sample.w - low3) / (high3 - low3), 0.0, 1.0);
			mapped.w = sample.w;
		}
	} else if(which == 1) 
	{
		mapped = textureSampleLevel(annotation_tex, s, pos, 0.0);
		// we only show the annotation of the currently selected annotation volume
		if(param.param0.z == 0.0) {
			mapped = vec4<f32>(mapped.x, 0.0, 0.0, 0.0);
		}
		if(param.param0.z == 1.0) {
			mapped = vec4<f32>(0.0, mapped.y, 0.0, 0.0);
		}
		if(param.param0.z == 2.0) {
			mapped = vec4<f32>(0.0, 0.0, mapped.z, 0.0);
		}
		if(param.param0.z == 3.0) {
			mapped = vec4<f32>(0.0, 0.0, 0.0, mapped.w);
		}		
	}
	
	return mapped;
}

fn color_transfer(which : i32) -> vec3<f32> 
{
	return transferFunctionColor.color[which].xyz;
}

fn outline(coords : vec3<f32>) -> vec4<f32> 
{
	var color = vec4<f32>(0.0, 0.0, 0.0, 0.0);
	var epsilon = vec3<f32>(0.0005, 0.0005, 0.0005);
	epsilon = epsilon * 20.0;
	epsilon = epsilon / volumeRatios.ratio[0];
	
	if (abs(-1.0 + 2.0 * coords.x) > 1.0 - epsilon.x ||
		abs(-1.0 + 2.0 * coords.y) > 1.0 - epsilon.y ||
		abs(-1.0 + 2.0 * coords.z) > 1.0 - epsilon.z)
	{
		var ex = (1.0 - abs(-1.0 + 2.0 * coords.x)) / epsilon.x;
		var ey = (1.0 - abs(-1.0 + 2.0 * coords.y)) / epsilon.y;
		var ez = (1.0 - abs(-1.0 + 2.0 * coords.z)) / epsilon.z;

		ex = 0.5 - 0.5 * cos(3.141592654 * 2.0 * ex);
		ey = 0.5 - 0.5 * cos(3.141592654 * 2.0 * ey);
		ez = 0.5 - 0.5 * cos(3.141592654 * 2.0 * ez);

		var alpha = ex * ey * ez;
		alpha = clamp(alpha, 0.0, 1.0);
		//color = vec4<f32>(1.0, 0.0, 0.0, alpha);
		color = vec4<f32>(1.0, 0.0, 0.0, 1.0);
	}
	return color;
}

@fragment
fn main(
	@location(0) color: vec3<f32>,
	@location(1) tex_coords : vec2<f32>,
	@location(2) world_pos : vec4<f32>,
	@builtin(position) position : vec4<f32>) 
-> @location(0) vec4<f32> 
{
	var volumeRatio = volumeRatios.ratio[0];
	var fragColor = vec4<f32>(0.12, 0.12, 0.12, 0.6);
	
	var coords = world_pos.xyz;
	coords = coords / volumeRatio;
	coords = coords * 0.5 + 0.5;
	
	var zo = param.param0.x;
		
	var annotationColorA = vec4<f32>(1.0, 0.0, 0.0, 1.0);
	var annotationColorB = vec4<f32>(0.0, 1.0, 0.0, 1.0);
	var annotationColorC = vec4<f32>(0.0, 0.0, 1.0, 1.0);
	var annotationColorD = vec4<f32>(1.0, 0.0, 1.0, 1.0);
	
	var e = 0.005;
	if(abs(-1.0 + 2.0 * coords.x) >= 1.0 - e ||
		abs(-1.0 + 2.0 * coords.y) >= 1.0 - e ||
		abs(-1.0 + 2.0 * coords.z) >= 1.0 - e)
	{
		discard;
	}
	
	var outline_color = outline(coords) * pow(1.0 - zo, 0.4);
	
	
	fragColor = fragColor * pow(1.0 - zo, 0.4);
			
	var masks = dataRead(coords, 0);
	var annotations = dataRead(coords, 1);
	
	var annotation = textureSampleLevel(annotation_tex, s, coords, 0.0);
		
	var result_color = masks.x * color_transfer(0) +
				 masks.y * color_transfer(1) +
				 masks.z * color_transfer(2);
				 
	if(param.param0.w == 1.0) {
		result_color = vec3<f32>(masks.w);
	}
	
				 
	var stripes : array<vec3<f32>, 3>;
	var stripeStrength = 0.2;
	
	for (var i: i32 = 0; i < 3; i = i + 1)
	{
		stripes[i] = color_transfer(i);
		if (sin((position.x + position.y) * 100.0) > 0.0)
		{
			stripes[i] = mix(stripes[i], vec3<f32>(0.0, 0.0, 0.0), stripeStrength);
		}
		else
		{
			stripes[i] = mix(stripes[i], vec3<f32>(1.0, 1.0, 1.0), stripeStrength);
		}
	}
	
	for (var i: i32 = 0; i < 3; i = i + 1)
	{
		result_color = mix(result_color, stripes[i], annotations[i]);		
	}		
					
	
	var sample = vec4<f32>(result_color.x, result_color.y, result_color.z, 1.0);
	
	
	if(param.param0.w == 1.0 || length(sample.xyz) > 0.0001) 
	{
		sample.w = 1.0;
		fragColor = sample;
	}

	var coord = vec2<i32>(position.xy);
	var depth = textureLoad(depthTex, coord, 0);
	
	var depth_range_near = 0.0;
	var depth_range_far = 1.0;
	
	var frag_depth = position.z;
	//var frag_depth = position.z / position.w *
	//			(depth_range_far - depth_range_near) * 0.5 +
	//			(depth_range_near + depth_range_far) * 0.5; 
	
	if(zo == 0.0 && frag_depth > depth + 0.000001 && depth != 0.0) 
	{
		discard;
	}
		
	var result = fragColor;
	result.x = result.x + outline_color.x;
	result.y = result.y + outline_color.y;
	result.z = result.z + outline_color.z;
	//result.xyz = result.xyz + annotationColor.xyz + outline_color.xyz;
	
	return result;
	//return fragColor + outline_color + annotation * annotationColor;
	//return fragColor + outline_color + vec4<f32>(annotation, annotation,annotation, 1.0) * annotationColor;
}