struct Camera 
{
	model : mat4x4<f32>,
	view : mat4x4<f32>,
	viewInv : mat4x4<f32>,
	proj : mat4x4<f32>,
}

struct Model {
	matrix : mat4x4<f32>,
}

struct Param 
{
	param0 : vec4<f32>,
	param1 : vec4<f32>,
}

@group(0) @binding(0) var<uniform> camera : Camera;
@group(0) @binding(1) var<uniform> param : Param;

struct VertexOutput {
	@builtin(position) position : vec4<f32>,
	@location(0) color: vec3<f32>,
	@location(1) tex_coords : vec2<f32>,
	@location(2) world_pos : vec4<f32>, 
}

@vertex fn main(
	@location(0) vertex : vec3<f32>,
	@location(1) txCoord : vec2<f32>)
-> VertexOutput
{
	var vertex_scaled = vertex * 2.0;
	var output : VertexOutput;	
	output.position = camera.proj * camera.view * camera.model * 
						vec4<f32>(vertex_scaled, 1.0);	
	output.world_pos = camera.model * vec4<f32>(vertex_scaled, 1.0);
	output.tex_coords = txCoord;
	output.color = vec3<f32>(1.0, 0.0, 0.0);
				
	return output;
}