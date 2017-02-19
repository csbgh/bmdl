#pragma once

#include "bmdl_common.h"

// =================================
// Basic Model : List
// Simple dynamic array implementation
// =================================

template<typename T>
class BmList
{
public:

	BmList() { count = capacity = 0; data = nullptr; }
	BmList(uint32_t size) : BmList() { reserve(size); }
	BmList(T* newData, uint32_t length) : BmList() { setData(newData, length); }

	~BmList() { if (data != nullptr) BM_FREE(data); }

	typedef T* iterator;

	inline T&		operator[](int32_t idx) { BM_ASSERT(idx < count); return data[idx]; }
	inline const T&	operator[](int32_t idx) const { BM_ASSERT(idx < count); return data[idx]; }

	inline void add(const T& value) { if (count == capacity) { reserve(grow(capacity + 1)); } data[count++] = value; }

	inline T& last() { BM_ASSERT(count > 0); return data[count - 1]; }

	inline void setData(T* newData, uint32_t length)
	{
		reserve(length);
		memcpy(data, newData, sizeof(T) * length);
		count = length;
	}

	inline void reserve(uint32_t newCapacity)
	{
		if (newCapacity <= capacity) return;

		T* newData = (T*)BM_ALLOC(sizeof(T) * newCapacity);
		if (data != nullptr)
		{
			memcpy(newData, data, sizeof(T) * capacity);
			BM_FREE(data);
		}
		data = newData;
		capacity = newCapacity;
	}

private:

	inline uint32_t grow(uint32_t newSize) { return capacity != 0 ? (capacity + capacity / 2) : defaultCapacity; }

public:

	uint32_t count;
	uint32_t capacity;
	T* data;

	static const uint32_t defaultCapacity = 4;

};

// =================================

// =================================
// Basic Model : Data Table
// A simple hash table that only uses strings as keys because that's all we require
// =================================

template<class T>
class BmDataTable
{
public:

	BmDataTable(const bool uniqueKeys = true) : elementCount(0), uniqueKeys(uniqueKeys) { buckets = nullptr;  Grow(initialBucketSize); }

	~BmDataTable()
	{
		for (int32_t b = 0; b < bucketCount; b++)
		{
			if (buckets[b])
				delete buckets[b];
		}

		if (buckets)
			BM_FREE(buckets);
	}

	class BucketNode
	{
	public:

		T			val;
		const char* key;

	private:

		DECLARE_BM_ALLOCATOR()

		BucketNode(const char* key, const T& val, const uint32_t hash) :
			key(key), val(val), hash(hash), next(nullptr)
		{}

		~BucketNode() { FreeNode(); }

		uint32_t hash;
		BucketNode*	next;

		inline BucketNode* FindInsertUnique(const char* newKey)
		{
			if (strcmp(key, newKey) == 0)
				return nullptr;

			if (!next) // if this is the last node insert
				return this;

			return next->FindInsertUnique(newKey);
		}

		inline BucketNode* FindInsert(const char* newKey, BucketNode* lastNode = nullptr, bool lastStrEqual = false)
		{
			bool strEqual = strcmp(key, newKey) == 0;
			if (lastNode && lastStrEqual && !strEqual) // if the last key was was equal, and this node is not, insert node next to key of same value
			{
				return lastNode;
			}
			else
			{
				// if this is the last node insert
				if (!next)
					return this;

				return next->FindInsert(newKey, this, strEqual);
			}
		}

		void FreeNode()
		{
			BucketNode* curNode = next;
			while (curNode != nullptr)
			{
				BucketNode* tmpNode = curNode->next;
				curNode->next = nullptr;				// ensure the next node won't try to free its child nodes
				delete curNode;
				curNode = tmpNode;
			}
		}

		friend class BmDataTable<T>;
	};

	class TableIterator
	{
	public:

		TableIterator(const BmDataTable* table, int32_t idx, BucketNode* node) : table(table), idx(idx), node(node) {}
		TableIterator() : table(nullptr), idx(0), node(nullptr) {}

		BucketNode* operator->() const { return node; };

		bool operator==(const TableIterator &other) const { return (other.node == node && other.idx == idx); }
		bool operator!=(const TableIterator &other) const { return !(*this == other); }

		TableIterator&	operator++() { GetNext(); return (*this); }
		TableIterator	operator++(int) { TableIterator tmp = *this; GetNext(); return tmp; }

	private:

		void GetNext();
		void GetFirst();

		int32_t		idx;	// the current bucket index
		BucketNode* node;	// the current node in the current bucket
								// BucketNode* buckeIdxPtr;	// TODO : have one extra nullptr at end of bucket list to signal end

		const BmDataTable* table;
		friend class BmDataTable<T>;
	};

	typedef T				value_type;
	typedef TableIterator	iterator;

	bool		IsEmpty()	const { return elementCount <= 0; }
	uint32_t	Size()		const { return elementCount; }

	iterator	Begin() { iterator tmp(this, 0, nullptr); tmp.GetFirst(); return tmp; }
	iterator	End() const { return iterator(this, bucketCount, nullptr); }

	const iterator& CEnd() { return endIt; }

	//T&			operator[] (const char* key);
	//const int&	operator[] (const char* key) const;

	iterator& Insert(const char* key, const T& data);
	iterator& Find(const char* key) const;

private:

	// http://www.cse.yorku.ca/~oz/hash.html
	inline uint32_t Hash(const char* str) const
	{
		uint32_t hash = 5381;
		int32_t c;

		while (c = *str++)
			hash = ((hash << 5) + hash) ^ c;

		return hash;
	}

	void	 Grow(int32_t size);
	iterator Place(BucketNode* node, iterator& it);
	bool	 FindInsertPos(const char* key, uint32_t hash, iterator& it);

	BucketNode** buckets;

	int32_t bucketCount, bucketModulo, elementCount;

	iterator endIt;
	const bool uniqueKeys;

	static const int32_t initialBucketSize = 128;
	static const int32_t bucketGrowMultiplier = 2;

	friend class TableIterator;
};

template<class T>
typename BmDataTable<T>::iterator& BmDataTable<T>::Insert(const char* key, const T& data)
{
	if (elementCount >= bucketCount) { Grow(bucketCount * bucketGrowMultiplier); }

	uint32_t hash = Hash(key);

	iterator it;
	if (FindInsertPos(key, hash, it))
	{
		BucketNode* newNode = new BucketNode(key, data, hash);
		it = Place(newNode, it);

		elementCount++;
	}

	return it;
}

template<class T>
typename BmDataTable<T>::iterator& BmDataTable<T>::Find(const char* key) const
{
	uint32_t bucketIdx = Hash(key) & bucketModulo;
	BucketNode* curNode = buckets[bucketIdx];

	while (curNode)
	{
		if (strcmp(curNode->key, key) == 0)
		{
			return iterator(this, bucketIdx, curNode);
		}
		curNode = curNode->next;
	}

	return End();
}

template<class T>
void BmDataTable<T>::Grow(int32_t size)
{
	// store old bucket data
	BucketNode** oldBuckets = buckets;
	int32_t oldBucketCount = bucketCount;
	buckets = nullptr;

	// allocate and setup new bucket data
	bucketCount = size;
	bucketModulo = bucketCount - 1;
	buckets = BM_ALLOC_PTR_ARR(bucketCount, BucketNode);
	for (int32_t b = 0; b < bucketCount; b++)
		buckets[b] = nullptr;

	// if we currently have data in the buckets copy it to the new buckets and free memory
	if (oldBuckets)
	{
		for (int32_t b = 0; b < oldBucketCount; b++)
		{
			if (oldBuckets[b])
			{
				BucketNode* curNode = oldBuckets[b];
				while (curNode)
				{
					// save copy of next node so we can reset collisions
					BucketNode* tmpNode = curNode->next;
					curNode->next = nullptr;

					iterator it;
					FindInsertPos(curNode->key, curNode->hash, it);
					Place(curNode, it);

					curNode = tmpNode;
				}
			}
		}
		if (oldBuckets) { BM_FREE(oldBuckets); oldBuckets = nullptr; }
	}

	endIt = iterator(this, bucketCount, nullptr);
}

template<class T>
typename BmDataTable<T>::iterator BmDataTable<T>::Place(BucketNode* node, typename BmDataTable<T>::iterator& it)
{
	if (it.node)
	{
		if (uniqueKeys)
		{
			it.node->next = node;
		}
		else
		{
			BucketNode* tmp = it.node->next;
			it.node->next = node;
			node->next = tmp;
		}
	}
	else
	{
		buckets[it.idx] = node;
	}

	return iterator(this, it.idx, node);
}

template<class T>
bool BmDataTable<T>::FindInsertPos(const char* key, uint32_t hash, typename BmDataTable<T>::iterator& it)
{
	it.idx = hash & bucketModulo;
	it.node = buckets[it.idx];

	if (it.node) // if collision
	{
		BucketNode* tmpNode = nullptr;
		if (uniqueKeys)
		{
			tmpNode = it.node->FindInsertUnique(key);
		}
		else
		{
			tmpNode = it.node->FindInsert(key);
		}

		if (tmpNode)
		{
			it.node = tmpNode;
			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

template<class T>
void BmDataTable<T>::TableIterator::GetNext()
{
	if (node && node->next)
	{
		node = node->next;
		return;
	}

	if (idx < table->bucketCount)
	{
		idx++;
		node = nullptr;
		while (node == nullptr && idx < table->bucketCount)
		{
			node = table->buckets[idx];
			if (!node)
				idx++;
		}

		if (node)
			return;
	}

	idx = table->bucketCount;
	node = nullptr;
}

template<class T>
void BmDataTable<T>::TableIterator::GetFirst()
{
	node = nullptr; idx = 0;
	while (node == nullptr && idx < table->bucketCount)
	{
		node = table->buckets[idx];
		idx++;
	}
}

// =================================

// =================================
// Basic Model : Byte Stream
// DESCRIPTION...
// could use a bit stream instead but it probably isn't worth it
// =================================

class BmByteStream
{
public:

	BmByteStream(uint32_t reservedSize = 0) :
		buffer(nullptr), internalBuffer(true), writePos(0), readPos(0),
		bufferLength(0), dataLength(0)
	{
		Reserve(reservedSize);
	}

	BmByteStream(uint8_t* data, uint32_t size) : BmByteStream()
	{
		SetBuffer(data, size);
	}

	~BmByteStream()
	{
		if (buffer && internalBuffer)
			free(buffer);
	}

	template<typename T>
	void Write(const T& val)
	{
		if (bufferLength < (writePos + sizeof(T)))
			Expand(sizeof(T));

		*(reinterpret_cast<T*>(buffer + writePos)) = val;
		writePos += sizeof(T);
		dataLength += sizeof(T);
	}

	void WriteString(const char* name)
	{
		BM_ASSERT(name); // string must not be null

		uint32_t size = (strlen(name) + 1) * sizeof(char);
		if (bufferLength < (writePos + size))
			Expand(size);

		memcpy(buffer + writePos, name, size);
		writePos += size;
		dataLength += size;
	}

	void Write(uint8_t* data, uint32_t size)
	{
		BM_ASSERT(data);		// data must not be null
		BM_ASSERT(size > 0);	// length of the data must be greater than 0

		if (bufferLength < (writePos + size))
			Expand(size);

		memcpy(buffer + writePos, data, size);
		writePos += size;
		dataLength += size;
	}

	template<typename T>
	const T& Read()
	{
		BM_ASSERT(buffer);								// buffer must be initialized
		BM_ASSERT((readPos + sizeof(T)) <= dataLength);	// buffer underflow, not enough data to read type T

		T* readData = reinterpret_cast<T*>(buffer + readPos);
		readPos += sizeof(T);

		return *readData;
	}

	template<typename T>
	void Read(T& out)
	{
		BM_ASSERT(buffer);								// buffer must be initialized
		BM_ASSERT((readPos + sizeof(T)) <= dataLength);	// buffer underflow, not enough data to read type T

		T* readData = reinterpret_cast<T*>(buffer + readPos);
		out = *readData;
		readPos += sizeof(T);
	}

	// sets the internal buffer the byte stream will read/write to
	void SetBuffer(uint8_t* data, uint32_t size)
	{
		buffer = data;
		bufferLength = size;
		dataLength = size;
		Reset();
		internalBuffer = false;
	}

	void Reserve(uint32_t bytes)
	{
		if (bufferLength >= bytes)
			return;

		if (buffer != nullptr)
			buffer = (uint8_t*)realloc(buffer, bytes);
		else
			buffer = (uint8_t*)malloc(bytes);

		bufferLength = bytes;
	}

	// Get pointer to the internal data buffer
	const uint8_t* GetBuffer() const { return buffer; }

	// Get the byte length of the data held by the BitStream
	uint32_t GetLength() const { return dataLength; }

	// Reset stream to beginning
	void Reset() { readPos = 0; writePos = 0; }

	// Clear data held by stream
	void Clear() { Reset(); free(buffer); buffer = nullptr; dataLength = 0; }

	// set the current write and read position of the stream
	void SetReadPosition(uint32_t pos) { readPos = pos; }
	void SetWritePosition(uint32_t pos) { writePos = pos; }

private:

	void Expand(uint32_t requiredExpansion)
	{
		if (buffer == nullptr) { Reserve(requiredExpansion * 2); }

		uint32_t requiredLength = (dataLength + requiredExpansion);
		while (bufferLength < requiredLength)
		{
			bufferLength += bufferLength / 2;
		}

		buffer = reinterpret_cast<uint8_t*>(realloc(buffer, bufferLength));
	}

	uint8_t* buffer;
	bool internalBuffer;

	uint32_t writePos, readPos;
	uint32_t bufferLength, dataLength;
};

// =================================

// =================================
// Basic Model Data Block
// Description Here
// =================================

enum class BmAttributeType
{
	Unknown = 0,
	Bool	= 1,
	Int8	= 2,
	UInt8	= 3,
	Int16	= 4,
	UInt16	= 5,
	Int32	= 6,
	UInt32	= 7,
	Int64	= 8,
	UInt64	= 9,
	Float	= 10,
	Double	= 11,
	Vec2	= 12,
	Vec3	= 13,
	Vec4	= 16,
	Color32 = 17,
	String	= 18
};

// TODO : modify this so that it is a little less weird..
template<typename T>
class BmAttributeInfo
{
public:
	static BmAttributeType	GetType() { return BmAttributeType::Unknown; }
	static const char		*GetTypeName() { return "unknown"; }
	static const T&			GetDefaultValue() { BM_ASSERT(false); }
};

#define DECLARE_ATTRIBUTE_TYPE(_class, _attributeType, _attributeName, _defaultSetStatement)						\
	template< > class BmAttributeInfo<_class>																		\
	{																												\
	public:																											\
		static BmAttributeType	GetType()			{ return _attributeType; }										\
		static const char		*GetTypeName()		{ return _attributeName; }										\
		static const _class&	GetDefaultValue()	{ static _class value; _defaultSetStatement; return value;	}	\
	};																												\

DECLARE_ATTRIBUTE_TYPE(bool, BmAttributeType::Bool, "bool", value = false)
DECLARE_ATTRIBUTE_TYPE(int8_t, BmAttributeType::Int8, "int8", value = 0)
DECLARE_ATTRIBUTE_TYPE(uint8_t, BmAttributeType::UInt8, "uint8", value = 0)
DECLARE_ATTRIBUTE_TYPE(int16_t, BmAttributeType::Int16, "int16", value = 0)
DECLARE_ATTRIBUTE_TYPE(uint16_t, BmAttributeType::UInt16, "uint16", value = 0)
DECLARE_ATTRIBUTE_TYPE(int32_t, BmAttributeType::Int32, "int32", value = 0)
DECLARE_ATTRIBUTE_TYPE(uint32_t, BmAttributeType::UInt32, "uint32", value = 0)
DECLARE_ATTRIBUTE_TYPE(int64_t, BmAttributeType::Int64, "int64", value = 0)
DECLARE_ATTRIBUTE_TYPE(uint64_t, BmAttributeType::UInt64, "uint64", value = 0)
DECLARE_ATTRIBUTE_TYPE(float, BmAttributeType::Float, "float", value = 0.0f)
DECLARE_ATTRIBUTE_TYPE(double, BmAttributeType::Double, "double", value = 0.0f)
DECLARE_ATTRIBUTE_TYPE(BmVec2, BmAttributeType::Vec2, "vec2", value = BmVec2())
DECLARE_ATTRIBUTE_TYPE(BmVec3, BmAttributeType::Vec3, "vec3", value = BmVec3())
DECLARE_ATTRIBUTE_TYPE(BmVec4, BmAttributeType::Vec3, "vec4", value = BmVec4())
DECLARE_ATTRIBUTE_TYPE(BmColor32, BmAttributeType::Color32, "color32", value = BmColor32())
DECLARE_ATTRIBUTE_TYPE(const char*, BmAttributeType::String, "string", value = "")

class BmDataAttribute
{
public:

	DECLARE_BM_ALLOCATOR()

	template<class T>
	const T&	GetValue() const;
	const char*	GetValueString() const;
	template<>
	const char* const& GetValue<const char*>() const;

	template<class T>
	void SetValue(const T& va);
	void SetValue(const char* str);

	void Serialize(BmByteStream* stream);
	void Unserialize(BmByteStream* stream);

	BmAttributeType GetType() const { return type; }

private:

	BmDataAttribute(BmAttributeType type) :
		type(type), pAttrData(nullptr)
	{}

	BmDataAttribute() { FreeData(); }

	template<class T>
	void AllocateData();
	void FreeData() { BM_FREE(pAttrData); pAttrData = nullptr; }

	BmAttributeType type;
	void* pAttrData;
	uint32_t dataSize;

	friend class BmDataNode;
};

template<class T>
const T& BmDataAttribute::GetValue() const
{
	if (BmAttributeInfo<T>::GetType() == type)
		return *(T*)pAttrData; // return value

	return BmAttributeInfo<T>::GetDefaultValue(); // return default T value if this attribute is not type T
}

template<>
const char* const & BmDataAttribute::GetValue<const char*>() const
{
	return GetValueString();
}

// TODO : revisit this, fix warning...
inline const char* BmDataAttribute::GetValueString() const
{
	if (type == BmAttributeType::String)
		return (const char*)pAttrData;

	return "";
}

template<class T> void BmDataAttribute::SetValue(const T& val)
{
	type = BmAttributeInfo<T>::GetType();
	AllocateData<T>();
	*(T*)pAttrData = val;
}

inline void BmDataAttribute::SetValue(const char* str)
{
	type = BmAttributeType::String;

	// allocate/reallocate memory
	dataSize = strlen(str) + 1;
	FreeData();
	pAttrData = BM_ALLOC(dataSize);

	memcpy(pAttrData, str, dataSize);
}

inline void BmDataAttribute::Serialize(BmByteStream* stream)
{
	stream->Write(reinterpret_cast<uint8_t*>(pAttrData), dataSize);
}

inline void BmDataAttribute::Unserialize(BmByteStream* stream)
{

}

template<class T>
void BmDataAttribute::AllocateData()
{
	if (pAttrData)
		BM_FREE(pAttrData);

	dataSize = sizeof(T);
	pAttrData = BM_ALLOC(dataSize);
}

typedef BmDataTable<BmDataAttribute*>::iterator BmAttrIt;
typedef BmDataTable<BmDataNode*>::iterator BmNodeIt;
typedef BmDataTable<uint32_t>::iterator StrTableIt;

class BmDataNode
{
public:

	DECLARE_BM_ALLOCATOR()

	BmDataNode(const char* name = "root") :
		name(name), stringIndex(0), nodeTable(BmDataTable<BmDataNode*>(false))
	{}

	~BmDataNode()
	{
		for (BmAttrIt it = attrTable.Begin(); it != attrTable.CEnd(); it++)
		{
			delete it->val; // free attributes
		}

		for (BmNodeIt it = nodeTable.Begin(); it != nodeTable.CEnd(); it++)
		{
			delete it->val; // free nodes
		}
	}

	BmDataNode*		 AddNode(const char* name);
	BmDataNode*		 GetNode(const char* nodeName) const;
	bool			 TryGetNode(const char* name, BmDataNode*& node) const;

	template<class T>
	BmDataAttribute* AddAttribute(const char* name, const T& value);
	BmDataAttribute* AddAttribute(const char* name, const char* value);

	template<class T>
	const T&		 GetValue(const char* attrName) const;
	const char*		 GetValueString(const char* attrName) const;

	BmAttrIt&		 GetAttributeIterator() { return attrTable.Begin(); }
	BmNodeIt&		 GetNodeIterator() { return nodeTable.Begin(); }

	const char*		 GetName() const { return name; }
	uint16_t		 GetAttributeCount() const { return attrTable.Size(); }

	const BmAttrIt&	 GetAttributeEnd() { return attrTable.CEnd(); }
	const BmNodeIt&	 GetNodeEnd() { return nodeTable.CEnd(); }

private:

	void WriteNode(BmByteStream* stream, BmDataNode* node, BmDataTable<uint32_t>* strTable, uint16_t& nodeIndex);

	const char* name;
	uint16_t	stringIndex;
	BmDataTable<BmDataAttribute*>	attrTable;
	BmDataTable<BmDataNode*>		nodeTable;

	friend class BmDataBlock;
};

// adds a new node to this node, if a node with the name given already exists, returns that node instead
inline BmDataNode* BmDataNode::AddNode(const char* name)
{
	BmDataNode* newNode = new BmDataNode(name);
	nodeTable.Insert(name, newNode);
	return newNode;
}

// returns the node with the given name, or nullptr if the a node with nodeName does not exsit
inline BmDataNode* BmDataNode::GetNode(const char* nodeName) const
{
	BmNodeIt it = nodeTable.Find(nodeName);

	if (it != nodeTable.End())
		return it->val;
	else
		return nullptr;
}

inline bool BmDataNode::TryGetNode(const char* name, BmDataNode*& node) const
{
	BmDataNode* tempNode = GetNode(name);

	if (tempNode)
	{
		node = tempNode;
		return true;
	}

	return false;
}

template<class T>
BmDataAttribute* BmDataNode::AddAttribute(const char* name, const T& value)
{
	BmDataAttribute* newAttr = new BmDataAttribute(BmAttributeInfo<T>::GetType());
	newAttr->SetValue(value);

	attrTable.Insert(name, newAttr);

	return newAttr;
}

inline BmDataAttribute* BmDataNode::AddAttribute(const char* name, const char* value)
{
	return AddAttribute<const char*>(name, value);
}

template<class T>
const T& BmDataNode::GetValue(const char* attrName) const
{
	BmAttrIt it = attrTable.Find(attrName);

	if (it != attrTable.End())
		return it->val->GetValue<T>();
	else
		return BmAttributeInfo<T>::GetDefaultValue();
}

inline const char* BmDataNode::GetValueString(const char* attrName) const
{
	BmAttrIt it = attrTable.Find(attrName);

	if (it != attrTable.End())
		return it->val->GetValueString();

	return "";
}

class BmDataBlock
{
public:

	static bool SaveBlock(BmDataNode* node, const char* fileName);
	static bool WriteBlock(BmDataNode* node, BmByteStream* stream);

	// bool OpenBlock(const char* fileName);
	// bool ReadBlock(BmByteStream* stream);

	static const uint16_t versionMajor = 1;
	static const uint16_t versionMinor = 0;
	static const uint32_t fileID = 'B' | ('M' << 8) | ('D' << 16) | ('B' << 24);

private:

	static void WriteNode(BmByteStream* stream, BmDataNode* node, BmDataTable<uint32_t>* strTable, uint16_t& nodeIndex);
	static void PreProcessNode(BmDataNode* node, BmDataTable<uint32_t>* strTable, uint32_t& nodeCount);

};

struct BmDataBlockHeader
{
	BmDataBlockHeader() :
		dataBlockID(BmDataBlock::fileID),
		versionMajor(BmDataBlock::versionMajor),
		versionMinor(BmDataBlock::versionMinor),
		stringTableSize(0),
		nodeCount(0)
	{}

	uint32_t dataBlockID;
	uint16_t versionMajor;
	uint16_t versionMinor;
	uint32_t stringTableSize;
	uint32_t nodeCount;
};

struct BmDataNodeHeader
{
	BmDataNodeHeader(uint16_t parentIndex, uint16_t nameIndex, uint16_t numAttributes) :
		parentIndex(parentIndex), nameIndex(nameIndex), numAttributes(numAttributes) {}

	uint16_t parentIndex;	// index in to node table, 0 = root
	uint16_t nameIndex;		// index in to string table
	uint16_t numAttributes; // number of attributes in this node
};

struct BmDataAttributeHeader
{
	BmDataAttributeHeader(uint8_t dataType, uint16_t parentIndex, uint16_t nameIndex) :
		dataType(dataType), nameIndex(nameIndex) {}

	uint8_t	 dataType;
	uint16_t nameIndex;		// index in to string table
};

bool BmDataBlock::SaveBlock(BmDataNode* node, const char* fileName)
{
	BmByteStream bs;
	WriteBlock(node, &bs);

	if (bmdl::FileWriteAll(fileName, bs.GetBuffer(), bs.GetLength()))
	{
		printf("Succesfully wrote data block to %s\n", fileName);
		return true;
	}
	else
	{
		printf("Failed writing data block to %s\n", fileName);
		return false;
	}
}

bool BmDataBlock::WriteBlock(BmDataNode* node, BmByteStream* stream)
{
	BmDataBlockHeader header;
	BmDataTable<uint32_t> stringTable;

	// stringTable.Clear();
	uint32_t nodeCount = 0;
	PreProcessNode(node, &stringTable, nodeCount);

	// Write data block header
	header.stringTableSize = stringTable.Size();
	header.nodeCount = nodeCount;
	stream->Write(header);

	// write string table
	BmDataTable<uint32_t>::iterator it;
	for (it = stringTable.Begin(); it != stringTable.CEnd(); it++)
	{
		stream->WriteString(it->key);
	}

	// write nodes
	uint16_t nodeIndex = 0;
	WriteNode(stream, node, &stringTable, nodeIndex);

	return true;
}

void BmDataBlock::WriteNode(BmByteStream* stream, BmDataNode* node, BmDataTable<uint32_t>* strTable, uint16_t& nodeIndex)
{
	BmDataNodeHeader header(nodeIndex, node->stringIndex, node->GetAttributeCount());
	stream->Write(header);

	// write all attributes in the node
	for (BmAttrIt attIt = node->GetAttributeIterator(); attIt != node->GetAttributeEnd(); attIt++)
	{
		BmDataAttribute* tmpAtt = attIt->val;

		uint16_t nameIndex = strTable->Find(attIt->key)->val;
		BmDataAttributeHeader attrHeader(static_cast<uint8_t>(tmpAtt->GetType()), nodeIndex, nameIndex);
		stream->Write(attrHeader);

		// if string attribute type only write an index in to the string table
		if (tmpAtt->GetType() == BmAttributeType::String)
		{
			uint16_t strIndex = strTable->Find(tmpAtt->GetValueString())->val;
			stream->Write(strIndex);
		}
		else
		{
			tmpAtt->Serialize(stream);
		}
	}

	// write child nodes
	BmNodeIt nodeIt = node->GetNodeIterator();
	const BmNodeIt& nodeEnd = node->GetNodeEnd();
	for (nodeIt; nodeIt != nodeEnd; nodeIt++)
	{
		nodeIndex += 1;
		WriteNode(stream, nodeIt->val, strTable, nodeIndex);
	}
}

void BmDataBlock::PreProcessNode(BmDataNode* node, BmDataTable<uint32_t>* strTable, uint32_t& nodeCount)
{
	nodeCount++;
	StrTableIt strIt = strTable->Insert(node->GetName(), strTable->Size());
	node->stringIndex = strIt->val;

	// add all attribute strings
	BmDataAttribute* tmpAtt = nullptr;
	for (BmAttrIt attIt = node->GetAttributeIterator(); attIt != node->GetAttributeEnd(); attIt++)
	{
		tmpAtt = attIt->val;

		strTable->Insert(attIt->key, strTable->Size());
		if (tmpAtt->GetType() == BmAttributeType::String)
		{
			strTable->Insert(tmpAtt->GetValueString(), strTable->Size());
		}
	}

	// add node strings
	for (BmNodeIt nodeIt = node->GetNodeIterator(); nodeIt != node->GetNodeEnd(); nodeIt++)
	{
		PreProcessNode(nodeIt->val, strTable, nodeCount);
	}
}

// =================================