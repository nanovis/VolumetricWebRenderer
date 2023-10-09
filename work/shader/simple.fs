[[fragment]] 
fn main(
	[[location(0)]] vCol: vec3<f32>,
	[[location(1)]] direction: vec3<f32>,
	[[location(2)]] eye: vec3<f32>
) 
-> [[location(0)]] vec4<f32> 
{
	var fragColor = vec4<f32>(vCol, 1.0);
	fragColor = vec4<f32>(eye, 1.0);
	return fragColor;
}