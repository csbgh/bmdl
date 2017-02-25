import functools
import io
import struct
import time
    
from enum import Enum
from enum import IntEnum
from functools import singledispatch
from collections import defaultdict
from pprint import pprint
from copy import deepcopy
import numpy as np
        
def dump(obj):
    for attr in dir(obj):
        print("obj.%s = %s" % (attr, getattr(obj, attr)))


# TODO : REFACTOR
# =====================================
# So 3ds max can open files, limit names to 12 in length
# this is verry annoying for filenames!
name_unique = []  # stores str, ascii only
name_mapping = {}  # stores {orig: byte} mapping

def sane_name(name):
    name_fixed = name_mapping.get(name)
    if name_fixed is not None:
        return name_fixed

    # strip non ascii chars
    new_name_clean = new_name = name.encode("ASCII", "replace").decode("ASCII")[:12]
    i = 0

    while new_name in name_unique:
        new_name = new_name_clean + ".%.3d" % i
        i += 1

    # note, appending the 'str' version.
    name_unique.append(new_name)
    name_mapping[name] = new_name = new_name.encode("ASCII", "replace")
    return new_name
# ===================================

class Vec2:
    
    __slots__ = "x", "y"
    
    def __init__(self,x,y):
        self.x = x
        self.y = y
        
class Vec3:
    __slots__ = "x", "y", "z"
    def __init__(self,x,y,z):
        self.x = x
        self.y = y
        self.z = z
        
class Vec4:
    __slots__ = "x", "y", "z", "w"
    def __init__(self,x,y,z,w):
        self.x = x
        self.y = y
        self.z = z
        self.w = w

class ByteStream:
    def __init__(self):
        self.write = singledispatch(self.write)
        self.write.register(Vec2, self._write_vec2)
        self.write.register(Vec3, self._write_vec3)
        self.write.register(Vec4, self._write_vec4)
        self.write.register(ByteStream, self._write_byte_stream)
        
        # create stream and buffer
        self.stream = io.BytesIO()
        self.buf = io.BufferedRandom(self.stream)
    
    def save(self, file_path):
        file = open(file_path, 'wb')
        file.write(self.stream.getbuffer())
        file.close()
        
    def get_buffer(self):
        return self.stream.getbuffer()
        
    def get_byte_size(self):
        print("LENGTH = " + str(self.stream.getbuffer().nbytes))
        return self.stream.getbuffer().nbytes
    
    # write baic types
    def write_int8(self, val):
        self.stream.write(struct.pack("<b", val))
     
    def write_uint8(self, val):
        self.stream.write(struct.pack("<B", val))
        
    def write_int16(self, val):
        self.stream.write(struct.pack("<h", val))
        
    def write_uint16(self, val):
        self.stream.write(struct.pack("<H", val))

    def write_int32(self, val):
        self.stream.write(struct.pack("<i", val))
        
    def write_uint32(self, val):
        self.stream.write(struct.pack("<I", val))
        
    def write_bool(self, val):
        self.stream.write(struct.pack("<?", val))
        
    def write_float(self, val):
        self.stream.write(struct.pack("<f", val))
        
    def write_string(self, val, fixed_size=0):
        binary_format = ""
        if fixed_size != 0:
            binary_format = "<%ds" % fixed_size
        else:
            binary_format = "<%ds" % (len(val) + 1)
        self.stream.write(struct.pack(binary_format, val))
        
    # write complex types
    def write(self, obj):
        raise TypeError("Type not supported: {}".format(type(obj)))
        
    def _write_vec2(self, obj):
        self.stream.write(struct.pack("<2f", obj.x, obj.y))
        
    def _write_vec3(self, obj):
        self.stream.write(struct.pack("<3f", obj.x, obj.y, obj.z))
        
    def _write_vec4(self, obj):
        self.stream.write(struct.pack("<4f", obj.x, obj.y, obj.z, obj.w))
    
    def _write_byte_stream(self, obj):
        self.stream.write(obj.get_buffer())
        
# Basic Model Types and Definitions
# ========================

class BmIndexType(IntEnum):
    UInt8  = 0,
    UInt16 = 1,
    UInt32 = 2
        
class BmBaseType(IntEnum):
    Unknown = 0,
    Int8    = 1,
    Uint8   = 2,
    Int16   = 3,
    UInt16  = 4,
    Int32   = 5,
    UInt32  = 6,
    Int64   = 7,
    Uint64  = 8,
    Float   = 9,
    Double  = 10
    
class BmAttrMap(IntEnum):
    Unknown     = 0,
    Position    = 1,
    Normal      = 2,
    Tangent     = 3,
    BiTangent   = 4,

    Color_1     = 8,
    Color_2     = 9,
    Color_3     = 10,
    Color_4     = 11,

    Color32_1   = 16,
    Color32_2   = 17,
    Color32_3   = 18,
    Color32_4   = 19,

    TexCoord1   = 24,
    TexCoord2   = 25,
    TexCoord3   = 26,
    TexCoord4   = 27

# ========================

# File Format Objects and Definitions
# ========================

BM_FILE_ID = 1279544642
BM_VERSION_MAJOR = 0
BM_VERSION_MINOR = 1
BM_MAX_VERT_ATTRS = 32

class BmFileBlockType(IntEnum):
    MeshData		= 0,
    MaterialData	= 8,
    SceneData		= 16,
    ExtensionData	= 24,
    AnimationData	= 32

# basic model file header
class BmFileHeader(object):

    __slots__ = "file_id", "version_major", "version_minor" #, "file_type"

    def __init__(self):
        self.file_id = BM_FILE_ID
        self.version_major = BM_VERSION_MAJOR
        self.version_minor = BM_VERSION_MINOR
        #self.file_type = None

    def write(self, stream):
        # write basic model file header to stream
        stream.write_uint32(self.file_id)
        stream.write_uint16(self.version_major)
        stream.write_uint16(self.version_minor)

class BmVertAttr(object):
    
    __slots__ = "base_type", "components", "attr_map"
    
    def __init__(self):
        self.base_type = BmBaseType.Unknown
        self.components = 0
        self.attr_map = BmAttrMap.Unknown
        
    def write(self, stream):
        stream.write_uint8(self.base_type)
        stream.write_uint8(self.components)
        stream.write_uint8(self.attr_map)
        
# ========================

# Basic Model Objects
# ========================

class BmSubMesh(object):
    
    def __init__(self):
        self.indices = []
        self.material_name = ""
        self.material = None
        
    def write_header(self, stream, indiceOffset):
        stream.write_uint32(indiceOffset)
        stream.write_uint32(len(self.indices))
        stream.write_uint16(0) # material id

    def write_indices(self, stream, indiceType):
        if(indiceType == BmIndexType.UInt16):
            for i in self.indices:
                stream.write_uint16(i)
        elif(indiceType == BmIndexTypeUInt32):
            for i in self.indices:
                stream.write_uint32(i)
        elif(indiceType == BmIndexTypeUInt8):
            for i in self.indices:
                stream.write_uint8(i)
        
class VertPermutation(object):
    __slots__ = "orig_face", "vert_pos", "shared_indices"
    
    def __init__ (self, orig_face, vert_pos):
        self.orig_face = orig_face
        self.vert_pos = vert_pos
        self.shared_indices = []
        
    def add_indice(self, bm_face, vert_pos):
        self.shared_indices.append((bm_face, vert_pos))
        
# TODO : can't access by index... must be a better way?
def get_color(data, index):
    if index == 0:
        return data.color1
    elif index == 1:
        return data.color2
    elif index == 2:
        return data.color3
    elif index == 3:
        return data.color4
        
class UniqueVert(object):
    
    __slots__ = "permutations"
    
    def __init__(self):
        self.permutations = []
        
    def add_permutation(self, mesh, orig_face, vert_pos, bm_face):
        # if a permutation for this vertex already exists,
        if len(self.permutations) > 0:
            unique = True
            for perm in self.permutations:
                tessfaces = mesh.tessfaces
                
                normal_match = True
                uv_match = True
                color_match = True
                
                # check if normals for this indice match the current values for the vertex
                normals = tessfaces[orig_face].split_normals[vert_pos]
                perm_normals = tessfaces[perm.orig_face].split_normals[perm.vert_pos]
                if normals[0] != perm_normals[0] or normals[1] != perm_normals[1] or normals[2] != perm_normals[2]:
                    normal_match = False
                
                # check if all uv channels for this indice match the current values for the vertex
                if normal_match:
                    for uv_channel in mesh.tessface_uv_textures:
                        uv = uv_channel.data[orig_face].uv[vert_pos]
                        perm_uv = uv_channel.data[perm.orig_face].uv[perm.vert_pos]
                        if uv[0] != perm_uv[0] or uv[1] != perm_uv[1]:
                            uv_match = False
                            break
                
                # check if all color channels for this indice match the current values for the vertex
                if normal_match and uv_match: # only check color if the normal uv matched
                    for color_channel in mesh.tessface_vertex_colors:
                        color = get_color(color_channel.data[orig_face], vert_pos)
                        perm_color = get_color(color_channel.data[perm.orig_face], perm.vert_pos)
                        if color[0] != perm_color[0] or color[1] != perm_color[1] or color[2] != perm_color[2]:
                            color_match = False
                            break
                
                if normal_match and uv_match and color_match:
                    unique = False
                    perm.add_indice(bm_face, vert_pos)
                    break
            
            if unique:
                perm = VertPermutation(orig_face, vert_pos)
                self.permutations.append(perm)
                perm.add_indice(bm_face, vert_pos)
        
        else:
            perm = VertPermutation(orig_face, vert_pos)
            self.permutations.append(perm)
            perm.add_indice(bm_face, vert_pos)

class BmVert(object):

    __slots__ = "pos", "normal", "uv_channels", "color_channels"
    
    def __init__(self, pos, normal, uv_channels, color_channels):
        self.pos = pos
        self.normal = normal
        self.uv_channels = uv_channels
        self.color_channels = color_channels
            
class BmFace(object):

    __slots__ = "indices", "material_index"
    
    def __init__ (self, indices, material_index):
        self.indices = indices
        self.material_index = material_index
    
class BmMesh(object):

    def __init__(self, obj, mesh, matrix, uv_channels):
        self.mesh = mesh
        self.submeshes = defaultdict(BmSubMesh)
        self.vertices = []
        self.num_uv_channels = uv_channels
        
        self._permutations = []
        
        # properties
        self.meshName = "DEFAULT_NAME"
        
        self.indiceType = BmIndexType.UInt16;
        
        self.vertAttrCount = 0
        self.vertAttributes = [None] * BM_MAX_VERT_ATTRS
        self.interleaved = True
        
        # calculate split normals to preserve hard edges
        self.mesh.calc_normals_split()
        self.mesh.calc_tessface()
        
        self._create_mesh(obj,mesh,matrix)
        
    def write(self, stream):
        # write main mesh header
        self._write_mesh_header(stream)
        
        # write submesh header
        indice_count = 0
        for sm, submesh in self.submeshes.items():
            submesh.write_header(stream, indice_count)
            indice_count += len(submesh.indices)
        
        # write vertex data
        if self.interleaved:
            for i in range(0, len(self.vertices)):
                stream.write(self.vertices[i].pos)
                stream.write(self.vertices[i].normal)
                for uv in self.vertices[i].uv_channels:
                    stream.write(uv)
                #for color in self.vertices[i].color_channels:
                #    stream.write(color)
                    
        # write indice data
        for sm, submesh in self.submeshes.items():
            submesh.write_indices(stream, self.indiceType)
        
        print("Wrote {0} vertices, {1} indices, {2} submeshes".format(len(self.vertices), indice_count, len(self.submeshes)))
            
    def _write_mesh_header(self, stream):
        stream.write_string(sane_name(self.meshName), 64)
        
        stream.write_uint32(len(self.vertices))
        stream.write_bool(self.interleaved)
        
        indice_count = 0
        for sm, submesh in self.submeshes.items():
            indice_count += len(submesh.indices)
        
        stream.write_uint32(indice_count)
        stream.write_uint8(self.indiceType)
        
        stream.write_uint16(self.vertAttrCount)
        
        # TODO : Iterate numerically over dynamic list
        # ADD : add_attribute() method
        tempAttr = BmVertAttr()
        for attr in self.vertAttributes:
            if attr != None:
                attr.write(stream)
            else:
                tempAttr.write(stream)
                
        stream.write_uint16(len(self.submeshes))
        

    def _create_mesh(self, obj, mesh, matrix):
        for vert in mesh.vertices:
            pos = Vec3(vert.co[0], vert.co[1], vert.co[2])
            normal = Vec3(vert.normal[0], vert.normal[1], vert.normal[2])
            self._add_vertex(pos, normal, len(self.mesh.tessface_vertex_colors))
            
        self._extract_faces()
        self._split_unique_verts()
        self._get_submeshes()
        
    def _extract_faces(self):
        mesh = self.mesh
        
        # create an object to track permutations of uv and color for each vertex
        # later we will duplicate vertices that are shared between indices but differ in uv or color values
        self._permutations = [UniqueVert() for _ in range(len(mesh.vertices))]
        
        for f, face in enumerate(mesh.tessfaces):
            for i, vertex in enumerate(face.vertices):
                self._permutations[vertex].add_permutation(mesh, f, i, face)
                
    def _split_unique_verts(self):
        new_verts = 0
        
        # for each vertex check the number of attribute permutations attached to it
        # create new vertices if required and update indices to point to the new vertex
        vert_count = 0
        for v, vert in enumerate(self._permutations):
            # first permutation can just use the original vertex extra permutations require new vertices
            for p, perm in enumerate(vert.permutations):
                vert_count += 1
                if p == 0:
                    self._set_vertex_attribs(v, perm.orig_face, perm.vert_pos)
                else:
                    # copy original vertex and set permutation attributes
                    new_index = self._clone_vertex(v)
                    self._set_vertex_attribs(new_index, perm.orig_face, perm.vert_pos)
                    
                    # set all old indices to new vertex index
                    for bm_face, vert_pos in perm.shared_indices:
                        bm_face.vertices[vert_pos] = new_index
                        
                    new_verts += 1
        
        
    def _get_submeshes(self):
        mesh = self.mesh
        #self.submeshes = defaultdict(BmSubMesh)
        
        for f, face in enumerate(mesh.tessfaces):
            f_idx = face.vertices # retrieve index list for face
            
            # TODO : don't use defaultdict?
            submesh = self.submeshes[face.material_index]
            indices = submesh.indices
            if len(mesh.materials) > 0 and face.material_index < len(mesh.materials):
                submesh.material_name = mesh.materials[face.material_index].name
            
            if len(f_idx) == 3: # triangulated face
                indices.extend(f_idx)
            
            if len(f_idx) == 4: # triangulate quad face
                indices.extend((f_idx[0], f_idx[1], f_idx[2], f_idx[0], f_idx[2], f_idx[3]))
      
        #print("Submesh Count = " + str(len(self.submeshes)))
        
    # copy vertex attributes from blender object to our mesh representation
    def _set_vertex_attribs(self, index, face, vert_pos):
        vertex = self.vertices[index]
        
        # update normal
        normal = self.mesh.tessfaces[face].split_normals[vert_pos]
        vertex.normal = Vec3(normal[0], normal[1], normal[2])
        
        # update uv channels
        for u, uv_channel in enumerate(self.mesh.tessface_uv_textures):
            uv = uv_channel.data[face].uv[vert_pos]
            vertex.uv_channels[u] = Vec2(uv[0], uv[1])
        
        # update vertex color channels
        for c, color_channel in enumerate(self.mesh.tessface_vertex_colors):
            data = color_channel.data[face]
            color = get_color(data, vert_pos)
            vertex.color_channels[c] = Vec3(color[0], color[1], color[2])
        
    def _clone_vertex(self, index):
        pos = self.vertices[index].pos
        normal = self.vertices[index].normal
        self._add_vertex(pos, normal, len(self.mesh.tessface_vertex_colors))
        return len(self.vertices) - 1
        
    def _add_vertex(self, pos, normal, num_color_channels):
        uv_channels = []
        for uvc in range(0, self.num_uv_channels):
            uv_channels.append(Vec2(0,0))
        
        color_channels = []
        for color_channel in self.mesh.tessface_vertex_colors:
            color_channels.append(Vec3(0,0,0))
        
        self.vertices.append(BmVert(pos, normal, uv_channels, color_channels))
        
# ========================

def write_file(operator,
                context, filepath="",
                export_selected=True,
                global_matrix=None,
				uv_channels=0,
                ):
    
    # TODO: build vertex attributes from these options
    """
    position [T] : Bool
    normals [F] : Bool
    uv_channels [0] : int
    color channels [0] : int
    """
    
    import bpy
    import mathutils
    
    from bpy_extras.io_utils import create_derived_objects, free_derived_objects
    
    # Time the export
    time1 = time.clock()

    # REMOVE
    if global_matrix is None:
        global_matrix = mathutils.Matrix()

    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    scene = context.scene

    if export_selected:
        objects = (ob for ob in scene.objects if ob.is_visible(scene) and ob.select)
    else:
        objects = (ob for ob in scene.objects if ob.is_visible(scene))
    
    mesh_list = []
    mesh = None
    use_mesh_modifiers = True
    
    for obj in objects:
        # get derived objects
        free, derived = create_derived_objects(scene, obj)
        
        if derived is None:
            continue
        
        for ob_derived, mat in derived:
            try:
                mesh = obj.to_mesh(scene, True, 'PREVIEW')
            except:
                mesh = None
                
            if mesh:
                matrix = global_matrix * mat
                mesh.transform(matrix)
                mesh_list.append((obj, mesh, matrix))
            
        if free:
            free_derived_objects(obj)
        
    # Create and write mesh block
    mesh_block_stream = ByteStream()
     
    bm_mesh_list = []
    for obj, mesh, matrix in mesh_list:
        new_mesh = BmMesh(obj, mesh, matrix, uv_channels)
        bm_mesh_list.append(new_mesh)

    write_mesh_block(bm_mesh_list, mesh_block_stream)
    
    # Create main stream that will be the final output to file
    main_stream = ByteStream()
    
    header = BmFileHeader()
    header.write(main_stream)
    
    # Write other blocks to main stream
    #main_stream.write(mesh_block_stream)
    write_block(main_stream, mesh_block_stream, BmFileBlockType.MeshData)
    
    # Save our file data from memory to disk
    main_stream.save(filepath)
    
    # Debugging only: report the exporting time:
    print("bmf export time: %.2f" % (time.clock() - time1))

    return {'FINISHED'}
    
def write_block(stream, block, blockType):
    stream.write_uint16(blockType)
    stream.write_uint32(block.get_byte_size())
    stream.write(block)
    
def write_mesh_block(bm_mesh_list, stream):
    # write number of meshes in the block
    stream.write_uint16(len(bm_mesh_list))
    
    # output each mesh
    for mesh in bm_mesh_list:
        mesh.write(stream)
    

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        