#version 330 core

layout (points) in;
layout (line_strip, max_vertices=2) out;

in VS_OUT 
{
	int instance;
    vec3 points[4];
}gs_in[];


vec3 hermite(float u, vec3 p0, vec3 p1, vec3 t0, vec3 t1)
{
    float H0 = (1.0 - u) * (1.0 - u) * (1.0 + 2.0 * u);
    float H1 = u * (1.0 - u) * (1.0 - u);
    float H2 = u * u * (3.0 - 2.0 * u);
    float H3 = u * u * (u - 1.0);

    vec3 p = p0 * H0 + t0 * H1 + p1 * H2 + t1 * H3;

    return p;
}


uniform int discretization;
uniform mat4 P;

void main()
{
    vec3 p0 = gs_in[0].points[0];
    vec3 t0 = gs_in[0].points[1] - p0;
    vec3 p1 = gs_in[0].points[3];
    vec3 t1 = p1 - gs_in[0].points[2];

    gl_Position = P * vec4(hermite(gs_in[0].instance/float(discretization), p0, p1, t0, t1),1);
    EmitVertex();
    gl_Position = P * vec4(hermite((gs_in[0].instance+1)/float(discretization), p0, p1, t0, t1),1);
    EmitVertex();
    EndPrimitive();
    
}