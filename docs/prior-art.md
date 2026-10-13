# Prior art — C-12: Final Resistance (USA `SCUS_946.66`, SHA-256 `0b93d073ecc211a51431ee8e3eaf8e72f3aa33e6bc0b985a408c0f37a4cc9c87`, `title.json`)

There is no decompilation of this executable; the one tool found reads its level tables. A borrowed address is a lead until measured on our image.

| project | URL | kind | license | target build | build match (addresses checked) | what we may take | notes |
|---|---|---|---|---|---|---|---|
| C-12-Final-Resistance-Level-Geometry-Extractor | https://github.com/Dedok179/C-12-Final-Resistance-Level-Geometry-Extractor | level geometry extractor (C#, WinForms), reads the executable and the MWD archive | none (all rights reserved) | region not stated; load base `0x80010000`, header `0x800` | matched on our image (2026-10-13): load base `0x80010000`; level table `0x800D1F78` (92-byte entries, section count at `+0xF`, pointer at `+0x10`) holds counts 1/1/8/2 and pointers `0x800D0BB4..0x800D0F4C`, and Ghidra finds a data ref from `FUN_8003777c` (`0x80037870`); MWD file table at file offset `0xB0830` (`0x800D0030`, 36-byte entries) has sector/size fields that chain exactly (sector 1 + 75 = 76, 76 + 110 = 186, 186 + 1 = 187, 187 + 17 = 204) | evidence only (table layouts, PP20 decompression) | created and last pushed 2025-10-17 |

Not found (2026-10-13): a decompilation, symbol map, Ghidra/IDA database, PC port, recompilation, RAM map or modding SDK for `SCUS_946.66` on GitHub, GitLab, Codeberg, decomp.dev, Data Crystal or TCRF.
