#pragma once

#include "bmdl_common.h"
#include "bmdl_util.h"

#define BM_FUNC_DECL

// Forward Declarations
template<typename V = BmVert, typename I = uint16_t>
class BmModel; 

template<typename V = BmVert, typename I = uint16_t>
class BmMesh;

namespace bmdl
{

	// =================================
	// Basic Model : File Structure
	// Description here ...
	// =================================

	static const uint16_t BmVersionMajor = 0;
	static const uint16_t BmVersionMinor = 1;
	static const uint32_t BmFileID = 'B' | ('M' << 8) | ('D' << 16) | ('L' << 24);

	#pragma pack(push, 1)

	struct BmFileHeader
	{
		uint32_t fileID;
		uint16_t versionMajor;
		uint16_t versionMinor;
	};

	enum class BmFileBlockType : uint16_t
	{
		MeshData		= 0,
		MaterialData	= 8,
		SceneData		= 16,
		ExtensionData	= 24,
		AnimationData	= 32
	};

	struct BmFileBlock
	{
		BmFileBlockType type;
		uint32_t		blockLength;
	};

	struct BmMeshBlockHeader
	{
		uint16_t numMeshes;
	};

	struct BmMeshHeader
	{
		char		name[64];		// Mesh name

		uint32_t	vertCount;		// Number of vertices in the mesh
		bool		interleaved;	// Determines if vertex attributes are stored together or separately

		uint32_t	indiceCount;
		uint8_t		indiceType;		// Type of indices used by the mesh essentially the byte size of the indices

		uint16_t	vertAttrCount;	// Number of vertex attributes in the mesh
		BmVertAttr  verAttrList[MAX_VERTEX_ATTRIBS];	// List of vertex attributes

		uint16_t	subMeshCount;	// Number of submeshes that make up this mesh
		//BmMat4	transform;		// Base transform for this mesh
	};

	struct BmSubMeshHeader
	{
		uint32_t indiceOffset;
		uint32_t indiceCount;
		uint16_t materialID;
	};

	#pragma pack(pop)

	// =================================

	template<typename V = BmVert, typename I = uint16_t>
	BM_FUNC_DECL BmModel<V, I>* LoadModel(std::string name, BmVertLayout* vertLayout = &BmDefaultLayout, bool interleaved = true)
	{
		int32_t dataSize = 0;
		uint8_t* fileData = FileReadAll(name.c_str(), dataSize);

		if (fileData != nullptr)
		{
			BmModel<V,I>* newModel = LoadModel<V,I>(fileData, dataSize, vertLayout, interleaved);
			// BM_FREE(fileData);
			return newModel;
		}
		else
		{
			BmSetLastError("Unable to read file");
			return nullptr;
		}
	}

	template<typename V = BmVert, typename I = uint16_t>
	BM_FUNC_DECL BmModel<V, I>* LoadModel(uint8_t* fileData, uint32_t dataSize, BmVertLayout* vertLayout = &BmDefaultLayout, bool interleaved = true)
	{
		BmModel<V, I>* newModel = new BmModel<V, I>();

		uint32_t readPos = 0;

		// read Basic Model file header
		BmFileHeader* fileHeader;
		if ((dataSize - readPos) >= sizeof(BmFileHeader))
		{
			fileHeader = reinterpret_cast<BmFileHeader*>(fileData + readPos);
			readPos += sizeof(BmFileHeader);
		}
		else
		{
			BmSetLastError("Could not read file, not enough data to define Basic Model File Header");
			return nullptr;
		}

		// check that that this is a Basic Model file
		if (fileHeader->fileID != BmFileID)
		{
			BmSetLastError("Basic Model file ID did not match : incorrect file type or corrupt data.");
			return nullptr;
		}

		// check the type of file we are loading, if animation or scene file call a separate function
		/*if (fileHeader->fileType != BmFileType::MeshFile)
		{
			if(fileHeader->fileType == BmFileType::AnimationFile)
			{
				#ifdef BM_ANIMATION_INCLUDED
					// bmdl::LoadAnimatedModel(name, verLayout, interleaved);
					// bmdl::LoadAnimationData(...);
				#else
					BmSetLastError("File contains animation data but the animation module was not imported.");
					return nullptr;
				#endif
			}
		}*/

		printf("Read BMDL Header version %i.%i \n", fileHeader->versionMajor, fileHeader->versionMinor);

		// read file blocks and dispatch
		BmFileBlock* fileBlock;
		while ((dataSize - readPos) >= sizeof(BmFileBlock))
		{
			fileBlock = reinterpret_cast<BmFileBlock*>(fileData + readPos);
			readPos += sizeof(BmFileBlock);

			std::string blockTypeName = "UNKNOWN";

			switch (fileBlock->type)
			{
				case BmFileBlockType::MeshData:
					blockTypeName = "Mesh";
					ReadMeshBlock<V,I>(fileData + readPos, fileBlock->blockLength, newModel);
				break;
				default:
					blockTypeName = "UNKNOWN";
				break;
			}

			printf("Read Block of type %s with length %u\n", blockTypeName.c_str(), fileBlock->blockLength);

			readPos += fileBlock->blockLength;
		}

		printf("Read all blocks\n");
		printf("Loaded Successfully ...\n");

		return newModel;
	}

	template<typename V = BmVert, typename I = uint16_t>
	BM_FUNC_DECL bool ReadMeshBlock(uint8_t* data, uint32_t blockLength, BmModel<V, I> *model)
	{
		uint32_t readPos = 0;
		BmMeshBlockHeader* meshBlock = reinterpret_cast<BmMeshBlockHeader*>(data);
		readPos += sizeof(BmMeshBlockHeader);

		// read all meshes
		for (uint32_t m = 0; m < meshBlock->numMeshes; m++)
		{
			model->meshList.add(BmMesh<V, I>());
			BmMesh<V, I>& newMesh = model->meshList.last();

			BmMeshHeader* meshHeader = reinterpret_cast<BmMeshHeader*>(data + readPos);
			readPos += sizeof(BmMeshHeader);

			for (uint32_t sm = 0; sm < meshHeader->subMeshCount; sm++)
			{
				BmSubMeshHeader* subMeshHeader = reinterpret_cast<BmSubMeshHeader*>(data + readPos);
				readPos += sizeof(BmSubMeshHeader);
			}

			// TODO : Properly calculate this
			uint32_t bytesPerVert = sizeof(V);
			uint32_t bytesPerIndx = 2;

			// read vertex data
			V* vertexData = reinterpret_cast<V*>(data + readPos);
			newMesh.vertices.setData(vertexData, meshHeader->vertCount);
			readPos += bytesPerVert * meshHeader->vertCount;	// vertices

			// read index data
			I* indexData = reinterpret_cast<I*>(data + readPos);
			newMesh.indices.setData(indexData, meshHeader->indiceCount);
			readPos += bytesPerIndx * meshHeader->indiceCount;	// indices

			printf("Read Mesh with %i vertices, %i submeshes\n", meshHeader->vertCount, meshHeader->subMeshCount);
		}

		printf("found %i meshes in mesh block.\n", meshBlock->numMeshes);

		return true;
	}
}

struct BmSubMesh
{
	uint32_t indexOffset;
	uint32_t indexCount;
};

template<typename V = BmVert, typename I = uint16_t>
class BmMesh
{
public:

	BmMesh() {} 

	BmList<V> vertices; // interleaved vertex attributes
	BmList<I> indices;  

	BmList<BmSubMesh> subMeshList;

	BmMat4 transform;

private:


};

template<typename V = BmVert, typename I = uint16_t>
class BmModel
{
public:

	DECLARE_BM_ALLOCATOR()

	BmModel() { }

	BmList<BmMesh<V, I>> meshList;
};