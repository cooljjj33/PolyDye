# Third-Party Attribution and Modification Notice

CMY Labs PTY LTD's original copyrightable contributions are **NCL-1.0 only**, subject to
any separately granted commercial permission. Every upstream notice in this
document and the source files applies to its respective upstream material.
These notices are not alternative licenses for CMY Labs PTY LTD's contributions.
See LICENSE section 20 for the controlling scope rule.

`licensing/MANIFEST.json` records the files, contribution scopes, upstream
sources, and runtime dependencies in this source distribution. Complete
supplemental license texts are in `LICENSES/`.

## Jeroen Domburg (Sprite_tm) — printercart_simple

**Original author:** Jeroen Domburg, <jeroen@spritesmods.com>  
**Original repository:** https://github.com/Spritetm/printercart_simple  
**Upstream commit:** `4f6bbfad0e1d40917788b15a196d17a393c05139`  
**Original license:** THE BEER-WARE LICENSE (Revision 42)  
**PolyDye original additions and copyrightable modifications:** Noncommercial Copyleft License 1.0 (NCL-1.0), as set out in `LICENSE`.

### Modification notice and scope

PolyDye incorporates and adapts portions of Jeroen Domburg's `printercart_simple` printer-cartridge driver. Jeroen's original material remains licensed under its original Beerware terms, whether unchanged as a complete file or retained within a modified file. This notice does **not** attempt to relicense or impose NCL-1.0 restrictions on that original material. Subject to retaining the Beerware notice, users remain free to exercise the original Beerware permissions, including commercial uses of that original material.

CMY Labs PTY LTD licenses **only its own independently copyrightable additions and modifications, exclusively under NCL-1.0**. Files containing both bodies of material retain both sets of rights. The NCL-1.0 license is not a restriction on Jeroen's separately reusable original code.

### File inventory — PolyDye

All paths in this table are relative to the project `App/` root. 

| Upstream file | PolyDye file | Relationship |
| --- | --- | --- |
| `main/main.c` | `main/cartmain.c` | Renamed and adapted |
| `components/printcart/printcart_buffer_filler.c` | `components/printcart/printcart_buffer_filler.c` | Adapted |
| `components/printcart/printcart_buffer_filler.h` | `components/printcart/printcart_buffer_filler.h` | Adapted |
| `components/printcart/printcart_genwaveform.c` | `components/printcart/printcart_genwaveform.c` | Adapted |
| `components/printcart/printcart_genwaveform.h` | `components/printcart/printcart_genwaveform.h` | Adapted |
| `components/printcart/printcart_i2s.c` | `components/printcart/printcart_i2s.c` | Adapted |
| `components/printcart/printcart_i2s.h` | `components/printcart/printcart_i2s.h` | Adapted |
| `components/printcart/CMakeLists.txt` | `components/printcart/CMakeLists.txt` | Build file; adapted |
| `components/printcart/component.mk` | `components/printcart/component.mk` | Build file; adapted |

### Original Beerware notice (Revision 42)

```text
"THE BEER-WARE LICENSE" (Revision 42):
Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
this notice you can do whatever you want with this stuff. If we meet some day,
and you think this stuff is worth it, you can buy me a beer in return.
```

The centralized attribution supplements all original license notices in individual files. Preserve existing original headers when redistributing or modifying source files. No affiliation with, sponsorship by, or endorsement from Jeroen Domburg is implied.

## miniz (vendored source)

**Files:** `main/miniz.c`, `main/miniz.h`  
**Project:** https://github.com/richgel999/miniz  
**Version:** 3.0.2, commit `293d4db1b7d0ffee9756d035b9ac6f7431ef8492`. The two files are the upstream amalgamation, retained unchanged.

Includes MIT-licensed code, an Unlicense/public-domain dedication, and a public-domain PNG writer by Alex Evans. Original notices are retained in the source.

### MIT notice

```text
Copyright 2013-2014 RAD Game Tools and Valve Software
Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC
Copyright 2016 Martin Raiber
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## W3.CSS and normalize.css extract

**File:** `front/phantom-app/src/styles.scss`  
**W3.CSS authors:** Jan Egil and Borge Refsnes  
**Source and usage permission:** https://www.w3schools.com/w3css/w3css_downloads.asp

This file contains W3.CSS 4.15 (December 2020). W3Schools makes W3.CSS freely available for use without a license requirement. The normalize.css extract at lines 11–32 retains its MIT terms. CMY Labs PTY LTD application styles are in the other project stylesheets under NCL-1.0.

Includes an extract from normalize.css by Nicolas Gallagher and Jonathan Neal under the MIT license:

**normalize.css source:** https://github.com/necolas/normalize.css  
**License reference:** https://github.com/necolas/normalize.css/blob/7.0.0/LICENSE.md

```text
The MIT License (MIT)

Copyright (c) Nicolas Gallagher and Jonathan Neal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## Espressif example-derived source (Public Domain / CC0)

The following files retain Espressif Public Domain / CC0 material.
The original upstream headers are preserved, and the scope notices identify
CMY Labs PTY LTD's original additions as NCL-only.

- `main/cdc_acm_vcp.cpp`
- `main/decode_image.c` (adapted from the `esp_jpeg` get-started example)
- `main/decode_image.h`
- `main/esp_rest_main.c`
- `main/rest_server.c`
- `main/sd_card_main.c`
- `main/station.c` (adapted from the Wi-Fi station example)

**Copyright:** Espressif Systems (Shanghai) CO LTD, 2022, where specified in the original SPDX headers.

**Upstream SDK:** https://github.com/espressif/esp-idf/tree/v5.0.9  
**Wi-Fi station example:** https://github.com/espressif/esp-idf/blob/v5.0.9/examples/wifi/getting_started/station/main/station_example_main.c  
**CC0-1.0:** https://creativecommons.org/publicdomain/zero/1.0/

Original third-party portions retain the permissions granted by their respective licenses.


## ESP-IDF example build and configuration material

`main/CMakeLists.txt` and `main/Kconfig.projbuild` contain adapted material from
the v5.0.9 REST server example. They have no upstream per-file license override;
the repository's Apache-2.0 baseline is retained for that material.
Copyright (C) 2015-2022 Espressif Systems.
The files are marked as modified. CMY Labs PTY LTD's independently copyrightable
contributions remain NCL-only.

Reference: https://github.com/espressif/esp-idf/tree/v5.0.9/examples/protocols/http_server/restful_server/main
Upstream scope: https://github.com/espressif/esp-idf/blob/v5.0.9/docs/en/COPYRIGHT.rst
Complete Apache-2.0 license: `LICENSES/Apache-2.0.txt`.

## Angular CLI generated scaffolds

The generated frontend scaffold portions identified in `licensing/MANIFEST.json`
come from Angular CLI's `@schematics/angular` 9.1.15 package and retain its MIT
license. Copyright (c) 2017 Google, Inc. The complete notice is in
`LICENSES/Angular-CLI-MIT.txt`.

This scope includes the retained generated project configuration, documentation,
bootstrap, page, environment, module and test scaffolds listed in the manifest.
CMY Labs PTY LTD's original additions and modifications remain NCL-1.0 only.
The original CMY Labs PTY LTD PNGs and favicons are NCL-1.0 only.

## Original CMY Labs PTY LTD content

Project code, documentation, configuration, and assets outside the identified
upstream material are original CMY Labs PTY LTD content under NCL-1.0 only.
This includes the independently written LCD/GDMA additions and header
declarations, `main/rest_server.h`, the five PNG images, and both favicon forms.
Retained printercart expression within the I/O files remains Beerware.

## Bundled Espressif managed components

Each managed component retains its upstream license files and source
notices. The per-file entries in `licensing/MANIFEST.json` identify the package
baseline and the applicable CC0, Unlicense, and TJpgDec exceptions.

| Directory under `managed_components/` | Version | Upstream revision | Baseline / exceptions |
| --- | --- | --- | --- |
| `espressif__esp_jpeg` | 1.3.1 | [746e83ddbea0db9c3d24993a87c4c737a60337ae](https://github.com/espressif/idf-extra-components/tree/746e83ddbea0db9c3d24993a87c4c737a60337ae/esp_jpeg) | Apache-2.0; preserve per-file overrides; TJpgDec: ChaN permissive notice; example files: CC0 |
| `espressif__led_strip` | 2.5.5 | [60c14263f3b69ac6e98ecae79beecbe5c18d5596](https://github.com/espressif/idf-extra-components/tree/60c14263f3b69ac6e98ecae79beecbe5c18d5596/led_strip) | Apache-2.0; preserve per-file overrides |
| `espressif__mdns` | 1.11.1 | [09e0dafe9e53f0a0abcd83b20dc79f9fecf902bd](https://github.com/espressif/esp-protocols/tree/09e0dafe9e53f0a0abcd83b20dc79f9fecf902bd/components/mdns) | Apache-2.0; preserve per-file overrides |
| `espressif__usb_host_cdc_acm` | 2.4.0 | [d2674397bc5ce0b5d9262fb179fdb06c1090801c](https://github.com/espressif/esp-usb/tree/d2674397bc5ce0b5d9262fb179fdb06c1090801c/host/class/cdc/usb_host_cdc_acm) | Apache-2.0; preserve per-file overrides |
| `espressif__usb_host_ch34x_vcp` | 2.2.0 | [ebfa6f84ee7c7c070dcc244c79573eeab228298e](https://github.com/espressif/esp-usb/tree/ebfa6f84ee7c7c070dcc244c79573eeab228298e/host/class/cdc/usb_host_ch34x_vcp) | Apache-2.0; preserve per-file overrides |
| `espressif__usb_host_cp210x_vcp` | 2.2.0 | [ebfa6f84ee7c7c070dcc244c79573eeab228298e](https://github.com/espressif/esp-usb/tree/ebfa6f84ee7c7c070dcc244c79573eeab228298e/host/class/cdc/usb_host_cp210x_vcp) | Apache-2.0; preserve per-file overrides |
| `espressif__usb_host_ftdi_vcp` | 2.1.1 | [cb8e2bd4551b4ec5693ae2b14175e5f99722456e](https://github.com/espressif/esp-usb/tree/cb8e2bd4551b4ec5693ae2b14175e5f99722456e/host/class/cdc/usb_host_ftdi_vcp) | Apache-2.0; preserve per-file overrides |
| `espressif__usb_host_vcp` | 1.0.0~5 | [e4817e2e9aa456e86c0037739ce087571a9242ec](https://github.com/espressif/esp-usb/tree/e4817e2e9aa456e86c0037739ce087571a9242ec/host/class/cdc/usb_host_vcp) | Apache-2.0; preserve per-file overrides |


Across these packages, retained SPDX declarations include Apache-2.0,
`Unlicense OR CC0-1.0`, and CC0-1.0. TJpgDec's module notice is reproduced in
`LICENSES/TJpgDec-NOTICE.txt`; it permits commercial use and requires retaining
its copyright notice in source redistributions. A package-level Apache label
does not replace a more specific per-file or module notice.

## Frontend runtime dependencies

The following are the 36 non-development package entries in the frontend
lockfile, including transitive dependencies. Their license notices are under
`LICENSES/npm/`. The manifest's `runtime_dependencies` section records exact
versions, selected terms, package integrity values, and notice paths.

JSZip is used under its **MIT option** for this distribution. Its upstream
dual-license notice is retained; it is not a grant of GPL or MIT rights in CMY Labs PTY LTD code. Font Awesome icons retain CC BY 4.0 and associated code retains MIT.
The full CC BY 4.0 text is also supplied in `LICENSES/CC-BY-4.0.txt`.
Pako retains both its MIT and zlib notices.

| Package | Version | Applicable declared terms |
| --- | --- | --- |
| `@angular/animations` | 16.2.0 | MIT |
| `@angular/common` | 16.2.0 | MIT |
| `@angular/compiler` | 16.2.0 | MIT |
| `@angular/core` | 16.2.0 | MIT |
| `@angular/forms` | 16.2.0 | MIT |
| `@angular/platform-browser` | 16.2.0 | MIT |
| `@angular/platform-browser-dynamic` | 16.2.0 | MIT |
| `@angular/router` | 16.2.0 | MIT |
| `@fortawesome/angular-fontawesome` | 0.13.0 | MIT |
| `@fortawesome/fontawesome-common-types` | 6.3.0 | MIT |
| `@fortawesome/fontawesome-svg-core` | 6.3.0 | MIT |
| `@fortawesome/free-solid-svg-icons` | 6.3.0 | (CC-BY-4.0 AND MIT) |
| `@popperjs/core` | 2.11.8 | MIT |
| `bootstrap` | 5.3.1 | MIT |
| `core-util-is` | 1.0.3 | MIT |
| `file-saver` | 2.0.5 | MIT |
| `glur` | 1.1.2 | MIT |
| `immediate` | 3.0.6 | MIT |
| `inherits` | 2.0.4 | ISC |
| `isarray` | 1.0.0 | MIT |
| `jszip` | 3.10.1 | MIT |
| `readable-stream` | 2.3.8 | MIT |
| `safe-buffer` | 5.1.2 | MIT |
| `string_decoder` | 1.1.1 | MIT |
| `lie` | 3.3.0 | MIT |
| `multimath` | 2.0.0 | MIT |
| `object-assign` | 4.1.1 | MIT |
| `pako` | 1.0.11 | (MIT AND Zlib) |
| `pica` | 9.0.1 | MIT |
| `process-nextick-args` | 2.0.1 | MIT |
| `rxjs` | 7.8.1 | Apache-2.0 |
| `setimmediate` | 1.0.5 | MIT |
| `tslib` | 2.6.1 | 0BSD |
| `util-deprecate` | 1.0.2 | MIT |
| `webworkify` | 1.5.0 | MIT |
| `zone.js` | 0.13.1 | MIT |


Frontend builds copy this document, the root LICENSE, the license texts,
and the manifest into `licenses/` relative to the output directory. License
extraction is enabled. Include generated third-party notices with compiled
distributions. Separately installed SDKs and build tools retain their own terms.
