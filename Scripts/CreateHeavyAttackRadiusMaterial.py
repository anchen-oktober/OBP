import unreal


PACKAGE_PATH = "/Game/OneBullet/Materials"
ASSET_NAME = "M_HeavyAttackAuraDecal"


def connect_property(expression, output_name, material_property):
    unreal.MaterialEditingLibrary.connect_material_property(
        expression,
        output_name,
        material_property,
    )


def main():
    unreal.EditorAssetLibrary.make_directory(PACKAGE_PATH)
    asset_path = "{0}/{1}".format(PACKAGE_PATH, ASSET_NAME)
    material = (
        unreal.EditorAssetLibrary.load_asset(asset_path)
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path)
        else None
    )
    if material:
        unreal.log("[OneBullet] Material already exists: {0}".format(asset_path))
        return

    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME,
        PACKAGE_PATH,
        unreal.Material,
        unreal.MaterialFactoryNew(),
    )

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    tint = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -650, -120
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property(
        "default_value",
        unreal.LinearColor(0.22, 0.025, 0.008, 1.0),
    )

    opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -650, 80
    )
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 0.08)

    coordinates = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -650, 300
    )
    center = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionConstant2Vector, -650, 440
    )
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)

    circle = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionSphereMask, -390, 320
    )
    circle.set_editor_property("attenuation_radius", 0.5)
    circle.set_editor_property("hardness_percent", 5.0)
    unreal.MaterialEditingLibrary.connect_material_expressions(coordinates, "", circle, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(center, "", circle, "B")

    final_opacity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -120, 120
    )
    unreal.MaterialEditingLibrary.connect_material_expressions(opacity, "", final_opacity, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(circle, "", final_opacity, "B")

    connect_property(tint, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    connect_property(final_opacity, "", unreal.MaterialProperty.MP_OPACITY)

    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, False)
    unreal.log("[OneBullet] Created {0}".format(asset_path))


if __name__ == "__main__":
    main()
