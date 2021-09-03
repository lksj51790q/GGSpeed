#include "FbxLoader.h"
#include "targa.h"


namespace
{
	bool LoadTextureFromFile(const FbxString & pFilePath, unsigned int & pTextureObject)
	{
		if (pFilePath.Right(3).Upper() == "TGA")
		{
			tga_image lTGAImage;

			if (tga_read(&lTGAImage, pFilePath.Buffer()) == TGA_NOERR)
			{
				// Make sure the image is left to right
				if (tga_is_right_to_left(&lTGAImage))
					tga_flip_horiz(&lTGAImage);

				// Make sure the image is bottom to top
				if (tga_is_top_to_bottom(&lTGAImage))
					tga_flip_vert(&lTGAImage);

				// Make the image BGR 24
				tga_convert_depth(&lTGAImage, 32);

				// Transfer the texture date into GPU
				glGenTextures(1, &pTextureObject);
				glBindTexture(GL_TEXTURE_2D, pTextureObject);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, lTGAImage.width, lTGAImage.height, 0, GL_BGRA,
					GL_UNSIGNED_BYTE, lTGAImage.image_data);
				glBindTexture(GL_TEXTURE_2D, 0);
				tga_free_buffers(&lTGAImage);
				return true;
			}
		}
		return false;
	}
	void LoadCacheRecursive(FbxNode * pNode, FbxAnimLayer * pAnimLayer, bool pSupportVBO)
	{
		// Bake material and hook as user data.
		const int lMaterialCount = pNode->GetMaterialCount();
		for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
		{
			FbxSurfaceMaterial * lMaterial = pNode->GetMaterial(lMaterialIndex);
			if (lMaterial && !lMaterial->GetUserDataPtr())
			{
				FbxAutoPtr<MaterialCache> lMaterialCache(new MaterialCache);
				if (lMaterialCache->Initialize(lMaterial))
				{
					lMaterial->SetUserDataPtr(lMaterialCache.Release());
				}
			}
		}

		FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		if (lNodeAttribute)
		{
			// Bake mesh as VBO(vertex buffer object) into GPU.
			if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				FbxMesh * lMesh = pNode->GetMesh();
				if (pSupportVBO && lMesh && !lMesh->GetUserDataPtr())
				{
					FbxAutoPtr<VBOMesh> lMeshCache(new VBOMesh);
					if (lMeshCache->Initialize(lMesh))
					{
						lMesh->SetUserDataPtr(lMeshCache.Release());
					}
				}
			}
		}

		const int lChildCount = pNode->GetChildCount();
		for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		{
			LoadCacheRecursive(pNode->GetChild(lChildIndex), pAnimLayer, pSupportVBO);
		}
	}
	void UnloadCacheRecursive(FbxNode * pNode)
	{
		// Unload the material cache
		const int lMaterialCount = pNode->GetMaterialCount();
		for (int lMaterialIndex = 0; lMaterialIndex < lMaterialCount; ++lMaterialIndex)
		{
			FbxSurfaceMaterial * lMaterial = pNode->GetMaterial(lMaterialIndex);
			if (lMaterial && lMaterial->GetUserDataPtr())
			{
				MaterialCache * lMaterialCache = static_cast<MaterialCache *>(lMaterial->GetUserDataPtr());
				lMaterial->SetUserDataPtr(NULL);
				delete lMaterialCache;
			}
		}

		FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
		if (lNodeAttribute)
		{
			// Unload the mesh cache
			if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				FbxMesh * lMesh = pNode->GetMesh();
				if (lMesh && lMesh->GetUserDataPtr())
				{
					VBOMesh * lMeshCache = static_cast<VBOMesh *>(lMesh->GetUserDataPtr());
					lMesh->SetUserDataPtr(NULL);
					delete lMeshCache;
				}
			}
		}

		const int lChildCount = pNode->GetChildCount();
		for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		{
			UnloadCacheRecursive(pNode->GetChild(lChildIndex));
		}
	}
	void LoadCacheRecursive(FbxScene * pScene, FbxAnimLayer * pAnimLayer, const char * pFbxFileName, bool pSupportVBO)
	{
		// Load the textures into GPU, only for file texture now
		const int lTextureCount = pScene->GetTextureCount();
		for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
		{
			FbxTexture * lTexture = pScene->GetTexture(lTextureIndex);
			FbxFileTexture * lFileTexture = FbxCast<FbxFileTexture>(lTexture);
			if (lFileTexture && !lFileTexture->GetUserDataPtr())
			{
				// Try to load the texture from absolute path
				const FbxString lFileName = lFileTexture->GetFileName();

				// Only TGA textures are supported now.
				if (lFileName.Right(3).Upper() != "TGA")
				{
					FBXSDK_printf("Only TGA textures are supported now: %s\n", lFileName.Buffer());
					continue;
				}

				GLuint lTextureObject = 0;
				bool lStatus = LoadTextureFromFile(lFileName, lTextureObject);

				const FbxString lAbsFbxFileName = FbxPathUtils::Resolve(pFbxFileName);
				const FbxString lAbsFolderName = FbxPathUtils::GetFolderName(lAbsFbxFileName);
				if (!lStatus)
				{
					// Load texture from relative file name (relative to FBX file)
					const FbxString lResolvedFileName = FbxPathUtils::Bind(lAbsFolderName, lFileTexture->GetRelativeFileName());
					lStatus = LoadTextureFromFile(lResolvedFileName, lTextureObject);
				}

				if (!lStatus)
				{
					// Load texture from file name only (relative to FBX file)
					const FbxString lTextureFileName = FbxPathUtils::GetFileName(lFileName);
					const FbxString lResolvedFileName = FbxPathUtils::Bind(lAbsFolderName, lTextureFileName);
					lStatus = LoadTextureFromFile(lResolvedFileName, lTextureObject);
				}

				if (!lStatus)
				{
					FBXSDK_printf("Failed to load texture file: %s\n", lFileName.Buffer());
					continue;
				}

				if (lStatus)
				{
					GLuint * lTextureName = new GLuint(lTextureObject);
					lFileTexture->SetUserDataPtr(lTextureName);
				}
			}
		}
		LoadCacheRecursive(pScene->GetRootNode(), pAnimLayer, pSupportVBO);
	}
	void UnloadCacheRecursive(FbxScene * pScene)
	{
		const int lTextureCount = pScene->GetTextureCount();
		for (int lTextureIndex = 0; lTextureIndex < lTextureCount; ++lTextureIndex)
		{
			FbxTexture * lTexture = pScene->GetTexture(lTextureIndex);
			FbxFileTexture * lFileTexture = FbxCast<FbxFileTexture>(lTexture);
			if (lFileTexture && lFileTexture->GetUserDataPtr())
			{
				GLuint * lTextureName = static_cast<GLuint *>(lFileTexture->GetUserDataPtr());
				lFileTexture->SetUserDataPtr(NULL);
				glDeleteTextures(1, lTextureName);
				delete lTextureName;
			}
		}
		UnloadCacheRecursive(pScene->GetRootNode());
	}

	//Draw
	void DrawMesh(FbxNode* pNode, ShadingMode pShadingMode)
	{
		FbxMesh* lMesh = pNode->GetMesh();
		const int lVertexCount = lMesh->GetControlPointsCount();

		// No vertex to draw.
		if (lVertexCount == 0)
		{
			return;
		}

		const VBOMesh * lMeshCache = static_cast<const VBOMesh *>(lMesh->GetUserDataPtr());
		glPushMatrix();

		if (lMeshCache)
		{
			lMeshCache->BeginDraw(pShadingMode);
			const int lSubMeshCount = lMeshCache->GetSubMeshCount();
			for (int lIndex = 0; lIndex < lSubMeshCount; ++lIndex)
			{
				if (pShadingMode == SHADING_MODE_SHADED)
				{
					const FbxSurfaceMaterial * lMaterial = pNode->GetMaterial(lIndex);
					if (lMaterial)
					{
						const MaterialCache * lMaterialCache = static_cast<const MaterialCache *>(lMaterial->GetUserDataPtr());
						if (lMaterialCache)
						{
							lMaterialCache->SetCurrentMaterial();
						}
					}
					else
					{
						// Draw green for faces without material
						MaterialCache::SetDefaultMaterial();
					}
				}

				lMeshCache->Draw(lIndex, pShadingMode);
			}
			lMeshCache->EndDraw();
		}

		glPopMatrix();
	}
	void DrawNode(FbxNode* pNode, ShadingMode pShadingMode)
	{
		FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();

		if (lNodeAttribute)
		{
			if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
			{
				DrawMesh(pNode, pShadingMode);
			}
		}
	}
	void DrawNodeRecursive(FbxNode* pNode, ShadingMode pShadingMode)
	{
		if (pNode->GetNodeAttribute())
		{
			DrawNode(pNode, pShadingMode);
		}

		const int lChildCount = pNode->GetChildCount();
		for (int lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
		{
			DrawNodeRecursive(pNode->GetChild(lChildIndex), pShadingMode);
		}
	}

	

	//VBO
	const float ANGLE_TO_RADIAN = 3.1415926f / 180.f;
	const GLfloat BLACK_COLOR[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const GLfloat GREEN_COLOR[] = { 0.0f, 1.0f, 0.0f, 1.0f };
	const GLfloat WHITE_COLOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const GLfloat WIREFRAME_COLOR[] = { 0.5f, 0.5f, 0.5f, 1.0f };

	const int TRIANGLE_VERTEX_COUNT = 3;

	// Four floats for every position.
	const int VERTEX_STRIDE = 4;
	// Three floats for every normal.
	const int NORMAL_STRIDE = 3;
	// Two floats for every UV.
	const int UV_STRIDE = 2;

	// Get specific property value and connected texture if any.
	// Value = Property value * Factor property value (if no factor property, multiply by 1).
	FbxDouble3 GetMaterialProperty(const FbxSurfaceMaterial * pMaterial,
		const char * pPropertyName,
		const char * pFactorPropertyName,
		GLuint & pTextureName)
	{
		FbxDouble3 lResult(0, 0, 0);
		const FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
		const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
		if (lProperty.IsValid() && lFactorProperty.IsValid())
		{
			lResult = lProperty.Get<FbxDouble3>();
			double lFactor = lFactorProperty.Get<FbxDouble>();
			if (lFactor != 1)
			{
				lResult[0] *= lFactor;
				lResult[1] *= lFactor;
				lResult[2] *= lFactor;
			}
		}

		if (lProperty.IsValid())
		{
			const int lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
			if (lTextureCount)
			{
				const FbxFileTexture* lTexture = lProperty.GetSrcObject<FbxFileTexture>();
				if (lTexture && lTexture->GetUserDataPtr())
				{
					pTextureName = *(static_cast<GLuint *>(lTexture->GetUserDataPtr()));
				}
			}
		}

		return lResult;
	}
}

FbxLoader::FbxLoader(const char * pFileName, bool pSupportVBO)
: mFileName(pFileName), mStatus(UNLOADED),mSdkManager(NULL), mScene(NULL), mImporter(NULL), mCurrentAnimLayer(NULL), mShadingMode(SHADING_MODE_SHADED), mSupportVBO(pSupportVBO)
{
	if (mFileName == NULL)
		return;

	//Create the FBX Manager which is the object allocator for almost all the classes in the SDK
	mSdkManager = FbxManager::Create();
	if (!mSdkManager)
		return;

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create(mSdkManager, IOSROOT);
	mSdkManager->SetIOSettings(ios);

	//Create the Scene object
	mScene = FbxScene::Create(mSdkManager, mFileName);
	if (!mScene)
		return;

	// Create the importer.
	int lFileFormat = -1;
	mImporter = FbxImporter::Create(mSdkManager, "");
	if (!mSdkManager->GetIOPluginRegistry()->DetectReaderFileFormat(mFileName, lFileFormat))
	{
		// Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
		lFileFormat = mSdkManager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");;
	}
	if (mImporter->Initialize(mFileName, lFileFormat) == true)
	{
		mStatus = MUST_BE_LOADED;
	}
}
FbxLoader::~FbxLoader()
{
	if (mScene)
		UnloadCacheRecursive(mScene);
	if (mSdkManager)
		mSdkManager->Destroy();
}
bool FbxLoader::loadFile()
{
	if (!mSdkManager)
		return false;
	if (!mScene)
		return false;
	if (mStatus != MUST_BE_LOADED)
		return false;
	bool lResult = false;
	if (mImporter->Import(mScene) == true)
	{
		// Convert Axis System to what is used in this example, if needed
		FbxAxisSystem SceneAxisSystem = mScene->GetGlobalSettings().GetAxisSystem();
		FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		if (SceneAxisSystem != OurAxisSystem)
			OurAxisSystem.ConvertScene(mScene);

		// Convert Unit System to what is used in this example, if needed
		FbxSystemUnit SceneSystemUnit = mScene->GetGlobalSettings().GetSystemUnit();
		if (SceneSystemUnit.GetScaleFactor() != 1.0)
			FbxSystemUnit::cm.ConvertScene(mScene);

		// Get the list of all the animation stack.
		mScene->FillAnimStackNameArray(mAnimStackNameArray);

		// Convert mesh, NURBS and patch into triangle mesh
		FbxGeometryConverter lGeomConverter(mSdkManager);
		lGeomConverter.Triangulate(mScene, /*replace*/true);

		// Bake the scene for one frame
		LoadCacheRecursive(mScene, mCurrentAnimLayer, mFileName, mSupportVBO);

		mStatus = MUST_BE_REFRESHED;
		lResult = true;
	}
	else
		return false;

	// Destroy the importer to release the file.
	mImporter->Destroy();
	mImporter = NULL;

	return lResult;
}
void FbxLoader::draw(void)
{
	glPushAttrib(GL_ENABLE_BIT);
	//glPushAttrib(GL_LIGHTING_BIT);

	FbxPose * lPose = NULL;
	FbxAMatrix lDummyGlobalPosition;
	DrawNodeRecursive(mScene->GetRootNode(), mShadingMode);

	//glPopAttrib();
	glPopAttrib();
	return;
}

//VBO Mesh
VBOMesh::VBOMesh() : mHasNormal(false), mHasUV(false), mAllByControlPoint(true)
{
	// Reset every VBO to zero, which means no buffer.
	for (int lVBOIndex = 0; lVBOIndex < VBO_COUNT; ++lVBOIndex)
	{
		mVBONames[lVBOIndex] = 0;
	}
}

VBOMesh::~VBOMesh()
{
	// Delete VBO objects, zeros are ignored automatically.
	glDeleteBuffers(VBO_COUNT, mVBONames);

	//	FbxArrayDelete(mSubMeshes);

	for (int i = 0; i < mSubMeshes.GetCount(); i++)
	{
		delete mSubMeshes[i];
	}

	mSubMeshes.Clear();

}

bool VBOMesh::Initialize(const FbxMesh *pMesh)
{
	if (!pMesh->GetNode())
		return false;

	const int lPolygonCount = pMesh->GetPolygonCount();

	// Count the polygon count of each material
	FbxLayerElementArrayTemplate<int>* lMaterialIndice = NULL;
	FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
	if (pMesh->GetElementMaterial())
	{
		lMaterialIndice = &pMesh->GetElementMaterial()->GetIndexArray();
		lMaterialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
			if (lMaterialIndice->GetCount() == lPolygonCount)
			{
				// Count the faces of each material
				for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
				{
					const int lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
					if (mSubMeshes.GetCount() < lMaterialIndex + 1)
					{
						mSubMeshes.Resize(lMaterialIndex + 1);
					}
					if (mSubMeshes[lMaterialIndex] == NULL)
					{
						mSubMeshes[lMaterialIndex] = new SubMesh;
					}
					mSubMeshes[lMaterialIndex]->TriangleCount += 1;
				}

				// Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
				// if, in the loop above, we resized the mSubMeshes by more than one slot.
				for (int i = 0; i < mSubMeshes.GetCount(); i++)
				{
					if (mSubMeshes[i] == NULL)
						mSubMeshes[i] = new SubMesh;
				}

				// Record the offset (how many vertex)
				const int lMaterialCount = mSubMeshes.GetCount();
				int lOffset = 0;
				for (int lIndex = 0; lIndex < lMaterialCount; ++lIndex)
				{
					mSubMeshes[lIndex]->IndexOffset = lOffset;
					lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
					// This will be used as counter in the following procedures, reset to zero
					mSubMeshes[lIndex]->TriangleCount = 0;
				}
				FBX_ASSERT(lOffset == lPolygonCount * 3);
			}
		}
	}

	// All faces will use the same material.
	if (mSubMeshes.GetCount() == 0)
	{
		mSubMeshes.Resize(1);
		mSubMeshes[0] = new SubMesh();
	}

	// Congregate all the data of a mesh to be cached in VBOs.
	// If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
	mHasNormal = pMesh->GetElementNormalCount() > 0;
	mHasUV = pMesh->GetElementUVCount() > 0;
	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
	if (mHasNormal)
	{
		lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
		if (lNormalMappingMode == FbxGeometryElement::eNone)
		{
			mHasNormal = false;
		}
		if (mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}
	if (mHasUV)
	{
		lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
		if (lUVMappingMode == FbxGeometryElement::eNone)
		{
			mHasUV = false;
		}
		if (mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
		{
			mAllByControlPoint = false;
		}
	}

	// Allocate the array memory, by control point or by polygon vertex.
	int lPolygonVertexCount = pMesh->GetControlPointsCount();
	if (!mAllByControlPoint)
	{
		lPolygonVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
	}
	float * lVertices = new float[lPolygonVertexCount * VERTEX_STRIDE];
	unsigned int * lIndices = new unsigned int[lPolygonCount * TRIANGLE_VERTEX_COUNT];
	float * lNormals = NULL;
	if (mHasNormal)
	{
		lNormals = new float[lPolygonVertexCount * NORMAL_STRIDE];
	}
	float * lUVs = NULL;
	FbxStringList lUVNames;
	pMesh->GetUVSetNames(lUVNames);
	const char * lUVName = NULL;
	if (mHasUV && lUVNames.GetCount())
	{
		lUVs = new float[lPolygonVertexCount * UV_STRIDE];
		lUVName = lUVNames[0];
	}

	// Populate the array with vertex attribute, if by control point.
	const FbxVector4 * lControlPoints = pMesh->GetControlPoints();
	FbxVector4 lCurrentVertex;
	FbxVector4 lCurrentNormal;
	FbxVector2 lCurrentUV;
	if (mAllByControlPoint)
	{
		const FbxGeometryElementNormal * lNormalElement = NULL;
		const FbxGeometryElementUV * lUVElement = NULL;
		if (mHasNormal)
		{
			lNormalElement = pMesh->GetElementNormal(0);
		}
		if (mHasUV)
		{
			lUVElement = pMesh->GetElementUV(0);
		}
		for (int lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
		{
			// Save the vertex position.
			lCurrentVertex = lControlPoints[lIndex];
			lVertices[lIndex * VERTEX_STRIDE] = static_cast<float>(lCurrentVertex[0]);
			lVertices[lIndex * VERTEX_STRIDE + 1] = static_cast<float>(lCurrentVertex[1]);
			lVertices[lIndex * VERTEX_STRIDE + 2] = static_cast<float>(lCurrentVertex[2]);
			lVertices[lIndex * VERTEX_STRIDE + 3] = 1;

			// Save the normal.
			if (mHasNormal)
			{
				int lNormalIndex = lIndex;
				if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
				lNormals[lIndex * NORMAL_STRIDE] = static_cast<float>(lCurrentNormal[0]);
				lNormals[lIndex * NORMAL_STRIDE + 1] = static_cast<float>(lCurrentNormal[1]);
				lNormals[lIndex * NORMAL_STRIDE + 2] = static_cast<float>(lCurrentNormal[2]);
			}

			// Save the UV.
			if (mHasUV)
			{
				int lUVIndex = lIndex;
				if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
				{
					lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
				}
				lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
				lUVs[lIndex * UV_STRIDE] = static_cast<float>(lCurrentUV[0]);
				lUVs[lIndex * UV_STRIDE + 1] = static_cast<float>(lCurrentUV[1]);
			}
		}

	}

	int lVertexCount = 0;
	for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
	{
		// The material for current face.
		int lMaterialIndex = 0;
		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
		{
			lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
		}

		// Where should I save the vertex attribute index, according to the material
		const int lIndexOffset = mSubMeshes[lMaterialIndex]->IndexOffset +
			mSubMeshes[lMaterialIndex]->TriangleCount * 3;
		for (int lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
		{
			const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);

			if (mAllByControlPoint)
			{
				lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lControlPointIndex);
			}
			// Populate the array with vertex attribute, if by polygon vertex.
			else
			{
				lIndices[lIndexOffset + lVerticeIndex] = static_cast<unsigned int>(lVertexCount);

				lCurrentVertex = lControlPoints[lControlPointIndex];
				lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<float>(lCurrentVertex[0]);
				lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<float>(lCurrentVertex[1]);
				lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<float>(lCurrentVertex[2]);
				lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;

				if (mHasNormal)
				{
					pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
					lNormals[lVertexCount * NORMAL_STRIDE] = static_cast<float>(lCurrentNormal[0]);
					lNormals[lVertexCount * NORMAL_STRIDE + 1] = static_cast<float>(lCurrentNormal[1]);
					lNormals[lVertexCount * NORMAL_STRIDE + 2] = static_cast<float>(lCurrentNormal[2]);
				}

				if (mHasUV)
				{
					bool lUnmappedUV;
					pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
					lUVs[lVertexCount * UV_STRIDE] = static_cast<float>(lCurrentUV[0]);
					lUVs[lVertexCount * UV_STRIDE + 1] = static_cast<float>(lCurrentUV[1]);
				}
			}
			++lVertexCount;
		}
		mSubMeshes[lMaterialIndex]->TriangleCount += 1;
	}

	// Create VBOs
	glGenBuffers(VBO_COUNT, mVBONames);

	// Save vertex attributes into GPU
	glBindBuffer(GL_ARRAY_BUFFER, mVBONames[VERTEX_VBO]);
	glBufferData(GL_ARRAY_BUFFER, lPolygonVertexCount * VERTEX_STRIDE * sizeof(float), lVertices, GL_STATIC_DRAW);
	delete[] lVertices;

	if (mHasNormal)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBONames[NORMAL_VBO]);
		glBufferData(GL_ARRAY_BUFFER, lPolygonVertexCount * NORMAL_STRIDE * sizeof(float), lNormals, GL_STATIC_DRAW);
		delete[] lNormals;
	}

	if (mHasUV)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBONames[UV_VBO]);
		glBufferData(GL_ARRAY_BUFFER, lPolygonVertexCount * UV_STRIDE * sizeof(float), lUVs, GL_STATIC_DRAW);
		delete[] lUVs;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBONames[INDEX_VBO]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lPolygonCount * TRIANGLE_VERTEX_COUNT * sizeof(unsigned int), lIndices, GL_STATIC_DRAW);
	delete[] lIndices;

	return true;
}

void VBOMesh::UpdateVertexPosition(const FbxMesh * pMesh, const FbxVector4 * pVertices) const
{
	// Convert to the same sequence with data in GPU.
	float * lVertices = NULL;
	int lVertexCount = 0;
	if (mAllByControlPoint)
	{
		lVertexCount = pMesh->GetControlPointsCount();
		lVertices = new float[lVertexCount * VERTEX_STRIDE];
		for (int lIndex = 0; lIndex < lVertexCount; ++lIndex)
		{
			lVertices[lIndex * VERTEX_STRIDE] = static_cast<float>(pVertices[lIndex][0]);
			lVertices[lIndex * VERTEX_STRIDE + 1] = static_cast<float>(pVertices[lIndex][1]);
			lVertices[lIndex * VERTEX_STRIDE + 2] = static_cast<float>(pVertices[lIndex][2]);
			lVertices[lIndex * VERTEX_STRIDE + 3] = 1;
		}
	}
	else
	{
		const int lPolygonCount = pMesh->GetPolygonCount();
		lVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
		lVertices = new float[lVertexCount * VERTEX_STRIDE];

		lVertexCount = 0;
		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
		{
			for (int lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
			{
				const int lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
				lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<float>(pVertices[lControlPointIndex][0]);
				lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<float>(pVertices[lControlPointIndex][1]);
				lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<float>(pVertices[lControlPointIndex][2]);
				lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;
				++lVertexCount;
			}
		}
	}

	// Transfer into GPU.
	if (lVertices)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBONames[VERTEX_VBO]);
		glBufferData(GL_ARRAY_BUFFER, lVertexCount * VERTEX_STRIDE * sizeof(float), lVertices, GL_STATIC_DRAW);
		delete[] lVertices;
	}
}

void VBOMesh::Draw(int pMaterialIndex, ShadingMode pShadingMode) const
{
#if _MSC_VER >= 1900 && defined(_WIN64)
	// this warning occurs when building 64bit.
#pragma warning( push )
#pragma warning( disable : 4312)
#endif

	// Where to start.
	GLsizei lOffset = mSubMeshes[pMaterialIndex]->IndexOffset * sizeof(unsigned int);
	if (pShadingMode == SHADING_MODE_SHADED)
	{
		const GLsizei lElementCount = mSubMeshes[pMaterialIndex]->TriangleCount * 3;
		glDrawElements(GL_TRIANGLES, lElementCount, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid *>(lOffset));
	}
	else
	{
		for (int lIndex = 0; lIndex < mSubMeshes[pMaterialIndex]->TriangleCount; ++lIndex)
		{
			// Draw line loop for every triangle.
			glDrawElements(GL_LINE_LOOP, TRIANGLE_VERTEX_COUNT, GL_UNSIGNED_INT, reinterpret_cast<const GLvoid *>(lOffset));
			lOffset += sizeof(unsigned int) * TRIANGLE_VERTEX_COUNT;
		}
	}
#if _MSC_VER >= 1900 && defined(_WIN64)
#pragma warning( pop )
#endif
}

void VBOMesh::BeginDraw(ShadingMode pShadingMode) const
{
	// Push OpenGL attributes.
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glPushAttrib(GL_ENABLE_BIT);
	glPushAttrib(GL_CURRENT_BIT);
	glPushAttrib(GL_LIGHTING_BIT);
	glPushAttrib(GL_TEXTURE_BIT);

	// Set vertex position array.
	glBindBuffer(GL_ARRAY_BUFFER, mVBONames[VERTEX_VBO]);
	glVertexPointer(VERTEX_STRIDE, GL_FLOAT, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set normal array.
	if (mHasNormal && pShadingMode == SHADING_MODE_SHADED)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBONames[NORMAL_VBO]);
		glNormalPointer(GL_FLOAT, 0, 0);
		glEnableClientState(GL_NORMAL_ARRAY);
	}

	// Set UV array.
	if (mHasUV && pShadingMode == SHADING_MODE_SHADED)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mVBONames[UV_VBO]);
		glTexCoordPointer(UV_STRIDE, GL_FLOAT, 0, 0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	// Set index array.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVBONames[INDEX_VBO]);

	if (pShadingMode == SHADING_MODE_SHADED)
	{
		glEnable(GL_LIGHTING);

		glEnable(GL_TEXTURE_2D);

		glEnable(GL_NORMALIZE);
	}
	else
	{
		glColor4fv(WIREFRAME_COLOR);
	}
}

void VBOMesh::EndDraw() const
{
	// Reset VBO binding.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Pop OpenGL attributes.
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopAttrib();
	glPopClientAttrib();
}

//Material Cache
MaterialCache::MaterialCache() : mShinness(0)
{

}

MaterialCache::~MaterialCache()
{

}

bool MaterialCache::Initialize(const FbxSurfaceMaterial * pMaterial)
{
	const FbxDouble3 lEmissive = GetMaterialProperty(pMaterial,
		FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, mEmissive.mTextureName);
	mEmissive.mColor[0] = static_cast<GLfloat>(lEmissive[0]);
	mEmissive.mColor[1] = static_cast<GLfloat>(lEmissive[1]);
	mEmissive.mColor[2] = static_cast<GLfloat>(lEmissive[2]);

	const FbxDouble3 lAmbient = GetMaterialProperty(pMaterial,
		FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, mAmbient.mTextureName);
	mAmbient.mColor[0] = static_cast<GLfloat>(lAmbient[0]);
	mAmbient.mColor[1] = static_cast<GLfloat>(lAmbient[1]);
	mAmbient.mColor[2] = static_cast<GLfloat>(lAmbient[2]);

	const FbxDouble3 lDiffuse = GetMaterialProperty(pMaterial,
		FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, mDiffuse.mTextureName);
	mDiffuse.mColor[0] = static_cast<GLfloat>(lDiffuse[0]);
	mDiffuse.mColor[1] = static_cast<GLfloat>(lDiffuse[1]);
	mDiffuse.mColor[2] = static_cast<GLfloat>(lDiffuse[2]);

	const FbxDouble3 lSpecular = GetMaterialProperty(pMaterial,
		FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, mSpecular.mTextureName);
	mSpecular.mColor[0] = static_cast<GLfloat>(lSpecular[0]);
	mSpecular.mColor[1] = static_cast<GLfloat>(lSpecular[1]);
	mSpecular.mColor[2] = static_cast<GLfloat>(lSpecular[2]);

	FbxProperty lShininessProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
	if (lShininessProperty.IsValid())
	{
		double lShininess = lShininessProperty.Get<FbxDouble>();
		mShinness = static_cast<GLfloat>(lShininess);
	}

	return true;
}

void MaterialCache::SetCurrentMaterial() const
{
	glMaterialfv(GL_FRONT, GL_EMISSION, mEmissive.mColor);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mAmbient.mColor);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mDiffuse.mColor);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mSpecular.mColor);
	glMaterialf(GL_FRONT, GL_SHININESS, mShinness);

	glBindTexture(GL_TEXTURE_2D, mDiffuse.mTextureName);
}

void MaterialCache::SetDefaultMaterial()
{
	glMaterialfv(GL_FRONT, GL_EMISSION, BLACK_COLOR);
	glMaterialfv(GL_FRONT, GL_AMBIENT, BLACK_COLOR);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, GREEN_COLOR);
	glMaterialfv(GL_FRONT, GL_SPECULAR, BLACK_COLOR);
	glMaterialf(GL_FRONT, GL_SHININESS, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
}


