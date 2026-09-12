import unreal


ASSET_PATH = "/Game/Materials/M_ZombieDeath"


def create_or_update_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if material is None:
        material = asset_tools.create_asset(
            "M_ZombieDeath", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew()
        )
    if material is None:
        raise RuntimeError("Could not create M_ZombieDeath")

    # Rebuild deterministically so the checked-in asset always matches this script.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    material.set_editor_property("opacity_mask_clip_value", 0.3333)

    death_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -850, -240
    )
    death_color.set_editor_property("parameter_name", "DeathColor")
    death_color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.0, 0.0, 1.0))

    blink = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -850, -100
    )
    blink.set_editor_property("parameter_name", "BlinkAmount")
    blink.set_editor_property("default_value", 1.0)

    emissive = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -520, -200
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(death_color, "", emissive, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(blink, "", emissive, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )

    noise = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionNoise, -950, 180
    )
    noise.set_editor_property("scale", 38.0)
    noise.set_editor_property("quality", 1)
    noise.set_editor_property("levels", 3)
    noise.set_editor_property("output_min", 0.0)
    noise.set_editor_property("output_max", 1.0)

    noise_scale = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -670, 180
    )
    noise_scale.set_editor_property("const_b", 0.65)
    unreal.MaterialEditingLibrary.connect_material_expressions(noise, "", noise_scale, "A")

    dissolve = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -950, 380
    )
    dissolve.set_editor_property("parameter_name", "DissolveAmount")
    dissolve.set_editor_property("default_value", 0.0)

    dissolve_scale = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -670, 380
    )
    dissolve_scale.set_editor_property("const_b", -2.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(dissolve, "", dissolve_scale, "A")

    combine = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionAdd, -390, 260
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(noise_scale, "", combine, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(dissolve_scale, "", combine, "B")

    bias = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionAdd, -170, 260
    )
    bias.set_editor_property("const_b", 1.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(combine, "", bias, "A")
    unreal.MaterialEditingLibrary.connect_material_property(
        bias, "", unreal.MaterialProperty.MP_OPACITY_MASK
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log("Created Zombie Zero death material: " + ASSET_PATH)


create_or_update_material()
