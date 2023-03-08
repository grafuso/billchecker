# billchecker
Calculates your electricity bill in real time (spot prices)

Prerequisites
You need to install rapidjson and csv-parser header-only files and make the paths correct in the meson.build file.

How to run program?
meson setup <builddir>
meson compile -C <builddir>
<builddir>/billchecker --help
