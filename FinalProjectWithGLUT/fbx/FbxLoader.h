#include <windows.h>
#include <fbxsdk.h>
#include <GL/glew.h>
#include <gl/gl.h>
#include <gl/glu.h>

enum ShadingMode
{
	SHADING_MODE_WIREFRAME,
	SHADING_MODE_SHADED,
};

class FbxLoader
{
public:
	enum Status
	{
		UNLOADED,				// Unload file or load failure;
		MUST_BE_LOADED,			// Ready for loading file;
		MUST_BE_REFRESHED,		// Something changed and redraw needed;
		REFRESHED				// No redraw needed.
	};
	Status getStatus() const { return mStatus; }
	FbxLoader(const char * pFileName, bool pSupportVBO);
	~FbxLoader();
	bool loadFile(void);
	void draw(void);

private:
	const char			*mFileName;
	mutable Status		 mStatus;
	FbxManager			*mSdkManager;
	FbxScene			*mScene;
	FbxImporter			*mImporter;
	FbxArray<FbxString*> mAnimStackNameArray;
	FbxAnimLayer		*mCurrentAnimLayer;
	ShadingMode			 mShadingMode;
	bool				 mSupportVBO;
};

//VBO Mesh
class VBOMesh
{
public:
	VBOMesh();
	~VBOMesh();

	// Save up data into GPU buffers.
	bool Initialize(const FbxMesh * pMesh);

	// Update vertex positions for deformed meshes.
	void UpdateVertexPosition(const FbxMesh * pMesh, const FbxVector4 * pVertices) const;

	// Bind buffers, set vertex arrays, turn on lighting and texture.
	void BeginDraw(ShadingMode pShadingMode) const;
	// Draw all the faces with specific material with given shading mode.
	void Draw(int pMaterialIndex, ShadingMode pShadingMode) const;
	// Unbind buffers, reset vertex arrays, turn off lighting and texture.
	void EndDraw() const;

	// Get the count of material groups
	int GetSubMeshCount() const { return mSubMeshes.GetCount(); }

private:
	enum
	{
		VERTEX_VBO,
		NORMAL_VBO,
		UV_VBO,
		INDEX_VBO,
		VBO_COUNT,
	};

	// For every material, record the offsets in every VBO and triangle counts
	struct SubMesh
	{
		SubMesh() : IndexOffset(0), TriangleCount(0) {}

		int IndexOffset;
		int TriangleCount;
	};

	GLuint mVBONames[VBO_COUNT];
	FbxArray<SubMesh*> mSubMeshes;
	bool mHasNormal;
	bool mHasUV;
	bool mAllByControlPoint; // Save data in VBO by control point or by polygon vertex.
};

// Material Cache
class MaterialCache
{
public:
	MaterialCache();
	~MaterialCache();

	bool Initialize(const FbxSurfaceMaterial * pMaterial);

	// Set material colors and binding diffuse texture if exists.
	void SetCurrentMaterial() const;

	bool HasTexture() const { return mDiffuse.mTextureName != 0; }

	// Set default green color.
	static void SetDefaultMaterial();

private:
	struct ColorChannel
	{
		ColorChannel() : mTextureName(0)
		{
			mColor[0] = 0.0f;
			mColor[1] = 0.0f;
			mColor[2] = 0.0f;
			mColor[3] = 1.0f;
		}

		GLuint mTextureName;
		GLfloat mColor[4];
	};
	ColorChannel mEmissive;
	ColorChannel mAmbient;
	ColorChannel mDiffuse;
	ColorChannel mSpecular;
	GLfloat mShinness;
};
