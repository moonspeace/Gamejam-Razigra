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
    texcoord = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -700, 180)
    # Extract UV.y with a dot product. ComponentMask has the same UE 5.8
    # Python serialization defect as OneMinus and loses its input during cook.
    v_axis = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant2Vector, -700, 300)
    v_axis.set_editor_property("r", 0.0)
    v_axis.set_editor_property("g", 1.0)
    green = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionDotProduct, -500, 180)
    unreal.MaterialEditingLibrary.connect_material_expressions(texcoord, "", green, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(v_axis, "", green, "B")
    # MaterialExpressionOneMinus does not expose a reliably serializable input pin
    # through the UE 5.8 Python API (the asset compiled with "Missing 1-x input").
    # Build the identical 1 - V gradient from a regular Subtract node instead.
    one = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -500, 300)
    one.set_editor_property("r", 1.0)
    start_fade = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionSubtract, -310, 180)
    unreal.MaterialEditingLibrary.connect_material_expressions(one, "", start_fade, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(green, "", start_fade, "B")
    gradient_glow = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -170, -100)
    unreal.MaterialEditingLibrary.connect_material_expressions(glow, "", gradient_glow, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(start_fade, "", gradient_glow, "B")
    unreal.MaterialEditingLibrary.connect_material_property(gradient_glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    opacity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -420, 80)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    gradient_opacity = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionMultiply, -80, 80)
    unreal.MaterialEditingLibrary.connect_material_expressions(opacity, "", gradient_opacity, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(start_fade, "", gradient_opacity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(gradient_opacity, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

    # GlobalGameData is a saved asset, so a property added after that asset was
    # created can retain a stale/None override instead of the native CDO value.
    # Persist the intended laser material explicitly for editor and cooked runs.
    game_data = unreal.EditorAssetLibrary.load_asset("/Game/Data/GlobalGameData")
    if game_data is not None:
        game_data.set_editor_property("laser_trace_material", material)
        unreal.EditorAssetLibrary.save_loaded_asset(game_data, only_if_is_dirty=False)
        unreal.log("Laser material assigned to GlobalGameData: " + material.get_path_name())
    else:
        unreal.log_error("Could not load /Game/Data/GlobalGameData")
    unreal.log("Created Zombie Zero laser material: " + ASSET_PATH)


create_material()
