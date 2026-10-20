# Attribution and source status

This is an unofficial development adaptation of ConfiG (cgytrus)'s
[Level Shaders Fix](https://github.com/cgytrus/LevelShadersFix), version 1.0.3,
commit c481320e0e3e6e6d21e107365d228eb90cc6d92a.

The shader resources are unchanged upstream files. The uniform binding table,
native-resolution setup, pixelation correction and camera rotation math in
src/main.cpp derive from that project. This package does not claim to be an
official release by ConfiG.

No LICENSE file was present in the inspected upstream root. No license for
upstream code or shaders is invented or granted by this package. Confirm
redistribution terms with the author before a public release or index submission.

The references/*.bro files are selected class declarations from
[Geode bindings](https://github.com/geode-sdk/bindings) at the commit in
PROVENANCE.json, included for source review; they are not a replacement SDK.
The bindings repository carries its own licensing terms.
