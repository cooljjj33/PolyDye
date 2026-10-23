# License manifest

MANIFEST.json records the files and licensing scopes in this source release,
the locked frontend runtime dependencies, and the accompanying license texts.
All local paths are relative to the repository root.

For a mixed file, each license applies to its identified contribution.
An AND expression describes the licenses required for the combined file;
it does not impose NCL on the separately reusable upstream material.
Upstream OR expressions preserve choices granted by the upstream licensor.
No upstream license is an alternative grant for CMY Labs PTY LTD's original
content. The controlling scope rule is in LICENSE section 20.

The short LICENSES/npm/pNN/ folder IDs map to full package names and versions in
runtime_dependencies. They keep deployed filenames within the SPIFFS limit.
