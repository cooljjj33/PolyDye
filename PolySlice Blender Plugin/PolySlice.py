bl_info = {
    "name": "PolySlice",
    "author": "Jacques Goeman",
    "version": (1, 0),
    "blender": (4, 1, 1),
    "location": "View3D > Sidebar > PolySlice",
    "description": "3D Print Slicer for color models",
    "category": "Object",
}

import bpy
from bpy.props import FloatProperty, PointerProperty, StringProperty
from bpy.types import Operator, Panel, PropertyGroup
import re
from itertools import groupby
from operator import itemgetter
from mathutils import Vector
import bmesh

# Property Group to hold custom properties
class PolySliceProperties(PropertyGroup):
    sink_amount: FloatProperty(
        name="Sink Amount",
        description="Amount(mm) to move model below print bed",
        default=0.1,
        min=0.1,
        max=5.0,
    )
    color_thickness: FloatProperty(
        name="Color Thickness",
        description="Thickness(mm) of the color that will cover the wall",
        default=1.2,
        min=0.1,
        max=4.0,
    )
    output_directory: StringProperty(
        name="Output Directory",
        description="Directory to save outputs",
        default="",
        subtype='DIR_PATH',
    )
    stl_name: StringProperty(
        name="STL Filename",
        description="The stl file that will be saved to slice with your filament slicer",
        default="",
    )
    first_layer_height: FloatProperty(
        name="First Layer Height",
        description="Thickness(mm) of the first layer of the print - must match slicer",
        default=0.2,
        min=0.1,
        max=4.0,
    )
    layer_height: FloatProperty(
        name="Layer Height",
        description="Thickness(mm) of all layers of the print - must match slicer",
        default=0.13,
        min=0.1,
        max=4.0,
    )

# Operator for "Trim Bottom" button
class OBJECT_OT_trim_bottom(Operator):
    bl_idname = "object.trim_bottom"
    bl_label = "Trim Bottom"
    bl_options = {"REGISTER", "UNDO"}
    bl_description = "Boolean/Remove everything from model that is below print bed"

    def execute(self, context):
        selected_objects = context.selected_objects

        # Error checking
        if not selected_objects:
            self.report({'ERROR'}, "No objects selected.")
            return {'CANCELLED'}

        # Store the original active object
        original_active = context.view_layer.objects.active

        # Deselect all objects
        bpy.ops.object.select_all(action='DESELECT')

        # Add a cube
        bpy.ops.mesh.primitive_cube_add()
        cube = context.active_object

        if cube is None or cube.type != 'MESH':
            self.report({'ERROR'}, "Failed to create the cube.")
            return {'CANCELLED'}

        # Name and configure the cube
        cube.name = "Trim_Cube"
        #cube.hide_viewport = True  # Hide cube from viewport
        cube.hide_render = True    # Hide cube from render

        # Move the cube's vertices down by one unit in edit mode
        bpy.ops.object.mode_set(mode='EDIT')
        bm = bmesh.from_edit_mesh(cube.data)
        for vertex in bm.verts:
            vertex.co.z -= 1
        bmesh.update_edit_mesh(cube.data)
        bpy.ops.object.mode_set(mode='OBJECT')

        # Scale the cube by 20 units
        cube.scale = (80, 80, 80)

        # Re-select the original objects
        for obj in selected_objects:
            obj.select_set(True)

        # For each selected object, add a boolean modifier
        for obj in selected_objects:
            if obj.type != 'MESH':
                self.report({'WARNING'}, f"Object '{obj.name}' is not a mesh. Skipping.")
                continue

            # Check if object is editable
            if obj.library or obj.override_library:
                self.report({'WARNING'}, f"Object '{obj.name}' is linked or overridden. Skipping.")
                continue

            # Add boolean modifier
            bool_mod = obj.modifiers.new(name="TrimBottom", type='BOOLEAN')
            bool_mod.object = cube
            bool_mod.operation = 'DIFFERENCE'
            bool_mod.solver = 'FAST'

            # Apply the modifier
            try:
                context.view_layer.objects.active = obj
                bpy.ops.object.modifier_apply(modifier=bool_mod.name)
            except Exception as e:
                self.report({'ERROR'}, f"Failed to apply modifier on '{obj.name}': {e}")
                continue

        # Deselect all objects
        bpy.ops.object.select_all(action='DESELECT')
        # Select the cube and delete it using operator
        cube.select_set(True)
        context.view_layer.objects.active = cube
        
        bpy.ops.object.delete()

        # Restore the original active object
        if original_active:
            
            context.view_layer.objects.active = original_active

        self.report({'INFO'}, "Trim Bottom operation completed.")
        return {'FINISHED'}

# Operator for "Sink" button
class OBJECT_OT_sink(Operator):
    bl_idname = "object.sink"
    bl_label = "Sink"
    bl_options = {"REGISTER", "UNDO"}
    bl_description = "Move model below print bed to trim later in order to provide a flat first layer"

    def execute(self, context):
        props = context.scene.PolySlice_props

        sink_amount = props.sink_amount

        # Error checking
        if not context.selected_objects:
            self.report({'ERROR'}, "No objects selected.")
            return {'CANCELLED'}

        for obj in context.selected_objects:
            if obj.lock_location[2]:
                self.report({'WARNING'}, f"Object '{obj.name}' Z location is locked. Skipping.")
                continue

            if obj.type in {'MESH', 'CURVE', 'SURFACE', 'META', 'FONT'}:
                # Use the operator to move the object to support undo
                obj.select_set(True)
            else:
                self.report({'WARNING'}, f"Object '{obj.name}' cannot be moved. Skipping.")
                continue

        if context.selected_objects:
            bpy.ops.transform.translate(value=(0, 0, -sink_amount), orient_type='GLOBAL')

        return {'FINISHED'}

# Operator for "Slice" button
class OBJECT_OT_slice(Operator):
    bl_idname = "object.slice"
    bl_label = "Slice"
    bl_options = {"REGISTER", "UNDO"}
    bl_description = "Slice the model into color layers that will be interlaced between filament layers"

    def execute(self, context):
        props = context.scene.PolySlice_props
        color_thickness = props.color_thickness
        output_directory = props.output_directory
        

        first_layer_height = props.first_layer_height
        layer_height = props.layer_height

        # Error checking
        if not context.selected_objects:
            self.report({'ERROR'}, "No objects selected.")
            return {'CANCELLED'}

        tempsel = context.selected_objects
        bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)

                # Names of the objects involved
        target_object_name = "CalibrationTower"  # The object to be edited
        reference_object_name = context.selected_objects[0].name  # The object whose height will be used

        # Retrieve objects
        target_obj = bpy.data.objects[target_object_name]
        reference_obj = bpy.data.objects[reference_object_name]

        # Get the top Z height of the reference object (bounding box maximum Z value)
        world_matrix = reference_obj.matrix_world
        reference_height = max((world_matrix @ Vector(corner)).z for corner in reference_obj.bound_box)

        # Get the height (Z position) of the reference object
        #reference_height = reference_obj.location.z

        # Switch to object mode if not already
        if bpy.context.object.mode != 'OBJECT':
            bpy.ops.object.mode_set(mode='OBJECT')

        # Select and activate the target object
        bpy.ops.object.select_all(action='DESELECT')
        target_obj.select_set(True)
        bpy.context.view_layer.objects.active = target_obj

        # Switch to edit mode
        bpy.ops.object.mode_set(mode='EDIT')

        # Use bmesh to edit geometry
        bm = bmesh.from_edit_mesh(target_obj.data)

        # Ensure all vertices are deselected initially
        for v in bm.verts:
            v.select = False

        # Sort vertices by their Z height in descending order and select the topmost 6
        sorted_vertices = sorted(bm.verts, key=lambda v: (target_obj.matrix_world @ v.co).z, reverse=True)
        top_vertices = sorted_vertices[:6]

        # Move the top 6 vertices to the reference top Z height
        for v in top_vertices:
            v.select = True
            # Move each vertex to the correct height in the world space
            world_vertex_position = target_obj.matrix_world @ v.co
            world_vertex_position.z = reference_height
            v.co = target_obj.matrix_world.inverted() @ world_vertex_position

        # Update mesh and switch back to object mode
        bmesh.update_edit_mesh(target_obj.data)
        bpy.ops.object.mode_set(mode='OBJECT')
        # Select and activate the target object
        bpy.ops.object.select_all(action='DESELECT')
        reference_obj.select_set(True)
        bpy.context.view_layer.objects.active = reference_obj

        bpy.ops.object.duplicate()

        # The duplicated object becomes the new active object
        cloned_object = bpy.context.active_object
        bpy.context.object.hide_render = True

        # Rename the cloned object 
        cloned_object.name = "poly_stl_clone"
        bpy.ops.object.select_all(action='DESELECT')
        reference_obj.select_set(True)
        bpy.context.view_layer.objects.active = reference_obj

        bpy.context.object.hide_render = False

        # Store the original resolution
        original_resolution_x = bpy.context.scene.render.resolution_x
        original_resolution_y = bpy.context.scene.render.resolution_y

        # Store the original file format
        original_file_format = bpy.context.scene.render.image_settings.file_format

        # Set the output resolution temporarily
        bpy.context.scene.render.resolution_x = 300
        bpy.context.scene.render.resolution_y = 300
        if not output_directory:
            self.report({'ERROR'}, "No output path selected.")
            return {'CANCELLED'}
        bpy.context.scene.render.filepath = output_directory+"#"

        # Set the output file format to JPG
        original_file_format = bpy.context.scene.render.image_settings.file_format
        bpy.context.scene.render.image_settings.file_format = 'PNG'
        original_color_mode = bpy.context.scene.render.image_settings.color_mode

        bpy.context.scene.render.image_settings.file_format = 'PNG'
        bpy.context.scene.render.image_settings.color_mode = 'RGBA'
        bpy.context.scene.render.film_transparent = True

        # Set the output file path
        old_path = bpy.context.scene.render.filepath
        output_path = bpy.context.scene.render.filepath[:-1]+'/Render.png'  
        bpy.context.scene.render.filepath = output_path

        # Set the camera to render from
        camera = bpy.data.objects.get('CamRender')
        if camera:
            bpy.context.scene.camera = camera
        else:
            raise Exception("Camera 'CamRender' not found")

        # Render the image
        bpy.context.scene.render.use_compositing = False
        bpy.ops.render.render(write_still=True, use_viewport=True)


        # Restore the original resolution, file format, and color mode
        bpy.context.scene.render.resolution_x = original_resolution_x
        bpy.context.scene.render.resolution_y = original_resolution_y
        bpy.context.scene.render.image_settings.file_format = original_file_format
        bpy.context.scene.render.image_settings.color_mode = original_color_mode

        # Restore the compositing setting
        bpy.context.scene.render.use_compositing = True
        bpy.context.scene.render.filepath = old_path

        #print(f"Render saved to {output_path}")

        # Set the camera to render from
        camera = bpy.data.objects.get('Camera')
        if camera:
            bpy.context.scene.camera = camera
        else:
            raise Exception("Camera 'CamRender' not found")


        #Apply all transforms
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

        # Get the active object
        obj = bpy.context.active_object
        obj_name_o = obj.name
        obj_name = obj.name.split('.')[0]
        vobj_name = obj.name.split('.')[0]
        if vobj_name == '':
            vobj_name = obj_name_o
            obj_name = vobj_name

        # Check if an object is selected
        if obj is not None:
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.editmode_toggle()
            bpy.ops.mesh.remove_doubles()
            bpy.ops.object.editmode_toggle()
            
                # Recalculate normals
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.editmode_toggle()
            bpy.ops.mesh.select_all(action='SELECT')
            bpy.ops.mesh.normals_make_consistent(inside=False)
            bpy.ops.object.editmode_toggle()
            
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.editmode_toggle()
            bpy.ops.mesh.fill_holes()
            bpy.ops.object.editmode_toggle()  
            

        # Enable the 3D-Print Toolbox add-on if it's not enabled
            bpy.ops.preferences.addon_enable(module="object_print3d_utils")

            # Update user preferences
            #bpy.context.preferences.addons["object_print3d_utils"].preferences.use_mesh_analysis = True

            # Specify the name of the object you want to make manifold
            #object_name = "Cube"  

            # Get the object
            #obj = bpy.data.objects[object_name]

            # Make sure the object is the active object
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)

            # Call the Make Manifold operator from the 3D-Print Toolbox
            bpy.ops.mesh.print3d_clean_non_manifold()
            bpy.ops.mesh.print3d_clean_distorted(angle=0.785398)

        
        else:
            print("No active object selected.")
            
        
            
        ############BOOLEAN
        def slice_and_separate_object(obj, slice_thickness, fs):
            # Set the context to 3D View and enter edit mode
            bpy.context.area.ui_type = 'VIEW_3D'
            bpy.context.view_layer.objects.active = obj
            obj.select_set(True)
            bpy.ops.object.mode_set(mode='EDIT')

            bm = bmesh.from_edit_mesh(obj.data)

            min_z = min([v.co.z for v in bm.verts])
            max_z = max([v.co.z for v in bm.verts])

            z = min_z
            slice_index = 0
            
            while z < max_z:
                # Bisect the mesh at the current z level
                geom_cut = bmesh.ops.bisect_plane(
                    bm,
                    geom=bm.verts[:] + bm.edges[:] + bm.faces[:],
                    plane_co=Vector((0, 0, z)),
                    plane_no=Vector((0, 0, 1)),
                    clear_outer=False,
                    clear_inner=False
                )

                bmesh.update_edit_mesh(obj.data)

                cut_verts = [v for v in geom_cut['geom_cut'] if isinstance(v, bmesh.types.BMVert)]
                cut_edges = [e for e in geom_cut['geom_cut'] if isinstance(e, bmesh.types.BMEdge)]

                # Deselect everything
                bpy.ops.mesh.select_all(action='DESELECT')

                # Select and separate the cut geometry
                if (slice_index % 2 == 1):
                    for v in cut_verts:
                        if v.is_valid:
                            bmesh.utils.vert_separate(v, cut_edges)
                            slice_thickness = fs

                slice_index += 1
                z += slice_thickness
            
            # Return to object mode
            #bpy.ops.object.editmode_toggle()

        def process_and_slice_objects(obj, initial_slice_thickness, bottom_slice_thickness):
            # Ensure the object is selected
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            
            # Slice the original object
            slice_and_separate_object(obj, initial_slice_thickness, layer_height)
            
            # Separate into individual objects
            
            bpy.ops.mesh.separate(type='LOOSE')
            



            selected_objects = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]
            for obj in selected_objects:
                obj.select_set(True)
                
                
            # Regular expression pattern to match 'MyFrames.*' names and capture the numeric suffix
            pattern = re.compile(r'MyFrames\.(\d+)')

            # Get all selected objects
            selected_objects = bpy.context.selected_objects

            # Initialize max_suffix to 0
            max_suffix = 0

            # Search selected objects for 'MyFrames.*' names and find the highest numeric suffix
            for obj in selected_objects:
                match = pattern.match(obj.name)
                if match:
                    suffix = int(match.group(1))
                    if suffix > max_suffix:
                        max_suffix = suffix

            # Start renaming from max_suffix + 1
            start_suffix = max_suffix + 1

            # Function to calculate the highest local z-value in an object
            def calculate_highest_local_z(obj):
                # Ensure the object has vertices to evaluate and is not empty
                if hasattr(obj.data, 'vertices') and len(obj.data.vertices) > 0:
                    return max(v.co.z for v in obj.data.vertices)
                # If no vertices or empty, use the object's origin z-coordinate
                else:
                    return obj.location.z
                


            # The threshold for z-heights to be considered the same
            z_threshold = 1

            # Calculate the highest local z value for each object and sort the list of objects based on this
            selected_objects = sorted(selected_objects, key=calculate_highest_local_z, reverse=False)

            # Group objects that have similar z-heights
            grouped_objects = groupby(selected_objects, key=lambda obj: round(calculate_highest_local_z(obj) / z_threshold))

            # For each group of objects with similar z-heights
            for _, group in grouped_objects:
                group = list(group)  # Convert the group from an iterator to a list

                # If there's more than one object in the group, join them
                bpy.ops.object.mode_set(mode='OBJECT')
                if len(group) > 1:
                    bpy.ops.object.select_all(action='DESELECT')
                    for obj in group:
                        obj.select_set(True)
                    bpy.context.view_layer.objects.active = group[0]
                    bpy.ops.object.join()


            bpy.ops.object.select_all(action='DESELECT')
            
            print(vobj_name)
            split_objects = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]
            for split_obj in split_objects:
                split_obj.select_set(True)
                bpy.context.view_layer.objects.active = split_obj
                bpy.ops.object.origin_set(type='ORIGIN_CENTER_OF_MASS', center='BOUNDS')

            # Sort objects by their Z location, bottom-most first
            split_objects.sort(key=lambda o: o.location.z, reverse=True)

            # Reapply slice logic to each object except the bottom-most one
            bpy.ops.object.mode_set(mode='EDIT')
            for i, split_obj in enumerate(split_objects[:-1]):
                slice_and_separate_object(split_obj, layer_height, layer_height/2)
                
                bpy.ops.object.mode_set(mode='OBJECT')
                bpy.context.view_layer.objects.active = split_obj
                #bpy.ops.mesh.separate(type='LOOSE')
                bpy.ops.object.select_all(action='DESELECT')
                #split_obj.select_set(True)
                bpy.context.view_layer.objects.active = split_obj
                
                
            


            # Apply different slice thickness to the bottom-most object
            slice_and_separate_object(split_objects[-1], first_layer_height, layer_height/2)
            bpy.ops.mesh.separate(type='LOOSE')
            bpy.ops.object.mode_set(mode='OBJECT')
            bpy.ops.object.select_all(action='DESELECT')
            
                

        
        initial_object = bpy.context.object  
        initial_slice_thickness = 1.5


        process_and_slice_objects(initial_object, initial_slice_thickness, first_layer_height)

        split_objects = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]
        for split_obj in split_objects:
            split_obj.select_set(True)
            bpy.context.view_layer.objects.active = split_obj


        #bpy.ops.object.editmode_toggle()
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')

        bpy.ops.mesh.separate(type='LOOSE')


        #    bpy.ops.mesh.separate(type='LOOSE')
        selected_objects2 = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]
        bpy.context.scene.cursor.location = (0.0, 0.0, 0.0)

        for item in selected_objects2:
            item.select_set(True)
        #    bpy.context.view_layer.objects.active = item
        #    bpy.ops.mesh.separate(type='LOOSE')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.remove_doubles()
        bpy.ops.object.mode_set(mode = 'OBJECT')
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='BOUNDS')


        # Switch back to object mode







                
                
        #######JOIN Z
        print(obj_name)
        selected_objects = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]
        for obj in selected_objects:
            obj.select_set(True)
            
            
        # Regular expression pattern to match 'MyFrames.*' names and capture the numeric suffix
        pattern = re.compile(r'MyFrames\.(\d+)')

        # Get all selected objects
        selected_objects = bpy.context.selected_objects

        # Initialize max_suffix to 0
        max_suffix = 0

        # Search selected objects for 'MyFrames.*' names and find the highest numeric suffix
        for obj in selected_objects:
            match = pattern.match(obj.name)
            if match:
                suffix = int(match.group(1))
                if suffix > max_suffix:
                    max_suffix = suffix

        # Start renaming from max_suffix + 1
        start_suffix = max_suffix + 1

        # Function to calculate the highest local z-value in an object
        def calculate_highest_local_z(obj):
            # Ensure the object has vertices to evaluate and is not empty
            if hasattr(obj.data, 'vertices') and len(obj.data.vertices) > 0:
                return max(v.co.z for v in obj.data.vertices)
            # If no vertices or empty, use the object's origin z-coordinate
            else:
                return obj.location.z
            


        # The threshold for z-heights to be considered the same
        z_threshold = layer_height

        # Calculate the highest local z value for each object and sort the list of objects based on this
        selected_objects = sorted(selected_objects, key=calculate_highest_local_z, reverse=True)

        # Group objects that have similar z-heights
        grouped_objects = groupby(selected_objects, key=lambda obj: round(calculate_highest_local_z(obj) / z_threshold))

        # For each group of objects with similar z-heights
        for _, group in grouped_objects:
            group = list(group)  # Convert the group from an iterator to a list

            # If there's more than one object in the group, join them
            if len(group) > 1:
                bpy.ops.object.select_all(action='DESELECT')
                for obj in group:
                    obj.select_set(True)
                bpy.context.view_layer.objects.active = group[0]
                bpy.ops.object.join()

        
        # Regular expression pattern to match 'MyFrames.*' names and capture the numeric suffix
        pattern = re.compile(r'MyFrames\.(\d+)')

        # Get all selected objects
        selected_objects = [obj for obj in bpy.context.scene.objects if obj.name.startswith(vobj_name)]

        # Initialize max_suffix to 0
        max_suffix = 0

        # Start renaming from max_suffix + 1
        start_suffix = 1

        # Function to calculate the highest local z-value in an object
        def calculate_highest_local_z(obj):
            # Ensure the object has vertices to evaluate
            if hasattr(obj.data, 'vertices'):
                return max(v.co.z for v in obj.data.vertices)
            # If no vertices, use the object's origin z-coordinate
            else:
                return obj.location.z

        # Sort the selected objects based on their highest local z value (from high to low)
        selected_objects.sort(key=calculate_highest_local_z, reverse=False)

        # Rename all selected objects as 'MyFrames.*'
        for obj in selected_objects:
            obj.name = f"MyFrames.{start_suffix:03d}"
            start_suffix += 1    
            
        #Slices the object

        x = 1
        for i in selected_objects:
            objnamei = i.name
                
            bpy.data.objects[objnamei].hide_render = False
            bpy.data.objects[objnamei].keyframe_insert("hide_render", frame=x)
            for k in selected_objects:
                objnamek = k.name
                
                if (i.name != k.name):                
                    bpy.data.objects[objnamek].hide_render = True
                    bpy.data.objects[objnamek].keyframe_insert("hide_render", frame=x)
            x += 1

        bpy.context.scene.frame_end = x

        for obj in selected_objects:
            obj.select_set(True)
            
        #bpy.ops.object.editmode_toggle()
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.mesh.remove_doubles()   
        bpy.ops.object.mode_set(mode='OBJECT') 
        bpy.ops.object.modifier_add(type='NODES')
        # Get the modifier (it's usually the last one added)
        modifier = bpy.context.object.modifiers[-1]

        
        node_group = bpy.data.node_groups.get('Geometry Nodes')
        if node_group:
            modifier.node_group = node_group
        else:
            print("Node group not found")
            
        bpy.ops.object.make_links_data(type='MODIFIERS')
            

        
            
            

        return {'FINISHED'}        

# Operator for "Auto Place" button
class OBJECT_OT_auto_place(Operator):
    bl_idname = "object.auto_place"
    bl_label = "Auto Place"
    bl_description = "Not implemented yet"

    def execute(self, context):
        # Empty function
        return {'FINISHED'}

class OBJECT_OT_render_output(Operator):
    bl_idname = "object.render_output"
    bl_label = "Render/Save Output"

    def execute(self, context):
        # Error checking
        props = context.scene.PolySlice_props
        output_directory = props.output_directory
        stl_name = props.stl_name
        if not output_directory:
            self.report({'ERROR'}, "No output path selected.")
            return {'CANCELLED'}
        if not stl_name:
            self.report({'ERROR'}, "No STL name selected.")
            return {'CANCELLED'}

        bpy.context.scene.render.filepath = output_directory+"#"
        bpy.ops.render.render('INVOKE_DEFAULT',animation=True)

        bpy.ops.object.mode_set(mode='OBJECT')
        # Select and activate the target object
        bpy.ops.object.select_all(action='DESELECT')
        slice_object = bpy.data.objects["poly_stl_clone"]
        tow = bpy.data.objects["CalibrationTower"]
        pos = bpy.data.objects["Position"]
        slice_object.select_set(True)
        pos.select_set(True)
        tow.select_set(True)
        # Define the export path
        new_name = stl_name.lower().replace(".stl", "")
        absolute_path = bpy.path.abspath(output_directory)
        export_path = absolute_path+new_name+".stl"

        # Export only the selected objects
        bpy.ops.export_mesh.stl(filepath=export_path, use_selection=True)

        return {'FINISHED'}        

# Panel to display the UI elements
class VIEW3D_PT_PolySlice_panel(Panel):
    bl_label = "PolySlice"
    bl_idname = "VIEW3D_PT_PolySllice_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'PolySlice'

    def draw(self, context):
        layout = self.layout
        props = context.scene.PolySlice_props

        
        layout.prop(props, "sink_amount")
        layout.operator("object.sink", text="Sink")
        layout.operator("object.trim_bottom", text="Trim Bottom")
        #layout.operator("object.auto_place", text="Auto Place")
        #layout.prop(props, "color_thickness")
        
        layout.prop(props, "output_directory")
        layout.prop(props, "stl_name")
        layout.prop(props, "first_layer_height")
        layout.prop(props, "layer_height")
        layout.operator("object.slice", text="Slice!")
        layout.operator("object.render_output", text="Render/Save Output")

# Register and unregister classes
classes = (
    PolySliceProperties,
    OBJECT_OT_trim_bottom,
    OBJECT_OT_sink,
    OBJECT_OT_auto_place,
    VIEW3D_PT_PolySlice_panel,
    OBJECT_OT_slice,
    OBJECT_OT_render_output,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.Scene.PolySlice_props = PointerProperty(type=PolySliceProperties)

def unregister():
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    del bpy.types.Scene.PolySlice_props

if __name__ == "__main__":
    register()
