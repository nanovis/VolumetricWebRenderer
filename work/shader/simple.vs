let PI : f32 = 3.141592653589793;

fn radians(degs : f32) -> f32 {
	return (degs * PI) / 180.0;
}

[[block]] struct Camera {
	view : mat4x4<f32>;
	viewInv : mat4x4<f32>;
	proj : mat4x4<f32>;
};
[[group(0), binding(0)]] var<uniform> camera : Camera;

[[block]] struct Model {
	
	matrix : mat4x4<f32>;
};
[[group(0), binding(1)]] var<uniform> model : Model;

struct VertexOutput {
  [[location(0)]] vCol: vec3<f32>,
  [[location(1)]] direction: vec3<f32>,
  [[location(2)]] eye: vec3<f32>,
  [[builtin(position)]] Position : vec4<f32>,
};

[[stage(vertex)]] fn main(
[[location(0)]] aPos : vec2<f32>,
[[location(1)]] aCol : vec3<f32>)
-> VertexOutput
{
	var output : VertexOutput;			
	
	var vertex = vec3<f32>(aPos.x, aPos.y, 0.0);
	var fov : f32 = (45.0 / 360.0) * (2.0 * 3.141592654);
	var fl : f32 = 1.0 / tan(fov / 2.0);
	
	var tmp = vec4<f32>(vertex.x / (1080.0 / 1920.0), vertex.y, vertex.z - fl, 1.0);

	var ve : vec3<f32> = (camera.viewInv * tmp).xyz;
	
	output.eye = (camera.viewInv * vec4<f32>(0.0, 0.0, 0.0, 1.0)).xyz;
	output.direction = ve - output.eye;	
	
	output.vCol = aCol;
	output.Position = vec4<f32>(vec3<f32>(aPos, 0.0), 1.0);
		
	return output;
}