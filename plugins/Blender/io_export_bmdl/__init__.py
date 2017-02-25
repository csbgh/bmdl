# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8-80 compliant>

bl_info = {
    "name": "Basic Model Format",
    "author": "Chris Bruning",
    "blender": (2, 74, 0),
    "location": "File > Export",
    "description": "Export Meshes to BMF format",
    "warning": "",
    "wiki_url": ""
                "Scripts/Export/Basic_Model",
    "category": "Export"}

if "bpy" in locals():
    import importlib
    if "export_bmf" in locals():
        importlib.reload(export_bmf)


import bpy
from bpy.props import (
        BoolProperty,
        EnumProperty,
        IntProperty,
        FloatProperty,
        StringProperty,
        )
from bpy_extras.io_utils import (
        ImportHelper,
        ExportHelper,
        orientation_helper_factory,
        axis_conversion,
        )
IO3DSOrientationHelper = orientation_helper_factory("IO3DSOrientationHelper", axis_forward='Y', axis_up='Z')

class ExportBMF(bpy.types.Operator, ExportHelper, IO3DSOrientationHelper):
    """Export to BMF file format (.bmf)"""
    bl_idname = "export_mesh.bmdl_bmf"
    bl_label = 'Export BMF'

    filename_ext = ".bmf"
    
    filter_glob = StringProperty(
                default="*.bmf",
                options={'HIDDEN'},
            )

    position = BoolProperty(
                name="Position",
                description="Export vertex positions",
                default=True,
            )
            
    normals = BoolProperty(
                name="Normals",
                description="Export vertex normals",
                default=True,
            )
            
    uv_channels = IntProperty(
                name="UV Channels",
                description="Number of UV channels to export.",
                default=1,
            )
    
    color_channels = IntProperty(
                name="Color Channels",
                description="Number of Color channels to export.",
                default=1,
            )
            
    indice_type = EnumProperty(items=(('AUTO', "Automatic", "Automatically determine best indice type"),
                                        ('UInt16', "Short", "16 bit indices max 65536 vertices"),
                                        ('UInt32', "Integer", "32 bit indices"),
                                       ),
                                name="Indice Type",
                                description="Size type for mesh indices",
                                )
            
    export_selected = BoolProperty(
                name="Export Selected",
                description="Export selected objects only",
                default=False,
            )
            
    merge_objects = BoolProperty(
                name="Merge Objects",
                description="Merge all objects in to a single mesh",
                default=False,
            )
            
    """vertex_layout = EnumProperty(items=(('AUTO', "Automatic", "Automatically determine best layout"),
                                        ('DEFAULT', "Default", "pos,uv,normal,color32"),
                                        ('POSITION_ONLY', "Position Only", "pos"),
                                        ('CUSTOM', "Custom", "Set Custom vertex attribute output"),
                                       ),
                                name="Vertex Layout",
                                description="Defines vertex attributes to be exported",
                                )"""
            
    def execute(self, context):
        from . import export_bmf

        keywords = self.as_keywords(ignore=("axis_forward",
                                            "axis_up",
                                            "filter_glob",
                                            "check_existing",
                                            "merge_objects",
                                            "vertex_layout",
                                            "indice_type",
                                            "position",
                                            "normals",
                                            "color_channels"
                                            ))
        global_matrix = axis_conversion(to_forward=self.axis_forward,
                                        to_up=self.axis_up,
                                        ).to_4x4()
        keywords["global_matrix"] = global_matrix

        return export_bmf.write_file(self, context, **keywords)


# Add to menu
def menu_func_export(self, context):
    self.layout.operator(ExportBMF.bl_idname, text="Basic Model (.bmf)")

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()
