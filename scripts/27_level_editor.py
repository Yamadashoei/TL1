import bpy
import math
import bpy_extras
import gpu
import gpu_extras.batch
import copy
import mathutils
import json  # JSON形式で出力するために必要


bl_info = {
    "name": "レベルエディタ",
    "author": "Taro Kamata",
    "version": (1, 0),
    "blender": (3, 3, 1),
    "location": "",
    "description": "レベルエディタ",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Object"
}


# 頂点を伸ばすオペレータ
class MYADDON_OT_stretch_vertex(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_stretch_vertex"
    bl_label = "頂点を伸ばす"
    bl_description = "頂点座標を引っ張って伸ばします"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        try:
            bpy.data.objects["Cube"].data.vertices[0].co.x += 1.0
            self.report({'INFO'}, "頂点を伸ばしました。")
        except Exception as e:
            self.report({'ERROR'}, f"エラー: {e}")
        return {'FINISHED'}


# ICO球生成
class MYADDON_OT_create_ico_sphere(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_create_object"
    bl_label = "ICO球生成"
    bl_description = "ICO球を生成します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        bpy.ops.mesh.primitive_ico_sphere_add()
        print("ICO球を生成しました。")
        return {'FINISHED'}


# JSONでシーンをエクスポートするオペレータ
class MYADDON_OT_export_scene(bpy.types.Operator, bpy_extras.io_utils.ExportHelper):
    bl_idname = "myaddon.myaddon_ot_export_scene"
    bl_label = "シーン出力"
    bl_description = "シーン情報をExportします"
    filename_ext = ".json"  # 拡張子は .json に設定

    def export_json(self):
        """JSON形式でファイルに出力"""
        json_object_root = dict()
        json_object_root["name"] = "scene"
        json_object_root["objects"] = list()

        # シーン内の全オブジェクトを走査してパック
        for object in bpy.context.scene.objects:
            if object.parent:
                continue
            self.parse_scene_recursive_json(json_object_root["objects"], object, 0)

        # JSONに整形して文字列化（インデント付き）
        json_text = json.dumps(json_object_root, ensure_ascii=False, cls=json.JSONEncoder, indent=4)
        print(json_text)

        # UTF-8でファイルに書き出し
        with open(self.filepath, "wt", encoding="utf-8") as file:
            file.write(json_text)

    def parse_scene_recursive_json(self, data_parent, object, level):
        """オブジェクト1つを辞書化し、再帰的に子供も処理"""
        json_object = dict()
        json_object["type"] = object.type
        json_object["name"] = object.name

        # トランスフォーム情報（位置・回転・スケール）
        trans, rot, scale = object.matrix_local.decompose()
        rot = rot.to_euler()
        rot.x = math.degrees(rot.x)
        rot.y = math.degrees(rot.y)
        rot.z = math.degrees(rot.z)

        transform = dict()
        transform["translation"] = (trans.x, trans.y, trans.z)
        transform["rotation"] = (rot.x, rot.y, rot.z)
        transform["scaling"] = (scale.x, scale.y, scale.z)
        json_object["transform"] = transform

        # カスタムプロパティ：file_name
        if "file_name" in object:
            json_object["file_name"] = object["file_name"]

        # カスタムプロパティ：collider
        if "collider" in object:
            collider = dict()
            collider["type"] = object["collider"]
            collider["center"] = object["collider_center"].to_list()
            collider["size"] = object["collider_size"].to_list()
            json_object["collider"] = collider

        # 現在のオブジェクトを親リストに追加
        data_parent.append(json_object)

        # 子供オブジェクトを再帰処理
        if len(object.children) > 0:
            json_object["children"] = list()
            for child in object.children:
                self.parse_scene_recursive_json(json_object["children"], child, level + 1)

    def execute(self, context):
        print("シーン情報をExportします")
        self.export_json()
        self.report({'INFO'}, "シーン情報をExportしました")
        return {'FINISHED'}


class MYADDON_OT_add_filename(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_filename"
    bl_label = "FileName 追加"
    bl_description = "[file_name]カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        context.object["file_name"] = ""
        return {'FINISHED'}


# トップバーに追加するカスタムメニュー
class TOPBAR_MT_my_menu(bpy.types.Menu):
    bl_idname = "TOPBAR_MT_my_menu"
    bl_label = "MyMenu"
    bl_description = "拡張メニュー by " + bl_info["author"]

    def draw(self, context):
        layout = self.layout
        layout.operator(MYADDON_OT_stretch_vertex.bl_idname, text=MYADDON_OT_stretch_vertex.bl_label)
        layout.operator(MYADDON_OT_create_ico_sphere.bl_idname, text=MYADDON_OT_create_ico_sphere.bl_label)
        layout.operator(MYADDON_OT_export_scene.bl_idname, text=MYADDON_OT_export_scene.bl_label)

    def submenu(self, context):
        self.layout.menu(TOPBAR_MT_my_menu.bl_idname)


# プロパティウィンドウに file_name 表示用パネル
class OBJECT_PT_file_name(bpy.types.Panel):
    bl_idname = "OBJECT_PT_file_name"
    bl_label = "FileName"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        if "file_name" in context.object:
            self.layout.prop(context.object, '["file_name"]', text=self.bl_label)
        else:
            self.layout.operator(MYADDON_OT_add_filename.bl_idname)


# コライダー描画ユーティリティ
class DrawCollider:
    handle = None

    offsets = [
        [-0.5, -0.5, -0.5], [-0.5, -0.5, 0.5], [-0.5, 0.5, -0.5], [-0.5, 0.5, 0.5],
        [0.5, -0.5, -0.5], [0.5, -0.5, 0.5], [0.5, 0.5, -0.5], [0.5, 0.5, 0.5],
    ]
    size = [2, 2, 2]

    def draw_collider():
        vertices = {"pos": []}
        indices = []

        for obj in bpy.context.scene.objects:
            if "collider" not in obj:
                continue

            center = mathutils.Vector(obj["collider_center"])
            size = mathutils.Vector(obj["collider_size"])
            start = len(vertices["pos"])

            for offset in DrawCollider.offsets:
                pos = copy.copy(center)
                pos[0] += offset[0] * DrawCollider.size[0]
                pos[1] += offset[1] * DrawCollider.size[1]
                pos[2] += offset[2] * DrawCollider.size[2]
                pos = obj.matrix_world @ pos
                vertices['pos'].append(pos)

            indices += [
                [start + 0, start + 1], [start + 2, start + 3],
                [start + 4, start + 5], [start + 6, start + 7],
                [start + 0, start + 2], [start + 1, start + 3],
                [start + 4, start + 6], [start + 5, start + 7],
                [start + 0, start + 4], [start + 1, start + 5],
                [start + 2, start + 6], [start + 3, start + 7],
            ]

        shader = gpu.shader.from_builtin("UNIFORM_COLOR")
        batch = gpu_extras.batch.batch_for_shader(shader, "LINES", vertices, indices=indices)
        shader.bind()
        shader.uniform_float("color", [0.5, 1.0, 1.0, 1.0])
        batch.draw(shader)


# カスタムプロパティ collider の追加用オペレータ
class MYADDON_OT_add_collider(bpy.types.Operator):
    bl_idname = "myaddon.myaddon_ot_add_collider"
    bl_label = "コライダー追加"
    bl_description = "[collider]カスタムプロパティを追加します"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        context.object["collider"] = "Box"
        context.object["collider_center"] = mathutils.Vector((0, 0, 0))
        context.object["collider_size"] = mathutils.Vector((2, 2, 2))
        return {'FINISHED'}


# プロパティウィンドウに collider 表示用パネル
class OBJECT_PT_collider(bpy.types.Panel):
    bl_idname = "OBJECT_PT_collider"
    bl_label = "Collider"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"

    def draw(self, context):
        if "collider" in context.object:
            self.layout.prop(context.object, '["collider"]', text="Type")
            self.layout.prop(context.object, '["collider_center"]', text="Center")
            self.layout.prop(context.object, '["collider_size"]', text="Size")
        else:
            self.layout.operator("myaddon.myaddon_ot_add_collider")


# Blenderへの登録・登録解除
classes = (
    MYADDON_OT_stretch_vertex,
    MYADDON_OT_create_ico_sphere,
    MYADDON_OT_export_scene,
    TOPBAR_MT_my_menu,
    OBJECT_PT_file_name,
    MYADDON_OT_add_filename,
    MYADDON_OT_add_collider,
    OBJECT_PT_collider,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    bpy.types.TOPBAR_MT_editor_menus.append(TOPBAR_MT_my_menu.submenu)
    DrawCollider.handle = bpy.types.SpaceView3D.draw_handler_add(DrawCollider.draw_collider, (), "WINDOW", "POST_VIEW")
    print("レベルエディタが有効化されました。")

def unregister():
    bpy.types.TOPBAR_MT_editor_menus.remove(TOPBAR_MT_my_menu.submenu)
    bpy.types.SpaceView3D.draw_handler_remove(DrawCollider.handle, "WINDOW")
    for cls in reversed(classes):
        bpy.utils.unregister_class(cls)
    print("レベルエディタが無効化されました。")


if __name__ == "__main__":
    register()
