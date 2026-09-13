import unreal

asset = unreal.EditorAssetLibrary.load_asset('/Game/BP_Gate')
if not asset:
    unreal.log_error('BP_Gate could not be loaded')
else:
    unreal.log_warning('GATE asset class: ' + asset.get_class().get_name())
    generated = asset.generated_class()
    unreal.log_warning('GATE generated class: ' + generated.get_path_name())
    cdo = unreal.get_default_object(generated)
    unreal.log_warning('GATE CDO: ' + cdo.get_path_name())
    for name in sorted(dir(cdo)):
        low = name.lower()
        if any(word in low for word in ('gate', 'open', 'close', 'reset', 'timeline', 'door')):
            unreal.log_warning('GATE member: ' + name)
    scs = asset.get_editor_property('simple_construction_script')
    if scs:
        for node in scs.get_all_nodes():
            template = node.get_editor_property('component_template')
            unreal.log_warning('GATE component: ' + template.get_name() + ' / ' + template.get_class().get_name())
