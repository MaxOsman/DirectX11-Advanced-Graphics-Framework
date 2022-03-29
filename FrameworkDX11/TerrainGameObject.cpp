#include "TerrainGameObject.h"
#include <fstream>

#define GRID_SIZE 257

TerrainGameObject::TerrainGameObject() : DrawableGameObject()
{
    for (unsigned int i = 0; i < TERRAIN_TEX_SIZE; ++i)
    {
        m_pTerrainTextures[i] = nullptr;
    }
    m_pHeightTexture = nullptr;
    m_pNormalTexture = nullptr;

    heightArray = new float*[GRID_SIZE];
    for (unsigned int i = 0; i < GRID_SIZE; ++i)
    {
        heightArray[i] = new float[GRID_SIZE];
    }

    srand(time(0));
}

TerrainGameObject::~TerrainGameObject()
{
	cleanup();

    for (unsigned int i = 0; i < TERRAIN_TEX_SIZE; ++i)
    {
        if (m_pTerrainTextures[i])
            m_pTerrainTextures[i]->Release();
        m_pTerrainTextures[i] = nullptr;
    }

    if (m_pHeightTexture)
        m_pHeightTexture->Release();
    m_pHeightTexture = nullptr;

    if (m_pNormalTexture)
        m_pNormalTexture->Release();
    m_pNormalTexture = nullptr;

    for (unsigned int i = 0; i < GRID_SIZE; ++i)
    {
        delete[] heightArray[i];
    }
    delete[] heightArray;
}

int Random(int min = 0, int max = 255)
{
    return min + (rand() % int(max - min + 1));
}

void TerrainGameObject::Average(int x, int y, int sideLength)
{
    float counter = 0;
    float acc = 0;
    int halfSide = sideLength / 2;

    if (x != 0)
    {
        ++counter;
        acc += heightArray[y][x - halfSide];
    }
    if (y != 0)
    {
        ++counter;
        acc += heightArray[y - halfSide][x];
    }
    if (x != GRID_SIZE - 1)
    {
        ++counter;
        acc += heightArray[y][x + halfSide];
    }
    if (y != GRID_SIZE - 1)
    {
        ++counter;
        acc += heightArray[y + halfSide][x];
    }

    heightArray[y][x] = acc / counter - Random(-range, range);
}

void Clamp(float* value, int min, int max)
{
    if (*value < min)
        *value = min;
    else if (*value > max)
        *value = max;
}

void TerrainGameObject::DiamondStage(int sideLength)
{
    int halfSide = sideLength / 2;
    int centerX, centerY;
    for (unsigned int y = 0; y < GRID_SIZE / (sideLength - 1); ++y)
    {
        centerY = y * (sideLength - 1) + halfSide;
        for (unsigned int x = 0; x < GRID_SIZE / (sideLength - 1); ++x)
        {
            // Optimise this later!
            centerX = x * (sideLength - 1) + halfSide;

            int average = ( heightArray[x * (sideLength - 1)][y * (sideLength - 1)] +
                            heightArray[x * (sideLength - 1)][(y+1) * (sideLength - 1)] +
                            heightArray[(x+1) * (sideLength - 1)][y * (sideLength - 1)] +
                            heightArray[(x+1) * (sideLength - 1)][(y+1) * (sideLength - 1)]) / 4.0f;

            heightArray[centerX][centerY] = average + Random(-range, range);
        }
    }
}

void TerrainGameObject::SquareStage(int sideLength)
{
    int halfLength = sideLength / 2;
    for (unsigned int y = 0; y < GRID_SIZE / (sideLength - 1); ++y)
    {
        for (unsigned int x = 0; x < GRID_SIZE / (sideLength - 1); ++x)
        {
            Average(x * (sideLength - 1) + halfLength, y * (sideLength - 1), sideLength);
            Average((x+1) * (sideLength - 1), y * (sideLength - 1) + halfLength, sideLength);
            Average(x * (sideLength - 1) + halfLength, (y+1) * (sideLength - 1), sideLength);
            Average(x * (sideLength - 1), y * (sideLength - 1) + halfLength, sideLength);
        }
    }
}

void TerrainGameObject::DiamondSquareAlgorithm()
{
    heightArray[0][0] = Random(0, 32);
    heightArray[0][GRID_SIZE - 1] = Random(0, 32);
    heightArray[GRID_SIZE - 1][0] = Random(0, 32);
    heightArray[GRID_SIZE - 1][GRID_SIZE - 1] = Random(0, 32);

    int sideLength = GRID_SIZE / 2;

    DiamondStage(GRID_SIZE);
    SquareStage(GRID_SIZE);

    range /= 2;

    while (sideLength >= 2)
    {
        DiamondStage(sideLength + 1);
        SquareStage(sideLength + 1);

        sideLength /= 2;
        range /= 2;
    }

    for (unsigned int i = 0; i < GRID_SIZE; ++i)
    {
        for (unsigned int j = 0; j < GRID_SIZE; ++j)
        {
            Clamp(&heightArray[i][j], 0, 255);
        }
    }
}

HRESULT TerrainGameObject::initMesh(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pContext)
{
	vector<XMFLOAT3> positions;
	vector<XMFLOAT2> texCoords;

    //vector<float>* heights = LoadHeightMap();
    DiamondSquareAlgorithm();

    for (unsigned int i = 0; i < GRID_SIZE; ++i)
    {
        for (unsigned int j = 0; j < GRID_SIZE; ++j)
        {
            /*positions.push_back({   (float)i - GRID_WIDTH / 2,
                                    heights->at(j + i * GRID_WIDTH),
                                    (float)j - GRID_DEPTH / 2 });*/

            positions.push_back({ (float)i - GRID_SIZE / 2,
                                    heightArray[i][j],
                                    (float)j - GRID_SIZE / 2 });
        }
    }
    texCoords.push_back({ 0.0f, 0.0f });
    texCoords.push_back({ 1.0f, 0.0f });
    texCoords.push_back({ 0.0f, 1.0f });
    texCoords.push_back({ 1.0f, 1.0f });

	SimpleVertex* finalVertices = new SimpleVertex[GRID_SIZE * GRID_SIZE * 6];

    for (unsigned int i = 0; i < GRID_SIZE - 1; ++i)
    {
        for (unsigned int j = 0; j < GRID_SIZE - 1; ++j)
        {
            finalVertices[6 * (i * GRID_SIZE + j) + 0] = { positions.at(i * GRID_SIZE + j), {0,0,0}, texCoords.at(0) };
			finalVertices[6 * (i * GRID_SIZE + j) + 1] = { positions.at(i * GRID_SIZE + j + 1), {0,0,0}, texCoords.at(1) };
			finalVertices[6 * (i * GRID_SIZE + j) + 2] = { positions.at((i + 1) * GRID_SIZE + j), {0,0,0}, texCoords.at(2) };
			finalVertices[6 * (i * GRID_SIZE + j) + 3] = { positions.at((i + 1) * GRID_SIZE + j), {0,0,0}, texCoords.at(2) };
			finalVertices[6 * (i * GRID_SIZE + j) + 4] = { positions.at(i * GRID_SIZE + j + 1), {0,0,0}, texCoords.at(1) };
			finalVertices[6 * (i * GRID_SIZE + j) + 5] = { positions.at((i + 1) * GRID_SIZE + j + 1), {0,0,0}, texCoords.at(3) };
        }
    }

	CalculateModelVectors(finalVertices, GRID_SIZE * GRID_SIZE * 6);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * GRID_SIZE * GRID_SIZE * 6;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	// Create vertex buffer
	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = finalVertices;
	HRESULT hr = pd3dDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// load and setup textures
	hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\darkdirt.dds", nullptr, &m_pTerrainTextures[0]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\grass.dds", nullptr, &m_pTerrainTextures[1]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\lightdirt.dds", nullptr, &m_pTerrainTextures[2]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\snow.dds", nullptr, &m_pTerrainTextures[3]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\Terrain\\stone.dds", nullptr, &m_pTerrainTextures[4]);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\rock_height.dds", nullptr, &m_pHeightTexture);
    hr = CreateDDSTextureFromFile(pd3dDevice, L"Resources\\rock_bump.dds", nullptr, &m_pNormalTexture);
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
	texCoords.clear();
    delete[] finalVertices;
    //heights->clear();
    //delete heights;

	return hr;
}

vector<float>* TerrainGameObject::LoadHeightMap()
{
    // A height for each vertex 
    vector<unsigned char> in(GRID_SIZE * GRID_SIZE);

    // Open the file.
    ifstream inFile;
    inFile.open("Resources\\rock_height.dds", ios_base::binary);

    if (inFile)
    {
        // Read the RAW bytes.
        inFile.read((char*)&in[0], (streamsize)in.size());
        inFile.close();
    }

    vector<float>* tempHeights = new vector<float>;
    for (UINT i = 0; i < GRID_SIZE * GRID_SIZE; ++i)
    {
        tempHeights->push_back((1 - (in[i] / 255.0f)) * height);
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

    pContext->PSSetShaderResources(4, 1, &m_pTerrainTextures[4]);
    pContext->PSSetShaderResources(5, 1, &m_pTerrainTextures[1]);
    pContext->PSSetShaderResources(6, 1, &m_pTerrainTextures[2]);
    pContext->PSSetShaderResources(7, 1, &m_pTerrainTextures[3]);
    pContext->DSSetShaderResources(8, 1, &m_pHeightTexture);
    pContext->DSSetShaderResources(1, 1, &m_pNormalTexture);
    pContext->DSSetSamplers(0, 1, &m_pSamplerLinear);
    pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    pContext->Draw(GRID_SIZE * GRID_SIZE * 6, 0);
}