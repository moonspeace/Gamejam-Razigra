import unreal

data = unreal.EditorAssetLibrary.load_asset('/Game/Data/GlobalGameData')
if not data:
    unreal.log_error('GlobalGameData missing')
else:
    for prop in ('hero_blueprint', 'zombie_class', 'zombie_mesh', 'zombie_animation_class', 'zombie_clothing_slots'):
        try:
            unreal.log_warning(f'CHARREF {prop}: {data.get_editor_property(prop)}')
        except Exception as exc:
            unreal.log_error(f'CHARREF {prop}: {exc}')

for path in (
    '/Game/Blueprints/BP_SharedHero',
    '/Game/ZombieMale_AAB/Meshes/SKM_ZombieM_Male_WholeBody',
    '/Game/ZombieMale_AAB/Animations/ABP_Zombie',
):
    asset = unreal.EditorAssetLibrary.load_asset(path)
    unreal.log_warning(f'CHARREF load {path}: {asset}')
