# Measuring tools, drawing standards and estimate structures (RU / UZ / EU) for a Kotlin/Android drawing + estimating app

Research date: 2026-11-06. "App use:" notes are the researcher's own inferences for the product. Everything in "Cited Findings" has a source. Uncertain items are marked **[UNVERIFIED]** or **[CONFLICT]**.

---

## Q1. Bluetooth laser distance meters: can a third-party Android app read them, how, and what data comes over?

### Takeaway
No major brand publishes an open BLE GATT spec. Leica gives its DISTO API/SDK only through a gated partner program (GPN). Bosch has no public SDK, but community projects have reverse-engineered both its old Classic-SPP protocol and its newer BLE protocol. Hilti, Stabila, DeWalt/Stanley and Mileseey are reachable mainly through partner integrations such as magicplan. The practical route is: (1) apply for vendor partner programs (Leica GPN, Bosch, Stabila); (2) in the meantime, use the reverse-engineered Leica and Bosch BLE profiles; (3) always add a "keyboard/HID mode" fallback, which needs no protocol work.

### Cited Findings

**Leica DISTO (D1, D2, D110, D510, D810, X3, X4, X6)**
- Leica's DISTO apps page says developers "must sign up with the Geosystems Developer Network" (GPN) to get the DISTO API and support forum. No public download was found. — [Leica DISTO Apps](https://shop.leica-geosystems.com/learn/leica-disto/leica-disto-apps)
- Leica's GPN SDK program, documented for its Reality Capture SDKs: partners apply, go through an ~8-week evaluation, then sign a paid membership agreement. SDK packages need a signed "GPN SDK Evaluation" or "GPN SDK Agreement". **[UNVERIFIED that the DISTO API uses exactly these terms.]** — [Leica GPN program description](https://rcsdk.leica-geosystems.com/global-partner-network-application/latest/program-description)
- An Android SDK exists in the wild as the Java package `ch.leica.sdk`, with BLE/GATT connection classes and a command layer. A developer gist notes that "unavoidable gatt 133 / gatt 62 errors" require multiple connection attempts. — [AppWerft gist](https://gist.github.com/AppWerft/676361b720bf741f9b258738bb93813b)
- Community reverse engineering of the DISTO D2 (d2relay, MIT licence, Linux/python-gatt) found this GATT layout:
  - Leica vendor service `3ab10100-f831-4395-b29d-570977d5bf94`
  - `3ab10101-…` Measurement (read, indicate): 4-byte IEEE754 float, in the device's preset unit (metres in the author's case)
  - `3ab10102-…` presumably the unit (2 bytes, coding not decoded)
  - `3ab10109-…` write-without-response (purpose unknown, probably commands)
  - `3ab1010a-…` 4 bytes (read, indicate)
  - `3ab1010c-…` model string ("D2")
  - Standard Battery (0x180F) and Device Information (0x180A) services; the chip reports "nRF51822"
  - Endianness, unit coding and remote trigger are **not** confirmed. The project's to-do list includes "detect error states such as 255 (too close)" and "trigger measurements over BT". — [d2relay notes](https://github.com/seichter/d2relay/blob/main/doc/notes.md); [d2relay README](https://github.com/seichter/d2relay)
- DISTO D2: Bluetooth Smart v4.0, range up to 100 m. Leica's data sheet states ±1.5 mm (one retailer claims ±1.0 mm). **[CONFLICT, manufacturer figure preferred]** — [Leica DISTO D2 data sheet](https://www.ics-schneider.de/wp-content/uploads/2023/11/Leica-DISTO-D2-data-sheet_2201_V1.0_EN.pdf)
- DISTO X4: Bluetooth Smart v4.0, up to 10 m BT range, "incl. keyboard mode", ±1.0 mm accuracy, 0.1 mm resolution. Its tilt sensor is accurate to ±0.2° (housing and laser beam), with 360° angle measurement using the DST 360 adapter. — [Leica DISTO X4 InfoSheet](https://cdn.shopify.com/s/files/1/0832/3658/5768/files/DISTO_X4_InfoSheet.pdf); [Leica X4 data sheet DE](https://shop.leica-geosystems.com/sites/default/files/2022-04/Leica%20DISTO%20X4%20data-sheet_2201_V1.0_DE.pdf)
- Leica X3/X4 may appear to a computer as a Bluetooth keyboard. Floor Plan Creator tells users to "Forget this device" before connecting through Web Bluetooth. — [Floor Plan Creator BT help](https://floorplancreator.net/help/bluetooth)

**Bosch (GLM 50 C, GLM 50-27 C/CG, GLM 100(-25) C, GLM 120 C, PLR 30/40/50 C, AdvancedDistance/UniversalDistance 50C)**
- No public SDK or partner program was found. MeasureOn's terms mention integration with "products and services provided by third parties", but users "may not permit any third parties to access the Services". MeasureOn supports "all Bluetooth-enabled Bosch laser rangefinders", including the GLM 50 C, GLM 100 C and PLR 50 C. Contact for SDK questions: Support.MeasureOn@bosch.com. — [Bosch MeasureOn terms](https://www.boschtools.com/ca/en/trade-solutions/solutions/connected/apps/measureon/mobile/end-user-license-agreement); [MeasureOn App Store listing](https://apps.apple.com/gb/app/measureon/id1468079574)
- GLM 50-27 CG specs:
  - Data link: "Bluetooth 4.2 Low Energy"
  - Tilt range: 0–360° (4×90°)
  - IP65, 30 stored readings, green laser
  — [Bosch Professional product page](https://www.bosch-professional.com/ae/ar/products/glm-50-27-cg-0601072U00). Retailers list 0.05–50 m, ±1.5 mm typical, tilt accuracy ±0.2°. — [RS Online](https://se.rs-online.com/web/p/lasermatare/2174391)
- **Two protocol generations exist:**
  - **Old, Classic Bluetooth SPP** (GLM 100C, GLM 50C, possibly PLR 30C/40C/50C): a Python remote-control script based on an EEVblog protocol post. The repo is archived and did not work for at least one GLM 50C variant. — [philipptrenz/BOSCH-GLM-rangefinder](https://git.tdem.in/philipptrenz/BOSCH-GLM-rangefinder)
  - **New, BLE "MT protocol"**:
    - `piannucci/pymtprotocol` re-implements the discovery, connection, ACK, fragmentation and reassembly stack that Bosch apps use, for the GLM 100 C (macOS-only). — [pymtprotocol](https://github.com/piannucci/pymtprotocol)
    - `pklaus/bsch` reports a reverse-engineered GLM 50-27 CG BLE interface. — [bsch](https://awesome.ecosyste.ms/projects/github.com%2Fpklaus%2Fbsch)
    - `ketan/Bosch-GLM50C-Rangefinder` was built from Bluetooth captures of the MeasureOn Android app, with an ESP32 (NimBLE) build and a Python port. — [ketan repo](https://github.com/ketan/Bosch-GLM50C-Rangefinder)
- **Bosch BLE protocol details** from CaveSurvey issue #150. These are community-sniffed, not official:
  - Service and characteristic:
    - Vendor service `0000fde8-0000-1000-8000-00805f9b34fb` ("Robert Bosch GmbH")
    - Characteristic `02a6c0d2-0451-4000-b000-fb3210111989`; enable notifications through CCCD 0x2902
  - Commands (written to the vendor characteristic):
    - `C0 55 02 01 00 1A`: enables measurement data in indications
    - `C0 56 01 00 1E`: triggers a measurement; it must be sent twice (arm, then measure)
    - `C0 55 02 F1 01 EE`: distance mode
    - `C0 55 02 F1 08 AC`: angle mode
  - Notification packet (~20 bytes): prefix `C0 55`, byte `10`, 2-byte type, 2-byte counter, three float32 little-endian values (metres), checksum
  - Type codes:

    | Code | Meaning |
    |---|---|
    | `0600` | Distance |
    | `2100` | Angle |
    | `3201` / `3600` | Two-point distance |
    | `0e01` / `1200` | Area (m²) |
    | `0a00` | Min/max |
    | `1601` / `1a01` / `1e00` | Volume (m³) |
    | `2e00` | Horizontal distance + angle |
    | `2a00` | Vertical distance + angle |

  - Device quirks:
    - The AD 50C does **not** support remote trigger and disconnects after a few seconds idle.
    - The GLM 50-27 C advertises **no name**, so filter by service UUID instead.
  — [CaveSurvey issue #150](https://github.com/lz1asl/CaveSurvey/issues/150)
- CaveSurvey (Android, MIT) reads Bluetooth meters over "SPP or LE profile depending on the measurement device". Distance is always read; compass/clino only "depending on the device". Its maintainer uses an official Bosch library, and the repo has `Bosch.md` and `no_bosch_library.patch`. **[UNVERIFIED how that library was obtained.]** — [CaveSurvey repo](https://github.com/lz1asl/CaveSurvey)

**Hilti (PD-C, PD-CS, PD-I)**
- PD-I: Hilti advertises "Bluetooth connectivity to partner apps like magicplan, imagemeter, Floor Plan Creator and WinWorker". Hilti's AU page adds that it does not warrant compatibility with these apps. — [Hilti CA PD-I](https://www.hilti.ca/r9121031); [Hilti AU PD-I](https://www.hilti.com.au/c/CLS_MEA_TOOL_INSERT_7127/CLS_LASER_METERS_7127/r9121031)
- PD-CS is an Android-based device with Wi-Fi, Bluetooth and micro-USB export. No developer documentation was found. — [newequipment.com](https://www.newequipment.com/product-directory/tools/product/55117399/27961-pd-cs-is-a-wi-fi-enabled-laser-measurer)

**Stabila, DeWalt/Stanley, Mileseey and others**
- magicplan's "PrecisionLink" (launched 2025-04) works with 20+ BT laser meters. Brand lists from magicplan integration pages:

  | Brand | Models listed |
  |---|---|
  | Leica | D1, D2, D110, D510, D810, X3, X4, X6, E7500i |
  | Stabila | LD 250 BT, LD 520, LD 530 BT |
  | DeWalt | DWHT78200-XJ, DW03201 ("only two tested") |
  | Stanley | TLM 99s, Fatmax TLM 165s |
  | Mileseey | S50R, S50C |
  | Bosch | GLM / PLR / Blaze series (help page names 400 CL Blaze, AdvancedDistance 50C, UniversalDistance 50C/40C, GLM 100 C) |

  The partner list also includes Hilti and Johnson Level. — [magicplan PrecisionLink blog](https://magicplan.app/blog/magicplan-launches-precisionlink); [magicplan laser meters help](https://help.magicplan.app/laser-distance-meters); [Leica](https://magicplan.app/integrations/leica-disto-laser-measures), [Stabila](https://magicplan.app/integrations/stabila-laser-measures), [DeWalt](https://magicplan.app/integrations/dewalt-laser-measures), [Stanley](https://magicplan.app/integrations/stanley-laser-measures), [Mileseey](https://magicplan.app/integrations/mileseey-laser-measure), [Bosch](https://magicplan.app/integrations/bosch-laser-measures)
- Floor Plan Creator, an Android app, supports **both Bluetooth Classic and BLE** meters. Listed models: Bosch GLM 120c; Hersch LEM 50; Leica X3, X4; Mileseey dTAPE 5t, M120, P7, T7; Suaoki P7. Its web version uses (experimental) Web Bluetooth for BLE only. — [Floor Plan Creator BT help](https://floorplancreator.net/help/bluetooth)
- No public Mileseey SDK or GATT documentation was found. Mileseey consumer models pair with Mileseey's own app, "Smart Life" (S7) or JOYPLAN (D9C). — [search summary of retailer listings; magicplan Mileseey page](https://magicplan.app/integrations/mileseey-laser-measure)

### Inferences
- **App use, architecture:** Build a `LaserMeterDriver` interface with three kinds of implementation:
  - `HidKeyboardDriver`: the meter types into a focused field. Works today with Leica X3/X4 keyboard mode and many Chinese meters, with zero protocol work.
  - Per-vendor BLE GATT drivers (Leica `3ab10100…` service; Bosch `fde8` service with the `02a6c0d2…` characteristic).
  - A Classic SPP driver for older Bosch GLM 50C/100C and some generic meters.
  - Scan by service UUID, not by name (the Bosch GLM 50-27 C advertises no name).
- **App use, Android BLE robustness:** Expect GATT 133/62 errors (seen with the Leica SDK on Android). Build automatic reconnect with backoff, treat idle disconnects as normal (Bosch AD 50C drops after a few seconds), and request MTU / handle fragmentation for Bosch MT packets.
- **App use, data model:** Store every reading as `{value_m: Double, kind: DISTANCE|ANGLE|AREA|VOLUME|HEIGHT_INDIRECT, angle_deg?: Double, source_device, raw_bytes}`. Bosch already sends computed area/volume/indirect height (type codes above), and Leica X-series and GLM 50-27 CG have ±0.2° tilt sensors. Always normalise to metres internally and display in mm per SPDS/ISO.
- **App use, legal/commercial:** The reverse-engineered protocols may break with firmware updates, and the vendor terms (Bosch MeasureOn) restrict third-party access to *their services*, though not explicitly to the device radio. For a commercial product, apply to Leica GPN and contact Bosch/Stabila partner channels. magicplan, Floor Plan Creator and imagemeter show that vendors do grant integrations.
- **Market fit (RU/UZ):** The Russian/Uzbek market has many ADA, Condtrol, RGK, Xiaomi (Duka/Atuman) and Mileseey OEM meters. None of the sources found document their BLE protocols, so HID-keyboard mode plus a "generic SPP text parser" are the realistic fallbacks.

### Gaps
- No official, public GATT documentation from Leica, Bosch, Hilti, Stabila, DeWalt or Mileseey was found. All UUIDs and commands above are community reverse engineering, and the Leica unit coding and remote trigger remain undecoded.
- Not found: protocols for ADA Instruments, Condtrol (Smart series), RGK, Xiaomi Duka/Atuman, DeWalt DW03201, or Stabila LD 520's BLE profile. Also unconfirmed: whether the Bosch GLM 100-25 C / GLM 120 C use the same `fde8` profile as the GLM 50-27 C (likely, since magicplan and Floor Plan Creator list the GLM 120 C).
- Not researched for lack of time/tool budget: Bluetooth digital levels/inclinometers (e.g., Bosch/Stabila/Leica digital levels) and Bluetooth moisture meters.

---

## Q2. Phone LiDAR / ARCore measurement vs. laser meters

### Takeaway
Phone-based measuring is centimetre-class at best (iPhone LiDAR ~1–2 cm). Android ARCore without LiDAR can be off by decimetres, while BT laser meters give ±1–1.5 mm. Use AR for quick room-shape capture and lasers for the dimensions that feed estimates.

### Cited Findings
- iPhone 13 Pro LiDAR vs. a Z+F terrestrial scanner: accuracy "reliable in the range of 1–2 cm", with edges rounded. Smartphone scanning "will not replace precise solutions based on terrestrial laser scanning or classical surveying". — [FIG 2022, Zaczek-Peplinska & Kowalska](https://fig.net/resources/proceedings/fig_proceedings/fig2022/papers/ts03g/TS03G_zaczek-peplinska_kowalska_11458_abs.pdf)
- ARCore depth vs. LiDAR in one indoor scene (OnePlus 6): mean absolute error ~23–30 cm, improving with higher confidence thresholds. — [arXiv 2204.01693](https://arxiv.org/pdf/2204.01693)
- An assistive-tech paper cites an average ARCore position error of 0.16 m (Morar et al.). — [arXiv 2408.13505 (AngleSizer)](https://arxiv.org/pdf/2408.13505)
- Google says the Raw Depth API is more accurate than the full Depth API but doesn't cover every pixel, comes with a confidence image, and is recommended for measurement. — [Google ARCore Raw Depth docs](https://developers.google.cn/ar/develop/unity-arf/depth/raw-depth?hl=en)
- A 2025 study evaluated ARCore distance in low light on a POCO X5 Pro against a laser rangefinder reference. Numbers were not retrieved. — [rt.nure.ua](https://rt.nure.ua/article/view/350221)
- Laser references: Leica X4 ±1.0 mm, Leica D2 ±1.5 mm, Bosch GLM 50-27 CG ±1.5 mm. — see Q1 sources.

### Inferences
- **App use:** Offer an AR "sketch room" mode using ARCore (plane detection + Raw Depth). Mark AR-derived dimensions as *estimated* (±several cm). Prompt the user to "confirm with laser" for the dimensions that drive quantities (wall lengths, heights, openings). Store `measurement_source` and `confidence` per dimension so estimates can flag low-confidence quantities.
- Most Android phones in the UZ/RU market lack LiDAR/ToF, so the ARCore numbers (decimetre-level errors) are the realistic baseline.

### Gaps
- No peer-reviewed multi-device study of ARCore Depth API error against ground truth was found, and no direct head-to-head of a phone app vs. a BT laser meter for room dimensions.

---

## Q3. Russian SPDS drawing conventions (GOST R 21.101-2020, GOST 21.501-2018, GOST 21.201-2011, GOST 2.306-68, 2.302/2.303/2.304)

### Takeaway
For RU/UZ plans, follow SPDS:
- Dimensions in **mm without a unit**, closed by **45° ticks (засечки) 2–4 mm** rather than arrows.
- Coordination axes in **6–12 mm circles**: digits left→right, Cyrillic letters bottom→top, skipping Ё, З, Й, О, Х, Ц, Ч, Щ, Ъ, Ы, Ь.
- Level marks in **metres with 3 decimals**; the zero mark has no sign; others carry "+"/"−".
- Plan scales 1:50, 1:100 or 1:200.
- Material hatching per GOST 2.306 at 45°, spacing 1–10 mm.
- Room area in the room's bottom-right corner, underlined.

GOST 21.501-2018 is an interstate standard that Uzbekistan (Uzstandard) voted to adopt.

### Cited Findings

**GOST R 21.101-2020** (Russian national standard, effective 2021-01-01, Rosstandart order 23.06.2020 No. 282-ст, replaces GOST R 21.1101-2013) — [GOST R 21.101-2020 full text PDF](https://www.modul.org/wp-content/uploads/2023/10/gost-r-21.101-2020.pdf)
- 5.1.3: lines per GOST 2.303. 5.1.5: fonts per GOST 2.304, or other computer fonts if accessible to users. 5.1.6: scales per GOST 2.302. Scales are generally **not written** on drawings except where an SPDS standard requires it, and then in brackets after the image title.
- 5.2.x: drawings use the main title block **form 3** on the first sheet and **form 6** on subsequent sheets. Layout and frame sizes are in Appendix И.
- **5.3.2 Axes:** thin dash-dot lines with long dashes. Marked in circles **6–12 mm** with Arabic digits and capital Russian letters, excluding **Ё, З, Й, О, Х, Ц, Ч, Щ, Ъ, Ы, Ь**, or Latin letters excluding I and O. No gaps are allowed in the sequence. Digits go on the side with more axes. If letters run out, use doubled letters: АА, ББ, ВВ.
- 5.3.3–5.3.4: digit axes run left→right and letter axes bottom→top. Markings normally go on the left and bottom sides, with top/right added where axes diverge.
- 5.3.5: intermediate axes are written as fractions, e.g. 1/1 between axes 1 and 2.
- 5.3.8: the 3D model origin is at the intersection of axes 1 and А at the zero level. X points toward increasing digits, Y toward increasing letters, Z up.
- **5.4.1 Units:** linear dimensions are given **without units**. Site plans and external networks use metres with 2 decimals; **all other drawings use millimetres**.
- **5.4.2 Terminators:** dimension lines are closed by **ticks 2–4 mm long, slanted right at 45°**. Dimension lines extend **0–3 mm** beyond the outer extension lines. **Arrows** are used for diameters/radii inside circles and for angular dimensions.
- **5.4.3 Level marks:** given in **metres, without unit, with 3 decimals, comma separator**. On facades and sections, the mark sits on extension lines with a 45° tick sign (2–4 mm). On plans it goes **in a rectangle**. "The zero mark… is given **without sign**; marks above zero with '+', below with '−'." The zero is usually the finished floor of the 1st storey (Appendix note to 5.4.3).
- 5.4.4: slopes are shown with an arrow on plans, in % or as a ratio such as 1:7. On sections a "∠" sign is used.
- 5.4.5: leaders end with a dot, or with an arrow when drawn from a line that represents a surface.
- 5.4.7: font for axes, marks and view titles is **1.5–2×** the dimension-number font.
- 5.5.2: sections (разрезы) are numbered with Arabic digits (or Russian capital letters), and the cutting line is an open line with arrows.

**GOST 21.501-2018** (interstate; in force in RU from 2019-06-01, replacing 21.501-2011) — [meganorm text](https://meganorm.ru/Data2/1/4293732/4293732743.htm); [PDF copy](https://tulsu.ru/sdoii/file.php/1/docs/GOST/GOST%2021.501-2018.pdf)
- Voting table (MGS protocol 30.08.2018 No. 111-П): Armenia, Belarus, Kyrgyzstan, Russia, **Uzbekistan (UZ, Uzstandard)**.
- 4.3: conventional images of building elements are taken from **GOST 21.201**. 4.4: material designations in sections come from **GOST 2.306**.
- 4.9, Table 1 scales:
  - Floor plans, sections, facades: 1:50, 1:100, 1:200 (also 1:400, 1:500)
  - Fragments: 1:50, 1:100
  - Details (узлы): 1:10, 1:20
- 5.3.1: the plan cutting plane is at window-opening level or 1/3 of floor height.
- 5.3.2 shows on plans: axes, distances between them and overall distance; wall thicknesses; openings with dimensions and ties to axes; level marks where floor levels differ.
- **Rooms:** the name and area go in the room, with the **area in the bottom-right corner, underlined**. Alternatively, room numbers in a **12–15 mm circle/oval** plus an **explication table (Form 2)** with the column "Площадь, м2". Numbering is floor-based, with "Ц" for basement and "П" for underground. For residential buildings the explication "как правило, не выполняют".
- Door/gate filling types are marked in **5–7 mm circles**. Window filling types are marked ОК1, ОК2…
- Sections show level marks for ground, finished floors, landings and the tops/bottoms of elements.

**GOST 21.201-2011** "Условные графические изображения элементов зданий, сооружений и конструкций" — interstate, enacted in RU by Rosstandart order 11.10.2012 No. 481-ст. Sections include Стены, Проемы и отверстия, Пандусы/лестницы/отмостки, Двери и ворота, Оконные переплеты (4.1–4.12). One source's voting list (MNTKS protocol 08.12.2011 No. 39) shows AZ, AM, KZ, KG, MD, UA and **does not show Uzbekistan**. **[UNVERIFIED for UZ]** — [dnaop.com text](https://dnaop.com/html/67539/doc-%D0%93%D0%9E%D0%A1%D0%A2_21.201-2011); [meganorm](https://meganorm.ru/mega_doc/norm/gost_gosudarstvennyj-standart/3/gost_21_201-2011_mezhgosudarstvennyy_standart_sistema.html)

**GOST 2.306-68** (material designations; in force since 1971-01-01) — [GOST 2.306-68 text PDF](https://www.helpeng.ru/public/normdoc/eskd/gost_2.306-68.pdf)
- Table 1 lists material-specific section patterns. Note 3 says pattern **No. 5** is used for **brick products** (fired/unfired), refractories, building ceramics and slag-concrete blocks. The table itself is an image.
- Rules:
  - Hatch lines at **45°** to the contour, its axis or the frame. Use **30° or 60°** if 45° coincides with contour or axis lines.
  - The same direction is used for all sections of one part.
  - Spacing **1–10 mm** depending on area.
  - **Adjacent sections use opposite directions**, or change spacing/offset.
  - Sections narrower than 2 mm may be filled black with ≥0.8 mm gaps.
  - For large areas and soil profiles, a hatched strip along the contour is allowed.
  - In building drawings, small sections may be shown as metal or with no hatch plus a note.
  - Pattern No. 3 and fill (засыпка) are drawn freehand.
- A draft **GOST R 2.306** (a "development" of 2.306-68) was in public review until 2024-01-10. — [normacs](https://www.normacs.info/discussions/9055)

**GOST 2.303** lines and **GOST 2.304** fonts: referenced normatively by GOST R 21.101-2020, 5.1.3 and 5.1.5. — [GOST R 21.101-2020](https://www.modul.org/wp-content/uploads/2023/10/gost-r-21.101-2020.pdf)

### Inferences
- **App use, SPDS profile defaults:**
  - Units: mm, with no unit label.
  - Dimension terminator: 45° tick, 2–4 mm, slanted right. Dimension lines overrun 0–3 mm.
  - Axes: 8–10 mm circles. Auto-label with the Cyrillic sequence А Б В Г Д Е Ж И К Л М Н П Р С Т У Ф Ш Э Ю Я (excluded letters skipped, then АА, ББ…). Digits run along the longer side.
  - Level marks: "0,000" for zero (the standard says no sign, though "±0,000" is widespread practice — make it configurable), then "+3,000" / "−0,450". Show in a rectangle on plans and with the 45° flag on sections.
  - Room tag: number in a circle plus area underlined bottom-right, 2 decimals (decimal count is practice, not stated in 21.501).
  - Window/door marks: ОК-n / Д-n.
  - Hatch library: brick, concrete, reinforced concrete, soil, insulation, wood, metal, glass mapped to GOST 2.306 Table 1 patterns.
- **App use, export:** DXF with layers per element type, line types per GOST 2.303 (solid thick, solid thin, dash-dot thin long dash for axes, dashed), and a GOST 2.304 type A/B font (ship an ISOCPEUR-like or GOST-type font). Title block form 3 is 185×55 mm; "185" appears in the Appendix И drawings. **[UNVERIFIED 55 mm height from text extraction]**
- Uzbekistan formally adopted GOST 21.501-2018. GOST R 21.101-2020 is a *Russian national* (ГОСТ Р) standard, so it is not automatically binding in Uzbekistan. In practice, Uzbek designers use the same SPDS conventions (inference, not verified).

### Gaps
- The exact figures of GOST 2.306 Table 1 (concrete = dots/triangles, natural soil, reinforced concrete, insulation) could not be text-extracted; the table is an image. Verify visually from the PDF before implementing hatches.
- The GOST 21.201-2011 symbol tables (window/door swing, stairs) were not extracted.
- Not verified: whether Uzbekistan applies GOST 21.201-2011 and the interstate GOST 21.101-97, or its own KMK/ShNK on drawing rules.

---

## Q4. European / ISO conventions and how they differ from SPDS

### Takeaway
- **ISO 129-1:2018** allows arrowheads, oblique strokes or dots, one style per drawing.
- **DIN 1356** lists 45° oblique stroke, dot, circle and arrow; architects usually choose oblique strokes.
- Room identification follows **ISO 4157-1/-2/-3 (1998, identical EN ISO)**.
- Main differences from SPDS:
  - Units: German practice often mixes m/cm and must state the unit in the title block, while SPDS uses mm without a unit.
  - Height marks and axis lettering differ (Latin letters).

### Cited Findings
- ISO 129-1:2018 sets general principles for presenting dimensions and tolerances on 2D drawings in all disciplines. Terminations may be arrowheads, oblique strokes or dots, with **only one style per drawing**. Oblique strokes are "often favoured by Architects". — [cadsetterout.com](https://cadsetterout.com/drawing-standards/dimension-styles/); [learnarchitecture.net](https://learnarchitecture.net/articles/40544-dimensioning-architectural-drawings.html)
- ISO 2595:1973 (building drawings, old): terminations are "short oblique lines, drawn at 45° clockwise from the projection line". — [ISO 2595 sample](https://cdn.standards.iteh.ai/samples/7575/a0148d58fab84730a88a04ed28f7958c/ISO-2595-1973.pdf)
- ISO 7519:1991 covers general principles for general arrangement and assembly drawings for building/architectural work. — [ISO 7519 sample](https://cdn.standards.iteh.ai/samples/14288/4686cac1237f435c860a8389c9b871bf/ISO-7519-1991.pdf)
- ISO 128-23:1999 covers lines on construction drawings. — [ISO 128-23 sample](https://cdn.standards.iteh.ai/samples/22292/a9f6396b7eb94397aa5e41513ebf09b1/ISO-128-23-1999.pdf)
- **ISO 4157 (1998), three parts:**
  - Part 1: designation system for buildings, spaces and elements.
  - Part 2: room names and numbers.
  - Part 3: room identifiers for the whole life cycle; "room identifiers shall be assigned to each and every room".
  - EN ISO 4157-1/-3 are identical. BS EN ISO 4157 superseded BS 1192-1:1984.
  — [standards.ie ISO 4157-1](https://shop.standards.ie/en-ie/standards/iso-4157-1-1998-583472_saig_iso_iso_1335968); [ISO 4157-3](https://shop.standards.ie/en-ie/standards/iso-4157-3-1998-583480_saig_iso_iso_1335990); [SIST EN ISO 4157-1](https://iteh.es/catalog/standards/sist/a6fa0c37-21dd-4e7a-83ce-aadf40583f74/sist-en-iso-4157-1-2002)
- **DIN 1356 (Bauzeichnungen):** terminators are a lying cross, dot, circle, arrow (mainly for arcs) or 45° oblique stroke. The dimension line overruns the extension line by 2–4 mm depending on scale. The unit must be clearly identifiable and is noted in the title block next to the scale. Height marks (Höhenangabe) are covered in DIN 1356-6 (2006-05, survey drawings). — [Uni Hamburg "Bauzeichnen I" lecture](https://epub.sub.uni-hamburg.de/epub/volltexte/2010/4886/pdf/Bauzeichnen_I.pdf)
- DIN 1356-1 status: editions 1995-02, then a 2024-04 draft (listed as withdrawn) and a new draft (E) 2026-09 with comment deadline 30.09.2026. **[CONFLICT / unclear which edition is currently valid]** — [DIN Media draft 2026](https://www.dinmedia.de/en/draft-standard/din-1356-1/403956420); [DIN 1356-1:1995](https://dinmedia.de/en/standard/din-1356-1/2463916); [bauprofessor.de](https://www.bauprofessor.de/bemassung-bauzeichnungen/)

### Inferences
- **App use:** Implement a `DrawingStandardProfile` enum with three values:

  | Profile | Units | Terminator | Axes | Levels | Rooms |
  |---|---|---|---|---|---|
  | `SPDS_RU_UZ` | mm, no unit | 45° tick | Cyrillic letters | 3-decimal m | — |
  | `ISO_EU` | mm (unit in title block) | oblique stroke default; arrow/dot allowed, one per drawing | Latin letters minus I/O | — | ISO 4157 room numbers like "1.02" |
  | `DIN_DE` | m/cm per DIN practice | — | — | ± OKFF heights | — |

  The geometry model stays identical across profiles; only rendering changes.

### Gaps
- Full texts of ISO 128 (incl. the 2020+ restructured parts), ISO 129-1, ISO 4157 and DIN 1356-1 are paywalled. The exact DIN conventions (e.g., superscript cm "86⁵", OKFF/OKRF height notation) were not verified from primary sources.

---

## Q5. Uzbekistan practice (drawings + estimates)

### Takeaway
Uzbekistan uses interstate SPDS standards: it adopted GOST 21.501-2018. For estimates it has its own **ShNK 4.02.xx "elemental resource estimate norms"** (being renumbered **СРН 4.02.xx**), structured by the same collection numbers as Russian GESN (01 earthworks, 06 cast-in-place concrete, 08 brick/block, 11 floors, 12 roofs, 15 finishing). State-funded projects use the **resource method with current prices**.

### Cited Findings
- Uzbekistan voted to adopt GOST 21.501-2018 (Uzstandard). — [GOST 21.501-2018 text](https://meganorm.ru/Data2/1/4293732/4293732743.htm)
- The Ministry of Construction norms page (gov.uz, last updated **2025-03-04**) lists ShNK estimate norms with new **СРН** numbers:
  - 4.02.00-04 (СРН 4.02.00) Regulation on developing/applying elemental resource estimate norms
  - 4.02.01-04 Earthworks
  - 4.02.06-04 Cast-in-place concrete & RC
  - 4.02.07-05 Precast concrete
  - 4.02.08-04 Brick and block structures
  - 4.02.11-04 Floors
  - 4.02.12-04 Roofs
  - 4.02.15-04 Finishing works
  - 4.02.16-07 Internal plumbing
  - 4.02.22/23-05 External water/sewer
  - 4.02.33-04 Power networks
  - 4.02.47-04 Landscaping
  - Repair series 4.02.51–4.02.70 (e.g., 4.02.53-07 walls, 4.02.61–64-07 plastering/painting/finishing)
  - "Supplement 1"
  — [gov.uz Minstroy: Shaharsozlik normalari va qoidalari](https://gov.uz/en/mc/pages/shaharsozlik-normalari-va-qoidalari)
- Example: ShNK 4.02.04-04 (collection 4, wells) was issued by Goskomarkhitektstroy in 2005, replacing KMK 4.02.04-96. "Supplement 10" to the ShNK resource norms was approved by order No. 57 of 2 May 2016. — [mc.uz PDF](https://mc.uz/uploads/mcuz_8768980753472.pdf) (search-snippet level)
- 2021: Uzbekistan's Ministry of Construction worked with Kazakhstan's KazNIISA on a concept to modernise construction pricing. Press: "projects with state investment currently use the resource method". — [Forbes.kz](https://forbes.kz/news/newsid_255759)
- 2020 critique (buxgalter.uz interview): estimate pricing relies on collections whose base is ~60 years old, with ~40,000 unit rates. Example: norm E16-4-2-1 gives >24 person-days for 100 m of Ø20 mm plastic pipe vs. 0.5–2 shifts in practice. Many cost items are set as centralised percentages. Local tenders average <1% discount vs. 20–30% on international tenders. — [buxgalter.uz](https://buxgalter.uz/publish/doc/text158087_cenoobrazovanie_v_stroitelstve_-_posledniy_perejitok_planovoy_ekonomiki_uzbekistana)

### Inferences
- **App use:** Model a "norm base" abstraction that can load **ShNK/СРН 4.02 (UZ)** or **GESN/FSNB-2022 (RU)** with identical resource-line semantics: labour person-hours, machine hours, materials with qty per unit. Collection numbers line up (08 = masonry, 15 = finishing), so one mapping table serves both countries.
- Currency: UZS for Uzbekistan, RUB for Russia. Uzbek private builders price per unit of work in UZS, often referencing USD for materials (inference; not verified).

### Gaps
- No 2024–2026 Uzbek decree on the current estimate methodology (resource method, price monitoring, indices) was found on lex.uz or mc.uz in this pass. Edition years of the СРН collections are also not stated on the gov.uz page.
- No current Uzbek private-market rates (e.g., masonry "per m³" in UZS, Tashkent 2026) were found.

---

## Q6. Russian estimate structure: Methodology 421/pr, resource-index method, GESN/FSNB-2022, ENiR, private-builder practice

### Takeaway
A Russian local estimate (ЛСР) under **Methodology 421/pr (04.08.2020)** is built **resource-by-resource**:
- GESN/FSNB-2022 norms give labour person-hours, machine-hours and material quantities per unit (e.g., per 1 m³ masonry).
- These are priced at current prices from FGIS CS, or base prices × indices.
- Overheads (НР, Methodology 812/pr, amended 2026 by 209/pr) and estimate profit (СП) are added.

FSNB-2022 (order 1046/pr) has been in force since 25.02.2023 and is updated by amendment packages; package 18 has been in force since 25.05.2026.

### Cited Findings
- **Methodology 421/pr:** Minstroy order 04.08.2020 No. 421/pr "Методика определения сметной стоимости строительства, реконструкции, капитального ремонта, сноса объектов…", with amendments incl. 557/pr and 55/pr of 30.01.2024. ConsultantPlus lists a редакция dated 30.01.2026. **[verify]** — [ConsultantPlus 421/pr](https://www.consultant.ru/document/cons_doc_LAW_370561/020b18f2aefee4b1bbc7b2e8b4246bfb9d5cb74b/); [smeta.ru export doc](https://help11.smeta.ru/export_gge/export_gge.pdf)
- Estimate software following 421/pr structures the estimate tree as **project → stage (очередь) → start-up complex → object → local estimate**. It has an option "apply indices per 421/pr" with rounding rules, and cites clause 75 (auxiliary non-normed resources). — [Smeta.ru 11.7 release notes](https://cloud.smeta.ru/upload/11.7.0.0/upgrade.pdf)
- **Overheads:** "Методика по разработке и применению нормативов накладных расходов" (order 21.12.2020 No. 812/pr), amended by order **06.04.2026 No. 209/pr**. — [Kontur: order 209/pr](https://www.kontur-extern.ru/info/normativ/document/1/508716-prikaz-minstroya-rf-ot-06-04-2026-n-209-pr)
- **FSNB-2022:**
  - Approved by Minstroy order **30.12.2021 No. 1046/pr**; in force **25.02.2023** (after 5 amendments and several postponements).
  - More than 53,000 estimate norms and 42,000 construction resources in prices as of 01.01.2022.
  - Its entry into force means regions move to the resource-index method (RIM).
  — [smetchik.com](https://mail.smetchik.com/info/news/poetapnyy-perekhod-regionov-na-rim-startoval)
- RIM transition target dates slipped: from Q2 2022 to **Q1 2024**. Package 9 (508 new norms) by order 16.02.2024 No. 102/pr. Current FSNB is published in XML on FGIS CS. — [normacs news 74086](https://www.normacs.info/news/74086); [normacs 68880](https://www.normacs.info/news/68880)
- **Package 18** of FSNB-2022 changes:
  - Minstroy order **15.05.2026 No. 301/pr**, in force **25.05.2026**.
  - 232 new and 2,649 updated norms.
  - 967 new, 277 updated and 362 removed materials (FSBTs 81-01 2022).
  - 15 new and 11 updated machines.
  — [normacs news 79009](https://www.normacs.info/news/79009)
- **What a GESN item contains.** Example: **GESN 81-02-08-2020, Collection 8 "Конструкции из кирпича и блоков"** (Appendix 8 to order 26.12.2019 No. 871/pr). Table **08-02-001 "Кладка стен из кирпича"**, unit (измеритель) **1 m³**:
  - Work composition: masonry; niches for heating/ventilation/smoke channels; joints; architectural details.
  - **08-02-001-01** (external simple walls, storey height ≤4 m):

    | Resource line | Code | Unit | Qty per 1 m³ |
    |---|---|---|---|
    | 1 Workers' labour | — | person-h | **4.54** (avg grade 2.7) |
    | 2 Machine operators' labour | — | person-h | 0.4 |
    | 3 Tower crane 8 t | 91.05.01-017 | machine-h | 0.4 |
    | 4 Water | 01.7.03.01-0001 | m³ | 0.44 |
    | Softwood bars | 11.1.03.01-0080 | m³ | 0.0005 |
    | Cement-lime mortar | 04.3.01.12 | m³ | **0.24** |
    | Ceramic or silicate brick | 06.1.01.05 | 1000 pcs | **0.38** |

  - Variants:

    | Norm | Description | Labour, person-h/m³ |
    |---|---|---|
    | 08-02-001-03 | Medium complexity | 4.76 |
    | 08-02-001-05 | Complex | 5.22 |
    | 08-02-001-07 | Internal walls | 4.38 (0.38 thousand bricks, 0.234 m³ mortar) |
    | 08-02-001-10 | Frame infill | 5.12 |

  — [GESN 81-02-08-2020 text (smeta.ru FTP)](https://ftp.smeta.ru/pub/norma/FER-text/GCN-08.pdf)
- GESN Collection 8, technical part:
  - Wall complexity is classed by the share of "complicated parts" (cornices, pilasters, bay windows, curved opening surrounds, niches…) in the facade area: **simple ≤10%**, **medium ≤20%**, **complex ≤40%**, **особо сложные >40%**.
  - Norms assume brick **250×120×65 mm** and ceramic/silicate stones **250×120×138 mm**. For 250×120×88 brick, material use is corrected per Appendix 8.1; other sizes follow the project.
  — [GESN 81-02-08-2020 text](https://ftp.smeta.ru/pub/norma/FER-text/GCN-08.pdf)
- FER (federal unit rates) of the 2001 base were developed in base prices **as of 01.01.2000**. — [Minstroy FER page (search snippet)](https://minstroyrf.gov.ru/trades/dwd-state-fer.php?ID=7)
- **ENiR E3 "Каменные работы"** (Gosstroy USSR, 1987) gives time norms and wage rates (Н.вр., Расц.) per m³ of masonry by wall thickness and complexity. Example rule: for bricks lighter than 3 t per 1000 pcs, multiply time norms and rates by 0.9. No source found on its current official status, and no cancellation order was found. — [dnaop.com ENiR E3](https://dnaop.com/html/54431/doc-kamennyje-raboty)

### Inferences
- **App use, estimate data model** (mirrors GESN/ShNK and works for RU/UZ):
  - `Estimate → Section (раздел: e.g., "Стены", "Перекрытия", "Отделка") → Line (норма/работа) → ResourceLines[]`
  - Each `ResourceLine` = `{type: LABOUR|MACHINE_LABOUR|MACHINE|MATERIAL, code, name, unit, qty_per_unit, price_per_unit, price_source}`
  - Line totals = Σ(qty_per_unit × work_qty × price). Then add `overhead%` and `profit%`, which are configurable; the official RU values come from 812/pr and the profit methodology.
- **App use, quantity takeoff → norm:** The drawing provides wall length × height × thickness minus openings = m³ masonry. GESN 08-02-001-01 then gives 380 bricks/m³, 0.24 m³ mortar and 4.54 person-hours. Expose wall "complexity" (simple/medium/complex by % of decorated facade) because the norm choice depends on it.
- **App use, private-builder mode:** Private crews in RU/UZ price **"работа за куб"** (per m³ masonry), per m² plaster/screed/tile, or per 1000 bricks. Provide a simple "rate card" mode: user-entered price per unit (m³, m², m, pcs) next to material quantities derived from norms. Normative person-hours can still be shown as a productivity sanity check.
- ENiR is historic (1987). Use it only as an optional reference for crew-wage splits, not as a pricing basis.

### Gaps
- No verified current (2026) market rates were found for masonry "per m³" or plaster "per m²" in Moscow/RU regions or Tashkent; search results were classifieds without units.
- GESN-2022 (FSNB-2022) values for 08-02-001-01 were not extracted; the figures above are the **GESN-2020** edition and may differ slightly. Collections 06 (concrete), 11 (floors) and 15 (finishing) were not extracted.
- The full text of 421/pr (LSR form appendix, RIM formulas, rules on how material quantities include "трудноустранимые потери") and the estimate-profit methodology number/date were not retrieved. They are believed to be 774/pr of 11.12.2020 **[UNVERIFIED]**.

---

## Q7. Germany (DIN 276, VOB/C, STLB-Bau, GAEB) and UK (NRM) estimate / bill-of-quantities structures

### Takeaway
German cost planning uses **DIN 276:2018-12** cost groups. Execution-oriented bills of quantities (Leistungsverzeichnis, LV) are organised by trade **Leistungsbereiche (STLB-Bau LB 012 masonry, 013 concrete, 023 plaster)** governed by **VOB/C ATV** (DIN 18330 Mauerarbeiten, DIN 18331 Betonarbeiten). They are exchanged as **GAEB DA XML** (X83 tender with LV, X84 priced bid). UK uses **NRM1/2/3** with elemental, work-section or work-package BoQ structures.

### Cited Findings
- **DIN 276:2018-12** replaced DIN 276-1:2008-12, DIN 276-4:2009-08 and DIN 277-3:2005-04. HOAI still references the 2008-12 version.
  - Cost-determination stages: Kostenrahmen, Kostenschätzung, Kostenberechnung, Kostenvoranschlag, Kostenanschlag, Kostenfeststellung.
  - Level-1 cost groups: **100** Grundstück, **200** Vorbereitende Maßnahmen, **300** Bauwerk – Baukonstruktionen, **400** Bauwerk – Technische Anlagen, **500** Außenanlagen und Freiflächen, **600** Ausstattung und Kunstwerke, **700** Baunebenkosten, **800** Finanzierung.
  - **KG 300:** 310 Baugrube/Erdbau; 320 Gründung, Unterbau; 330 Außenwände/Vertikale Baukonstruktionen, außen; 340 Innenwände/Vertikale Baukonstruktionen, innen; 350 Decken/Horizontale Baukonstruktionen; 360 Dächer; 370 Infrastrukturanlagen; 380 Baukonstruktive Einbauten; 390 Sonstige Maßnahmen.
  - **KG 400:** 410 Abwasser-, Wasser-, Gasanlagen; 420 Wärmeversorgung; 430 Raumlufttechnik; 440 Elektrische Anlagen; 450 Kommunikations-/Sicherheits-/IT; 460 Förderanlagen; 470 Nutzungsspezifische Anlagen; 480 Gebäude- und Anlagenautomation; 490 Sonstige.
  - An execution/trade-oriented breakdown is allowed (e.g., for modernisation). At Kostenanschlag/Kostenfeststellung, an additional breakdown by Vergabeeinheiten is required. Austria's equivalent is ÖNORM B 1801-1.
  — [Wikipedia DE: DIN 276](https://de.wikipedia.org/wiki/DIN_276)
- **STLB-Bau Leistungsbereiche** (version 2017-04, introduced by BMUB decree B I 7 of 26.06.2017): **012 Mauerarbeiten**, **013 Betonarbeiten**, **023 Putz- und Stuckarbeiten, Wärmedämmsysteme**. The same appear in the 2016-10 list. STLB-BauZ (time contracts) uses different numbers (630 Mauerarbeiten, 631 Betonarbeiten, 650 Putz). — [AIK-SH decree PDF](https://www.aik-sh.de/wp-content/uploads/2023/08/2017_06_26_BI7_81063_4_3_3_STLB-Bau_4.pdf); [DIN STLB-BauZ 2026-07](https://www.din.de/resource/blob/1282302/bd4097b18749354409f988ac1976679a/stlb-bauz-buchausgaben-2026-07-data.pdf)
- ATV DIN 18330 (Mauerarbeiten) and ATV DIN 18331 (Betonarbeiten) are VOB/C parts, tied to STLB-Bau (concrete = LB 013). Editions include DIN 18330:2006-10 and draft 2019-09. — [lehmanns/DIN listings (search summary)](https://www.din.de/de/wdc-beuth:din21:91001810/toc-9756503/download)
- **GAEB DA XML** exchange phases:
  - **X83** = Angebotsaufforderung (tender with LV)
  - **X84** = Angebotsabgabe (bid with OZ = Ordnungszahl, Einheitspreis, price components, Gesamtbetrag)
  - Latest official version cited is 3.2 (2018); parsers support 3.3 (phases X80–X87, X89 invoices)
  - **[CONFLICT]** on X81/X82/X85/X86 meanings between secondary sources
  — [streit-software GAEB formats](https://www.streit-software.de/wissen/gaeb-formate); [pyGAEB docs](https://pygaeb.readthedocs.io/en/latest/guides/parsing/); [bambamboole/gaeb](https://packagist.org/packages/bambamboole/gaeb)
- **UK NRM:** NRM1 covers cost estimating/cost planning for capital works, NRM2 detailed measurement for building works (rules for BoQ and schedules of rates), NRM3 maintenance works (2014). NRM2 defines three BoQ breakdowns:
  - **Elemental:** Preliminaries, Facilitating works, Substructure, Superstructure, Internal finishes, Fittings, Services, External works, Risks…
  - **Work section:** e.g., masonry, carpentry, glazing, decoration…
  - **Work package**
  NRM uses its own indexing, not CAWS. — [Designing Buildings: NRM](https://www.designingbuildings.co.uk/wiki/New_Rules_of_Measurement); [NRM2](https://designingbuildings.co.uk/wiki/NRM2)

### Inferences
- **App use:** Give each estimate line **two optional classification tags**:
  - Cost-element tag: DIN 276 KG, e.g., 330 for external walls, or NRM1 element.
  - Trade tag: STLB-Bau LB / VOB/C ATV, e.g., LB 012 Mauerarbeiten, or the RU "раздел ЛСР" / GESN collection.
  This lets the same takeoff be shown "by element" (owner budgeting) or "by trade" (crew contracts). For a DE/EU export, write GAEB DA XML X83/X84 with hierarchical OZ (e.g., 01.02.0010).
- Map: GESN 08 / ShNK 4.02.08 ↔ STLB-Bau LB 012 / DIN 18330 ↔ NRM2 "Masonry". GESN 06 ↔ LB 013 / DIN 18331. GESN 15 (plaster part) ↔ LB 023.

### Gaps
- The official GAEB phase table, VOB/C 2023/2024 edition status and the current STLB-Bau 2026 LB list were not confirmed from primary sources.
- BKI cost-index data (Germany) was not researched.

---

## Q8. IFC / BIM classification for export (IFC4 entities, quantity sets, Uniclass/OmniClass)

### Takeaway
Export walls, slabs, stairs, doors and windows as IFC4 `IfcWall`, `IfcSlab`, `IfcStair`, `IfcDoor`, `IfcWindow`, and attach standard base-quantity sets (`Qto_WallBaseQuantities`, `Qto_SlabBaseQuantities`) so estimating tools can read quantities. Add classification references (Uniclass 2015 / OmniClass) through `IfcClassificationReference`.

### Cited Findings
- **IFC4 `IfcWall`:**
  - The applicable quantity set is `Qto_WallBaseQuantities`.
  - `PredefinedType` is optional and should be used only when no `IfcWallType` is assigned.
  - If `USERDEFINED`, `ObjectType` must be set.
  — [buildingSMART IFC4 IfcWall](https://standards.buildingsmart.org/IFC/RELEASE/IFC4/FINAL/HTML/schema/ifcsharedbldgelements/lexical/ifcwall.htm)
- **`Qto_WallBaseQuantities`** (IFC4.3 ADD2 official):
  - Properties: Length, Width (only for constant-thickness walls), Height, Gross/Net FootprintArea, Gross/Net SideArea, Gross/Net Volume.
  - Gross volume ignores openings/recesses.
  `Qto_SlabBaseQuantities` includes an extruded area for prismatic slabs. — [IFC4.3 Qto_WallBaseQuantities](https://standards.buildingsmart.org/IFC/RELEASE/IFC4_3/HTML/lexical/Qto_WallBaseQuantities.htm); [Qto_SlabBaseQuantities](https://standards.buildingsmart.org/IFC/RELEASE/IFC4_3/HTML/lexical/Qto_SlabBaseQuantities.htm)
- GOST R 21.101-2020 5.3.8 requires the 3D model origin at axes 1/А at the zero level: X along digit axes, Y along letter axes, Z up. This aligns with an IFC local placement convention. — [GOST R 21.101-2020](https://www.modul.org/wp-content/uploads/2023/10/gost-r-21.101-2020.pdf)
- **Uniclass 2015:**
  - Codes have 4–5 pairs of characters (table/group/sub-group/section/object), with tables such as Co, EF, Ss, Pr.
  - NBS aligned Uniclass with IFC.
  - Uniclass Pr ≈ OmniClass Table 23 (Products); EF/Ss ≈ OmniClass Table 21 (Elements). All are aligned to ISO 12006-2, but alignment is imperfect.
  — [Wikipedia: Uniclass](https://en.wikipedia.org/wiki/Uniclass); [2018 classification comparison](https://www.4bt.us/wp-content/uploads/2022/01/2018_Classification-system-comparison.pdf); [NBS OmniClass critique](https://www.thenbs.com/knowledge/omniclass-a-critique); [Uniclass CSVs](https://github.com/buildig/uniclass-2015)

### Inferences
- **App use:**
  - Produce IFC4 (ADD2 TC1) with IfcProject → IfcSite → IfcBuilding → IfcBuildingStorey (one per floor, Elevation = SPDS level mark).
  - Walls as IfcWall with IfcMaterialLayerSet (brick/insulation/plaster).
  - Openings via IfcOpeningElement + IfcRelFillsElement to IfcDoor/IfcWindow.
  - Rooms as IfcSpace with ISO 4157 / SPDS room numbers.
  - Attach Qto_* base quantities and a custom Pset with GESN/ShNK norm codes, DIN 276 KG and Uniclass Ss/Pr codes.
  - Android-side IFC writing libraries were not researched. IfcOpenShell is the reference toolkit — [IfcOpenShell IfcWall](https://ifcopenshell.github.io/docs/rst_files/class_ifc4_1_1_ifc_wall.html) — but it is C++/Python. Kotlin would need a server-side export or a custom STEP writer.

### Gaps
- IfcStair/IfcDoor/IfcWindow pages and their quantity sets were not fetched; they exist in the same IFC4 IfcSharedBldgElements schema, but details are unverified here.
- No Russian/Uzbek national classifier (e.g., Russian КСИ, the construction information classifier mandated for BIM) was researched. It is relevant for RU BIM exports and is a recommended follow-up.
