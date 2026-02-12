# Blender exporter: writes simple JSON level 
import bpy, json, sys, os

argv = sys.argv
if "--" in argv:
    argv = argv[argv.index("--") + 1:]
else:
    argv = []
outPath = argv[0] if len(argv) > 0 else "level.json"
outPath = bpy.path.abspath("//" + outPath)  # write relative to the blend file

def makeJsonable(serializable):
    try:
        json.dumps(serializable)
        return serializable
    except (TypeError, OverflowError):
        return str(serializable)

def worldTransform(obj):
    worldMatrix = obj.matrix_world
    location = worldMatrix.to_translation()
    rotation = worldMatrix.to_euler('XYZ')
    scale = worldMatrix.to_scale()
    return {
        "translation":[location.x, location.y, location.z],
        "rotation":[rotation.x, rotation.y, rotation.z],
        "scale":[scale.x, scale.y, scale.z]
    }

level = { "entities": [] }

for obj in bpy.context.scene.objects:
    if obj.get("export_as_entity", False):
        mesh_name = obj.data.name if obj.type == 'MESH' and obj.data else None
        entity = 
        {
            "name": obj.name,
            "mesh": mesh_name,
            "trs": worldTransform(obj),
            "props": {}
        }
        # copy all custom properties
        for custom, serializable in obj.items():
            if custom != '_RNA_UI' and custom != 'export_as_entity':
                entity["props"][custom] = makeJsonable(serializable)
        level["entities"].append(entity)

# collect materials/meshes referenced
with open(outPath, "w", encoding="utf-8") as collected:
    json.dump(level, collected, indent=2)
print("Exported", len(level["entities"]), "entities to", outPath)