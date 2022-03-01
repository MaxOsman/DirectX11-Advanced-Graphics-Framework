#include "TerrainGameObject.h"
#include <fstream>

#define GRID_WIDTH 32
#define GRID_DEPTH 32
#define HEIGHT_SCALE 20.0f

TerrainGameObject::TerrainGameObject() : DrawableGameObject()
{
    for (unsigned int i = 0; i < TERRAIN_TEX_SIZE; ++i)
    {
        m_TerrainTextures[i] = nullptr;
    }
}

TerrainGameObject::~TerrainGameObject()
{
	cleanup();

    for (unsigned int i = 0; i < TERRAIN_TEX_SIZE; ++i)
    {
        if (m_TerrainTextures[i])
            m_TerrainTextures[i]->Release();
        m_TerrainTextures[i] = nullptr;
    }
}

HRESULT TerrainGameObject::initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
{
	vector<XMFLOAT3> positions;
	vector<XMFLOAT3> normals;
	vector<XMFLOAT2> texCoords;
    //vector<SimpleVertex> finalVertices;

    vector<float>* heights = LoadHeightMap();

    for (unsigned int i = 0; i < GRID_WIDTH; ++i)
    {
        for (unsigned int j = 0; j < GRID_DEPTH; ++j)
        {
            positions.push_back({ 1.0f * i - GRID_WIDTH / 2,
                                heights->at(j + i * GRID_WIDTH),
                                1.0f * j - GRID_DEPTH / 2 });
        }
    }
    normals.push_back({ 0.0f, 1.0f, 0.0f });
    texCoords.push_back({ 0.0f, 0.0f });
    texCoords.push_back({ 1.0f, 0.0f });
    texCoords.push_back({ 0.0f, 1.0f });
    texCoords.push_back({ 1.0f, 1.0f });

	SimpleVertex* finalVertices = new SimpleVertex[GRID_WIDTH * GRID_DEPTH * 6];

    for (unsigned int i = 0; i < GRID_WIDTH - 1; ++i)
    {
        for (unsigned int j = 0; j < GRID_DEPTH - 1; ++j)
        {
			finalVertices[6 * (i * GRID_WIDTH + j) + 0] = { positions.at(i * GRID_WIDTH + j), normals.at(0), texCoords.at(0) };
			finalVertices[6 * (i * GRID_WIDTH + j) + 1] = { positions.at(i * GRID_WIDTH + j + 1), normals.at(0), texCoords.at(1) };
			finalVertices[6 * (i * GRID_WIDTH + j) + 2] = { positions.at((i + 1) * GRID_WIDTH + j), normals.at(0), texCoords.at(2) };
			finalVertices[6 * (i * GRID_WIDTH + j) + 3] = { positions.at((i + 1) * GRID_WIDTH + j), normals.at(0), texCoords.at(2) };
			finalVertices[6 * (i * GRID_WIDTH + j) + 4] = { positions.at(i * GRID_WIDTH + j + 1), normals.at(0), texCoords.at(1) };
			finalVertices[6 * (i * GRID_WIDTH + j) + 5] = { positions.at((i + 1) * GRID_WIDTH + j + 1), normals.at(0), texCoords.at(3) };
        }
    }

	CalculateModelVectors(finalVertices, GRID_WIDTH * GRID_DEPTH * 6);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * GRID_WIDTH * GRID_DEPTH * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	// Create vertex buffer
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = finalVertices;
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// load and setup textures
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\darkdirt.dds", nullptr, &m_TerrainTextures[0]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\grass.dds", nullptr, &m_TerrainTextures[1]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\lightdirt.dds", nullptr, &m_TerrainTextures[2]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\snow.dds", nullptr, &m_TerrainTextures[3]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\stone.dds", nullptr, &m_TerrainTextures[4]);
	if (FAILED(hr))
		return hr;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = pd3dDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);

	positions.clear();
	normals.clear();
	texCoords.clear();
    delete[] finalVertices;
    heights->clear();
    delete heights;

	return hr;
}

vector<float>* TerrainGameObject::LoadHeightMap()
{
    // A height for each vertex 
    vector<unsigned char> in(GRID_WIDTH * GRID_DEPTH);

    // Open the file.
    ifstream inFile;
    inFile.open("Resources\\Terrain\\terrain.raw", ios_base::binary);

    if (inFile)
    {
        // Read the RAW bytes.
        inFile.read((char*)&in[0], (streamsize)in.size());
        inFile.close();
    }

    vector<float>* tempHeights = new vector<float>;
    for (UINT i = 0; i < GRID_WIDTH * GRID_DEPTH; ++i)
    {
        tempHeights->push_back((in[i] / 255.0f) * HEIGHT_SCALE);
    }

    return tempHeights;
}

void TerrainGameObject::draw(ID3D11DeviceContext* pContext, ID3D11ShaderResourceView* texture)
{
    draw(pContext);
}

void TerrainGameObject::draw(ID3D11DeviceContext* pContext)
{
    // Set vertex buffer
    UINT stride = sizeof(SimpleVertex);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

    // Set index buffer
    //pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    //pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

    pContext->PSSetShaderResources(4, 1, &m_TerrainTextures[4]);
    pContext->PSSetShaderResources(5, 1, &m_TerrainTextures[1]);
    pContext->PSSetShaderResources(6, 1, &m_TerrainTextures[2]);
    pContext->PSSetShaderResources(7, 1, &m_TerrainTextures[3]);
    pContext->GSSetSamplers(0, 1, &m_pSamplerLinear);
    pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    pContext->Draw(GRID_WIDTH * GRID_DEPTH * 6, 0);
}