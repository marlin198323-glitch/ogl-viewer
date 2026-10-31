#!/usr/bin/env python3
"""Install an optional, native Firestorm graphics preset for NexVerse.

The preset ONLY changes graphics settings if the user selects it in the OGL
Viewer. It does not edit settings.xml or change any logged-in viewer session.
This is a raster/PBR visual-quality preset, NOT an RTX ray-tracing renderer.
"""
from __future__ import annotations
import argparse
import os
from pathlib import Path
from urllib.parse import quote
from xml.etree import ElementTree as ET

NAME = "NexVerse Cinematic"
# All values are supported by Firestorm 7.2.5's graphics preset controls.
# Designed for good NVIDIA GPU; heavy reflections can reduce FPS at ultrawide.
CHANGES = {
    "RenderShadowDetail": "2",
    "RenderShadowResolutionScale": "1.5",
    "RenderReflectionProbeLevel": "3",
    "RenderReflectionProbeDetail": "1",
    "RenderScreenSpaceReflections": "1",
    "RenderScreenSpaceReflectionIterations": "32",
    "RenderWaterRefResolution": "1024",
    "RenderAnisotropic": "1",
    "RenderFSAASamples": "2",
    "RenderFSAAType": "2",
    "RenderVolumeLODFactor": "3.0",
    "RenderAvatarLODFactor": "1.0",
    "RenderTerrainLODFactor": "2.0",
    "RenderTreeLODFactor": "1.0",
    "RenderFarClip": "192.0",
    "RenderGlow": "1",
    "RenderDeferredSSAO": "1",
    "RenderTransparentWater": "1",
    "RenderReflectionsEnabled": "1",
}
PROFILE = Path.home() / ".local/share/ogl-firestorm-test/user_settings"
PRESET_NAME = quote(NAME, safe="") + ".xml"


def settings(tree: ET.ElementTree):
    root = tree.getroot()
    if root.tag != "llsd" or len(root) != 1 or root[0].tag != "map":
        raise ValueError("Expected Firestorm LLSD graphics preset")
    m = root[0]
    values = {}
    for key, value in zip(m[0::2], m[1::2]):
        if key.tag != "key" or value.tag != "map":
            raise ValueError("Unexpected graphics preset entry")
        nested = {k.text: v for k, v in zip(value[0::2], value[1::2]) if k.tag == "key"}
        values[key.text] = nested
    return values


def build(source: Path, destination: Path, install: bool) -> None:
    tree = ET.parse(source)
    fields = settings(tree)
    for name, value in CHANGES.items():
        if name not in fields or "Value" not in fields[name]:
            raise ValueError(f"Unsupported Firestorm setting: {name}")
        v = fields[name]["Value"]
        if v.tag not in {"integer", "boolean", "real"}:
            raise ValueError(f"Unexpected setting type {name}: {v.tag}")
        v.text = value

    print(f"Preset: {NAME} ({len(CHANGES)} quality controls)")
    print("Renderer: OpenGL/PBR with SSR; actual Vulkan RTX effects: NOT implemented")
    print(f"Output: {destination}")
    if not install:
        print("Dry run: no settings or files changed. Use --install to add preset.")
        return
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        raise FileExistsError("Preset already exists; refusing to overwrite it")
    # Write file exclusively. Existing Default and user preferences untouched.
    with destination.open("xb") as out:
        ET.indent(tree, space="  ")
        tree.write(out, encoding="utf-8", xml_declaration=True)
    print("Installed. Choose 'NexVerse Cinematic' in OGL's graphics presets.")
    print("Default remains available to revert; FPS and visual differences depend on the region.")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true", help="write new preset, do not activate it")
    parser.add_argument("--profile", type=Path, default=PROFILE,
                        help="viewer user_settings directory (for testing/alternate profiles)")
    args = parser.parse_args()
    folder = args.profile / "presets" / "graphic"
    build(folder / "Default.xml", folder / PRESET_NAME, args.install)


if __name__ == "__main__":
    main()
