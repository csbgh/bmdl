#pragma once

#include <stdint.h>
#include <cstdlib>
#include <string.h> 
#include <stdio.h>
#include <assert.h>
#include <string>

//#define _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 4996 ) // Disable secure crt warnings

//#define BM_ALLOCATOR_FN_PTR
//#define BM_ALLOCATOR_INTERFACE

#if defined(BM_ALLOCATOR_FN_PTR)
	#define BM_ALLOC(_size)	bmdl::settings.bmMemAllocFn(_size)
	#define BM_FREE(_ptr)	bmdl::settings.bmMemFreeFn(_ptr)
#elif defined(BM_ALLOCATOR_INTERFACE)
	#define BM_ALLOC(_size)	bmdl::settings.allocatorInterface->BmAllocate(_size)
	#define BM_FREE(_ptr)	bmdl::settings.allocatorInterface->BmFree(_ptr)
#else
	#define BM_ALLOC(_size)	malloc(_size)
	#define BM_FREE(_ptr)	free(_ptr)
#endif

#define BM_ASSERT(_expression) assert(_expression)

#define BM_ALLOC_PTR_ARR(_count, _type) reinterpret_cast<_type**>(BM_ALLOC(sizeof(_type*) * _count));

#define DECLARE_BM_ALLOCATOR()													\
		inline void* operator new	(size_t count) { return BM_ALLOC(count); }	\
		inline void* operator new[]	(size_t count) { return BM_ALLOC(count); }	\
		inline void	 operator delete(void* ptr)	{ BM_FREE(ptr);	} 				\

class BmAllocatorI
{
public:
	virtual void*	BmAllocate(size_t size) = 0;
	virtual void	Free(void* ptr) = 0;
};

namespace bmdl
{
	struct BmSettings
	{
		BmSettings() : bmMemAllocFn(malloc), bmMemFreeFn(free), allocatorInterface(nullptr) {}

		// TODO : don't put in struct..?
		// provide custom memory allocation/free functions
		void*	(*bmMemAllocFn)	(size_t size);
		void(*bmMemFreeFn)	(void*	ptr);

		BmAllocatorI* allocatorInterface;
	};

	static BmSettings settings;

	static void BmSetLastError(const char* msg)
	{
		printf(msg);
	}

	static const char* BmGetLastError()
	{
		return "No Error";
	}

	static uint8_t* FileReadAll(const char* fileLoc, int32_t &dataSize)
	{
		// attempt to open the file
		FILE *pFile = fopen(fileLoc, "rb");
		if (pFile == nullptr)
		{
			printf("fopen failed");
			return nullptr;
		}
		dataSize = 0;

		// Seek to the end of the file get the size and return to beginning, close file and return if error
		int32_t fileSize;
		if (fseek(pFile, 0, SEEK_END) || (fileSize = ftell(pFile)) == -1 || fseek(pFile, 0, SEEK_SET))
		{
			fclose(pFile);
			return nullptr;
		}

		// attempt to allocate buffer large enough to hold file
		void* buffer = BM_ALLOC(fileSize);
		if (buffer == nullptr)
		{
			fclose(pFile);
			return nullptr;
		}

		// read all the data from the file and close it
		size_t bytesRead = fread(buffer, 1, fileSize, pFile);
		if (bytesRead != static_cast<size_t>(fileSize))
		{
			fclose(pFile);
			BM_FREE(buffer);
			return nullptr;
		}
		fclose(pFile);

		dataSize = fileSize;

		return static_cast<uint8_t*>(buffer);
	}

	static bool FileWriteAll(const char* fileLoc, const uint8_t* data, int32_t dataSize)
	{
		// attempt to open the file
		FILE *pFile = fopen(fileLoc, "wb");
		if (pFile == nullptr)
		{
			printf("failed opening file to write");
			return false;
		}

		size_t bytesWritten = fwrite(data, 1, dataSize, pFile);
		if (bytesWritten != static_cast<size_t>(dataSize))
		{
			printf("Only wrote %u of %u to file", bytesWritten, dataSize);
			fclose(pFile);
			return false;
		}

		fclose(pFile);
		return true;
	}
}

// =================================
// Basic Model : Basic Types
// Description here ...
// =================================

struct BmVec2
{
	BmVec2(float s = 0.0f) : BmVec2(s, s) {}
	BmVec2(float x, float y) : x(x), y(y) {}

	float x, y;
};

struct BmVec3
{
	BmVec3(float s = 0.0f) : BmVec3(s, s, s) {}
	BmVec3(float x, float y, float z) : x(x), y(y), z(z) {}

	float x, y, z;
};

struct BmVec4
{
	BmVec4(float s = 0.0f) : BmVec4(s, s, s, s) {}
	BmVec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float & operator[](uint32_t i) { return (&x)[i]; }

	float x, y, z, w;
};

struct BmMat4
{
	BmVec4 data[4];

	BmVec4 & operator[](uint32_t i) { return data[i]; }
};

struct BmColor32
{
	BmColor32(uint8_t v = 0) : BmColor32(v, v, v) {}
	BmColor32(uint8_t r, uint8_t g, uint8_t b) : BmColor32(r, g, b, 255) {}
	BmColor32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) :
		r(r), g(g), b(b), a(a)
	{}

	uint8_t r, g, b, a;
};

struct BmVert
{
	BmVec3 position;
	BmVec2 texCoord;
	BmVec3 normal;
	BmColor32 color;
};

typedef void* pBmVertexData;

static uint32_t GetColorUInt32(const BmColor32& c) { return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a; }

// =================================

static const uint32_t MAX_VERTEX_ATTRIBS = 32;

enum class BmIndexType : uint8_t
{
	UInt8  = 0,
	UInt16 = 1,
	UInt32 = 2
};

enum class BmBaseType : uint8_t
{
	None	= 0,
	Int8	= 1,
	Uint8	= 2,
	Int16	= 3,
	UInt16	= 4,
	Int32	= 5,
	UInt32	= 6,
	Int64	= 7,
	Uint64	= 8,
	Float	= 9,
	Double	= 10
};

enum class BmMeshType : uint8_t
{
	StaticMesh = 0,
	AnimatedMesh = 16,
};

// maps Easy Mesh vert attributes for other format importing
enum class BmAttrMap : uint8_t
{
	None		= 0,
	Position	= 1,
	Normal		= 2,
	Tangent		= 3,
	BiTangent	= 4,

	Color_1		= 8,
	Color_2		= 9,
	Color_3		= 10,
	Color_4		= 11,

	Color32_1	= 16,
	Color32_2	= 17,
	Color32_3	= 18,
	Color32_4	= 19,

	TexCoord1	= 24,
	TexCoord2	= 25,
	TexCoord3	= 26,
	TexCoord4	= 27
};

struct BmVertAttr
{
	BmVertAttr() 
	{
		baseType = BmBaseType::Float;
		components = 3;
		attrMap = BmAttrMap::Position;
	}

	BmVertAttr(BmBaseType _type, uint8_t _components, BmAttrMap _attrMap)
	{
		baseType = _type;
		components = _components;
		attrMap = _attrMap;
	}

	BmBaseType	baseType;
	uint8_t		components;
	BmAttrMap	attrMap;
};

class BmVertLayout
{
public:
	BmVertLayout() {};

	BmVertAttr attributes[MAX_VERTEX_ATTRIBS];
	uint8_t attributeCount;
};

#define BM_CREATE_LAYOUT(l_name)				\
	class l_name ## _C : public BmVertLayout	\
	{											\
		public:									\
		l_name ## _C (){						\
			attributeCount = 0;					\

#define BM_VERT_ATTR(type, components, mapping)										\
			attributes[attributeCount++] = BmVertAttr(type, components, mapping);	\

#define BM_END_LAYOUT(l_name) }		\
	};								\
	static l_name ## _C l_name;		\

BM_CREATE_LAYOUT(BmDefaultLayout)
	BM_VERT_ATTR(BmBaseType::Float, 3, BmAttrMap::Position)
	BM_VERT_ATTR(BmBaseType::Float, 2, BmAttrMap::TexCoord1)
	BM_VERT_ATTR(BmBaseType::Float, 3, BmAttrMap::Normal)
	BM_VERT_ATTR(BmBaseType::Uint8, 4, BmAttrMap::Color32_1)
BM_END_LAYOUT(BmDefaultLayout)