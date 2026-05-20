import unreal

names = [name for name in dir(unreal.OBEnemyType)]
with open(r"D:\MyProjects\Prototypes\OneBulletLeft\Saved\PythonApiNames.txt", "w", encoding="utf-8") as handle:
    for name in sorted(names):
        handle.write(name + "\n")
