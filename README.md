# MAKE ME DIRTY

[Русская инструкция](README.ru.md)

**Make modern audio sound like it came from a forgotten game in 1999.**

MAKE ME DIRTY is a real-time mono/stereo degradation effect for Windows and macOS. It combines a duration-preserving granular pitch shifter, bit-depth quantization, sample-and-hold rate reduction, three distortion characters, narrow-band tone shaping, texture noise, and a latency-aligned global dry/wet control. Every control is host-automatable.

## Formats

- Windows: VST3 and Standalone
- macOS: VST3, Audio Unit (AU), and Standalone

The Windows release ZIP contains `MAKE ME DIRTY.vst3` for a DAW and `MAKE ME DIRTY.exe` for use without a DAW. A VST3 on Windows is a folder with the `.vst3` extension; the actual binary DLL is inside `Contents/x86_64-win/`. Copy the complete `.vst3` folder, not just the DLL, to `%CommonProgramFiles%\VST3\`, then rescan plug-ins in the DAW. Run the standalone EXE directly. AU can only be built on macOS.

The display name and visual design changed from RETROGRADE FX. Internal plug-in identifiers and parameter IDs remain the same so existing host projects can find their settings. Existing user presets continue to load from the original per-user preset location.

The pitch shifter uses two overlapping granular read heads. Its unity-pitch grain-center latency is reported to the host, and the dry path is delayed by the same amount so parallel mixing remains time aligned. At 48 kHz the reported latency is approximately 24 ms; shifted material is granular by design and may have a small time spread around that center.

## Build prerequisites

- CMake 3.22 or newer
- A C++17 compiler: Visual Studio 2022 on Windows, or current Xcode on macOS
- Git or internet access for CMake to download the pinned JUCE 8.0.4 source archive

No separate VST3 SDK is required; current JUCE releases include what is needed for VST3 and AU builds. JUCE's licensing terms still apply to anyone distributing a compiled product.

### Windows

From a Developer PowerShell:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target RetrogradeFX_All RetrogradeTests
ctest --test-dir build -C Release --output-on-failure
```

Artifacts are produced below `build/RetrogradeFX_artefacts/Release/`. Copy the generated `.vst3` bundle to `%CommonProgramFiles%\VST3\`, or configure with `-DJUCE_COPY_PLUGIN_AFTER_BUILD=ON` if your local permissions allow it. The Standalone executable can run without a DAW.

### macOS

```bash
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release --target RetrogradeFX_All RetrogradeTests
ctest --test-dir build -C Release --output-on-failure
```

The AU component is generated only on Apple platforms. For distribution outside your own machine, sign and notarize the VST3, AU, and Standalone bundles using your Apple Developer identity.

## Using the effect

### Quick start

1. Download and extract the Windows x64 ZIP from this repository or its Releases page.
2. Copy the complete `MAKE ME DIRTY.vst3` folder to `C:\Program Files\Common Files\VST3\`. Windows may request administrator permission. Restart your DAW and rescan its audio plug-ins.
3. Add MAKE ME DIRTY as an audio effect on a track or bus. Alternatively, launch `MAKE ME DIRTY.exe`, choose an audio input and output in Settings, and process live audio. For audio files, use a VST3 host.
4. Select a preset and play audio. Adjust the knobs with the mouse; double-click a knob to restore its default. Start with Dry/Wet around 30–50% and watch the output level. SAVE stores a user preset; the `...` menu manages presets; BYPASS switches the effect off.

BITCRUSHER reduces bit depth, SAMPLE RATE lowers the effective sample rate, PITCH shifts pitch, DISTORTION adds clipping, LOW CUT and HIGH CUT filter the sound, MID TONE adjusts the midrange, and NOISE / TEXTURE adds noise. The mode selectors change the character of their respective effects. INPUT and OUTPUT set gain and DRY / WET blends the processed and original signals.

64-bit VST3 hosts such as FL Studio, REAPER, Ableton Live and Cubase are intended targets; individual hosts have not been verified. In Premiere, open Audio preferences and Audio Plug-in Manager; in Audition, choose Effects → Audio Plug-in Manager and scan for plug-ins. Direct VST3 hosting in After Effects has not been verified: process the audio in a DAW or Audition and import the rendered file. If the plug-in is missing, check the full VST3 folder, 64-bit host and rescan settings. A macOS build must be produced on macOS; no macOS binary is included.

Insert MAKE ME DIRTY on an individual sound track or on a shared SFX bus. Choose one of the eight factory presets, then use **BITCRUSHER**, **SAMPLE RATE**, **PITCH**, and **DRY / WET** as the fastest route to a useful result. The `...` menu renames or deletes user presets and restores the factory set. User presets are JSON files in the platform's per-user application-data folder; DAW project state is serialized independently by the host.

Signal flow:

`Input → Input gain → Pitch → Bit crusher → Rate reducer → Distortion → Tone filters → Noise → Dry/Wet → Output gain`

The final stage includes a safety limiter at 0 dBFS. The global bypass crossfades to the latency-aligned original signal.

## DSP notes

- **Bit crusher:** fractional 4–16-bit quantization with clean, TPDF-dithered, and asymmetric/error-colored characters.
- **Rate reducer:** independent effective-rate sample-and-hold with optional linear interpolation and intentional aliasing.
- **Pitch:** dual-Hann-grain variable-delay shifter, ±12 semitones, continuous stream duration, mono/stereo, and parameter smoothing.
- **Distortion:** soft (`tanh`), hard, or coarse digital clipping with separate drive trim.
- **Tone:** stable one-pole high/low cuts and a broad midrange emphasis/cut centered around the game-audio presence band.
- **Noise:** hiss, quantized digital noise, or sparse static from an allocation-free xorshift generator.
- **Real-time safety:** processing allocates buffers only in `prepareToPlay`; the audio callback contains no locks, file access, or UI calls.

## Factory presets

1. PS1 Horror
2. Half-Life 1998
3. CS 1.6
4. VHS Tape
5. Broken Digital
6. Old Computer
7. Dark Ambient
8. Subtle Retro

These are aesthetic references, not claims of reproducing proprietary game processing.

## Host compatibility

- FL Studio and other 64-bit VST3 hosts should scan the standard VST3 folder. Host validation is still recommended because every host has its own scanner and sandbox behavior.
- Adobe's current Premiere documentation explicitly lists VST, VST3, and AU (AU on macOS) third-party audio effects. Use **Preferences → Audio → Audio Plug-in Manager** to scan/enable MAKE ME DIRTY. Audio effects can be placed on clips or tracks; a shared bus/track insert matches the intended workflow.
- After Effects has built-in audio effects, but Adobe does not document After Effects as a general VST3/AU audio host. Do not assume this VST3 will load directly there. Process in Premiere, Audition/a DAW, or the Standalone application and render the result before using it in After Effects.

Vendor references (checked 2026-10-28):

- Adobe Premiere third-party plug-ins: https://helpx.adobe.com/premiere/desktop/use-premiere-with-other-apps/plug-ins.html
- Adobe Premiere audio preferences and plug-in manager: https://helpx.adobe.com/premiere/desktop/get-started/preferences-and-settings/audio-preferences.html
- Adobe After Effects audio effects: https://helpx.adobe.com/after-effects/using/audio-effects.html
- JUCE VST3/AU tutorial: https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/

## Tests

`RetrogradeTests` covers bit-depth alteration, sample-hold behavior, filter attenuation, pitch-change tolerance and unchanged frame count, zero-percent dry/wet identity, silence and boundary finiteness, preset state serialization, mono/stereo paths, and a basic processing-time guard. See the delivery notes for the exact tests that were actually run on the delivery machine.

## License and product naming

Project source is provided for the user's project. JUCE is fetched from its upstream repository and is not relicensed here. Review JUCE's GPL/commercial licensing before distributing binaries. Third-party product and game names belong to their owners and appear only as descriptive aesthetic references.

