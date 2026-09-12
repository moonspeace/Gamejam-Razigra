import unreal


ASSET_PATH = "/Game/Materials/M_LaserTrace"


def create_material():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if material is None:
        material = tools.create_asset("M_LaserTrace", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("used_with_static_lighting", True)

    color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -700, -180)
    color.set_editor_property("parameter_name", "LaserColor")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.04, 0.01, 1.0))
    intensity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -700, -40)
    intensity.set_editor_property("parameter_name", "Intensity")
    intensity.set_editor_property("default_value", 35.0)
    glow = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -420, -130)
    unreal.MaterialEditingLibrary.connect_material_expressions(color, "", glow, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(intensity, "", glow, "B")
    unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    opacity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, 80)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    unreal.MaterialEditingLibrary.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log("Created Zombie Zero laser material: " + ASSET_PATH)


create_material()
