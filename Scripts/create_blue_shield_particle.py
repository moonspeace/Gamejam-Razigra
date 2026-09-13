import unreal

source_path = "/Game/ParagonMurdock/FX/Particles/Abilities/GunShield/fx/P_GunShield"
destination_path = "/Game/Effects/P_GunShield_Blue"
source = unreal.EditorAssetLibrary.load_asset(source_path)
particle = unreal.EditorAssetLibrary.load_asset(destination_path)
if particle is None:
    particle = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
if particle is None:
    raise RuntimeError("Could not duplicate the Paragon shield particle system")

blue = unreal.Vector(0.0, 0.16, 1.0)
changed = 0
for emitter in particle.get_editor_property("emitters"):
    if emitter is None:
        continue
    for lod in emitter.get_editor_property("lod_levels"):
        if lod is None:
            continue
        modules = list(lod.get_editor_property("modules"))
        required = lod.get_editor_property("required_module")
        if required is not None:
            modules.append(required)
        for module in modules:
            if module is None:
                continue
            if module.get_class().get_name() == "ParticleModuleColor":
                raw = module.get_editor_property("start_color")
                distribution = raw.get_editor_property("distribution")
                if distribution and distribution.get_class().get_name() == "DistributionVectorConstant":
                    distribution.set_editor_property("constant", blue)
                    changed += 1

unreal.EditorAssetLibrary.save_loaded_asset(particle, only_if_is_dirty=False)
unreal.log("Created blue Paragon shield particle; recolored {} color modules".format(changed))
