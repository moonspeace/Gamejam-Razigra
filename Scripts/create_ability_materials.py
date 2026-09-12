import unreal


def scalar(material, name, value, x, y):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def color(material, default):
    node = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -800, -160)
    node.set_editor_property("parameter_name", "EffectColor")
    node.set_editor_property("default_value", default)
    return node


tools = unreal.AssetToolsHelpers.get_asset_tools()

# Opaque emissive material: reliable bloom on instanced healing-cross geometry.
healing = unreal.EditorAssetLibrary.load_asset("/Game/Materials/M_HealingPlus")
if healing is None:
    healing = tools.create_asset("M_HealingPlus", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(healing)
healing.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
hcolor = color(healing, unreal.LinearColor(0.02, 1.0, 0.08, 1.0))
hintensity = scalar(healing, "Intensity", 8.0, -800, 20)
hglow = unreal.MaterialEditingLibrary.create_material_expression(healing, unreal.MaterialExpressionMultiply, -480, -100)
unreal.MaterialEditingLibrary.connect_material_expressions(hcolor, "", hglow, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(hintensity, "", hglow, "B")
unreal.MaterialEditingLibrary.connect_material_property(hglow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(healing)
unreal.EditorAssetLibrary.save_loaded_asset(healing, only_if_is_dirty=False)

# Fresnel rim plus animated noisy energy makes the shield translucent but visually active.
shield = unreal.EditorAssetLibrary.load_asset("/Game/Materials/M_PlayerShield")
if shield is None:
    shield = tools.create_asset("M_PlayerShield", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
unreal.MaterialEditingLibrary.delete_all_material_expressions(shield)
shield.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
shield.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
shield.set_editor_property("two_sided", True)
scolor = color(shield, unreal.LinearColor(0.0, 0.12, 0.8, 0.1))
sintensity = scalar(shield, "Intensity", 2.0, -800, -20)
fresnel = unreal.MaterialEditingLibrary.create_material_expression(shield, unreal.MaterialExpressionFresnel, -800, 160)
glow = unreal.MaterialEditingLibrary.create_material_expression(shield, unreal.MaterialExpressionMultiply, -500, -100)
unreal.MaterialEditingLibrary.connect_material_expressions(scolor, "", glow, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(sintensity, "", glow, "B")
rim = unreal.MaterialEditingLibrary.create_material_expression(shield, unreal.MaterialExpressionMultiply, -250, -30)
unreal.MaterialEditingLibrary.connect_material_expressions(glow, "", rim, "A")
unreal.MaterialEditingLibrary.connect_material_expressions(fresnel, "", rim, "B")
unreal.MaterialEditingLibrary.connect_material_property(rim, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
opacity_scale = unreal.MaterialEditingLibrary.create_material_expression(shield, unreal.MaterialExpressionMultiply, -250, 150)
opacity_scale.set_editor_property("const_b", 0.22)
unreal.MaterialEditingLibrary.connect_material_expressions(fresnel, "", opacity_scale, "A")
unreal.MaterialEditingLibrary.connect_material_property(opacity_scale, "", unreal.MaterialProperty.MP_OPACITY)
unreal.MaterialEditingLibrary.recompile_material(shield)
unreal.EditorAssetLibrary.save_loaded_asset(shield, only_if_is_dirty=False)
unreal.log("Created Zombie Zero ability materials")
