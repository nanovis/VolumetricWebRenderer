struct VertexOutput {
	@location(0) eye : vec3<f32>,
	@location(1) direction : vec3<f32>,
	@location(2) light : vec3<f32>,
	@location(3) lightPos : vec3<f32>,
	@location(4) fragPos : vec3<f32>,
	@location(5) tex_coords : vec2<f32>,
	@builtin(position) Position : vec4<f32>,
}


@vertex fn main(
	@location(0) vertex : vec3<f32>,
	@location(1) txCoord : vec2<f32>
)-> VertexOutput
{
	var output : VertexOutput;
	
	output.tex_coords = txCoord;
	output.fragPos = vertex;
	output.Position = vec4<f32>(vertex, 1.0);

	return output;	
}