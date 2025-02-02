CRAS = ChromiumOS Audio Server
===

# Directories
- [src/server](src/server) - the source for the sound server
- [src/libcras](src/libcras) - client library for interacting with cras
- [src/common](src/common) - files common to both the server and library
- [src/tests](src/tests) - tests for cras and libcras
- [src/fuzz](src/fuzz) - source code and build scripts for coverage-guided
  fuzzers for CRAS

# Building from source

## Run tests

Using gcc:

```
bazel test //...
```

Or using clang:

```
bazel test //... --config=local-clang
```

Or with sanitizers:

```
bazel test //... --config=local-clang --config=asan
```

Refer to [.bazelrc](../.bazelrc) to see which `--config`s are available.

## Build and package for distribution

```
bazel run //dist -- /path/to/dist
```

## Code completion for for editors

```
bazel run //:compdb
```

Then you'll get `compile_commands.json` for editor.
Import the JSON file to your editor and you'll get useful code complete
features for CRAS and its unit tests.

# Configuration:

## Device Blocklisting:

Blocklist of certain USB output device(s) is possible by modifying the config
file `/etc/cras/device_blocklist`.

The format of this file is as follows:
```
[USB_Outputs]
  <vendor_id>_<product_id>_<checksum>_<device_index> = 1
```
Where vendor_id and product id are the USB identifiers for the card to
blocklist. The checksum is the output of "cksum" command applied to the
sysfs "descriptors" file of the device. The device index specifies the
index of the output device in the card to blocklist.  This is a bool
parameter, so '= 1' enables the option.

Example, blocklisting the non-functional output device reported by the C-Media
based CAD-u1 mic:
```
[USB_Outputs]
  0d8c_0008_00000000_0 = 1
```

## Card Configuration:

There can be a config file for each sound alsa card on the system.  This file
lives in `/etc/cras/`.  The file should be named with the card name returned by
ALSA, the string in the second set of '[]' in the aplay -l output.  The ini file
has the following format.

```
[<output-node-name>] ; Name of the mixer control for this output.
  <config-option> = <config-value>
```
output-node-name can be speficied in a few ways to link with the real node:
- UCM device name - The name string following the SectionDevice label in UCM
    config, i.e. HiFi.conf
- Jack name - Name of the mixer control for mixer jack, or the gpio jack name
    listed by 'evtest' command.
- Mixer control name - e.g. "Headphone" or "Speaker", listed by
    'amixer scontrols' command.

Note that an output node matches to the output-node-name label in card config by
priorty ordered above. For example if a node has UCM device, it will first
search the config file for the UCM device name. When not found, jack name will
be used for searching, and lastly the mixer output control name.

config-option can be the following:
- volume_curve - The type of volume curve, "simple_step" or "explicit".
- Options valid and mandatory when volume_curve = simple_step:
  - max_volume - The maximum volume for this output specified in dBFS * 100.
  - volume_step - Number of dB per volume 'tick' specified in  dBFS * 100.
- Options valid and mandatory when volume_curve = explicit:
  - dB_at_N - The value in dB*100 that should be used for the volume at step
      "N".  There must be one of these for each setting from N=0 to 100
      inclusive.


Example:
This example configures the Headphones to have a max volume of -3dBFS with a
step size of 0.75dBFS and the Speaker to have the curve specified by the steps
given, which is a 1dBFS per step curve from max = +0.5dBFS to min = -99.5dBFS
(volume step 10 is -89.5dBFS).

```
[Headphone]
  volume_curve = simple_step
  volume_step = 75
  max_volume = -300
[Speaker]
  volume_curve = explicit
  dB_at_0 = -9950
  dB_at_1 = -9850
  dB_at_2 = -9750
  dB_at_3 = -9650
  dB_at_4 = -9550
  dB_at_5 = -9450
  dB_at_6 = -9350
  dB_at_7 = -9250
  dB_at_8 = -9150
  dB_at_9 = -9050
  dB_at_10 = -8950
  dB_at_11 = -8850
  dB_at_12 = -8750
  dB_at_13 = -8650
  dB_at_14 = -8550
  dB_at_15 = -8450
  dB_at_16 = -8350
  dB_at_17 = -8250
  dB_at_18 = -8150
  dB_at_19 = -8050
  dB_at_20 = -7950
  dB_at_21 = -7850
  dB_at_22 = -7750
  dB_at_23 = -7650
  dB_at_24 = -7550
  dB_at_25 = -7450
  dB_at_26 = -7350
  dB_at_27 = -7250
  dB_at_28 = -7150
  dB_at_29 = -7050
  dB_at_30 = -6950
  dB_at_31 = -6850
  dB_at_32 = -6750
  dB_at_33 = -6650
  dB_at_34 = -6550
  dB_at_35 = -6450
  dB_at_36 = -6350
  dB_at_37 = -6250
  dB_at_38 = -6150
  dB_at_39 = -6050
  dB_at_40 = -5950
  dB_at_41 = -5850
  dB_at_42 = -5750
  dB_at_43 = -5650
  dB_at_44 = -5550
  dB_at_45 = -5450
  dB_at_46 = -5350
  dB_at_47 = -5250
  dB_at_48 = -5150
  dB_at_49 = -5050
  dB_at_50 = -4950
  dB_at_51 = -4850
  dB_at_52 = -4750
  dB_at_53 = -4650
  dB_at_54 = -4550
  dB_at_55 = -4450
  dB_at_56 = -4350
  dB_at_57 = -4250
  dB_at_58 = -4150
  dB_at_59 = -4050
  dB_at_60 = -3950
  dB_at_61 = -3850
  dB_at_62 = -3750
  dB_at_63 = -3650
  dB_at_64 = -3550
  dB_at_65 = -3450
  dB_at_66 = -3350
  dB_at_67 = -3250
  dB_at_68 = -3150
  dB_at_69 = -3050
  dB_at_70 = -2950
  dB_at_71 = -2850
  dB_at_72 = -2750
  dB_at_73 = -2650
  dB_at_74 = -2550
  dB_at_75 = -2450
  dB_at_76 = -2350
  dB_at_77 = -2250
  dB_at_78 = -2150
  dB_at_79 = -2050
  dB_at_80 = -1950
  dB_at_81 = -1850
  dB_at_82 = -1750
  dB_at_83 = -1650
  dB_at_84 = -1550
  dB_at_85 = -1450
  dB_at_86 = -1350
  dB_at_87 = -1250
  dB_at_88 = -1150
  dB_at_89 = -1050
  dB_at_90 = -950
  dB_at_91 = -850
  dB_at_92 = -750
  dB_at_93 = -650
  dB_at_94 = -550
  dB_at_95 = -450
  dB_at_96 = -350
  dB_at_97 = -250
  dB_at_98 = -150
  dB_at_99 = -50
  dB_at_100 = 50
```

[bear]: https://github.com/rizsotto/Bear
[compile commands]: https://clang.llvm.org/docs/JSONCompilationDatabase.html
[language server plugins in editors]: https://clangd.llvm.org/installation.html#editor-plugins

## DBus docs

See [cras/dbus_bindings/org.chromium.cras.Control.xml](dbus_bindings/org.chromium.cras.Control.xml)
