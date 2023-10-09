[[block]] struct CubeOutput 
{
	t : f32,
	fr : f32,
};

[[block]] struct VolumeRatios
{
	ratio : array<vec3<f32>>,
};

[[block]] struct TransferFunctionColor
{
	color : array<vec4<f32>>,
};

[[block]] struct TransferFunctionRamp
{
	ramp : array<f32>,
};


[[block]] struct Param 
{
	enableEarlyRayTermination : i32,
	enableJittering : i32,
	enableAmbientOcclusion : i32,
	enableSoftShadows : i32,
	slice : f32,
	sampleRate : f32,
	aoRadius : f32,
	aoStrength : f32,
	aoNumSamples : i32,
	shadowQuality : f32,
	shadowStrength : f32,
};
[[group(0), binding(2)]] var s : sampler;
[[group(0), binding(3)]] var volume0 : texture_3d<f32>;
[[group(0), binding(4)]] var volume1 : texture_3d<f32>;
[[group(0), binding(5)]] var volume2 : texture_3d<f32>;
[[group(0), binding(6)]] var volume3 : texture_3d<f32>;
[[group(0), binding(7)]] var volume4 : texture_3d<f32>;
[[group(0), binding(8)]] var<uniform> param : Param;
[[group(0), binding(9)]] var<storage, read> transferFunctionColor: TransferFunctionColor;
[[group(0), binding(10)]] var<storage, read> transferFunctionRamp1: TransferFunctionRamp;
[[group(0), binding(11)]] var<storage, read> transferFunctionRamp2: TransferFunctionRamp;
[[group(0), binding(12)]] var<storage, read> volumeRatios: VolumeRatios;

var<private> seedGlobal : u32;
var<private> lightRadius : f32;

fn wang_hash(seedIn : u32) -> u32
{
	var seed = seedIn;
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed = seed * 9u;
    seed = seed ^ (seed >> 4u);
    seed = seed * u32(0x27d4eb2d);
    seed = seed ^ (seed >> 15u);
    return seed;
}

fn rand() -> f32
{
	seedGlobal = wang_hash(seedGlobal);
	var f = f32(seedGlobal) * (1.0 / 4294967296.0);
	return f
	/cos(f)
	;
}


fn cube(eye : vec3<f32>, dir : vec3<f32>, size : vec3<f32>) -> CubeOutput
{
	var output : CubeOutput;
	var far : f32;
	var near : f32;
	var t1 : f32;
	var t2 : f32;
	var t3 : f32;
	output.fr = 0.0;
	far = 9999.0;
	near = -9999.0;
	var i : i32 = 0;
	loop
	{
		if (i >= 3) { break; }

		if (dir[i] == 0.0)
		{
			if (eye[i]<-0.0001 || eye[i]>0.0001)
			{
				output.t = -1.0;
				return output;
			}
		}
		else
		{
			t1 = (-size[i] - eye[i]) / dir[i];
			t2 = (size[i] - eye[i]) / dir[i];
			if (t2 > t1)
			{
				t3 = t1;
				t1 = t2;
				t2 = t3;
			}

			if (t2 > near) {
				near=t2;
			}
			if (t1 < far) {
				far=t1;
			}
			if (far < 0.0) { //eye lies behind the cube and looks away
				output.t = 0.0;
				return output;
			}
			if (near > far) { //eye is drunk
				output.t = -1.0;
				return output;
			}
		}
		i = i + 1;
	}
	if (near < 0.0) //eye lies within the cube
	{
		output.fr = far;
		output.t = 0.0;
	}
	else
	{
		output.fr = far;
		output.t = near;
	}
	return output;
}

fn dataRead(pos : vec3<f32>, which : i32) -> f32
{
	var uvy = pos.xyz;
	
	//var realSize = vec3<f32>(1024.0, 1440.0, 448.0);
	var realSize = vec3<f32>(1024.0, 1440.0, 448.0);
	var realZRatio = realSize.y / realSize.z;
	var realXRatio = realSize.y / realSize.x;
	
	uvy.z = uvy.z - 0.5;
	uvy.z = uvy.z * realZRatio;
	uvy.z = uvy.z + 0.5;
	
	uvy.x = uvy.x - 0.5;
	uvy.x = uvy.x * realXRatio;
	uvy.x = uvy.x + 0.5;
	
	
	var sample : f32;
	
	if (which == 0)
	{
		sample = textureSample(volume0, s, pos).x * 1.0;
	}
	elseif (which == 1)
	{
		sample = textureSample(volume1, s, pos).x * 1.0;
	}
	elseif (which == 2)
	{
		sample = textureSample(volume2, s, pos).x * 1.0;
	}	
	elseif (which == 3)
	{
		sample = textureSample(volume3, s, pos).x * 1.0;
	}
	elseif (which == 4)
	{
		sample = textureSample(volume4, s, pos).x * 1.0;
	}	
	
	var low = transferFunctionRamp1.ramp[which];
	var high = transferFunctionRamp2.ramp[which];
	var mapped = clamp((sample - low) / (high - low), 0.0, 1.0);	
	return mapped;
}

// color transfer function
fn color_transfer(intensity : f32, which : i32) -> vec3<f32> 
{
    //var high = vec3<f32>(1.0, 0.0, 0.0);
    //var low = vec3<f32>(0.0, 0.0, 1.0);
	
	/*var low = transferFunctionColor1.color[which].xyz;
	var high = transferFunctionColor2.color[which].xyz;
			
    var alpha = (exp(intensity) - 1.0) / (exp(1.0) - 1.0);
	return vec3<f32>(intensity * high + (1.0 - intensity) * low);*/
	
	return transferFunctionColor.color[which].xyz;
	
    //return vec4<f32>(intensity * high + (1.0 - intensity) * low, alpha);
}

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
	//var size = vec3<f32>(1.0, 1.0, 1.0);
	var size = volumeRatios.ratio[0];
	
	// intialize random seed
	//seedGlobal = u32(tex_coords.x * 21032.0 * tex_coords.y * 43242.0);
	seedGlobal = u32(tex_coords.x * tex_coords.y * 1000000.0);
	
	lightRadius = 10.0;
		
	//get intersection with the bounding cube (in form of distance on the ray dir * t + eye)
	var outputCube = cube(eye, direction, size);
	
	var stepSize = 0.05;
	stepSize = stepSize / param.sampleRate;

	// ray offset jittering
	if (bool(param.enableJittering))
	{
		outputCube.t = outputCube.t + rand() * stepSize * 1.0;
	}
	
	var iterations = i32((outputCube.fr - outputCube.t) / stepSize);
		
	var src = vec4<f32>(0.0, 0.0, 0.0, 0.0);
	var dst = vec4<f32>(0.0, 0.0, 0.0, 0.0);

	// D3D backend will unroll loops and cause following error
	// error X3511: unable to unroll loop, loop does not appear to terminate in a timely manne
	// It's a known problem with the FXC compiler used by Chrome
	// https://bugs.chromium.org/p/tint/issues/detail?id=1112	
	
	// raymarching
	var max = 0.0;

	var accumA = 0.0;
	var accumC = vec3<f32>(0.0);
	
	for (var i: i32 = 0; i < iterations; i = i + 1) 
	{
		//calculate intersection along the ray
		var tmp = f32(i) * stepSize + outputCube.t;
		var isec0 = tmp * direction + eye;
		
		//transform it to match the bounding box centered on [0, 0, 0]
		var isec1 = (isec0 / size) * 0.5 + 0.5;
	
		if (isec1.x < 0.0 || isec1.x > 1.0 ||
			isec1.y < 0.0 || isec1.y > 1.0 ||
			isec1.z < 0.0 || isec1.z > 1.0)
		{
			continue;
		} 
		
		for (var which: i32 = 0; which < 3; which = which + 1) 
		{
			var mask = 0.0;
			
			var alpha = dataRead(isec1, which);
			//var color = vec3<f32>(0.8, 0.5, 0.3);
			
			alpha = alpha * dataRead(isec1, 4);
			
			var color = color_transfer(alpha, which) * 1.0;
			
			//alpha normalization based on the stepSize
			alpha = alpha * stepSize / 0.025;
			
			//alpha normalization - correct, but slower formula
			//var ds = stepSize * 20.0;
			//alpha = 1.0 - pow(1.0 - alpha, ds);

			
			if (alpha > max)
			{
				max = alpha;
			}
			
			//object space ambient occlusion
			var ao = 0.0;
			if (bool(param.enableAmbientOcclusion)) 
			{
				//var radius = 0.01;
				var radius = param.aoRadius;
				//radius = 0.1;
				
				for (var j: i32 = 0; j < param.aoNumSamples; j = j + 1)
				{
					var randomOffset = vec3<f32>(-1.0 + 2.0 * rand(), rand(), -1.0 + 2.0 * rand());
					randomOffset = randomOffset * radius;
					
					var occlusion = 1.0 - dataRead(isec1 + randomOffset, 3);
					ao = ao + occlusion;
				}			
				ao = ao / f32(param.aoNumSamples);
				//ao = (1.0 - ao);
				ao = ao * 1.0;
				//ao = (1.0 - ao * param.aoStrength) ;
				//ao = ao * 10.0 * param.aoStrength;
			}
			
		
			var shadow = 0.0;
			if (bool(param.enableSoftShadows)) 
			{
				var sTotal : f32;
				
				sTotal = 0.0;
				
				//if (accumA > 0.0 || mx.a > 0.0)
				
				var quality : i32;
				quality = 1;
				
				var samplingRadiusShadow = 0.3;
				var numSamplesShadow = f32(i32(param.shadowQuality * 15.0));
				var stepShadow = 1.0 / numSamplesShadow;
				var shadowStrength = param.shadowStrength;
				var shadowSampleCount = 0;
				
				for (var q: i32 = 0; q < quality; q = q + 1)
				{	
					for (var t: f32 = 0.05; t < 1.0; t = t + stepShadow)
					{			
						var randomDirection = vec3<f32>(-1.0 + 2.0 * rand(), -1.0 + 2.0 * rand(), -1.0 + 2.0 * rand());
						randomDirection = normalize(randomDirection) * rand();

						var halfV = normalize(lightPos + randomDirection * mix(0.0, lightRadius, t) - isec1);

						var sp = isec1 + halfV * t * samplingRadiusShadow;

						//s += dataRead(isec1 + halfV * t, which) * (0.25 - t) * 5.5;
						/*var occlusion0 = dataRead(sp, 0);
						var occlusion1 = dataRead(sp, 1);
						var occlusion2 = dataRead(sp, 2);
						var occlusion = occlusion0 + occlusion1 + occlusion2;*/
						var occlusion = 1.0 - dataRead(sp, 3);

						shadow = shadow + (occlusion) * pow(1.0 - t, 4.0); //pow(1.0 - pow(t, 2.1), 10.01);
						//s += dataRead(isec1 + halfV * t * 1.0, 1); //pow(1.0 - pow(t, 2.1), 10.01);
					}
					
					//shadow = shadow * stepShadow;			
					sTotal = sTotal + shadow;
					
					//sample count for the shadow normalization
					shadowSampleCount = shadowSampleCount + 1;
				}
				shadow = sTotal / f32(shadowSampleCount * shadowSampleCount);
				shadow = shadow * shadowStrength;
				shadow = clamp(shadow, 0.0, 1.0);				
			}
			
			
			
			color = mix(color, vec3<f32>(0.0, 0.01, 0.02), ao);
			color = mix(color, vec3<f32>(0.0, 0.01, 0.02), shadow);

			// front to back blending
			//color = color * alpha;
			//color = color * (1.0 - shadow) /** param.aoStrength*/;
			//src = vec4<f32>(color, alpha);
			//dst = (1.0 - dst.a) * src + dst;
			
			
			accumC = accumC + (1.0 - accumA) * color.xyz * alpha;
			accumA = accumA + (1.0 - accumA) * alpha;  

			// break from loop on high enough alpha value
			if (bool(param.enableEarlyRayTermination) && accumA >= 0.95)
			{
				break;
			}
		}
	}
	
	
	accumC = accumC * 1.7;
	
	if (accumC.x > 0.8) 
	{
		var it = (accumC.x - 0.8) / 0.2;
		it = it * it;
		accumC = accumC + it;
	}
	
	//var coords = vec3<f32>(tex_coords.x, tex_coords.y, 0.6);	
	//var coords = vec3<f32>(param.slice, tex_coords.x, tex_coords.y);	
	//var coords = vec3<f32>(tex_coords.x, param.slice, tex_coords.y);	
	//var coords = vec3<f32>(tex_coords.x, param.slice, tex_coords.y);		
	//var sample = 1.0 - textureSample(volume4, s, coords).x;
	
		
	//var fragColor = vec4<f32>(eye, 1.0);

	var fragColor = vec4<f32>(accumC.x, accumC.y, accumC.z, accumA);
	
	

	//fragColor = vec4<f32>(eye.x, eye.y, eye.z, 1.0);
	
	//fragColor = vec4<f32>(rand(), 0.0,0.0,1.0);
	
	//fragColor = vec4<f32>(sample, sample, sample, 1.0);
		
	//fragColor = vec4<f32>(fragPos, 1.0);
	//fragColor = vec4<f32>(direction, 1.0);
	
	/*if(outputCube.t < 0.0) {
		fragColor = vec4<f32>(1.0, 0.0, 0.0, 1.0);
	}*/
	//fragColor = vec4<f32>(sample, 0.0, 0.0, 1.0);
		
	//fragColor = vec4<f32>(max, max, max, 1.0);

	//fragColor = vec4<f32>(0.0, 1.0, 0.0, 1.0);
	return fragColor;
}