# Architecture, Stair, Fireplace/Chimney, Pool and Roof Rules a Drawing + Estimating App Can Check (RU / DE-EU / UZ)

Research date: 2026-11-06. Primary texts I downloaded and read: SP 7.13130.2013 (consolidated to 12.03.2020, MChS copy), SP 55.13330.2016 (with Amendment 1), SP 54.13330.2022, SP 1.13130.2020 §4.4 (consolidated to 16.06.2025), SP 17.13330.2017 (with Amendments 1–4), SP 310.1325800.2017 (2018 official edition), the German Muster-FeuVO (2007, amended 2016/2017), § 9 BayFeuV, § 19 1. BImSchV, the Uzbek ShNQ 2.08.01-24 (lex.uz) and SanPiN RUz 0146-04 (lex.uz). DIN 18065 comes from the WEKA excerpt, not the full DIN text. The "App use:" lines are my design suggestions, not quotes from the standards.

Notation: h = riser height (подступенок, Steigung s); b = tread depth (проступь, Auftritt a); slope 1:n means rise/run. Converted to degrees: 1:1 = 45°, 1:1.25 = 38.7°, 1:1.5 = 33.7°, 1:1.75 = 29.7°.

---

## 1. Stairs: riser/tread limits, flight length, landings, headroom, railings, comfort formula (RU / DIN 18065 / UZ)

### Takeaway
For single-family houses (Ф1.4), Russian codes do **not** set numeric riser or tread limits. SP 55.13330.2016 §7.5 states that width and slope of internal stairs in two-storey houses are not regulated. It requires only uniform step heights (§8.2) and railings ≥ 0.9 m (§8.3). The app's hard numbers should therefore come from SP 54.13330.2022 Table 6.1 (in-unit stairs ≥ 0.9 m wide, slope ≤ 1:1.25), SP 1.13130.2020 §4.4 (where a stair is treated as an evacuation stair: riser 5–22 cm, tread ≥ 25 cm, 3–16 risers, headroom 2.2 m), DIN 18065:2020-08 (residential buildings with ≤ 2 dwellings: s 14–20 cm, a 23–37 cm, 2s + a = 59–65 cm, headroom 2.0 m), and Uzbek ShNQ 2.08.01-24 Table 8, which matches SP 54. Blondel's 2h + b = 600–640 mm is an ergonomic recommendation and appears in no Russian code.

### Cited Findings
**Russia: single-family houses, SP 55.13330.2016 "Дома жилые одноквартирные" (Amendment 1 of 10.07.2018 No. 415/пр)**
- §7.5: in two-storey houses, internal open stairs of the 2nd type may serve as evacuation routes, as may spiral stairs and stairs with winder steps. "Предел огнестойкости и класс пожарной опасности элементов лестницы, а также ее ширина и уклон не регламентируются." — [SP 55.13330.2016 text (sro-a.ru PDF)](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: in "private house, ≤2 storeys" mode, present riser, tread and width checks as **recommendations** (yellow), not code violations (red). Example text: "SP 55 does not set a limit for a private house, but h = 215 mm is above the comfort range."
- §7.6: in three-storey houses, an open internal stair counts as an evacuation route only if the exit requires going up or down no more than one level. If the top floor must descend two levels, all of these must hold: (a) every room usable for sleeping has at least one window with its sill **≤ 1 m above the floor**; (b) those rooms open directly into a corridor or hall with a balcony exit; (c) those windows and the balcony are **≤ 7 m above ground**. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: when floors = 3 and the stair is open, check each bedroom's window sill height (≤ 1000 mm) and window height above grade (≤ 7 m). Warning: "3rd-floor bedroom: sill 1.2 m > 1.0 m, so the open stair does not count as an evacuation route (SP 55 §7.6)."
- §8.2: the slope and width of flights, riser heights, tread widths, landing widths and headroom along stairs "должны обеспечивать удобство и безопасность передвижения…". "Применение лестниц с разной высотой ступеней не допускается." — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: a hard check that every riser in a flight is equal. Compute h = floor-to-floor height ÷ number of risers and flag any non-uniform step. DIN 18065 tolerance is ±5 mm (see below).
- §8.3: railings on stairs, balconies, loggias, terraces, roofs and other dangerous drops must be **≥ 0.9 m** high, continuous, fitted with handrails, and designed for a load of **≥ 0.3 kN/m**. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: input railing height. Error if < 900 mm. Also pass the 0.3 kN/m design load to the railing-post estimator.

**Russia: multi-apartment building code SP 54.13330.2022 (in force since 14.06.2022, replaces SP 54.13330.2016), used as the reference for in-unit stairs**
- Table 6.1: flights to basements and plinth floors, technical crawl spaces, and **in-unit (внутриквартирные) stairs: minimum width 0.9 m, maximum slope 1:1.25**. Sectional 2-storey buildings: 1.05 m and 1:1.5. Sectional 3+ storeys: 1.05 m and 1:1.75. Corridor/gallery type: 1.2 m and 1:1.75. Width is measured between railings, or between wall and railing. — [SP 54.13330.2022 PDF](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
  - App use: default private-house check is clear width ≥ 900 mm and h/b ≤ 0.8 (≤ 38.7°). Warning: "Flight slope 41° is steeper than 1:1.25 (38.7°), the limit for in-unit stairs in SP 54 Table 6.1."
- §6.4.3: **3 to 18 risers** per flight. Steps of different height or depth are not allowed. In multi-level apartments, spiral or winder stairs are allowed if the tread at its middle is **≥ 0.18 m** and the railing is **≥ 0.9 m**. — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
  - App use: for winder or spiral stairs, measure the tread on the walking line (centre). Error if < 180 mm.
- §6.4.4: railings on external flights and landings, balconies, French balconies, loggias, terraces and floor-level drops **≥ 0.45 m** must be **≥ 1.2 m**. Balconies and terraces < 0.45 m above grade need **≥ 0.9 m**. §6.4.5: internal flights and landings need railings with handrails **≥ 0.9 m**, rising to **1.2 m** when the horizontal gap between flights or railings is **> 0.12 m**. §6.4.9: if glazing starts **< 0.9 m** above the finished floor, safety measures per GOST R 56926 are required. — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
  - App use: for terraces or balconies, ask "height above grade". If ≥ 450 mm, require a 1.2 m railing (SP 54) and say that SP 55 for private houses only asks for 0.9 m. Show a warning when the stairwell gap (between flights) is > 120 mm and the railing is < 1.2 m.

**Russia: fire evacuation code SP 1.13130.2020 (MChS order 19.03.2020 No. 194, consolidated to 16.06.2025)**
- §4.4.1: minimum evacuation stair width is **0.9 m** for "all other cases" (a list of building classes gets 1.05–1.6 m; Ф1.4 single-family houses are excluded from the 1.2 m case). "Высота пути эвакуации должна быть не менее 2,2 м." — [SP 1.13130.2020 §4.4 (sudact.ru)](https://sudact.ru/law/prikaz-mchs-rossii-ot-19032020-n-194/sp-1.13130.2020-svod-pravil/4/4.4/)
- §4.4.2: landings must be at least as wide as the flight. §4.4.3: slope of evacuation stairs "как правило, не более 1:1", tread "как правило, не менее 25 см" (except external stairs), riser **≤ 22 cm and ≥ 5 cm**. Curved stairs may narrow to 22 cm at the narrow end. Stairs to attics, technical floors and non-accessible roofs may narrow to 12 cm. — [SP 1.13130.2020 §4.4](https://sudact.ru/law/prikaz-mchs-rossii-ot-19032020-n-194/sp-1.13130.2020-svod-pravil/4/4.4/)
- §4.4.4: **3–16 risers** per flight between landings (curved stairs excepted). Single-flight stairs, and one flight of a 2- or 3-flight stair on the ground floor, may have up to **18**. — [SP 1.13130.2020 §4.4](https://sudact.ru/law/prikaz-mchs-rossii-ot-19032020-n-194/sp-1.13130.2020-svod-pravil/4/4.4/)
- §4.4.7: external stairs of the 3rd type need railings **≥ 1.2 m**. — [SP 1.13130.2020 §4.4](https://sudact.ru/law/prikaz-mchs-rossii-ot-19032020-n-194/sp-1.13130.2020-svod-pravil/4/4.4/)
  - App use: an "evacuation stair (SP 1.13130)" profile, used for 3-storey houses or when the user selects it, with h 50–220 mm, b ≥ 250 mm, slope ≤ 45°, 3–16 risers (≤ 18 on the ground floor), headroom ≥ 2200 mm and landing depth ≥ flight width. Warning: "17 risers in one flight; SP 1.13130 §4.4.4 allows at most 16, or 18 on the ground floor."

**Comfort formula (Blondel)**
- A Russian article on stair calculation gives **2h + b = 60–64 cm**, plus h + b = 45–47 cm and b − h = 12 cm. It presents these as recommendations, not code clauses. Its recommended ranges: riser 140–160 mm with tread 300–340 mm (30–35°, "optimal for houses"), and 180–200 mm with 220–260 mm only for basements and attics. It also says SP 55.13330 requires tread ≥ 250 mm and riser ≤ 200 mm for private houses. **That claim is not in the SP 55.13330.2016 text I read** (see §7.5/§8.2 above), so treat it as unreliable. — [gostcheck.ru](https://gostcheck.ru/articles/raschet-lestnitsy-formula-blondelya)
  - App use: show 2h + b live. Green at 600–640 mm, yellow at 590–650 mm (the DIN band), red outside.

**Germany: DIN 18065:2020-08 "Gebäudetreppen" (WEKA excerpt)**
- Limits for **residential buildings with ≤ 2 dwellings and inside dwellings**, legally required ("baurechtlich notwendige") stairs: **Steigung s 140–200 mm, Auftritt a 230–370 mm, minimum usable width 80 cm**. General buildings: s 140–190 mm, a 260–370 mm, 100 cm. Non-required stairs: s 140–210 mm, a 210–370 mm, 50 cm. **Schrittmaßregel 2s + a = 590–650 mm.** Limits apply to the finished stair; tolerances may not be counted towards them. — [WEKA: DIN 18065](https://www.weka.de/architekten-ingenieure/din-18065/)
- Headroom (lichte Treppendurchgangshöhe) **≥ 200 cm**. In ≤ 2-dwelling buildings it may be reduced by up to 25 cm at one or both edge strips, provided a central strip ≥ 50 cm wide keeps full height. — [WEKA](https://www.weka.de/architekten-ingenieure/din-18065/)
- Railing height **≥ 90 cm** for drops ≤ 12 m, 110 cm for drops > 12 m. Railing openings ≤ 12 cm in general buildings, **no requirement** in ≤ 2-dwelling buildings. Handrail height 80–115 cm. Landing depth ≥ usable flight width. An intermediate landing is required after 18 risers in general buildings, not in ≤ 2-dwelling buildings. Winder steps: minimum tread at the inner edge 100 mm in general buildings, 50 mm in ≤ 2-dwelling buildings. Nosing overlap (Unterschneidung) ≥ 30 mm for open stairs. Tolerance on s and a is ±5 mm. Adjacent steps may differ by at most 5 mm. — [WEKA](https://www.weka.de/architekten-ingenieure/din-18065/)
  - App use: an "EU/DE" rule pack: 140 ≤ h ≤ 200, 230 ≤ b ≤ 370, 590 ≤ 2h + b ≤ 650, headroom ≥ 2000 mm, width ≥ 800 mm, handrail 800–1150 mm.
- A forum post cites a DIN 18065:2022-09 edition. This conflicts with WEKA's 2020-08 edition. — [bau.de forum](https://bau.de/forum/treppen/11797.php)

**Uzbekistan: ShNQ 2.08.01-24 "Turar joy obyektlarini loyihalash" (Ministry of Construction order No. 01/2-57 of 14.08.2024, Ministry of Justice reg. No. 293 of 25.09.2024)**
- Cl. 178: steps must be uniform, with **3 to 18** steps per flight. Cl. 179 / Table 8: flight width ≥ 1.05 m for sectional 2-storey and 3+ storey buildings, 1.2 m for corridor type. Maximum slope 1:1.5 (2 storeys) or 1:1.75 (3+ storeys and corridor type). **Basement and in-unit stairs: ≥ 0.9 m, slope ≤ 1:1.25.** Cl. 179: handrails where the level difference exceeds 45 cm. Cl. 180: guards **≥ 1.2 m** at stairs, ramps, balconies, terraces and roofs with level changes. — [lex.uz ShNQ 2.08.01-24](https://lex.uz/en/acts/-7121730) (read through a summarising fetch; verify Cl. 180's scope before treating 1.2 m as mandatory for internal stairs in houses).
  - App use: for an Uzbek project, apply the same 0.9 m / 1:1.25 / 3–18 checks as SP 54. Show a note that ShNQ 2.08.01-24 Cl. 180 may require 1.2 m guards.

### Inferences
- A practical default for private houses in RU, UZ and DE: riser 150–180 mm, tread 260–300 mm, 2h + b 600–640 mm, width ≥ 900 mm, headroom ≥ 2.0 m (DIN) or 2.2 m (SP 1.13130 evacuation), railing ≥ 900 mm, all risers equal. A stair meeting this passes every rule set above.
- The app should treat "different riser heights" as the only hard numeric-free RU error for private stairs (SP 55 §8.2). The rest are recommendations unless the user selects an "evacuation stair" (SP 1.13130) or a multi-apartment building (SP 54 / ShNQ).
- Slope formula for the app: angle = atan(h/b). 1:1.25 means h/b ≤ 0.8.

### Gaps
- **Baluster gap (Russia):** neither SP 54.13330.2022 nor SP 55.13330.2016 contains a 0.10 m baluster-spacing rule. SP 54 uses the 0.12 m gap between flights only to decide railing height. GOST 25772 (railings) was not checked. DIN 18065 allows 12 cm in general buildings and sets no requirement in ≤ 2-dwelling houses.
- **GOST 9818 / GOST 8717** (precast concrete flights, landings and steps): editions (assumed GOST 9818-2015, GOST 8717-2016) and step sizes were not verified.
- **Landing length in Russian codes:** SP 1.13130 §4.4.2 says only "≥ flight width". No separate private-house rule was found.
- **Uzbek private-house stair rules:** ShNQ 2.08.01-24 targets multi-unit and dormitory buildings (single-family houses appear only in the Ф1.4 classification, Cl. 148). No Uzbek norm specific to individual houses (yakka tartibdagi uy) was found.

---

## 2. Rooms: ceiling heights, minimum areas and widths, natural light, windows, ventilation (RU / UZ / DE)

### Takeaway
Russian single-family houses (SP 55.13330.2016 §6.2) need clear height **≥ 2.5 m** in living rooms and kitchens, or **≥ 2.7 m** in cold subregions IА, IБ, IГ, IД and IIА. Mansard or sloped rooms need ≥ 2.3 m, and corridors ≥ 2.1 m. The window-to-floor ratio must be **≥ 1:8** (≥ 1:10 in mansards). Kitchens must exhaust ≥ 60 m³/h and bathrooms or WCs ≥ 25 m³/h. Uzbekistan's ShNQ 2.08.01-24 sets **2.7 m** for living rooms and kitchens and **2.4 m** for auxiliary rooms. Its older SanPiN 0146-04 gives a light ratio of 1:4.5–1:8 and requires the kitchen exhaust of 60 m³/h. Minimum room areas for private (non-state) Russian houses are now "per design assignment" under SP 55 Amendment 2, so the app should present area checks as recommendations.

### Cited Findings
**Russia: SP 55.13330.2016 (single-family houses)**
- §6.2: "Высота помещений жилых комнат и кухни в климатических подрайонах IА, IБ, IГ, IД и IIА по СП 131.13330 должна быть не менее 2,7 м, а в остальных — не менее 2,5 м". Rooms in a mansard or with sloped ceilings may be **≥ 2.3 m**. Corridors and mezzanines (антресоли) may be **≥ 2.1 m**. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
- Amendment-2 wording of §6.2: in rooms and kitchens on sloped-roof or mansard floors, height below the norm is allowed on **≤ 50% of the room area**. §3.21: a mansard is a floor whose sloped ceiling meets the external wall at **≤ 1.5 m** above the floor. §3.20: a storey is counted when clear height is **≥ 1.8 m**. — [cadastre.ru SP 55 (Amendments 1 and 2)](https://cadastre.ru/source/6)
  - App use: input the room's ceiling polyline or slope. Compute the share of floor area below 2.5 m (or 2.7 m); error if > 50%. Flag any room under 2.3 m (2.1 m for corridors). Region picker sets 2.5 or 2.7 m. Warning: "Living room clear height 2.40 m < 2.50 m (SP 55 §6.2)."
- §6.1 (Amendment 1, 10.07.2018 No. 415/пр): minimum areas are **16 m²** for the living room (**14 m²** if the house has one room), **8 m²** for a bedroom (10 m² for two people, 7 m² in a mansard), 9 m² for a wheelchair user's bedroom, **9 m²** for a kitchen, and **6 m²** for a kitchen niche or kitchen zone. Widths follow SP 137.13330. For reconstructed houses: living room 12 m², bedroom 8 m² (7 in a mansard), kitchen 6 m². Minimum widths there: kitchen 1.7 m, hall 1.4 m, corridors 0.85 m, bathroom 1.5 m, WC 0.8 m. WC depth 1.2 m with the door opening out, 1.5 m with it opening in. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
- Amendment 2, §6.1: "Для домов частного жилищного фонда по 4.2 минимальные площади помещений следует принимать по заданию на проектирование с учетом эргономических показателей и расстановки оборудования". — [cadastre.ru SP 55 (Amendments 1 and 2)](https://cadastre.ru/source/6)
  - App use: room-area checks against 16/14/8/9/6 m² as **recommendations** ("below the SP 55 §6.1 benchmark; allowed for private houses per design assignment"). Width checks: kitchen ≥ 1.7 m, corridor ≥ 0.85 m, WC ≥ 0.8 m, bath ≥ 1.5 m.
- §9.x (light): "Отношение площади световых проемов к площади пола жилых помещений и кухонь должно быть не менее 1:8. Для мансардных этажей … не менее 1:10." — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: for each room, sum window opening areas (frame opening W × H) and divide by floor area. Error if < 0.125 (< 0.10 in a mansard). Warning: "Bedroom windows 1.6 m² / 16 m² = 1:10 < 1:8 (SP 55)."
- §9.7 (Amendment 1): minimum ventilation is **1 air change per hour** in rooms with permanent occupancy. Kitchen exhaust **≥ 60 m³/h**. Bathroom, WC, shower or combined WC **≥ 25 m³/h**. Non-working mode: ≥ 0.2 room volume per hour. §9.6: exhaust from kitchen, WC, bathroom and shower. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: every kitchen and wet room needs an exhaust symbol (duct or fan). Size duct or fan to 60 and 25 m³/h. Error if a kitchen or WC has no exhaust.
- §7.13: gas or liquid-fuel heat generators must be factory-made automated units, placed in a ventilated room on the first or plinth floor, in a basement, or on the roof. **Units up to 35 kW may go in the kitchen.** The gas pipe must enter directly into the kitchen or boiler room. An LPG cylinder **≤ 50 L** is allowed inside a house of **≤ 2 storeys**. §7.15: gas fireplaces must be factory-made and vent into a chimney. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)
  - App use: if a boiler > 35 kW is placed in the kitchen, raise an error that it needs a separate boiler room.
- §4.5: a house may include a sauna (per SP 2.13130 / SP 2.1.3678), a garage, and a **pool designed per SP 310.1325800**. §7.16: sauna electric heaters need automatic shut-off after 8 h. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf); [cadastre.ru](https://cadastre.ru/source/6)

**Russia: SP 54.13330.2022 (multi-apartment), the reference SP 55 points to**
- §5.12: living rooms and kitchens need **≥ 2.7 m** in subregions IА, IБ, IГ, IД **and IVА**, and ≥ 2.5 m elsewhere. In-unit corridors, halls, entrance halls and mezzanines need ≥ 2.1 m. Sloped or mansard rooms may be lower on ≤ 50% of their area. — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
- §5.11: minimum areas are 14 m² for the living room of a 1-room flat, 16 m² with 2+ rooms, 8 m² per bedroom (10 m² for two), **8 m²** for the kitchen, 6 m² for a kitchen zone, and 5 m² for a kitchen or kitchen niche in a 1-room flat. A mansard bedroom or kitchen may be 7 m² if the living room is ≥ 16 m². — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
- §7.13: window-to-floor ratio for living rooms and kitchens is **not more than 1:5.5 and not less than 1:8**, and not less than 1:10 for sloped-roof windows. It may instead be calculated by daylight factor (KEO) per SP 52.13330 and SP 367.1325800. — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
  - App use: add an upper bound, with a warning (not an error) if windows exceed 1:5.5, which signals overheating or heat loss. This matters in hot Uzbekistan.

**Uzbekistan: ShNQ 2.08.01-24 (current, 2024)**
- Cl. 38: living rooms and kitchens **≥ 2.7 m** floor to ceiling. Auxiliary rooms **≥ 2.4 m**. Cl. 62: basements below 2.2 m may not be used. Cl. 35: room in a 1-room flat ≥ 14 m², living room ≥ 16 m² with 2+ rooms, bedroom ≥ 8 m² (12 m² for two), kitchen ≥ 6 m² (8 m² with 2+ rooms), hallway ≥ 4 m², bathroom ≥ 4 m². Cl. 36: bedroom width ≥ 2.4 m, living room width ≥ 3.0 m. Cl. 39: kitchen width ≥ 1.7 m with fittings on one wall, ≥ 2.2 m with fittings on two walls. Cl. 13: lighting per ShNQ 2.01.05-24. Table 11 lists a kitchen air exchange of 60 m³/h. — [lex.uz ShNQ 2.08.01-24](https://lex.uz/en/acts/-7121730) (read through a summarising fetch; only clauses up to about 262 were seen).
  - App use: Uzbek preset with height ≥ 2.7 m (aux ≥ 2.4 m), bedroom width ≥ 2.4 m, living room width ≥ 3.0 m, kitchen width ≥ 1.7 m or 2.2 m.
- Predecessor ShNK 2.08.01-19: Amendment 1 (25.09.2020) and Amendment 2 (order No. 22 of 18.02.2022). An amendment let living rooms go in semi-basement (цокольный) floors of new houses if clear height is **≥ 2.4 m**, groundwater is ≥ 3 m deep, at least ⅓ of the floor is above ground, and insolation is ≥ 2 h. — [lex.uz amendment](https://lex.uz/docs/5888568); [anhor.uz](https://anhor.uz/ru/news/basement)
- SanPiN RUz 0146-04 "Проектирование жилых домов в климатических условиях Узбекистана" (approved 2004; current status not shown on the page). Light coefficient (window : floor) for living rooms and kitchens **1:4.5 to 1:8**. KEO ≥ 0.5% for living rooms and kitchens. Kitchen width ≥ 2.5 m. Kitchen exhaust ≥ 60 m³/h. Bathrooms and WCs 3 air changes per hour. Living-room windows should not face **200–290° (west)**, with exceptions. Air conditioning is required where the outdoor design temperature is ≥ 33 °C ("all cities of the republic"). Room depth with one-sided light ≤ 6 m and ≤ 2× width. Stove heating only in buildings of **≤ 2 storeys**. One stove serves ≤ 3 rooms on one floor. — [lex.uz SanPiN 0146-04](https://lex.uz/acts/1876151)
  - App use: Uzbek orientation check. If a living room's only window faces 200–290°, warn "western orientation is restricted by SanPiN 0146-04 §8.2". Light ratio band 1:4.5–1:8.

### Inferences
- SP 55 (2.7 m in "IIА") and SP 54.13330.2022 (2.7 m in "IVА") list different subregions. SP 54-2022 adds the hot-dry subregion IVА, which is relevant to Central Asia. Using 2.7 m for southern RU and UZ projects agrees with both SP 54-2022 and ShNQ 2.08.01-24.
- Window-ratio checks need the frame or glazed opening area, not the glass area. The app should store window W × H as rough opening.

### Gaps
- **Door widths:** no numeric door-width rule for single-family houses was found in SP 55 or SP 54. Accessibility values (SP 59.13330) were not researched.
- **Window sill height:** the only numeric RU values found are SP 55 §7.6a (bedroom sill ≤ 1 m, in 3-storey houses only) and SP 54 §6.4.9 (glazing < 0.9 m needs safety measures).
- **SP 55 Amendment 2:** the approval order number and effective date were not found. A draft was in public discussion until 25.08.2022 ([normacs](https://www.normacs.info/discussions/8402)). cadastre.ru shows the text with Amendments 1 and 2.
- **Gas kitchen rules** (volume ≥ 15 m³, window with transom, etc.) live in SP 62.13330 / SP 402.1325800. These were not researched.
- **German room rules** (MBO § 47: clear height of habitable rooms ≥ 2.40 m, window ≥ 1/8 of floor area) were not verified in this session.

---

## 3. Fireplaces, stoves and chimneys: SP 7.13130.2013, refractory brick, sizing rules, DE/EU (MFeuVO, BImSchV)

### Takeaway
SP 7.13130.2013 (Amendments 1 and 2 of 2020) is still the current Russian fire code for stoves and chimneys in 2026. MChS issued a clarification letter on it on 28.10.2024, and Amendment 3 was only a draft. Key checkable numbers:
- chimney **≥ 5 m** from grate to top;
- **≥ 500 mm** above a flat roof;
- **≥ 500 mm** above the ridge when within 1.5 m of it;
- **not below the ridge** at 1.5–3 m;
- **not below a 10° line** from the ridge beyond 3 m;
- brick chimney walls **≥ 120 mm**;
- cuts (разделки) **500 mm** to combustible structures and **380 mm** to protected ones;
- clearance **≥ 130 mm** to rafters;
- **700 × 500 mm** metal sheet in front of the firebox;
- **1250 mm** from the firebox door to the opposite wall;
- **8 cm² per kW** for factory-made fireplace flues.

Germany (MFeuVO § 9) requires the outlet to clear the ridge by **40 cm** or the roof surface by **1 m**. Since 2022, 1. BImSchV § 19 requires new solid-fuel appliances to have a ridge-near outlet **≥ 40 cm above the ridge**.

### Cited Findings
**Russia: SP 7.13130.2013 "Отопление, вентиляция и кондиционирование. Требования пожарной безопасности" (MChS order 21.02.2013 No. 116; Amendment 1 by order 27.02.2020 No. 119; Amendment 2 by order 12.03.2020 No. 152)** — all clauses below are from the consolidated text hosted by MChS: [SP 7.13130.2013 (22.mchs.gov.ru, KonsultantPlus copy)](https://22.mchs.gov.ru/uploads/resource/2022-12-16/f9d9249185f94cf3d29efb0c52736b9e.docx)
- §5.4: maximum stove surface temperature **110 °C** on ≤ 15% of the surface and **120 °C** on ≤ 5% (90 °C in kindergartens and clinics).
- §5.5: one stove heats **≤ 3 rooms on one floor**. Two-storey buildings may use two-tier stoves with separate fireboxes and flues per floor. No wooden beams in the floor between tiers.
- §5.7: **each stove has its own flue**. Two stoves in one dwelling on one floor may share a chimney, with a separating wall (рассечка) **≥ 1 m** high from the bottom of the junction.
  - App use: count stoves per flue. Error if > 1 without the "same floor, same dwelling + 1 m рассечка" condition.
- §5.8: minimum brick or heat-resistant-concrete flue section is **140 × 140 mm** up to 3.5 kW, **140 × 200 mm** for 3.5–5.2 kW, and **140 × 270 mm** for 5.2–7 kW. A round flue must have at least the same area.
  - App use: input stove kW and flue section. Error if the section is too small.
- §5.9: solid-fuel stove flues need a damper with an opening of **≥ 15 × 15 mm**.
- §5.10: chimney height from the grate to the top is **≥ 5 m**. Height above the roof:
  - **≥ 500 mm above a flat roof**;
  - **≥ 500 mm above the ridge or parapet** when the chimney is **≤ 1.5 m** from it;
  - **not below the ridge or parapet** at **1.5–3 m**;
  - **not below a line drawn down from the ridge at 10°** when **> 3 m** away;
  - above the roofs of taller adjoining buildings.

  Ventilation ducts next to a chimney must be the same height as the chimney.
  - App use: in the roof or section view, read the chimney axis distance d from the ridge. Required top elevation: if d ≤ 1.5 m, ridge + 0.5 m; if 1.5 < d ≤ 3 m, ridge; if d > 3 m, ridge − d·tan10° (= ridge − 0.176·d). Also check grate-to-top ≥ 5.0 m. Warning: "Chimney top is 0.3 m below the required elevation (SP 7.13130 §5.10)."
- §5.11: chimneys must be vertical, without offsets, of clay brick with walls **≥ 120 mm** or heat-resistant concrete **≥ 60 mm**, with a **250 mm** deep cleaning pocket at the base and a door. Asbestos-cement pipes are allowed for flue gas ≤ 300 °C. Factory-made double-wall insulated stainless pipes are allowed for ≤ 400 °C. Offsets are allowed at **≤ 30°** from vertical with a horizontal shift **≤ 1 m**, keeping the same cross-section.
  - App use: in the brick chimney estimator, minimum wall is ½ brick (120 mm). Offset check: angle ≤ 30°, shift ≤ 1000 mm.
- §5.12: the chimney top must be protected from rain. Caps must not obstruct the exit. §5.13: wood or peat stoves on buildings with **combustible roofs** need a spark arrester mesh with openings **≤ 5 × 5 mm and ≥ 1 × 1 mm**.
- §5.14 + Annex B.1: cuts (разделки), including the stove wall thickness, are **500 mm** to combustible structures and **380 mm** to structures protected per §5.21b. The cut must be **70 mm thicker than the floor or ceiling slab** and must not be rigidly connected to the building. §5.15: in combustible walls, the cut runs the full height within the room and is at least as thick as the wall. §5.16: gaps are filled with non-combustible material.
  - App use: where a chimney passes a timber floor, the distance from the flue's inner face to timber must be ≥ 500 mm (380 mm if protected), and cut height = slab thickness + 70 mm. Estimate the cut's brick volume.
- §5.17 + Table B.1 (отступки, the clearance from the stove's outer face to a combustible wall):

  | Stove wall thickness | Clearance type | Unprotected wall | Protected wall |
  |---|---|---|---|
  | 120 mm | open | 260 mm | 200 mm |
  | 120 mm | closed | 320 mm | 260 mm |
  | 65 mm | open | 320 mm | 260 mm |
  | 65 mm | closed | 500 mm | 380 mm |

  Walls rated REI 60 with flame-spread class RP0 have no clearance requirement. Wall and floor protection must extend **≥ 150 mm** beyond the stove's outline. A closed clearance needs vent grilles of **≥ 150 cm²** at the bottom and top, and its floor 70 mm above the room floor. For factory-made stoves, use the manufacturer's data (§5.27).
  - App use: lookup table keyed by stove wall thickness and clearance type. Measure the plan distance from the stove to the nearest combustible wall.
- §5.18: gap from the stove top to a combustible ceiling (three-course brick top) is **250 mm** for periodic stoves and **700 mm** for long-burning stoves when the ceiling is protected. Unprotected: **350 / 1000 mm**. With a two-course top, multiply by 1.5. Metal stoves: **800 mm** (insulated top, protected ceiling) or **1200 mm** (uninsulated top, unprotected ceiling).
- §5.20: chimney to rafters, battens and other combustible roof parts is **≥ 130 mm** clear for brick or concrete chimneys and **250 mm** for uninsulated ceramic pipes (130 mm if insulated with R ≥ 0.3 m²·K/W). The gap is covered with non-combustible roofing.
  - App use: rafter layout around the chimney opening must keep ≥ 130 mm on all sides. Feed the trimmer framing into the estimate.
- §5.21: (a) a combustible floor under the firebox door needs a **700 × 500 mm metal sheet** on 8 mm insulating board, long side along the stove; (b) a combustible wall meeting the stove front at an angle needs 25 mm plaster on mesh, or metal on 8 mm board, from the floor to **250 mm above the firebox door**. **Distance from the firebox door to the opposite wall ≥ 1250 mm.**
  - App use: place a 700 × 500 hearth plate automatically. Error if the plan distance from the firebox door to the opposite wall is < 1250 mm.
- §5.22: with a combustible floor, the ash-pit bottom must be ≥ 140 mm and the flue bottom ≥ 210 mm above the floor. §5.23: stoves on legs over a combustible floor need steel on 10 mm board and ≥ 100 mm clearance. §5.24: connectors **≤ 0.4 m** long, ≥ 0.5 m from an unprotected combustible ceiling (0.4 m if protected), and ≥ 0.14 m above a combustible floor.
- §5.25: solid-fuel fireplaces are allowed in multi-storey buildings if **each fireplace has its own flue** (or a collective flue via an air lock, per Amendment 1). §5.26: factory-made fireplace flues need **≥ 8 cm² per 1 kW** of rated output. §5.27: cuts and clearances for factory appliances and flues follow the manufacturer's documentation.
  - App use: fireplace insert kW × 8 cm² = minimum flue area. Example: 12 kW gives 96 cm², about Ø110 mm, so a Ø150 or Ø200 insert collar should govern.
- Status: MChS issued an information letter on applying SP 7.13130.2013 (28.10.2024 No. 43-6858-19), which shows it was still in force. ABOK described a **draft** Amendment 3 (new definitions). No replacement "SP 7.13130.2024" was found. — [MChS letter (search result)](https://mchs.gov.ru/uploads/document/2024-10-30/8103cff2c987d35f7d4af34cf132c0e8.pdf); [ABOK on draft Amendment 3](https://www.abok.ru/for_spec/articles/44/8902/8902.pdf)

**Russia: SP 55.13330.2016 cross-references**
- §7.14: heat generators, including solid-fuel stoves and fireplaces, cookers and chimneys, must meet SP 60.13330 and SP 7.13130 plus the manufacturer's instructions. Solid-fuel storage may be on the first or plinth floor or in the basement. §6.x: structures must carry the extra load of stoves and fireplaces. — [SP 55.13330.2016](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf)

**Refractory (fireclay) brick**
- GOST 390-2018 "Изделия огнеупорные шамотные и полукислые общего назначения и массового производства" took effect **01.04.2019** (registration 974-ст of 09.11.2018). The earlier GOST R 53406-2009 is cancelled. It specified the normal brick as **230 × 114 × 64(65) mm**. — [NormaCS](https://www.normacs.info/ntds/9427); [russiangost GOST R 53406-2009](https://russiangost.com/p-59562-gost-r-53406-2009.aspx)
- Sellers list straight fireclay brick ШБ-5 as **230 × 114 × 65 mm**. — [satu.kz listing](https://satu.kz/Shb-5.html)
  - App use: refractory lining estimator with brick 230 × 114 × 65 mm and joint 2–3 mm (refractory mortar). Count bricks = lining area ÷ (0.230 × 0.065) for stretcher-laid ½-brick lining. Joint thickness is not from a cited source; mark it as a user parameter.

**Fireplace proportions (practice, not code)**
- The flue opening should be about **8–15 times smaller** than the firebox (portal) opening, typically 0.02–0.04 m². — [ukrsmeta.ua](https://ukrsmeta.ua/shirokie-kaminnye-topki)
- A German forum gives the same rule as **1:10 to 1:15** (flue cross-section : fireplace opening). — [bau.de forum](https://bau.de/forum/kachelofen/10624.php)
  - App use: portal W × H ÷ flue area. Green at 10–15, yellow at 8–10, red if < 8 or > 15. This is a heuristic warning only.

**Germany: Muster-Feuerungsverordnung (MFeuVO, as of Sept 2007, amended 28.01.2016 and 27.09.2017)** — [MFeuVO PDF (viega.de)](https://viega.de/content/dam/viegadm/content-assets/de/de-brandschutz/downloads-brandschutz/Muster-Feuerungsverordnung_DE_09_2017.PDF)
- § 4(7): combustible building parts must not exceed **85 °C**. If the manufacturer gives no clearance, keep **≥ 40 cm** from the appliance.
- § 4(8): in front of solid-fuel firing openings, a combustible floor needs a non-combustible covering extending **≥ 50 cm forward and ≥ 30 cm sideways** beyond the opening.
- § 4(9): **open fireplaces** need combustible parts **≥ 80 cm** above and to the sides of the firebox opening, or **40 cm** with a ventilated radiation shield.
  - App use: in EU/DE mode, a hearth plate of opening width + 2 × 300 mm by ≥ 500 mm deep, and an 800 mm combustible-free zone around an open fireplace opening.
- § 8(2): for flues up to 400 °C with R ≥ 0.12 m²K/W and 90 min fire resistance, keep **5 cm** to combustible parts, or **2 cm** to timber beams. No clearance is needed to small items such as skirting or roof battens. Other flues up to 400 °C need **40 cm**. Flue pipes up to 300 °C outside shafts need 20 cm, or 5 cm if clad ≥ 2 cm or flue gas ≤ 160 °C. § 8(3): passing through combustible parts needs a 20 cm protective sleeve or 20 cm non-combustible cladding.
- § 9(1): outlets must (1) **clear the ridge by ≥ 40 cm or be ≥ 1 m from the roof surface** (40 cm suffices for room-sealed gas or oil appliances ≤ 50 kW with fans); (2) clear roof structures, openings to rooms and unprotected combustible parts by **≥ 1 m** when those are **< 1.5 m** away; (3) for solid fuel on roofs that are not hard roofing (e.g., thatch), exit at the ridge and clear it by **≥ 80 cm**.
- Bavaria's version (BayFeuV § 9, valid until 31.12.2028) adds: clear the top edges of ventilation openings, windows and doors by **≥ 1 m** within **15 m** for solid fuel ≤ 50 kW (8 m for liquid or gas), with the radius growing per extra 50 kW up to 40 m. — [BayFeuV § 9](https://www.gesetze-bayern.de/Content/Document/BayFeuV2008-9)

**Germany: 1. BImSchV § 19 (since 1 January 2022)** — [gesetze-im-internet.de](https://www.gesetze-im-internet.de/bimschv_1_2010/__19.html)
- Solid-fuel installations built after 31.12.2021 need an outlet that is **"firstnah" and ≥ 40 cm above the ridge**. "Firstnah" means the horizontal distance to the ridge is less than the distance to the eaves, and the height above the ridge exceeds the horizontal distance to it. For roofs < 20°, measure against a fictitious ridge at 20°. The outlet must also clear windows, doors and ventilation openings within **15 m by 1 m** (≤ 50 kW). For larger outputs the radius and clearance increase: 17 m/2 m, 19 m/3 m, 21 m/3 m. Alternatively, the height may be determined by VDI 3781 Part 4.
- Plants installed before 2022 and later substantially changed: roof ≤ 20°, ridge + 40 cm or 1 m from the roof surface; roof > 20°, ridge + 40 cm or **2.30 m** horizontal distance from the roof surface.
  - App use: in DE/EU mode for a wood fireplace, check that (H_top − H_ridge) ≥ 0.4 m and also exceeds the horizontal distance to the ridge. Also check every window within 15 m.

### Inferences
- Russian chimney height is the stricter rule near the ridge (+0.5 m within 1.5 m). The German rule is stricter for solid fuel since 2022: always ≥ 0.4 m above the ridge and ridge-near, with no 10° relaxation. An app serving both should keep two rule packs.
- Brick chimney estimate: a ½-brick (120 mm) wall around a 140 × 140 flue gives an outer section of about 380 × 380 mm (the classic "1.5 × 1.5 brick" chimney). Courses = height ÷ (65 mm + about 10 mm joint). Bricks per course = 5 for a 140 × 140 flue. This is standard masonry practice, not quoted from a code; verify with the user's bonding pattern.
- Uzbekistan: no Uzbek stove or chimney fire code was found. SanPiN 0146-04 repeats SP 7.13130's "≤ 3 rooms per stove" and "own flue per stove". It is reasonable to apply SP 7.13130 numbers to UZ projects with a "reference: RU SP" label.

### Gaps
- **EN 13384-1** (chimney thermal and fluid calculation) and **DIN 18160-1** (Abgasanlagen planning) were not fetched. Their key numeric content, such as minimum draught and minimum effective height, is unverified.
- **GOST 8691-2018** (refractory shapes and sizes) was not verified. The edition and exact naming of ША-5 vs ШБ-5 (ША = higher alumina class) were not confirmed from the standard text.
- No source was found for the "firebox depth = ½–⅔ of portal height" rule or for refractory mortar standards (e.g., МШ-28/МШ-39, GOST 6137).
- No Uzbek national fire norm text for stoves (e.g., an Uzbek analogue of SP 7.13130) was retrieved.
- The German state FeuVOs differ. Only the Muster and Bavarian versions were read.

---

## 4. Swimming pools: SP 310.1325800.2017, private-pool structural practice, EU standards, quantity take-off

### Takeaway
SP 310.1325800.2017 "Бассейны для плавания" (Minstroy order 26.12.2017 No. 1716/пр; consolidated to 28.12.2023) is the code SP 55 refers to for house pools. Its checkable numbers:
- drainage slope of the bottom **1–4%** toward the outlets;
- walkway slope **1–2%** toward the overflow gutter;
- walkway width **≥ 1.5 m** indoor and **≥ 2 m** outdoor (public pools);
- recreational pool depth **1.0–1.25 m**;
- rounded pool edges;
- overflow grille flush with the walkway;
- waterproofing turned **300 mm** up the walls of wet rooms;
- no hollow, silicate or cellular materials.

No Russian or German code sets wall thickness, rebar or concrete grade for private pools. Practice (forums, unverified) points to C25/30 (B25) watertight concrete, a 20–30 cm slab, ≥ 20–25 cm walls, two layers of reinforcement and ≥ 4 cm cover. All of this must be designed by an engineer.

### Cited Findings
**Russia: SP 310.1325800.2017** — [SP 310 official edition PDF (meganorm)](https://meganorm.ru/Data2/1/4293739/4293739689.pdf); edition status "ред. от 28.12.2023" per [meganorm](https://meganorm.ru/mega_doc/norm/pravila/0/sp_310_1325800_2017_svod_pravil_basseyny_dlya_plavaniya.html)
- §5.2: the bottom should slope so people of different height can choose a depth (values in Annex A). §5.3: a technological drainage slope toward the outlets of **1%–4%**, usually perpendicular to the depth slope.
  - App use: bottom slope input. Error if the drainage slope is < 1% or > 4%. Compute the deep and shallow end from length × slope.
- Annex A, Table A.1: recreational pools (place of residence, mass recreation) have **depth 1.0–1.25 m** and "arbitrary" dimensions. Pools for pre-school children: 0.6–0.8 m. Training pools: 0.7–0.9 m (6 × 10/12 m) and 1.2–1.8 m. Design water area is 3, 4 or 5 m² per person by type. Note 1: the depth values define the slope along the length.
  - App use: for a family pool, warn if depth > 1.25 m in a "recreational" profile or if the shallow end > 0.8 m when "children" is ticked. Capacity = water area ÷ 5 m² per person (recreational).
- §5.4: rooms for pools **≥ 10 m long** need ≥ 6 m clear height above the walkway. §5.10: the pool edge must be rounded. §5.9: the overflow grille must be flush with the walkway. §5.11: pools **deeper than 1.2 m** need a resting ledge along the long walls. §5.15/§5.52: exit ladders in stainless steel, width 0.6 m, rung spacing 0.3 m, in wall niches 0.8–1.0 × 0.2–0.25 m.
- §5.19: walkway width **≥ 1.5 m** indoors and **≥ 2 m** outdoors, without starting blocks. §5.21: walkway non-slip, sloping **1%–2%** toward the overflow gutter. §5.22: heated walkways recommended for outdoor pools.
  - App use: slope the deck tile layer 1–2% toward the gutter. Deck width warning (recommendation for private pools).
- §8.2: silicate brick, hollow brick, light and cellular concrete and other moisture-absorbing materials are not recommended. §8.5: waterproofing in floors of wet rooms, turned up walls and columns **300 mm**, with an extra layer **100 mm** each side at precast joints. §8.6: wet-room walls tiled full height, or to at least 1.8 m with waterproof paint above. §8.7: wet-room floor **30 mm** below adjacent rooms. §8.8: recommended bowl structures are reinforced concrete finished with tile, PVC liner or elastic coating; welded stainless steel; or prefabricated metal with PVC liner. §8.10: bowl finishes must be light-coloured and resist pool chemicals, with grouted joints.
  - App use: in the pool-hall room type, auto-add 300 mm waterproofing upturn to the waterproofing area and a −30 mm floor step. Warning if the bowl material is "aerated block".
- §8.1: protect RC and steel against corrosion per SP 28.13330, timber per SP 64.13330.
- Amendment 1 to SP 310.1325800.2017 exists ([nav.tn.ru PDF](https://nav.tn.ru/cloud/iblock/bfc/bfcffabacd6fe377558c1ff7397d468f/SP-310.1325800.2017-_Izmenenie-_-1_.pdf)). Its content was not checked. The clause numbers above are from the 2018 official edition.

**Private-pool structural practice (forums only; low reliability; some posts may be AI-generated)**
- Minimum C25/30 with WU (watertight) class per DIN 1045-2, **two-layer reinforcement** (e.g., mesh Q188 or Q257) and **≥ 4 cm concrete cover**. Construction joints between slab and walls and penetrations for skimmers and pipes are the critical leak points. — [bau.de forum: WU-Beton Poolbau](https://bau.de/forum/sauna/10178.php); [bau.de forum: Swimmingpool aus WU-Beton](https://bau.de/forum/sauna/10113.php)
- Typical pool base slab **20–30 cm**, to be calculated. — [bau.de forum: Pool-Bodenplatte](https://bau.de/forum/keller/11292.php)
- One poster states a "white tub" needs about **25 cm** walls to be truly watertight, so 20 cm walls are borderline. — [bau.de forum](https://bau.de/forum/sauna/10178.php)
  - App use: defaults of wall 200–250 mm, slab 200–300 mm, concrete B25 (C25/30) W8, two mesh layers, cover 40 mm. Show the label "typical, must be verified by a structural engineer". Do **not** present them as code values.

### Inferences
Quantity take-off formulas for a rectangular in-ground pool. These are pure geometry, so the app can compute them. Inputs: inner L × W, depths D_shallow and D_deep, wall t_w, slab t_s, lean-concrete base t_b, working gap g (e.g., 0.5–0.8 m), overdig.
  - Water volume ≈ L × W × (D_shallow + D_deep)/2.
  - Excavation ≈ (L + 2t_w + 2g) × (W + 2t_w + 2g) × (D_avg + t_s + t_b + gravel), plus slope batter if the ground is unsupported.
  - Wall concrete = 2 × (L + W + 2t_w) × t_w × average wall height (add the beam or coping height).
  - Slab concrete = (L + 2t_w) × (W + 2t_w) × t_s.
  - Rebar (two meshes Ø12 @ 200) ≈ for each face: area × (1/0.2 + 1/0.2) bars per m × 0.888 kg/m, plus 10–15% laps and waste. Ø12 is 0.888 kg/m (standard steel table).
  - Waterproofing area = floor + walls (inner surface, using sloped-floor length) + upturns.
  - Tile area = same inner surface + coping top + deck. Count perimeter gutters and skimmers as items.
- For an overflow vs skimmer pool, the app should switch between "overflow gutter length = perimeter" (SP 310 §5.9 style) and "skimmer count". The skimmer-per-area ratio was not sourced (see Gaps).
- For Uzbekistan, outdoor pools face high evaporation and heat, and frost resistance (F-grade) matters less in the south. This is an inference; no UZ pool norm was found.

### Gaps
- No normative Russian or German source gives wall or slab thickness, rebar or W-grade for **private** pools. The 200–250 mm, Ø12 @ 200 double mesh and W8–W12 figures remain industry practice without a primary citation in this research.
- **EN 16582-1/-2/-3** (domestic swimming pools: general / in-ground / above-ground), **DIN 19643** (pool water treatment), EN 15288 and EN 13451 were not fetched. Their numeric requirements (e.g., step riser limits, entry and exit provisions, turnover times) are unverified.
- The **skimmer count rule** (commonly quoted as 1 skimmer per 25–50 m² of water surface, placed downwind) was not sourced.
- No Uzbek pool code was found.
- The changes in SP 310 Amendment 1 (and any later amendments up to the 28.12.2023 consolidation) were not reviewed.

---

## 5. Roofs: minimum slopes by roofing type (SP 17.13330.2017) and snow/wind regions

### Takeaway
SP 17.13330.2017 "Кровли" (with Amendments 1–4) Table 4.1 sets minimum roof slopes:

| Roofing | Minimum slope |
|---|---|
| Metal tile and corrugated steel sheet (профнастил) | 20% (12°) |
| Bitumen shingles | 20% (12°) |
| Cement-sand or ceramic tile | 40% (22°) |
| Metal standing-seam tile | 47% (25°) |
| Standing-seam sheet metal (оцинковка, медь и т.п.) | 10% (6°) |
| Bituminous roll roofing with coarse topping | 1.5% (1°) |

Snow and wind loads come from SP 20.13330.2016 maps (Annex E), but the region values could not be verified in this session.

### Cited Findings
- SP 17.13330.2017 (approved by Minstroy order 31.05.2017 No. 827/пр, in force 01.12.2017, replaces SP 17.13330.2011). Amendment 1 was approved by order 18.02.2019 No. 111/пр (effective 19.08.2019), Amendment 2 by order 29.12.2020 No. 890/пр (effective 30.06.2021), followed by Amendments 3 and 4. — [SP 17.13330.2017 with Amendments 1–4 (fkr-spb.ru PDF)](https://fkr-spb.ru/upload/iblock/ff4/v6xtf7xvcrma4l27t6f8zjv40338nkv2.pdf)
- **Table 4.1** (Amendments 1, 2, 4), minimum roof slope in % (°):
  - Roll or mastic, non-accessible: bituminous rolls with gravel 1.5–10 (1–6); with factory coarse topping or foil 1.5 (1); mastic with gravel 1.5–10; mastic with paint protection ≥ 1.5 (1); polymer membranes or mastics ≥ 1.5 (1). Accessible, inverted and green roofs: 1.5–3.0 (1–2).
  - Tiles: cement-sand, ceramic or polymer-cement **≥ 40 (22)**; **bitumen shingles ≥ 20 (12)**; metal standing-seam tile **≥ 47 (25)**; chrysotile-cement, slate, composite or fibre-cement plates ≥ 40 (22).
  - Corrugated sheets: chrysotile-cement and bitumen ≥ 20 (12); fibre-cement ≥ 36 (20). **Metal profiled sheets, including metal tile (металлочерепица), ≥ 20 (12).**
  - Metal roll or sheet (galvanised with polymer coating, stainless, copper, zinc-titanium, aluminium, i.e., standing seam) **≥ 10 (6)**.
  - Precast RC trough panels with factory mastic 5–10 (3–6).
  - Footnote: bituminous membranes at slopes ≥ 25% need anti-slip measures.

  — [SP 17.13330.2017](https://fkr-spb.ru/upload/iblock/ff4/v6xtf7xvcrma4l27t6f8zjv40338nkv2.pdf)
  - App use: a roofing-type dropdown sets the minimum. slope% = 100·tan(angle). Error if the drawn slope is below the minimum. Example: "Metal tile at 10° < 12° minimum (SP 17 Table 4.1, item 2.3)."
- §4.3 allows slopes below Table 4.1 only with additional waterproofing measures, but does not say under what conditions. — [normacs expert answer (search summary)](https://www.normacs.info/answers/5992)
- A draft Amendment 5 to SP 17.13330.2017 was in public discussion until 18.07.2024. Its adoption was not confirmed. — [normacs](https://www.normacs.info/discussions/9221)
- Snow loads: before Amendment 2 to SP 20.13330.2016, ground snow load S_g was taken from Table 10.1 by snow region (Annex E maps). In SP 20.13330.2016, S_g is a *characteristic* (нормативное) value; in the 2011 edition it was a *design* value. — [normacs answer](https://www.normacs.info/answers/3355)

### Inferences
- Many metal-tile manufacturers allow 12–14°. Use 12° (20%) as the code minimum and let the manufacturer's value override upward.
- For Uzbekistan, no Uzbek roof-slope norm was found. SP 17 values are a reasonable reference, labelled as such.

### Gaps
- **Snow and wind region values** were not verified from the SP 20.13330.2016 text. My recollection, to be checked: S_g = 0.5 / 1.0 / 1.5 / 2.0 / 2.5 / 3.0 / 3.5 / 4.0 kPa for regions I–VIII, and w₀ = 0.17 / 0.23 / 0.30 / 0.38 / 0.48 / 0.60 / 0.73 / 0.85 kPa for regions Ia–VII. One search snippet showed different values for the 2011 edition (0.6 / 0.9 / 1.3 / 1.8 / 2.3 / 2.8 / 3.4 / 4.0). Before coding these, fetch the current SP 20.13330 text, including its amendments and any newer edition.
- Uzbek snow and wind loads (ShNQ 2.01.07 "Нагрузки и воздействия" or its successor) were not researched.

---

## 6. Which editions are current (Nov 2026) and which are superseded?

### Takeaway
Russian codes still current for this scope:
- SP 55.13330.2016 with Amendments 1 and 2;
- SP 54.13330.2022, which replaced SP 54.13330.2016;
- SP 1.13130.2020, consolidated to 16.06.2025;
- SP 7.13130.2013 with Amendments 1 and 2, no new edition, Amendment 3 a draft only;
- SP 17.13330.2017 with Amendments 1–4 (Amendment 5 drafted in 2024);
- SP 310.1325800.2017, consolidated to 28.12.2023.

In Uzbekistan, **ShNQ 2.08.01-24** (14.08.2024) is the current residential design norm. ShNK 2.08.01-19, amended in 2020 and 2022, appears superseded, which needs confirmation. In Germany, DIN 18065 is cited as 2020-08 (one forum mentions 2022-09), the MFeuVO stands as amended in 2017, and 1. BImSchV § 19 has applied since 2022.

### Cited Findings
- SP 54.13330.2022: "Применяется с 14.06.2022 взамен СП 54.13330.2016". — [SP 54.13330.2022](https://fkr-spb.ru/upload/iblock/e62/7vxc9o7pegu5vnd1xegbtxzg4ffdhry5.pdf)
- SP 1.13130.2020: MChS order 19.03.2020 No. 194 "ред. от 16.06.2025". — [sudact.ru](https://sudact.ru/law/prikaz-mchs-rossii-ot-19032020-n-194/sp-1.13130.2020-svod-pravil/4/4.4/)
- SP 7.13130.2013: amended by MChS orders 27.02.2020 No. 119 and 12.03.2020 No. 152. — [MChS-hosted text](https://22.mchs.gov.ru/uploads/resource/2022-12-16/f9d9249185f94cf3d29efb0c52736b9e.docx)
- SP 55.13330.2016: approved by Minstroy order 20.10.2016 No. 725/пр, in force 21.04.2017. Amendment 1 by order 10.07.2018 No. 415/пр. Amendment 2 appears in the consolidated text on cadastre.ru. — [SP 55 PDF](https://sro-a.ru/upload/medialibrary/abc/SP-55.13330.2016.-Svod-pravil.-Doma-zhilye-odnokvartirnye.-SN.pdf); [cadastre.ru](https://cadastre.ru/source/6)
- SP 17.13330.2017 with Amendments 1–4. — [fkr-spb.ru PDF](https://fkr-spb.ru/upload/iblock/ff4/v6xtf7xvcrma4l27t6f8zjv40338nkv2.pdf)
- ShNQ 2.08.01-24: approved 14.08.2024 (order No. 01/2-57), registered 25.09.2024 No. 293. It covers residential buildings up to 50 m / 16 storeys. The list of norms it repeals is in Appendix 2 of the order and was not visible. — [lex.uz](https://lex.uz/en/acts/-7121730)
- ShNK 2.08.01-19: amendments of 25.09.2020 and 18.02.2022. — [lex.uz amendment](https://lex.uz/docs/5888568)
- DIN 18065:2020-08 per WEKA. — [WEKA](https://www.weka.de/architekten-ingenieure/din-18065/). A forum cites 2022-09. — [bau.de](https://bau.de/forum/treppen/11797.php)
- GOST 390-2018 (from 01.04.2019) replaces GOST R 53406-2009 for fireclay products. — [NormaCS](https://www.normacs.info/ntds/9427)

### Inferences
- The app should store each rule with {standard, edition, clause, jurisdiction, mandatory | recommended}. That lets you swap ShNQ 2.08.01-24 for -19, or add SP 7.13130 Amendment 3, without code changes.

### Gaps
- Whether ShNQ 2.08.01-24 Appendix 2 explicitly repeals ShNK 2.08.01-19 was not confirmed.
- No Uzbek norm specifically for individual (single-family) houses was identified.
- Whether a new SP 20.13330 edition exists in 2026 was not checked.
- The approval date of SP 55.13330.2016 Amendment 2 is unknown.
