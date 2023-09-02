#include "stdafx.h"
#include "DrawImpl.h"



unsigned int g_pos_vertex;
Vertex g_Vertex_buff[0x80000];

void TriangleEx(const Vertex& v1, const Vertex& v2, const Vertex& v3)
{
	g_Vertex_buff[g_pos_vertex] = v1; g_pos_vertex++;
	g_Vertex_buff[g_pos_vertex] = v2; g_pos_vertex++;
	g_Vertex_buff[g_pos_vertex] = v3; g_pos_vertex++;
}

float ToRadians(float degrees)
{
	return (degrees * float(D3DX_PI / 180.f));
}

float ToDegrees(float radians)
{
	return (radians * float(180.f / D3DX_PI));
}

void Circle(float x, float y, float radius, float deg_start, float deg_end, int Sides, D3DCOLOR Color)
{
	const float Step = (float(D3DX_PI * 2.0) / Sides);

	for (auto temp(ToRadians(deg_start)); temp <= ToRadians(deg_end) - Step / 10; temp += Step) {
		float X1 = radius * cos(temp) + x;
		float Y1 = radius * sin(temp) + y;
		float X2 = radius * cos(temp + Step) + x;
		float Y2 = radius * sin(temp + Step) + y;
		TriangleEx({ x,y,Color }, { X1,Y1,Color }, { X2,Y2,Color });
	}
}

void Render(LPDIRECT3DDEVICE9 Device)
{
	Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, g_pos_vertex, g_Vertex_buff, sizeof(Vertex));
	ZeroMemory(g_Vertex_buff, sizeof(g_Vertex_buff));
	g_pos_vertex = 0u;
}
