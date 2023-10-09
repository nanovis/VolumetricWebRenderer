struct Camera {
	view : mat4x4<f32>,
	viewInv : mat4x4<f32>,
	proj : mat4x4<f32>,
	aspectRatio : f32,
	fov : f32,
}

@group(0) @binding(0) var<uniform> camera : Camera;


struct VertexOutput {
	@location(0) eye : vec3<f32>,
	@location(1) direction : vec3<f32>,
	@location(2) light : vec3<f32>,
	@location(3) lightPos : vec3<f32>,
	@location(4) fragPos : vec3<f32>,
	@location(5) tex_coords : vec2<f32>, 
	@builtin(position) position : vec4<f32>,
}

@vertex fn main(
@location(0) vertex : vec3<f32>,
@location(1) txCoord : vec2<f32>)
-> VertexOutput
{
	var output : VertexOutput;	
	
	var fov : f32 = (camera.fov / 360.0) * (2.0 * 3.141592654);
	var fl : f32 = 1.0 / tan(fov / 2.0);
	
	//var tmp = vec4<f32>(vertex.x / (1080.0 / 1920.0), vertex.y, vertex.z - fl, 1.0);
	var tmp = vec4<f32>(vertex.x / (1.0 / camera.aspectRatio), vertex.y, vertex.z - fl, 1.0);

	// vertex position
	var ve : vec3<f32> = (camera.viewInv * tmp).xyz;
	
	output.eye = (camera.viewInv * vec4<f32>(0.0, 0.0, 0.0, 1.0)).xyz;
	output.direction = ve - output.eye;	
	output.lightPos = (camera.viewInv * vec4<f32>(0.0, 1.0, 1.0, 1.0)).xyz;
	output.light = normalize(output.lightPos);
	output.fragPos = vertex;
	output.tex_coords = txCoord;	
	
	output.position = vec4<f32>(vertex, 1.0);

	//output.Position = camera.view * output.Position;
	//output.Position = camera.proj * camera.view * output.Position;

	
	return output;
}