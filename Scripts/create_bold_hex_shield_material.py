import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
path = "/Game/Materials/M_PlayerShieldHexBold"
material = unreal.EditorAssetLibrary.load_asset(path)
if material is None:
    material = tools.create_asset("M_PlayerShieldHexBold", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)

def scalar(name, value, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node

color = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -1000, -180)
color.set_editor_property("parameter_name", "EffectColor")
color.set_editor_property("default_value", unreal.LinearColor(0.0, 0.16, 1.0, 1.0))
intensity = scalar("Intensity", 5.0, -1000, -40)
hex_intensity = scalar("HexIntensity", 1.35, -600, 360)
opacity = scalar("Opacity", 0.11, 200, 260)
fresnel = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionFresnel, -600, 40)
fresnel_gain = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 40)
fresnel_gain.set_editor_property("const_b", 0.72)
unreal.MaterialEditingLibrary.connect_material_expressions(fresnel, "", fresnel_gain, "A")

uv = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -1000, 300)
uv_scale = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -800, 300)
uv_scale.set_editor_property("const_b", 4.0)
unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", uv_scale, "A")
texture = unreal.EditorAssetLibrary.load_asset("/Game/ParagonMurdock/FX/Textures/Tile/Tech/T_HexPattern_Packed")
hex_sample = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -600, 230)
hex_sample.set_editor_property("texture", texture)
unreal.MaterialEditingLibrary.connect_material_expressions(uv_scale, "", hex_sample, "Coordinates")
hex_gain = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -350, 260)
unreal.MaterialEditingLibrary.connect_material_expressions(hex_sample, "R", hex_gain, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(hex_intensity, "", hex_gain, "B")
pattern = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionAdd, -100, 110)
unreal.MaterialEditingLibrary.connect_material_expressions(fresnel_gain, "", pattern, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(hex_gain, "", pattern, "B")

base_glow = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -600, -140)
unreal.MaterialEditingLibrary.connect_material_expressions(color, "", base_glow, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(intensity, "", base_glow, "B")
final_glow = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, 120, -50)
unreal.MaterialEditingLibrary.connect_material_expressions(base_glow, "", final_glow, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(pattern, "", final_glow, "B")
unreal.MaterialEditingLibrary.connect_material_property(final_glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

final_opacity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, 420, 170)
unreal.MaterialEditingLibrary.connect_material_expressions(pattern, "", final_opacity, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(opacity, "", final_opacity, "B")
unreal.MaterialEditingLibrary.connect_material_property(final_opacity, "", unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
unreal.log("Created prominent emissive hex shield material")
