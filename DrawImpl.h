#pragma once
#define GetOriginValue(name) \
DWORD o_##name;\
g_Device->GetRenderState(name, &o_##name);

#define SetOriginValue(name) g_Device->SetRenderState(name, o_##name);

struct Vertex {
	Vertex() = default;
	Vertex(float x, float y, D3DCOLOR col) {
		this->x = x;
		this->y = y;
		this->col = col;
		z = 1.f;
		ht = 1.f;
	};
	Vertex(float x, float y, float z, float ht, D3DCOLOR col) {
		this->x = x;
		this->y = y;
		this->col = col;
		this->z = z;
		this->ht = ht;
	};
	float x, y, z, ht;
	D3DCOLOR col;
};


struct Paint {
	ImVec2 Pos;
	float Sz;
};

extern std::vector<Paint> Mask;
extern unsigned int g_pos_vertex;
extern Vertex g_Vertex_buff[0x80000];


void Circle(float x, float y, float radius, float deg_start, float deg_end, int Sides, D3DCOLOR Color);

void Render(LPDIRECT3DDEVICE9 Device);