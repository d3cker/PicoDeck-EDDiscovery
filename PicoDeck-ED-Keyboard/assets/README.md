# Elite Dangerous icon assets

`icons.c` is generated from the **Sidewinder Orange** theme in
[`streamdeck-elite-icons`](https://github.com/Ordo-Corona-Stellarum/streamdeck-elite-icons)
at commit `d2f58e41d86374dfc67efe15098d06f021d4cb79`, which is the submodule
revision used by the original EDDPiDeck `devel` branch.

The images are Copyright (C) 2020 Keath Milligan and licensed under the
[Creative Commons Attribution-ShareAlike 3.0 Unported License](https://creativecommons.org/licenses/by-sa/3.0/).
The generated four-level alpha masks in `icons.c` are derivative image assets
and remain available under the same license. They are compiled into the
Keyboard UF2; distributing that binary therefore also requires preserving the
icon attribution, modification notice, and CC BY-SA 3.0 license link or text.

The repository-level MIT License applies to original PicoDeck code and does not
relicense these masks. See [UPSTREAM_LICENSE.md](UPSTREAM_LICENSE.md) and the
repository's [third-party notices](../../THIRD_PARTY_NOTICES.md).

The generator does not download the source artwork. Regenerate the masks from
an extracted copy of the icon repository:

```powershell
.\tools\generate-icons.ps1 -SourceRoot C:\path\to\streamdeck-elite-icons
```
