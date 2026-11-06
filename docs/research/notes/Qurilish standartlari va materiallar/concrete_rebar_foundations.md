# Concrete, cement, reinforcement and foundation rules (RU / EU / UZ) for an estimating + drawing app

Research date: 2026-11-06. Primary texts were read from full-text mirrors (meganorm.ru, base.garant.ru) because docs.cntd.ru refused connections, and mc.uz PDFs could not be downloaded because of a TLS certificate mismatch. "Verbatim" means the number was read from the standard's own text. "Estimate" means trade/secondary data or my own arithmetic, and it is labelled that way. The "App use:" notes are proposals for the app, not requirements from the standards.

---

## 1. Concrete classes (B / M / C), W and F grades, exposure classes, typical classes per element

### Takeaway
Design in Russia and Uzbekistan uses class **B** (MPa at 95 % confidence). Suppliers still sell by mark **M** (kgf/cm²). The official bridge is the GOST 26633-91 Appendix 1 table (B15 ≈ M200, B20 ≈ M250, B22.5 ≈ M300, B25 ≈ M350, B30 ≈ M400). Reinforced concrete must be at least **B15** (SP 63.13330.2018 §6.1.6). Under EN 206 Annex F, foundations in exposure class XC2 need at least **C25/30** with w/c ≤ 0.60.

### Cited Findings
**Standards and their editions**
- **SP 63.13330.2018** "Бетонные и железобетонные конструкции. Основные положения" was approved by Minstroy order No. 832/пр of 19.12.2018 and took effect 6 months later. It replaced SP 63.13330.2012, which is no longer applied. Amendment No. 1 was approved by order No. 717/пр of 22.11.2019. — [legalacts.ru](https://legalacts.ru/doc/prikaz-minstroja-rossii-ot-19122018-n-832pr-ob-utverzhdenii/); [rulaws.ru Amendment 1](https://rulaws.ru/acts/Prikaz-Minstroya-Rossii-ot-22.11.2019-N-717_pr); [meganorm card, effective 20.06.2019](https://meganorm.ru/Index2/1/4293732/4293732352.htm)
- **GOST 26633-2015** "Бетоны тяжелые и мелкозернистые. ТУ" took effect 01.09.2016 and replaced GOST 26633-2012. Amendment No. 1 took effect 01.07.2023 (Rosstandart order 414-ст of 21.06.2023). — [meganorm card](https://meganorm.ru/mega_doc/norm/gost_gosudarstvennyj-standart/0/gost_26633-2015_mezhgosudarstvennyy_standart_betony.html); [meganorm Amendment 1](https://meganorm.ru/mega_doc/norm/prikaz/13/izmenenie_N_1_gost_26633-2015_betony_tyazhelye_i.html)

**Class and grade lists**
- GOST 26633-2015 §4.3.1 (with Amendment 1):
  - Compressive classes: B3.5; B5; B7.5; B10; B12.5; B15; B20; B22.5; B25; B27.5; B30; B35; B40 … B120.
  - Frost-resistance grades: F1 50, F1 75, F1 100, F1 150, F1 200, F1 300 … F1 1000 by the first basic method, and F2 100 … F2 500 by the second.
  - Water-resistance grades: W2, W4, W6, W8, W10, W12 … W20.
  - Source: [GOST 26633-2015 text, meganorm](https://meganorm.ru/mega_doc/norm/gost_gosudarstvennyj-standart/0/gost_26633-2015_mezhgosudarstvennyy_standart_betony.html)
  - **App use:** use these lists as the dropdown enumerations for class, F and W.
- GOST 26633-2015 §4.4.3: mixes for frost grade **F1 200 (F2 100) and higher** must contain air-entraining or gas-forming admixtures, with entrained air **≥ 4 %**. — [same source](https://meganorm.ru/mega_doc/norm/gost_gosudarstvennyj-standart/0/gost_26633-2015_mezhgosudarstvennyy_standart_betony.html)
  - **App use:** if the user selects F ≥ 200, add the note "air-entrained mix, ≥ 4 % air" to the order spec.
- SP 63.13330.2018 Table 6.1 lists heavy-concrete classes B3.5 … B100. Table 6.4 gives W2–W20 for heavy concrete. §3.9 defines W as the maximum water pressure (MPa·10) a specimen withstands without leaking. — [SP 63.13330.2018 full text, meganorm](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- SP 63.13330.2018 §6.1.6 (verbatim): "Для железобетонных конструкций следует применять класс бетона по прочности на сжатие **не ниже В15**". Prestressed concrete must be ≥ B20. — [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
  - **App use:** hard warning if a reinforced element is assigned a class below B15 (M200).
- SP 63.13330.2018 §6.1.8–6.1.9: F and W grades are assigned according to **SP 28.13330** (corrosion protection), based on freeze-thaw exposure and how aggressive the environment is. SP 63 itself gives no fixed F/W per element type. — [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- SP 63.13330.2018 §6.1.5: the default design age is **28 days**. — [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)

**Design strengths, SP 63.13330.2018 Table 6.8 (heavy concrete, MPa)**

| Class | Rb (compression) | Rbt (tension) |
|---|---|---|
| B15 | 8.5 | 0.75 |
| B20 | 11.5 | 0.90 |
| B25 | 14.5 | 1.05 |
| B30 | 17.0 | 1.15 |

- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- **App use:** Rbt is an input to the anchorage and lap calculator in section 4.

**Class B to mark M mapping, GOST 26633-91 Appendix 1**

Mean strength R was calculated at a coefficient of variation of 13.5 % and 95 % confidence.

| Class | Mean strength R, kgf/cm² | Nearest mark |
|---|---|---|
| B7.5 | 98.2 | M100 |
| B10 | 131.0 | M150 |
| B12.5 | 163.7 | M150 |
| B15 | 196.5 | M200 |
| B20 | 261.9 | M250 |
| B22.5 | 294.5 | M300 |
| B25 | 327.4 | M350 |
| B27.5* | 359.9 | M350 |
| B30 | 392.9 | M400 |
| B35 | 458.4 | M450 |
| B40 | 523.9 | M550 |

- Source: [zakonbase.ru, GOST 26633-91 Appendix 1](https://zakonbase.ru/content/part/464925?print=1)
- *The page renders this row as "B26,5". R = 359.9 matches B27.5 (≈13.09 × B).
- GOST 26633-91 is superseded, but this table is still the reference people use for the M↔B bridge. Suppliers in Uzbekistan advertise the same pairs, for example "B22.5 (M300)" and "B25 (M350)". — [prom.uz listing](https://www.prom.uz/ads/tovarnyi-beton-klassa-v25-m350/); [satu.kz](https://satu.kz/Beton-m200.html)
- **App use:** store B as the canonical value, show "B20 (M250)" in the UI, and also display the EN equivalent from the Inferences below.

**EN 206 limiting values (Annex F, recommended), carbonation and chloride classes**

| | XC1 | XC2 | XC3 | XC4 | XS1 | XS2 | XS3 |
|---|---|---|---|---|---|---|---|
| Max w/c | 0.65 | 0.60 | 0.55 | 0.50 | 0.50 | 0.45 | 0.45 |
| Min strength class | C20/25 | C25/30 | C30/37 | C30/37 | C30/37 | C35/45 | C35/45 |
| Min cement, kg/m³ | 260 | 280 | 280 | 300 | 300 | 320 | 340 |

- Source: [RILEM workshop paper (2003), reproducing Annex F of EN 206-1](https://www.rilem.net/images/publis/pro029-022.pdf). This is the EN 206-1 edition. A StruSoft summary of EN 206:2013+A1:2016 Table F.1 gives the same XC1 (0.65, C20/25) and XC2 (0.60, C25/30) values and notes that national annexes vary. — [StruSoft wiki (search extract)](https://wiki.pre-stress.strusoft.com/xwiki/bin/view/PRE-Stress/Theory%20PRE-Stress/Exposure%20class/)
- **App use:** in "EU mode", default foundations to XC2 → C25/30, w/c ≤ 0.60, cement ≥ 280 kg/m³.
- Irish guidance says **XF1** concrete should be air-entrained and at least C25/30. Other national tables differ. — [StruSoft search extract / Irish tables](https://Roadstone.ie/sites/default/files/2023-03/Roadstone%20EN206%20Concrete%20Specification%20RS.BRO-10%20Rev%2001%20Feb%20%E2%80%9823_0.pdf)

**Water resistance for pools and buried work**
- SP 310.1325800.2017 "Бассейны для плавания" §8.1 only requires pool RC to be protected from corrosion according to SP 28.13330. It gives **no specific W grade**. — [SP 310 text, meganorm](https://meganorm.ru/Data2/1/4293739/4293739689.htm)
- A table-note from an SP 28 edition (Table B.2) says liquid media are non-aggressive at any bicarbonate alkalinity for concrete **W6 and above**. For W4 this holds only if the soil filtration coefficient is below 0.1 m/day. — [dnaop.com (edition not confirmed as 2017)](https://dnaop.com/html/67285_7.html)
- Under SP 28.13330 the minimum W for piles is W6. — [normacs expert answer](https://www.normacs.info/answers/2151)
- **App use:** default pool shell to W6 at minimum. Suggest W8 for pools and for basements below the water table, labelled as practice (see Gaps).

### Inferences
- **Approximate B ↔ C pairs** (mine, from cube strength; RU B ≈ EN cube characteristic value): B15 ≈ C12/15, B20 ≈ C16/20, B25 ≈ C20/25, B30 ≈ C25/30. A Russian "B25 (M350)" is therefore close to EN C20/25. It is *not* C25/30. The app should not imply that an RU foundation mix satisfies EN XC2 (C25/30) unless it is B30 (M400).
- **Suggested defaults by element (practice-based, editable):**

| Element | Class | Mark | W | F |
|---|---|---|---|---|
| Strip footing | B20–B22.5 | M250–M300 | W4–W6 | F100–F150 |
| Slab-on-ground (plita) | B22.5–B25 | M300–M350 | W6 | F150 |
| Pool shell | B25–B30 | M350–M400 | W6–W8 | F150–F200 + air entrainment if F ≥ 200 |
| Stairs and floor slabs | B20–B25 | M250–M350 | — | — |
| Blinding | B7.5 | M100 | — | — |

  - The B15 floor (§6.1.6) and the B7.5 blinding minimum (SP 22 §4.25 note, section 6) are normative. The other values are estimates.

### Gaps
- I could not fetch EN 206 Annex F values for XF1–XF4 and XA1–XA3. Secondary sources were inconsistent. Commonly cited values are XF1 0.55/C30/37/300 and XA1 0.55/C30/37/300, but I did **not** verify them.
- I did not reach the SP 28.13330.2017 tables of minimum W/F by exposure. I found no normative "pool = W6–W8" rule; that figure is practice.
- I did not verify the EN 206 Table 1 descriptions (for example "XC2 = wet, rarely dry; many foundations") in this session.

---

## 2. Mix proportions per m³, cement standards (GOST 31108 / EN 197-1), bags

### Takeaway
No normative "recipe" exists. GOST requires mixes to be designed by laboratory trial, so per-m³ tables are trade estimates. Typical figures are **≈ 245–285 kg/m³ of cement for M200 and ≈ 340–385 kg/m³ for M300**, with M500 (CEM I 42.5N) needing about 10–15 % less than M400. The cement class itself is normative: CEM I 42.5N must reach **≥ 10 MPa at 2 days and 42.5–62.5 MPa at 28 days**.

### Cited Findings
**Cement strength classes, GOST 31108 Table 2 (2003 text on meganorm)**

| Class | 2-day min, MPa | 7-day min, MPa | 28-day, MPa | Initial set, min | Expansion, mm |
|---|---|---|---|---|---|
| 22.5N | — | 11 | 22.5–42.5 | ≥ 75 | ≤ 10 |
| 32.5N | — | 16 | 32.5–52.5 | ≥ 75 | ≤ 10 |
| 32.5B | 10 | — | 32.5–52.5 | ≥ 75 | ≤ 10 |
| 42.5N | 10 | — | 42.5–62.5 | ≥ 60 | ≤ 10 |
| 42.5B | 20 | — | 42.5–62.5 | ≥ 60 | ≤ 10 |
| 52.5N | 20 | — | ≥ 52.5 | — | — |

- Source: [GOST 31108-2003 text, meganorm](https://meganorm.ru/Data2/1/4294844/4294844984.htm)
- A review of the 2020 revision says 22.5N and 32.5N keep the 7-day requirement and all other classes keep the 2-day requirement. — [meganorm Data1/11715](https://meganorm.ru/Data1/11/11715/index.htm)
- An Uzbek producer (AGMK) catalogue shows the same 42.5N values (2 d ≥ 10; 28 d 42.5–62.5) for current output. — [agmk.uz](https://agmk.uz/en/products/portlandtsement-obschestroitelnyj)
- Dyckerhoff gives the same 42.5–62.5 MPa range for EN 197-1 CEM I 42.5 N. — [Dyckerhoff datasheet](https://res.dyckerhoff.com/deliveryprogram/files/germany/LE/24100101TDen.pdf)
- **App use:** in the cement picker, map the legacy "М400" to ≈ 32.5 and "М500" to ≈ 42.5. This mapping is an estimate.

**Per-m³ cement consumption (trade estimates)**

| Mark | s-b-1.ru, M400 cement | s-b-1.ru, M500 cement | buildingclub.ru, M400 | buildingclub.ru, M500 |
|---|---|---|---|---|
| M200 | 285 | 245 | 240–260 | 210–230 |
| M250 | 335 | 295 | 290–320 | 260–290 |
| M300 | 385 | 340 | 320–340 | 290–310 |

- Sources: [s-b-1.ru](https://s-b-1.ru/blog/raskhod-tsementa-na-kub-rastvora/); [buildingclub.ru](https://buildingclub.ru/rashod-cementa-na-kub-betona/)
- Calc101 gives these full M400-cement mixes (cement / sand / crushed stone, kg/m³):
  - M200: 241 / 844 / 1350
  - M300: 329 / 790 / 1415
  - Source: [calc101.ru](https://calc101.ru/rashod-betona.html)
- Mass ratios C : S : G for PC400 are M200 1 : 2.8 : 4.8 (≈ 280 kg cement, 0.78 t sand, 1.34 t stone), M250 1 : 2.1 : 3.9, and M300 1 : 1.9 : 3.7. — [schebenpesok.ru](https://schebenpesok.ru/kalkulyator/beton-m200/)
- Indicative w/c: 0.65 for B15 (M200), 0.60 for B20 (M250), 0.55 for B22.5 (M300). — [buildingclub.ru](https://buildingclub.ru/rashod-cementa-na-kub-betona/)
- One calculator warns that M200–M250 designed with PC500 falls below a cement minimum of about 220 kg/m³ "по ГОСТ" and becomes harsh and lean. It recommends PC300–400 for these low marks. — [toolfox.ru concrete calculator (via search extract)](https://toolfox.ru/tools/concrete-calculator)
- **App use:** ship an editable "recipe table" flagged as an estimate, and recommend buying ready-mix with a supplier passport (паспорт).

### Inferences
- **Bags per m³** (50 kg bags; my arithmetic from the s-b-1 figures):
  - M200 needs 285 kg (5.7 bags) with M400 cement, or 245 kg (4.9 bags) with M500.
  - M300 needs 385 kg (7.7 bags) with M400, or 340 kg (6.8 bags) with M500.
  - **App use:** `bags = ceil(cement_kg_per_m3 × V × (1 + loss) / 50)`.
- **Water (L/m³)** ≈ w/c × cement. For example, M200 on M400 is 0.65 × 285 ≈ 185 L. The real figure depends on slump and aggregate moisture, so treat it as an estimate.
- A sensible default for an EN cross-check is the EN 206 XC2 minimum of 280 kg/m³ (section 1). It sits between the RU trade figures for M250 and M300.

### Gaps
- No normative per-m³ recipe exists in GOST 26633 / GOST 27006 (laboratory selection is required), so all recipe numbers are trade estimates and the sources disagree by ±15 %.
- I could not verify the GOST 31108-**2020** text directly; I used the 2003 table plus the review. I did not confirm the status of the legacy GOST 10178-85 (M400/M500 marks).
- The "≈220 kg/m³ minimum" in the calculator warning is unattributed; I don't know which GOST clause it refers to.

---

## 3. Mixer trucks, ordering, pumps, waste allowance

### Takeaway
Order volume should be the geometric volume plus the normative placement loss: **1.8 %** for foundations and slabs, **2.8 %** for columns and beams, **3.5 %** for ribbed slabs. Round the total up to whole truckloads. The most common Russian mixer is **7 m³**, then 9 m³ and 5 m³.

### Cited Findings
- Russian makers supply truck mixers of 2, 4, 5, 6, 7, 8, 9, 10, 11, 12 and 14 m³. The **7 m³** size makes up **57 %** of supplied units, 9 m³ 17 % and 5 m³ 15 %. — [Taimarov & Lavirko, Izvestiya KGASU 2020](https://izvestija.kgasu.ru/files/2_2020/81_88_Taimarov_Lavirko.pdf)
- A Ukrainian 2011 source says 7 and 9 m³ drums are the most economical and make up to 75 % of plant fleets. — [KNUBA repository](https://repositary.knuba.edu.ua/bitstreams/70a5b9f4-4943-42e4-9825-ed694636bb6d/download)
- 10–12 m³ mixers are typically 4-axle; 5–9 m³ units have 2–3 axles. — [booksite technical reference](https://booksite.ru/FullText/1/001/008/114/821.htm)
- Heavy mixers face axle-load limits; one undated trade article says only about a third of roads allow 10 t per axle. — [zr.ru](https://www.zr.ru/archive/reis/2010/01/dva-diela-srazu)
- **App use:** default the truck size to 7 m³, let the user pick from {5, 7, 9, 10, 12}, and show a warning such as "check site access and axle load for ≥ 10 m³".

**Normative placement losses, Сборник типовых норм потерь (supplement to RDS 82-202-96, Gosstroy letter ВБ-20-276/12 of 03.12.1997), Table 1, item 42**

| Operation | Loss |
|---|---|
| Placing concrete in foundations, monolithic supports and slab spans | **1.8 %** |
| Columns (подколонники) and beams (ригели) | 2.8 % |
| Ribbed slabs | 3.5 % |
| Wet joints | 4.15 % |
| Underwater by tremie (ВПТ) | 10.1 % |
| Cement-sand mortar as a preparation layer under foundations | 10.4 % |
| Crushed-stone base, first layer | 10.5 % |
| Crushed-stone base, each next layer | 4.5 % |

- Source: [kontur-extern.ru text of the Сборник](https://www.kontur-extern.ru/info/normativ/document/8/12895-sbornik-tipovykh-norm-poter-materialnykh-resursov-v-stroitelstve)
- The Сборник has **no** separate norm for rebar cutting waste or tie wire. — [same](https://www.kontur-extern.ru/info/normativ/document/8/12895-sbornik-tipovykh-norm-poter-materialnykh-resursov-v-stroitelstve)
- **App use:** `loss_factor` per element type, with defaults foundation/slab 0.018, beam/column 0.028, ribbed 0.035.

### Inferences
- **Ordering formula:**
  - `V_order = V_geom × (1 + loss) + V_overpour`.
  - `trucks = ceil(V_order / truck_m3)`.
  - Also show the last-truck fill percentage so the user can decide whether to round down and top up from the next pour.
  - The overpour/uneven-subgrade allowance is not in the standards. A user-editable 0–3 % default (estimate) is reasonable, and it is higher on unblinded earth trenches.
- **Pump:** suggest a pump when the truck cannot reach within the chute length (commonly ~3–4 m, estimate) or when the free drop would exceed the SP 70 limits in section 5.

### Gaps
- I found no authoritative source on minimum order volumes, short-load surcharges or pump minimums; these vary by supplier. I also found no Uzbek-specific truck-fleet data.
- One trade snippet gave a 10 min/m³ unloading norm but I could not attribute it, so it is excluded.

---

## 4. Rebar: classes, mass, lengths, laps/anchorage, cover, stirrups, ties

### Takeaway
GOST 34028-2016 replaced GOST 5781-82, GOST 10884-94 and GOST R 52544-2006. It defines A240 (smooth), A400 and A500, with a "C" suffix for weldable bars. Standard bar lengths are 6, 9 and 12 m (11.7/11.9 m), up to 18 m. SP 63.13330.2018 §10.3 sets the cover, spacing, anchorage and lap rules. For laps the minimum is **≥ 0.4·α·L0,an, ≥ 20d and ≥ 250 mm**; for anchorage it is **≥ 0.3·L0,an, ≥ 15d and ≥ 200 mm**. Cover in foundations is **40 mm with blinding and 70 mm without** (SP 63) versus **40 mm / 75 mm** in EN 1992-1-1.

### Cited Findings
**GOST 34028-2016 status and classes**
- Rosstandart put GOST 34028-2016 into force as a national standard from 01.01.2018 (order 232-ст of 31.03.2017). — [meganorm card](https://meganorm.ru/mega_doc/norm/gost_gosudarstvennyj-standart/1/gost_34028-2016_mezhgosudarstvennyy_standart_prokat.html)
- Other sources report that the effective date was postponed to 01.01.2019 (order 2100-ст), and that it replaced GOST 5781-82, GOST 10884-94 and GOST R 52544-2006 (for class B500C). — [NormaCS news](https://www.normacs.info/ntds/5750)
- GOST 34028-2016 §4.1:
  - Classes A240, A400, A500 and A600 cover precast and cast-in-situ RC; Ап600, A800 and A1000 are for prestressing.
  - Suffixes: **С** = weldable by all methods; **Н** = increased ductility; **Е** = high ductility (seismic); **К** = resistant to corrosion cracking; **У** = fatigue; **Р** = relaxation.
  - A240 is smooth; the others are ribbed (profile forms 1ф–4ф).
  - Source: [GOST 34028-2016 text, meganorm](https://meganorm.ru/Data2/1/4293746/4293746568.htm)
  - **App use:** use an "Е" filter for seismic regions (most of Uzbekistan) as a suggestion.
- GOST 34028-2016 Table 5 mechanical properties:
  - A240: σт 240 / σв 380 N/mm², δ5 25 %.
  - A500: σт 500 / σв 600, σв/σт ≥ 1.05, δ5 14 %, δmax 2.5 %.
  - A400: the meganorm copy shows σт 390 / σв 590 and δ5 16 %. This matches the old A-III values and is consistent with SP 63-2018 Rs = 340 MPa (390/1.15).
  - Source: [GOST 34028 text](https://meganorm.ru/Data2/1/4293746/4293746568.htm)

**Bar lengths, GOST 34028-2016 §5.1.5 (verbatim)**
- Bars are supplied 6.0 to 18.0 m long, either cut to length (МД) or random length (НД, 6.0–12.0 m).
- "Предпочтительная мерная длина прутков — **6,0; 9,0; 12,0 (11,7 и 11,9); 18,0 м**".
- With МД1 delivery, short bars (≥ 2 m) may make up ≤ 3 % of the lot mass.
- A240, A400 and A500 come in bars or coils.
- Source: [GOST 34028 text](https://meganorm.ru/Data2/1/4293746/4293746568.htm)
- **App use:** default stock length 11.7 m, with 6/9/12 as options. Run a cutting-list optimiser per diameter.

**Nominal mass per metre, GOST 34028-2016 Table 1 (kg/m, at a steel density of 7.85 g/cm³)**

| Ø, mm | 6 | 8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 25 | 28 | 32 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Area, mm² | 28.3 | 50.3 | 78.5 | 113.1 | 153.9 | 201.1 | 254.5 | 314.2 | 380.1 | 490.9 | 615.8 | 804.3 |
| kg/m | 0.222 | 0.395 | 0.617 | 0.888 | 1.208 | 1.578 | 1.998 | 2.466 | 2.984 | 3.853 | 4.834 | 6.313 |

- Ø40 is 9.865 kg/m.
- Permitted mass deviation for group ОМ1: ±8 % up to Ø8, ±6 % for Ø8.5–12, ±5 % for Ø13–20, ±4 % for Ø22–40.
- Source: [GOST 34028 text](https://meganorm.ru/Data2/1/4293746/4293746568.htm)
- **App use:** `mass_kg = Σ length_m × kg_per_m[Ø]`. Optionally show a "weighbridge tolerance ±x %" note.

**Design resistance Rs, SP 63.13330.2018 Table 6.14 (tension / compression, MPa)**
- A240: 210 / 210
- A400: **340** / 340
- A500: 435 / 435 (400)
- B500: 415
- γs = 1.15 for class A (§6.2.8).
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- **App use:** these values feed the anchorage formula below.

**Concrete cover, SP 63.13330.2018 §10.3.2 and Table 10.1 (verbatim minimums for working bars)**

| Condition | Min cover |
|---|---|
| Closed rooms, normal or reduced humidity | 20 mm |
| Closed rooms, high humidity | 25 mm |
| Outdoors | 30 mm |
| In soil, or in monolithic foundations **with blinding** | **40 mm** |
| Monolithic foundations **without blinding** (bottom bars only) | **70 mm** |

- Further rules in §10.3.2:
  - Cover is never less than the bar diameter and never less than 10 mm.
  - Precast elements: −5 mm.
  - Constructive (non-working) bars: −5 mm.
  - Where cover exceeds 50 mm in bending or tension members (except foundations), add a crack-control mesh.
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- SP 22.13330.2016 §4.25 note 2 independently states the same 70 mm rule when there is no blinding. — [normacs answer quoting SP 22](https://www.normacs.info/answers/6288)
- **App use:** auto-set cover from element type plus a "blinding yes/no" toggle. Show an error if the drawn bar position violates it.

**Bar spacing, SP 63.13330.2018**
- §10.3.5 minimum clear spacing between bars: not less than the largest Ø, and not less than:
  - 25 mm for bottom bars in 1–2 rows (horizontal casting);
  - 30 mm for top bars;
  - 50 mm for more than 2 rows or for vertically cast bars.
- §10.3.6 minimum longitudinal reinforcement: **0.1 %** of b·h0 in bending members. Elements below this count as plain concrete.
- §10.3.8 maximum spacing:
  - beams and slabs: **200 mm when h ≤ 150 mm**; **1.5h and ≤ 400 mm when h > 150 mm**;
  - walls: vertical bars ≤ 2t and ≤ 400 mm; horizontal bars ≤ 400 mm.
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- **App use:** use these as mesh-spacing validators.

**Transverse bars (stirrups/clamps), SP 63.13330.2018**
- §10.3.12: stirrup Ø ≥ 6 mm in tied cages of bending members. In compressed members it must also be ≥ 0.25 × the largest longitudinal Ø.
- §10.3.13:
  - Where shear is not carried by concrete alone, stirrup pitch is **≤ 0.5·h0 and ≤ 300 mm**.
  - Solid slabs, and beams under 150 mm deep, may omit stirrups where concrete carries the shear.
  - Beams ≥ 150 mm deep need stirrups at **≤ 0.75·h0 and ≤ 500 mm** even where concrete carries the shear.
- §10.3.14: in compressed members, ≤ 15d and ≤ 500 mm, tightened to ≤ 10d and ≤ 300 mm if compression steel per face exceeds 1.5 %.
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)
- **App use:** strip-footing cage default is Ø6–8 A240 clamps at min(0.5·h0, 300 mm) — the "300 mm" default — with a warning above 300 mm.

**Anchorage, SP 63.13330.2018 §10.3.24–10.3.25 (verbatim structure)**
- Basic anchorage length: `L0,an = Rs·As / (Rbond·us)`.
- Bond strength: `Rbond = η1·η2·Rbt`.
- η1 for non-prestressed bars: **1.5** for smooth, **2.0** for cold-deformed ribbed, **2.5** for hot-rolled ribbed (A400/A500).
- η2: **1.0 for Ø ≤ 32 mm**, 0.9 for Ø36 and Ø40.
- Required length: `Lan = α·L0,an·As,cal/As,ef`, with α = 1.0 for tension and 0.75 for compression (straight ribbed bars).
- It may be reduced by at most 30 % for transverse bars or hooks.
- "В любом случае фактическую длину анкеровки принимают **не менее 15 ds и 200 мм**, а для ненапрягаемых стержней также **не менее 0,3 L0,an**."
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)

**Laps, SP 63.13330.2018 §10.3.29–10.3.30 (verbatim structure)**
- Lap splices without welding are allowed for bars ≤ 40 mm.
- Lap length: `Ll = α·L0,an·As,cal/As,ef`.
- α for straight ribbed bars: **1.2 in tension**, **0.9 in compression**, provided that:
  - ≤ 50 % of ribbed bars are spliced in one section (≤ 25 % for smooth bars with hooks);
  - clear gap between lapped bars ≤ 4d;
  - adjacent laps are ≥ 2d and ≥ 30 mm apart.
- With up to **100 %** spliced in one section, α = **2.0** in tension and 1.2 in compression; interpolate in between.
- "One section" is a length of 1.3·Ll.
- End hooks or welded cross-bars allow ≤ 30 % reduction.
- "фактическая длина перепуска должна быть **не менее 0,4·α·L0,an, не менее 20ds и не менее 250 мм**."
- Source: [SP 63 text](https://meganorm.ru/Data2/1/4293732/4293732352.htm)

**EN 1992-1-1 rules**
- Cover:
  - `cnom = cmin + Δcdev`, with **Δcdev = 10 mm** recommended.
  - `cmin = max{cmin,b; cmin,dur + Δcdur,γ − Δcdur,st − Δcdur,add; 10 mm}`.
  - cmin,b = bar Ø.
  - Table 4.4N, structural class S4, XC2/XC3: **cmin,dur = 25 mm** for ≤ C30/37, 20 mm for ≥ C35/45 after the Table 4.3N reduction.
  - Source: [Dlubal knowledge base](https://www.dlubal.com/en/support-and-learning/support/knowledge-base/001707)
- S4 is the recommended structural class for a 50-year life. Δcdev may be 5–10 mm. For concrete cast against prepared ground including blinding, **k1 = 40 mm**; cast directly against soil, **k2 = 75 mm** (clause 4.4.1.3(4)). — [FIN EC help](https://www.finesoftware.eu/help/finec/en/reinforcement-cover-01/)
- Laps (8.7.3):
  - `l0 = α1·α2·α3·α5·α6·lb,rqd ≥ l0,min = max{0.3·α6·lb,rqd; 15φ; 200 mm}`.
  - α6 = (ρ1/25)^0.5, limited to 1.0–1.5. Tabulated: 1.0 for < 25 % lapped, 1.15 for 33 %, 1.4 for 50 %, 1.5 for > 50 %.
  - `lb,rqd = (φ/4)·(σsd/fbd)`, `fbd = 2.25·η1·η2·fctd`.
  - Source: [Concrete Centre EC2 detailing lecture 9 (2017)](https://concretecentre.com/TCC/media/TCCMediaLibrary/PDF%20attachments/Lecture-9-Detailing-PHG-A1-Rev-12-16-Nov-17.pdf)
- Typical slab values (IStructE Manual Table 5.25, ≤ 33 % lapped, Ø ≤ 32):

| Concrete | Anchorage lbd, good bond | Lap l0, good bond | Lap l0, poor bond |
|---|---|---|---|
| C25/30 | 40φ | 46φ | 66φ |
| C30/37 | 36φ | 42φ | 59φ |

  - Laps must never be less than 15φ or 200 mm.
  - Source: [Concrete Centre lecture 9](https://concretecentre.com/TCC/media/TCCMediaLibrary/PDF%20attachments/Lecture-9-Detailing-PHG-A1-Rev-12-16-Nov-17.pdf)

### Inferences
**Computed SP 63 basic anchorage L0,an = Rs·d / (4·η1·η2·Rbt)** (my arithmetic from Table 6.8 / 6.14 values; η2 = 1)
- A500 (η1 = 2.5): B15 **58.0d**; B20 **48.3d**; B25 **41.4d**; B30 **37.8d**.
- A400 (Rs = 340, η1 = 2.5): B15 45.3d; B20 37.8d; B25 32.4d; B30 29.6d.
- A240 (Rs = 210, η1 = 1.5): B15 46.7d; B20 38.9d.

**Computed tension lap Ll with α = 1.2, ≤ 50 % spliced, As,cal/As,ef = 1**
- A500: B15 69.6d; B20 **58.0d** (Ø12 → 696 mm → 700 mm); B25 49.7d; B30 45.4d.
- A400: B20 45.3d; B25 38.9d.
- With 100 % spliced in one section (α = 2.0), A500/B20 rises to 96.7d.
- **App use:**
  - Implement the formula, then apply the floors (0.4·α·L0,an, 20d, 250 mm).
  - Let the user enter an As,cal/As,ef reduction.
  - Show a warning: "stagger laps, ≤ 50 % per 1.3·Ll zone".
- Site practice often quotes "40–50d" for laps. That is shorter than the SP 63 result for A500 in B20 unless As,cal/As,ef < 1, so the app should show the SP-computed value and state its assumption.

**Cover comparison**
- RU and EU nearly agree on foundation cover: 40 mm on blinding (both codes), 70 mm (RU) or 75 mm (EN) directly on soil.
- **App use:** one rule for both codes, `cover_bottom = blinding ? 40 : (EN ? 75 : 70)`.

**Rebar order quantity (estimate)**
- `ordered_kg = Σ(cut lengths incl. laps and anchorage) × kg/m`, then round to whole stock bars per Ø from the cutting optimiser.
- Leftover off-cuts stay visible as waste; the norms give no fixed % (see section 3).

**B500B (EN 10080)**
- Commonly specified for EU work. It is roughly equivalent to A500C in strength.
- I did not fetch the EN 1992-1-1 Annex C ductility values (Class B: k ≥ 1.08, εuk ≥ 5 %) — see Gaps.

### Gaps
- **Tie-wire consumption:** no normative figure found (the Сборник потерь has none). A trade rule of thumb (~1–2 % of rebar mass, or ~10–20 cm of 1.2–1.5 mm wire per intersection) is **unverified**.
- **EN 10080 / B500B properties** (fyk 500 MPa, (ft/fy)k ≥ 1.08, εuk ≥ 5 % per EN 1992-1-1 Annex C) are from memory, not verified this session.
- **Full EN 1992-1-1 Table 4.4N** (X0 10, XC1 15, XC4 30, XD1 35, XD2 40, XD3 45 mm at S4) was not fetched from a primary source; only the XC2/XC3 value of 25 mm was confirmed.
- **The A400 σт = 390 N/mm²** reading of GOST 34028 Table 5 comes from the meganorm transcription; verify it against the official copy.
- **Newer SP 63 amendments** (No. 2 onward), if any, were not checked.

---

## 5. Curing, strength gain, stripping, cold/hot weather, when to load

### Takeaway
Under SP 70.13330.2012 (as amended):
- Vertical formwork may be stripped at **0.5 MPa**.
- Slab or beam soffits may be stripped at **70 %** of design strength for spans ≤ 6 m and **80 %** for spans > 6 m.
- People may walk on the concrete and upper formwork may be erected at **2.5 MPa**.
- Full design load is allowed only at **100 %** (stated in the cold-weather Table 5.7).
- Special winter measures start when the daily mean is below **+5 °C** *and* the daily minimum is below 0 °C.
- Hot-weather rules (> 25 °C, RH < 50 %) are directly relevant to Uzbek summers.

### Cited Findings
**Standard and edition**
- **SP 70.13330.2012** "Несущие и ограждающие конструкции" (an update of SNiP 3.03.01-87) was approved by Gosstroy order 109/ГС of 25.12.2012 and took effect 01.07.2013. — [garant](https://base.garant.ru/70353868/)
- Amendment No. 1 was approved by Minstroy order 983/пр of 16.12.2016. — [meganorm](https://meganorm.ru/Data2/0/4294803/4294803202.htm)
- An Amendment No. 8 was reportedly approved by order 353/пр of 03.06.2026. I did not verify its content. — [rulaws.ru](https://rulaws.ru/acts/Prikaz-Minstroya-Rossii-ot-03.06.2026-N-353_pr/)
- The Uzbek analog is **КМК 3.03.01-98** "Несущие и ограждающие конструкции", with additions dated 10.03.2022. — [gov.uz Ministry list](https://gov.uz/oz/mc/pages/shaharsozlik-normalari-va-qoidalari)

**Stripping, SP 70 Table 5.11 row 10 (verbatim, current meganorm text)**
- Minimum strength of unloaded monolithic structures at stripping:
  - vertical faces (to keep shape): **0.5 МПа**;
  - horizontal and inclined faces, span **up to 6 m: 70 % of design strength**; **over 6 m: 80 %**.
- Row 11: for loaded structures (including load from concrete above), the strength is set by the ППР (works plan) and agreed with the designer.
- §5.17.9: with intermediate props, stripping strength may be lower if the ППР allows.
- Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- The old SNiP 3.03.01-87 gave 0.2–0.3 MPa for vertical faces; it is superseded. — [zakonbase](https://zakonbase.ru/content/part/358483)
- **App use:** compute the stripping date from a strength-gain curve. Thresholds: walls/footings sides 0.5 MPa; slab soffit 0.7·R (span ≤ 6 m) or 0.8·R.

**Early access and loading**
- SP 70 §5.4.3: "Движение людей по забетонированным конструкциям и установка опалубки вышележащих конструкций допускаются после достижения бетоном прочности **не менее 2,5 МПа**." — [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- SP 70 Table 5.7 item 2: "Загружение конструкций расчетной нагрузкой допускается после достижения бетоном прочности **не менее 100 % проектной**." — [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- **App use:** "full design load (e.g., all storeys of masonry)" is gated at 100 %. Show a softer "start masonry" suggestion earlier, labelled as practice (Inferences).

**Curing**
- SP 70 §5.4.1: protect fresh concrete from evaporation and rain immediately after placing, and keep doing so until it reaches **≥ 70 %** strength. — [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- **Hot weather, SP 70 §5.12** (air > 25 °C and RH < 50 %):
  - Rapid-hardening cements are recommended; normal-hardening cement is allowed for ≥ B22.5.
  - Pozzolanic and alumina cements are not allowed above ground.
  - Cement must not be hotter than 50 °C.
  - Mix temperature ≤ **30 °C** (surface modulus > 3) or ≤ **25 °C** for massive members.
  - Curing starts immediately and lasts until **70 %** strength (50 % if justified).
  - After **1.5 MPa**, keep the surface continuously wet (moisture-retaining cover, ponding or misting). **"Periodic watering … is not allowed."**
  - Protect from direct sun with reflective film (> 50 % reflectance) or similar.
  - Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
  - **App use:** this becomes a summer checklist for Tashkent, Samarkand and Nukus pours.

**Cold weather**
- SP 70 §5.11.1 (verbatim): "При среднесуточной температуре наружного воздуха **ниже 5 °С и минимальной суточной температуре ниже 0 °С** необходимо принимать специальные меры по выдерживанию уложенного бетона". §5.11.17: below a +5 °C daily mean, keep a concrete temperature log. — [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
  - **App use:** weather-forecast trigger for a "winter concreting" banner.
- SP 70 §5.11.7 winter methods: thermos; anti-freeze admixtures; electro-heating; warm air or enclosures. §5.11.8: the thermos method requires an initial concrete temperature of 5–10 °C held for 5–7 days. — [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- **Critical strength before freezing, SP 70 Table 5.7 item 1**

| Case | Min % of design strength |
|---|---|
| No anti-freeze admixture, indoor members / foundations without dynamic loads: up to B10 | 50 % |
| Same: up to B25 | 40 % |
| Same: B30 and above | 30 % |
| Members exposed to freeze-thaw while saturated (with air-entraining admixture) | 80 % |
| Spans ≤ 6 m | 70 % |
| Spans > 6 m | 80 % |
| Prestressed | 80 % |
| With anti-freeze admixture: up to B15 | 30 % |
| With anti-freeze admixture: up to B25 | 25 % |
| With anti-freeze admixture: B30 and above | 20 % |

  - Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- Other Table 5.7 limits:
  - Mix leaving the mixer: ≤ 35 °C (water ≤ 70 °C) for normal-hardening cement.
  - Thermos: placed mix ≥ 5 °C.
  - Heating: maximum 80 °C for Portland cement.
  - Heating rate ≤ 5/10/15 °C/h for surface modulus ≤ 4 / 5–10 / > 10.
  - Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)

**Placing, SP 70 Table 5.2**
- Maximum free drop:

| Member | Max drop, m |
|---|---|
| Columns | 3.5 |
| Slabs | **1.0** |
| Walls | 4.5 |
| Unreinforced | 6.0 |
| Lightly reinforced underground members in dry cohesive soil | 4.5 |
| Densely reinforced | 3.0 |

- Layer thickness with surface vibrators: ≤ 25 cm unreinforced, ≤ 15 cm with single reinforcement, ≤ 12 cm with double reinforcement. With hand internal vibrators: ≤ 1.25 × the vibrator's working length.
- Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- **App use:**
  - Pump/chute suggestion when the drop exceeds the limit.
  - Lift-height planner for pool walls.

**Formwork tolerances, SP 70 Table 5.11**
- Plumb: 5 mm per 1 m height, 20 mm over the full height for foundations.
- Axis offset for foundations: 15 mm.
- Internal dimension: ±5 mm.
- Local unevenness: 3 mm under a 2 m straightedge.
- Source: [SP 70 text](https://meganorm.ru/Data2/1/4293782/4293782487.htm)
- **App use:** QA checklist for the drawing module.

**Strength gain with time, EN 1992-1-1 §3.1.2(6)**
- At 20 °C: `fcm(t) = βcc(t)·fcm`, `βcc(t) = exp{s·[1 − (28/t)^0.5]}`.
- s = **0.20** for class R (CEM 42.5R, 52.5N, 52.5R); **0.25** for class N (CEM 32.5R, **42.5N**); **0.38** for class S (CEM 32.5N).
- Source: [Structville summary of EC2](https://structville.com/2020/02/formulas-for-calculating-different-properties-of-concrete-from-compressive-strength.html); [blueprints EC2 formula 3.2](https://blueprints.readthedocs.io/en/stable/API%20reference/codes/eurocode/en_1992_1_1_2004/chapter_3_materials/formula_3_2/)
- One handout lists s = 0.35 for class N; the other sources say 0.25. — [search extract](https://www.dhiperiti.com/wp-content/uploads/2023/01/Course-B-_compressed-1.pdf)

### Inferences
**Computed βcc(t) at +20 °C** (my arithmetic, mean strength relative to 28 days)

| Cement class | s | 3 d | 7 d | 14 d | 28 d | 90 d |
|---|---|---|---|---|---|---|
| Class N (CEM I 42.5N) | 0.25 | 60 % | 78 % | 90 % | 100 % | ~112 % |
| Class R | 0.20 | 66 % | 82 % | 92 % | 100 % | — |
| Class S (32.5N) | 0.38 | 46 % | 68 % | 85 % | 100 % | — |

- This roughly matches the Russian rule of thumb "≈ 50 % at 3 d, ≈ 70 % at 7 d, ≈ 90 % at 14 d" (popular, not normative).

**Time to stripping thresholds at +20 °C** (solve βcc(t) = target)

| Threshold | Class N (42.5N) | Class S (32.5N) |
|---|---|---|
| 70 % | ≈ 4.8 days | ≈ 7.5 days |
| 80 % | ≈ 7.8 days | ≈ 11 days |

- **App use:**
  - Default slab-stripping suggestion: "≥ 5 d (≤ 6 m span) / ≥ 8 d (> 6 m) at 20 °C with CEM 42.5N".
  - Scale by temperature-adjusted age.
  - EN 1992-1-1 Annex B gives `tT = Σ exp(−(4000/(273+T) − 13.65))·Δt`. I quote this from memory; verify it before coding.
  - At ≤ +10 °C the dates should roughly double. This is an estimate consistent with SP 70's cold-weather concern; it is not normative.

**When masonry can start on a foundation (practice estimate)**
- Light work (first courses) can start at ≥ 70 % (~7 d at 20 °C). Full building load requires 100 % (SP 70 Table 5.7 item 2).
- The app should show the 70 % date as "earliest recommended start (practice)" and the 28-day/100 % date as "design load allowed".

### Gaps
- The Russian table "relative strength vs concrete temperature (−3…+50 °C) × age" (НИИЖБ/ЦНИИОМТП winter-concreting manuals) was not retrieved; the dnaop page I tried was a different document.
- I did not find a normative clause for "masonry may start at X %"; only the 100 % design-load rule and the 2.5 MPa access rule exist.
- I did not verify the content of SP 70 Amendment No. 8 (2026).

---

## 6. Foundations: frost depth, depth rules, blinding/cushion, waterproofing, slab-on-ground, Uzbek norms, Eurocode 7

### Takeaway
Under SP 22.13330.2016, normative frost depth is `d_fn = d0·√Mt`, with d0 = 0.23 / 0.28 / 0.30 / 0.34 m by soil type and Mt = the sum of |negative monthly mean temperatures|. Design depth is `d_f = kh·d_fn`, with kh from Table 5.2 (0.4–1.0 heated, 1.1 unheated). Under 2025–2026 climate normals the formula gives **≈0 m for Tashkent, Samarkand and Andijan** and **≈0.6–0.9 m for Nukus**. Russian cities: **Moscow 1.03–1.52 m**, **Kazan 1.38–2.04 m**, **Yekaterinburg 1.53–2.27 m** (loam → coarse soil). The current Uzbek norms are **ШНК 2.02.01-19** (foundations) and **ШНК 2.01.01-22** (climate), which replaced KMK 2.02.01-98 and KMK 2.01.01-94 respectively; I could not read the Uzbek frost-depth tables.

### Cited Findings
**Standard and edition**
- **SP 22.13330.2016** "Основания зданий и сооружений" (an update of SNiP 2.02.01-83*) was approved by Minstroy order 970/пр of 16.12.2016 and took effect 17.07.2017. — [garant](https://base.garant.ru/71706442/)
- Amendment No. 3 exists. — [meganorm](https://meganorm.ru/Data2/0/4294789/4294789908.htm)

**Frost depth, SP 22.13330.2016 (verbatim)**
- §5.5.2: d_fn is the mean of the annual maximum frost depths over ≥ 10 years, measured on an open, snow-cleared horizontal site with groundwater below the frost depth.
- §5.5.3: where frost depth does not exceed 2.5 m, `d_fn = d0·√Mt` (formula 5.3).
  - d0 = **0.23 m** for loams and clays; **0.28 m** for sandy loams and fine or silty sands; **0.30 m** for gravelly, coarse and medium sands; **0.34 m** for coarse-clastic soils.
  - Mt = the sum of the absolute values of negative monthly mean temperatures, taken **from SP 131.13330**, or from a comparable weather station if the location is missing.
  - Where depth exceeds 2.5 m, and in mountain areas, use a heat-engineering calculation per SP 25.13330.
- Source: [SP 22 text, garant](https://base.garant.ru/71706442/)
- **App use:**
  - Frost calculator: inputs are soil type and either Mt or a city; outputs are d_fn and d_f.
  - Warn "use heat-engineering calc" if d_fn > 2.5 m or the site is mountainous.

**Design frost depth, SP 22.13330.2016 §5.5.4 and Table 5.2**
- `d_f = kh·d_fn`. kh = **1.1** for outer and inner foundations of unheated buildings (except where the mean annual temperature is negative).
- Table 5.2, kh for outer foundations of heated buildings by indoor temperature of the space next to the foundation:

| Building type | 0 °C | 5 °C | 10 °C | 15 °C | ≥ 20 °C |
|---|---|---|---|---|---|
| No basement, floor on ground | 0.9 | 0.8 | 0.7 | 0.6 | 0.5 |
| No basement, floor on joists over ground | 1.0 | 0.9 | 0.8 | 0.7 | 0.6 |
| No basement, floor on insulated ground-floor slab | 1.0 | 1.0 | 0.9 | 0.8 | 0.7 |
| With basement or technical underfloor space | 0.8 | 0.7 | 0.6 | 0.5 | 0.4 |

- Notes to Table 5.2:
  - The values apply when the footing projects ≤ 0.5 m beyond the wall face. For a projection ≥ 1.5 m, add 0.1 (capped at kh = 1.0); interpolate in between.
  - For intermediate temperatures, round down to the nearest tabulated value.
- Source: [SP 22 text, garant](https://base.garant.ru/71706442/)
- **App use:** dropdowns for building type, floor type and indoor temperature give kh.

**Founding depth versus soil and groundwater, SP 22.13330.2016 §5.5.6–5.5.7 and Table 5.3**
- Founding depth depends on the soil under the footing and the groundwater level dw.
- Rock, coarse-clastic soils with sand filler, and gravelly/coarse/medium sands: depth **does not depend on d_f**. Note 1: these soils must extend at least to d_fn.
- Clays and loams: the visible fragment shows the minimum "**не менее 0,5** d_f" for one case.
- §5.5.7: for unheated buildings, depth is measured from the finished grade, or from the basement floor if there is one.
- Source: [SP 22 text, garant](https://base.garant.ru/71706442/) — the formula cells were rendered as images, so only fragments were legible.
- **App use:** soil-type selector. If the soil is non-heaving (sands, coarse), allow a shallow footing with a note. Otherwise require depth ≥ d_f.

**Blinding and waterproofing, SP 22.13330.2016 §4.25**
- Monolithic foundations on any non-rock soil need a **concrete preparation (blinding)**. Alternatives allowed: crushed-stone or sand preparation with a cement screed, or geomembranes.
- Note 1: preparation **≥ 100 mm** of concrete **≥ B7.5**. Whether it needs reinforcement depends on loads.
- Note 2: without blinding, the rebar cover is **≥ 70 mm**.
- §5.4.15: where groundwater may degrade the soil, provide continuous perimeter waterproofing.
- Source: [normacs expert answer quoting SP 22](https://www.normacs.info/answers/6288)
- SP 45.13330.2017 §15.2: waterproofing goes onto concrete or cement-sand preparation and is then protected by a cement-sand screed. — [same normacs answer](https://www.normacs.info/answers/6288)
- The Manual to SNiP 2.02.01-83 §3.182 gives a compacted-soil preparation **≥ 0.15 m** under waterproof aprons. — [same](https://www.normacs.info/answers/6288)
- **App use:** add a 100 mm B7.5 blinding item by default (area × 0.10 m³ plus a 10.4 % loss if it is cement-sand mortar, see section 3). Untick it and the cover switches to 70 mm.

**Computed normative frost depth for Russian cities (toolfox, d_fn per SP 22 formula 5.3, Mt from SP 131.13330)**

| City | Loam | Sandy loam / fine sand | Medium sand | Coarse-clastic |
|---|---|---|---|---|
| Moscow | 1.03 | 1.25 | 1.34 | 1.52 |
| Kazan (Mt = 35.9) | 1.38 | 1.68 | 1.80 | 2.04 |
| Yekaterinburg | 1.53 | 1.87 | 2.00 | 2.27 |
| St Petersburg | 0.91 | 1.11 | 1.18 | 1.34 |

- Values in metres. Source: [toolfox.ru frost calculator](https://toolfox.ru/tools/soil-frost-depth-calculator)
- The page says it takes Mt from "**SP 131.13330.2025**". I did not verify that a 2025 edition is in force; SP 131.13330.2020 with Amendment No. 2 (order 469/пр, 30.06.2023) was current earlier. — [normacs on SP 131 revision](https://www.normacs.info/projects/11772); [pravo.ppt.ru Amendment 2](https://pravo.ppt.ru/prikaz/minstroy/n-469-pr-285373)
- Arithmetic check: 0.28·√20.05 = 1.25 ✓.

**Uzbek norms in force (Ministry of Construction list)**
- ШНК 2.01.01-22 "Лойиҳалаш учун иқлимий ва физикавий-геологик маълумотлар" (climate)
- ШНК 2.02.01-19 "Основания зданий и сооружений"
- ШНҚ 2.02.03-21 (pile foundations)
- КМК 2.01.03-19 "Строительство в сейсмических районах"
- ШНҚ 2.03.01-24 "Бетон ва темир-бетон конструкциялар"
- КМК 2.01.07-96 (loads)
- КМК 3.03.01-98 (load-bearing structures, works)
- KMK 2.02.01-98 and KMK 2.01.01-94 do **not** appear in the current list.
- Source: [gov.uz Ministry of Construction page](https://gov.uz/oz/mc/pages/shaharsozlik-normalari-va-qoidalari)
- A 2023 Tashkent paper confirms ШНК 2.01.01-22 is the new version replacing KMK 2.01.01-94. — [Zenodo paper](https://zenodo.org/records/7679704/files/TA'LIM_FIDOYILARI-2023-FEVRAL-1-qism%20-185-191.pdf)
- **App use:**
  - In UZ mode, cite ШНК 2.02.01-19 and ШНК 2.01.01-22, and keep a per-region frost table that the user can edit.
  - Show a seismic reminder (КМК 2.01.03-19) on all foundation designs.

**Uzbek city climate normals (monthly mean °C) used to compute Mt**

| City | Jan | Feb | Dec | Source |
|---|---|---|---|---|
| Tashkent | +2.3 | +4.2 | +3.5 | [Wikipedia (Pogoda.ru.net)](https://en.wikipedia.org/wiki/Tashkent) |
| Samarkand | +2.3 | +4.0 | +3.7 | [Wikipedia (pogodaiklimat)](https://en.wikipedia.org/wiki/Samarkand) |
| Andijan | −0.1 | +2.8 | +1.3 | [Wikipedia (WMO 1991–2020)](https://en.wikipedia.org/wiki/Andijan) |
| Nukus | −3.7 | −1.4 | −2.0 | [Wikipedia (NOAA/WMO)](https://en.wikipedia.org/wiki/Nukus) |

- Tashkent and Samarkand have no month below 0 °C.

**Slab-on-ground (plita) practice values**
- 200–300 mm thick; Ø12–16 bars at 200–300 mm in two meshes; concrete M200–M300. — [toolfox slab calculator](https://toolfox.ru/tools/foundation-slab-calculator)
- Two meshes: the bottom one carries bending from soil reaction. A 200×200 mm cell is the standard. — [teletype trade article](https://teletype.in/@rolled-metal-metal-base-aksvil/PzeBMK4vHL0)
- Design examples show Ø12 @ 200 everywhere with Ø12 @ 100 added near supports. — [search extract of design examples](https://bim.vc/upload/iblock/4df/uvb2wu2a7oly85b15awqtijef0plv6lx.pdf)
- These are practice values, not normative. The normative checks are the SP 63 §10.3.8 spacing (≤ 1.5h and ≤ 400 mm for h > 150 mm) and the §10.3.6 minimum 0.1 % reinforcement.
- **App use:**
  - Default 250 mm, Ø12 A500C @ 200 top and bottom, B22.5 (M300), editable.
  - Validator: `As_bottom ≥ 0.001·b·h0`, spacing ≤ min(1.5h, 400).
  - Mass: per-m² rebar = 2 meshes × 2 directions × (1000/200) bars/m × 0.888 kg/m ≈ **17.8 kg/m²** plus laps (my arithmetic).

### Inferences
**Uzbek frost depths from formula 5.3** (my arithmetic; estimates only, not official Uzbek values)
- **Nukus:** Mt = 3.7 + 1.4 + 2.0 = 7.1, so d_fn = 0.61 m (loam), 0.75 m (sandy loam/fine sand), 0.80 m (medium sand), 0.91 m (coarse-clastic).
- **Andijan:** Mt ≈ 0.1, so d_fn ≈ 0.07 m.
- **Tashkent / Samarkand:** Mt = 0, so the formula gives 0. Like Krasnodar on toolfox, this does **not** mean "no frost"; short cold spells still freeze the top soil.
- **App use:**
  - Do not output 0 m.
  - Apply a configurable floor, for example ≥ 0.5 m (practice estimate). This should also exceed the loose topsoil or fill layer.
  - Show "consult ШНК 2.01.01-22 / local survey".
  - In Uzbekistan, loess (subsiding/collapsible) soils and seismicity usually govern depth and details more than frost does.

**Worked RU design-depth example**
- Moscow, loam, heated house, no basement, floor on ground, 20 °C: d_f = 0.5 × 1.03 ≈ **0.52 m**.
- The same house unheated during construction (kh = 1.1) gives 1.13 m. SP 22 §5.5.8 requires preventing frost under the footings during construction.
- **App use:** when the house will stand unheated, warn "winter exposure before heating — protect footing soil".

**Strip footing width**
- Width is governed by bearing pressure, not by wall thickness: `b ≥ N / (R − γ_avg·d)`, with R per SP 22 §5.6 (formula 5.7).
- Footings also usually project beyond the wall on each side. Table 5.2 note 1 shows that projections > 0.5 m increase kh.
- **App use:** compute b from line load and a user-entered or default R. Warn if b < wall thickness. "Width = wall + 100–200 mm" is practice, not normative.

**Sand/gravel cushion of 100–200 mm under blinding**
- Common practice. SP 22 §4.25 only recognises "crushed-stone or sand preparation with a cement screed" as an alternative to concrete blinding, with no thickness.
- **App use:** default 150 mm compacted sand or gravel as an editable estimate; volume = area × t × a compaction factor of ~1.1–1.2 (estimate).

**Eurocode 7 (EN 1997-1)**
- EN 1997-1 §6.4 requires founding below the frost-susceptible depth but gives no national numbers. EU countries take frost depth from national annexes or standards (for example EN ISO 13793 for frost-protected shallow foundations).
- This is from general knowledge and was **not verified** this session. The app should treat EU frost depth as a user input.

### Gaps
- **Official Uzbek frost-depth values** (ШНК 2.01.01-22 table/map, or old KMK 2.01.01-94/2.02.01-98) were not retrieved, because mc.uz PDFs failed on a TLS certificate mismatch and I found no mirror. The ШНК 2.02.01-19 text was not read; whether it keeps the SP 22 formula and Table 5.2 is unconfirmed.
- **SP 22 Table 5.3** was only partly legible (formula cells are images). The full matrix (fine sands, sandy loams IL<0 / IL≥0, clays IL<0.25 / IL≥0.25, against dw ≤ d_f + 2 m / dw > d_f + 2 m) should be read from an official copy.
- I did not verify whether a newer SP 22 edition (2024+) has replaced SP 22.13330.2016.
- I did not verify that SP 131.13330.2025 is in force; the toolfox page claims it.
- There is no normative source for the minimum strip width versus wall thickness, the 100–200 mm sand cushion, or slab thickness; all are practice.
- I did not read the EN 1997-1 frost clause text.
