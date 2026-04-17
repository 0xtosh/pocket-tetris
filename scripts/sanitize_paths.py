import os

Import("env")


def normalize(path):
    return os.path.normpath(path).replace("\\", "/")


prefix_maps = []

project_dir = env.subst("$PROJECT_DIR")
if project_dir:
    prefix_maps.append((normalize(project_dir), "."))

user_profile = os.environ.get("USERPROFILE")
if user_profile:
    prefix_maps.append((normalize(user_profile), "/user"))

seen = set()
for source, replacement in prefix_maps:
    if not source or source in seen:
        continue
    seen.add(source)
    env.Append(
        CCFLAGS=[
            f"-ffile-prefix-map={source}={replacement}",
            f"-fmacro-prefix-map={source}={replacement}",
            f"-fdebug-prefix-map={source}={replacement}",
        ]
    )