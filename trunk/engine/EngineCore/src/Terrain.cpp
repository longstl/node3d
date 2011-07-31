#include "Terrain.h"
#include <fstream>
#include "RenderSystem.h"
#include "Graphics.h"

CTerrain::CTerrain():
m_pVB(NULL),
m_pIB(NULL),
m_pGrassVB(NULL),
m_pGrassIB(NULL),
m_bShowBox(false),
m_nLightMap(0),
m_uShowTileIBCount(0)
{
	m_Tiles[0] = "Terrain.00";
	m_Tiles[1] = "Terrain.01";
	m_Tiles[2] = "Terrain.02";
	m_Tiles[3] = "Terrain.03";
	m_Tiles[4] = "Terrain.04";
	m_Tiles[5] = "Terrain.05";
	m_Tiles[6] = "Terrain.06";
	m_Tiles[7] = "Terrain.07";
	m_Tiles[8] = "Terrain.08";
}

CTerrain::~CTerrain()
{
	S_DEL(m_pVB);
	S_DEL(m_pIB);
	S_DEL(m_pGrassVB);
	S_DEL(m_pGrassIB);
}

void CTerrain::setTileMaterial(int nTileID, const std::string& strMaterialName)
{
	m_Tiles[nTileID] = strMaterialName;
}

CMaterial& CTerrain::getMaterial(const char* szMaterialName)
{
	return GetRenderSystem().getMaterialMgr().getItem(szMaterialName);
}

void CTerrain::updateIB()
{
	if (m_pTerrainData==NULL)
	{
		return;
	}
	m_RenderTileSubsLayer[0].clear();
	m_RenderTileSubsLayer[1].clear();
	// 写IB
	if (m_pIB)
	{
		for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); ++it)
		{
			for (size_t y = (*it)->box.vMin.z; y<(*it)->box.vMax.z; ++y)
			{
				for (size_t x = (*it)->box.vMin.x; x<(*it)->box.vMax.x; ++x)
				{
					TerrainCell& cell = *m_pTerrainData->getCell(x,y);
					if ((cell.uAttribute&0x8)==0)// 透明
					{
						unsigned long uIndex = y*(m_pTerrainData->getWidth()+1)+x;
						const unsigned char uTileID0 = cell.uTileID[0];
						if (uTileID0!=0xFF)// 透明
						{
							m_RenderTileSubsLayer[0][uTileID0].myVertexIndex(uIndex);
						}
						const unsigned char uTileID1 = cell.uTileID[1];
						if (uTileID1!=0xFF)// 透明
						{
							m_RenderTileSubsLayer[1][uTileID1].myVertexIndex(uIndex);
						}
					}
				}
			}
		}

		// resize grass vertex.
		//size_t uGrassCount = m_RenderTileSubsLayer[0][0].icount/6;
		//if (uGrassCount>0)
		{
			size_t uGrassCount = 0;
			for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); ++it)
			{
				for (size_t y = (*it)->box.vMin.z; y<(*it)->box.vMax.z; ++y)
				{
					for (size_t x = (*it)->box.vMin.x; x<(*it)->box.vMax.x; ++x)
					{
						if (m_pTerrainData->hasGrass(x,y))
						{
							uGrassCount++;
						}
					}
				}
			}
			if (uGrassCount)
			{
				CreateGrassVB(uGrassCount);
				CreateGrassIB(uGrassCount);
				m_GrassSub.vbase=0;	// base vertex
				m_GrassSub.vstart=0;	// first vertex
				m_GrassSub.vcount=m_pGrassVB->getVerticesCount();	// num vertices
				m_GrassSub.istart=0;	// first index
				m_GrassSub.icount=m_pGrassIB->getIndexesCount();	// num indices
			}
		}

		m_uShowTileIBCount = 0;
		for (int nLayer=0; nLayer<2; nLayer++)
		{
			for (auto it = m_RenderTileSubsLayer[nLayer].begin(); it!=m_RenderTileSubsLayer[nLayer].end(); it++)
			{
				it->second.istart = m_uShowTileIBCount;
				m_uShowTileIBCount += it->second.icount;
				it->second.icount = 0;
				it->second.vcount=it->second.vcount-it->second.vstart+1+(m_pTerrainData->getWidth()+1)+1;
			}
		}

		unsigned long uTempVertexXCount = m_pTerrainData->getWidth()+1;
		unsigned short* index = (unsigned short*)m_pIB->lock(0, m_uShowTileIBCount*sizeof(unsigned short),CHardwareBuffer::HBL_NO_OVERWRITE);
		for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); it++)
		{
			for (size_t y = (*it)->box.vMin.z; y<(*it)->box.vMax.z; ++y)
			{
				for (size_t x = (*it)->box.vMin.x; x<(*it)->box.vMax.x; ++x)
				{
					TerrainCell& cell = *m_pTerrainData->getCell(x,y);
					if ((cell.uAttribute&0x8)==0)// 透明
					{
						unsigned long uIndex = m_pTerrainData->getVertexIndex(x,y);
						for (int nLayer=0; nLayer<2; nLayer++)
						{
							const unsigned char uTileID = cell.uTileID[nLayer];
							TerrainSub& sub = m_RenderTileSubsLayer[nLayer][uTileID];
							if (uTileID!=0xFF)
							{
								// 2	 3
								//	*---*
								//	| / |
								//	*---*
								// 0	 1
								unsigned short* p = index+sub.istart+sub.icount;
								sub.icount+=6;
								*p = uIndex;
								p++;
								*p = uIndex+uTempVertexXCount;
								p++;
								*p = uIndex+uTempVertexXCount+1;
								p++;

								*p = uIndex;
								p++;
								*p = uIndex+uTempVertexXCount+1;
								p++;
								*p = uIndex+1;
								p++;
							}
						}
					}
				}
			}
		}
		m_pIB->unlock();
	}
}

void CTerrain::updateVB(int nBeginX, int nBeginY, int nEndX, int nEndY)
{
	if (m_pVB==NULL || m_pTerrainData==NULL)
	{
		return;
	}
	unsigned long uOffset	= sizeof(TerrainVertex)*m_pTerrainData->getVertexIndex(nBeginX,nBeginY);
	unsigned long uSize	= sizeof(TerrainVertex)*(m_pTerrainData->getVertexIndex(nEndX,nEndY)+1)-uOffset;
	if (uSize>0)
	{
		TerrainVertex*	pV = (TerrainVertex*)m_pVB->lock(uOffset, uSize, CHardwareBuffer::HBL_NO_OVERWRITE/*CHardwareBuffer::HBL_NORMAL*/);
		for (int y = nBeginY; y <= nEndY; ++y)
		{
			for (int x = nBeginX; x <= nEndX; ++x)
			{
				m_pTerrainData->getVertexByCell(x,y,*pV);
				pV++;
			}
		}
		m_pVB->unlock();
	}
}


void CTerrain::CreateGrassVB(size_t uGrassCount)
{
	if (m_pTerrainData==NULL)
	{
		return;
	}
	S_DEL(m_pGrassVB);
	m_pGrassVB = GetRenderSystem().GetHardwareBufferMgr().CreateVertexBuffer(uGrassCount*4,sizeof(TerrainVertex),CHardwareBuffer::HBU_DYNAMIC);
	if (m_pGrassVB)
	{
		TerrainVertex*	pV = (TerrainVertex*)m_pGrassVB->lock(CHardwareBuffer::HBL_NORMAL);
		for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); it++)
		{
			for (size_t y = (*it)->box.vMin.z; y<(*it)->box.vMax.z; ++y)
			{
				for (size_t x = (*it)->box.vMin.x; x<(*it)->box.vMax.x; ++x)
				{
					if (m_pTerrainData->hasGrass(x,y))
					{
						m_pTerrainData->getGrassVertexByCell(x,y,pV);
						pV+=4;
					}
				}
			}
		}
		m_pGrassVB->unlock();
	}
}

void CTerrain::CreateGrassIB(size_t uGrassCount)
{
	if (m_pTerrainData==NULL)
	{
		return;
	}
	S_DEL(m_pGrassIB);
	m_pGrassIB = GetRenderSystem().GetHardwareBufferMgr().CreateIndexBuffer(uGrassCount*6*sizeof(unsigned short),CHardwareIndexBuffer::IT_16BIT,CHardwareBuffer::HBU_WRITE_ONLY);
	unsigned short* index = (unsigned short*)m_pGrassIB->lock(0, uGrassCount*6*sizeof(unsigned short),CHardwareBuffer::HBL_NO_OVERWRITE);
	int i=0;
	for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); it++)
	{
		for (size_t y = (*it)->box.vMin.z; y<(*it)->box.vMax.z; ++y)
		{
			for (size_t x = (*it)->box.vMin.x; x<(*it)->box.vMax.x; ++x)
			{
				if (m_pTerrainData->hasGrass(x,y))
				{
					// 1	 2
					//	*---*
					//	| / |
					//	*---*
					// 0	 3
					*index = i;
					index++;
					*index = i+1;
					index++;
					*index = i+2;
					index++;

					*index = i;
					index++;
					*index = i+2;
					index++;
					*index = i+3;
					index++;
					i+=4;
				}
			}
		}
	}
	m_pGrassIB->unlock();
}

void CTerrain::setLightMapTexture(const std::string& strFilename)
{
	GetRenderSystem().GetTextureMgr().del(m_nLightMap);
	m_nLightMap = GetRenderSystem().GetTextureMgr().RegisterTexture(strFilename);
}

bool CTerrain::init(void* pData)
{
	m_pTerrainData = (CTerrainData*)pData;
	m_RenderTileSubsLayer[0].clear();
	m_RenderTileSubsLayer[1].clear();
	m_setRenderChunks.clear();

	S_DEL(m_pVB);
	m_pVB = GetRenderSystem().GetHardwareBufferMgr().CreateVertexBuffer(m_pTerrainData->getVertexCount(),sizeof(TerrainVertex));
	updateVB(0,0,m_pTerrainData->getWidth(),m_pTerrainData->getHeight());/*CHardwareBuffer::HBL_NORMAL*/

	S_DEL(m_pIB);
	m_pIB = GetRenderSystem().GetHardwareBufferMgr().CreateIndexBuffer(m_pTerrainData->getWidth()*m_pTerrainData->getHeight()*2*6*sizeof(unsigned short),CHardwareIndexBuffer::IT_16BIT,CHardwareBuffer::HBU_WRITE_ONLY);

	m_LightMapDecal.createBySize((m_pTerrainData->getWidth()+1), m_pTerrainData->getChunkSize(), m_pTerrainData->getChunkSize());
	m_LightDecal.createBySize((m_pTerrainData->getWidth()+1),8,8);
	return true;
}

bool CTerrain::prepare()const
{
	if (m_pVB==NULL||m_pIB==NULL)
	{
		return false;
	}
	CRenderSystem& R = GetRenderSystem();
	R.SetLightingEnabled(false);
	R.SetCullingMode(CULL_ANTI_CLOCK_WISE);
	R.SetDepthBufferFunc(true,true);

	R.SetFVF(TERRAIN_VERTEX_FVF);
	R.SetStreamSource(0, m_pVB, 0, sizeof(TerrainVertex));
	R.SetIndices(m_pIB);
	return true;
}

bool CTerrain::LightMapPrepare()
{
	CRenderSystem& R = GetRenderSystem();
	R.SetLightingEnabled(false);
	R.SetCullingMode(CULL_ANTI_CLOCK_WISE);
	//R.SetRenderState(D3DRS_FOGENABLE, false);
	R.SetDepthBufferFunc(true,false);
	R.SetAlphaTestFunc(false);
	R.SetBlendFunc(true,BLENDOP_ADD,SBF_DEST_COLOUR,SBF_ZERO);

	R.SetTextureColorOP(0,TBOP_SOURCE1,TBS_TEXTURE,TBS_DIFFUSE);
	R.SetTextureAlphaOP(0,TBOP_DISABLE);
	R.SetTextureColorOP(1,TBOP_DISABLE);
	R.SetTexCoordIndex(0, 2);
	//bump
	//static int nTexID = GetRenderSystem().GetTextureMgr().RegisterTexture("Data/Textures/Tile/road.dds");
	R.SetTexture(0,m_nLightMap);

	R.SetIndices(m_LightMapDecal.GetIB());
	return true;
}

void CTerrain::LightMapFinish()
{
	CRenderSystem& R = GetRenderSystem();
	//R.BlendFunc(D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA);
	//R.SetTexCoordIndex(0, 0);
	R.setTextureMatrix(0,TTF_DISABLE);
	R.setTextureMatrix(1,TTF_DISABLE);
	R.SetTextureColorOP(1,TBOP_DISABLE);
	R.SetTextureColorOP(2,TBOP_DISABLE);
}

void CTerrain::updateRender(const CFrustum& frustum)
{
	m_setRenderChunks.clear();
	m_pTerrainData->walkOctree(frustum,m_setRenderChunks);
	//
	updateIB();
}

void CTerrain::drawCubeBoxes(Color32 color)const
{
	CRenderSystem& R = GetRenderSystem();
	R.SetDepthBufferFunc(true,true);
	//for (size_t i=0; i<m_setRenderChunks.size(); ++i)
	{
	//	GetGraphics().drawBBox(m_setRenderChunks[i]->bbox,color);
	}
}

void CTerrain::drawLayer0()const
{
	CRenderSystem& R = GetRenderSystem();
	R.SetTexCoordIndex(0, 0);	// Diffuse
	R.SetTexCoordIndex(1, 1);	// lightmap
	for (auto it = m_RenderTileSubsLayer[0].begin(); it!=m_RenderTileSubsLayer[0].end(); ++it)
	{
		auto itTile = m_Tiles.find(it->first);
		if (itTile!=m_Tiles.end() && R.prepareMaterial(itTile->second.c_str()))
		{
			R.SetAlphaTestFunc(false);
			R.SetBlendFunc(false);
			R.SetTextureAlphaOP(0,TBOP_DISABLE);
			R.drawIndexedSubset(it->second);
			R.finishMaterial();
		}
	}
}

void CTerrain::drawLayer1()const
{
	CRenderSystem& R = GetRenderSystem();
	R.SetTexCoordIndex(0, 0);	// Diffuse
	R.SetTexCoordIndex(1, 1);	// lightmap
	for (auto it = m_RenderTileSubsLayer[1].begin(); it!=m_RenderTileSubsLayer[1].end(); ++it)
	{
		auto itTile = m_Tiles.find(it->first);
		if (itTile!=m_Tiles.end() && R.prepareMaterial(itTile->second.c_str()))
		{
			R.drawIndexedSubset(it->second);
			R.finishMaterial();
		}
	}
}

void CTerrain::renderGrass()const
{
	CRenderSystem& R = GetRenderSystem();
	{
		if (m_pGrassVB==NULL||m_pGrassIB==NULL)
		{
			return;
		}
		R.SetLightingEnabled(false);
		R.SetCullingMode(CULL_NONE);
		R.SetDepthBufferFunc(true,true);
		R.SetAlphaTestFunc(true,CMPF_GREATER_EQUAL,0x80);

		R.SetFVF(TERRAIN_VERTEX_FVF);
		R.SetStreamSource(0, m_pGrassVB, 0, sizeof(TerrainVertex));
		R.SetIndices(m_pGrassIB);
	}
	if (R.prepareMaterial("Terrain.Grass"))
	{
		R.drawIndexedSubset(m_GrassSub);
	}
	R.finishMaterial();
}

void CTerrain::render(const Matrix& mWorld, E_MATERIAL_RENDER_TYPE eRenderType)const
{
	if (m_bShowBox)
	{
		drawCubeBoxes();
	}
	if (eRenderType&MATERIAL_GEOMETRY)
	{
		if (prepare())
		{
			drawLayer0();
			drawLayer1();
			// 阴影
			//if (LightMapPrepare())
			//{
			//	drawTerrain();
			//}
			//LightMapFinish();
		}
	}
	if (eRenderType&MATERIAL_ALPHA)
	{
		renderGrass();		// Show grass.
	}
}

void CTerrain::draw()
{
	if (m_LightMapDecal.setIB())
	{
		for (auto it=m_setRenderChunks.begin(); it!=m_setRenderChunks.end(); ++it)
		{
			drawChunk(*it);
		}
	}
}

void CTerrain::drawChunk(TerrainChunk* pChunk)
{
	unsigned long uBaseVertexIndex = m_pTerrainData->getVertexIndex(pChunk->box.vMin.x, pChunk->box.vMin.y);
	m_LightMapDecal.Draw(uBaseVertexIndex);
}

void CTerrain::brushLightColor(MAP_EDIT_RECORD& mapEditRecord, float fPosX, float fPosY, Color32 colorPaint, float fRadius, float fHardness, float fStrength)
{
	/*if (isPointIn(fPosX, fPosY))
	{
		EditTarget editTarget;
		//editTarget.type = CTerrainBrush::BRUSH_TYPE_TERRAIN_COLOR;
		for (int y=fPosY-fRadius; y<fPosY+fRadius; y++)
		{
			for (int x=fPosX-fRadius; x<fPosX+fRadius; x++)
			{
				editTarget.x = x;
				editTarget.y = y;
				if (isPointIn(x,y))
				{
					Vec2D vLength(fPosX-x, fPosY-y);
					float fOffset = 1.0f-vLength.length()/fRadius;
					fOffset = min(fOffset/(1.0f-fHardness),1.0f);
					if (fOffset>0)
					{
						float fRate=fOffset*fStrength;
						Color32& colorDest = (Color32)mapEditRecord[editTarget].color;
						if(colorDest.c==0)
						{
							colorDest=getVertexColor(x,y);
						}
						colorDest += colorPaint*fRate;
					}
				}
			}
		}
	}*/
}

void CTerrain::drawLightDecal(float x, float y, float fSize, Color32 color)
{
	if (!m_LightDecal.setIB())
	{
		return;
	}
	CRenderSystem& R = GetRenderSystem();
	R.SetSamplerAddressUV(0,ADDRESS_BORDER,ADDRESS_BORDER);
	// 打开纹理矩阵

	float fScale = 0.5f/fSize;
	// －贴花中心Ｕ×缩放比＋０.５纹理偏移
	float fPosX = 0.5f-x*fScale;
	// １－（－贴花中心Ｖ×缩放比＋０.５纹理偏移）
	float fPosY = 0.5f+y*fScale;
	//
	Matrix mLightDecalTrans = Matrix(
		fScale,0,fPosX,0,
		0,-fScale,fPosY,0,
		0,0,0,0,
		0,0,0,0);
	R.setTextureMatrix(0,TTF_COUNT2,mLightDecalTrans);
	m_LightDecal.Draw((int)x,(int)y);

	//
	R.setTextureMatrix(0,TTF_DISABLE);
	R.SetSamplerAddressUV(0,ADDRESS_WRAP,ADDRESS_WRAP);
}

void CTerrain::loadTilesMaterial(const char* szFilename, const char* szParentDir)
{
	//GetRenderSystem().getMaterialMgr().load(szFilename, szParentDir);
}

void CTerrain::clearAllTiles()
{
	//m_Tiles.clear();
}